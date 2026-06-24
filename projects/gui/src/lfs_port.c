/******************************************************************************
 * @file     lfs_port.c
 * @brief    LittleFS 移植层：将 LittleFS 的 read/prog/erase/sync 操作
 *           映射到 TD1601 CSI spiflash 驱动接口
 *
 * 重要说明：
 *   !! 绝对不能在此文件中再次调用 csi_spiflash_qspi_init() !!
 *   系统启动时 SystemInit() -> sys_spiflash_init() 已经用
 *   csi_spiflash_qspi_init(&g_spiflash, 0, NULL) 完成了控制器初始化并
 *   开启了 4 线 XIP 模式。若在 XIP 执行期间重新 init，QSPI 控制器被
 *   复位，CPU 取指立刻失败，触发 CPU Exception NO.2 (Illegal Instruction)。
 *   这里通过 extern 直接复用系统全局 g_spiflash 句柄。
 *
 *   - program/erase 已由芯片 SDK 标注 ATTRIBUTE_DATA（.ram.code），
 *     在 XIP 状态下可安全调用
 *   - 所有 offset 是相对 Flash 起始地址 0x18000000 的字节偏移
 *
 * @version  V1.1
 * @date     2026-06-24
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <drv/spiflash.h>
#include "lfs_port.h"

/* -------------------------------------------------------------------------
 * 复用 system.c 中已由 SystemInit() 完成初始化的全局 spiflash 句柄
 * ---------------------------------------------------------------------- */
extern csi_spiflash_t g_spiflash;

/* -------------------------------------------------------------------------
 * 全局 LittleFS 实例
 * ---------------------------------------------------------------------- */
static lfs_t s_lfs;
static int   s_mounted = 0;

/* -------------------------------------------------------------------------
 * LittleFS 回调：读
 *   block  : LittleFS block 编号（0 ~ LFS_BLOCK_COUNT-1）
 *   off    : block 内字节偏移
 *   buffer : 读出缓冲
 *   size   : 读取字节数
 * ---------------------------------------------------------------------- */
static int lfs_bd_read(const struct lfs_config *c,
                       lfs_block_t block, lfs_off_t off,
                       void *buffer, lfs_size_t size)
{
    (void)c;
    uint32_t flash_off = LFS_PART_OFFSET
                         + (uint32_t)block * LFS_SECTOR_SIZE
                         + (uint32_t)off;

    int ret = csi_spiflash_read(&g_spiflash, flash_off, buffer, (uint32_t)size);
    if (ret < 0) {
        printf("[lfs] read err %d (off=0x%08x, size=%u)\r\n",
               ret, (unsigned)flash_off, (unsigned)size);
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/* -------------------------------------------------------------------------
 * LittleFS 回调：写（page program）
 * ---------------------------------------------------------------------- */
static int lfs_bd_prog(const struct lfs_config *c,
                       lfs_block_t block, lfs_off_t off,
                       const void *buffer, lfs_size_t size)
{
    (void)c;
    uint32_t flash_off = LFS_PART_OFFSET
                         + (uint32_t)block * LFS_SECTOR_SIZE
                         + (uint32_t)off;

    int ret = csi_spiflash_program(&g_spiflash, flash_off,
                                   buffer, (uint32_t)size);
    if (ret < 0) {
        printf("[lfs] prog err %d (off=0x%08x, size=%u)\r\n",
               ret, (unsigned)flash_off, (unsigned)size);
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/* -------------------------------------------------------------------------
 * LittleFS 回调：擦除（sector erase，1 block = 1 sector = 4KB）
 * ---------------------------------------------------------------------- */
static int lfs_bd_erase(const struct lfs_config *c, lfs_block_t block)
{
    (void)c;
    uint32_t flash_off = LFS_PART_OFFSET
                         + (uint32_t)block * LFS_SECTOR_SIZE;

    csi_error_t ret = csi_spiflash_erase(&g_spiflash, flash_off,
                                          LFS_SECTOR_SIZE);
    if (ret != CSI_OK) {
        printf("[lfs] erase err %d (off=0x%08x)\r\n",
               (int)ret, (unsigned)flash_off);
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/* -------------------------------------------------------------------------
 * LittleFS 回调：sync（SPI Flash 写后硬件自动完成，此处无需额外操作）
 * ---------------------------------------------------------------------- */
static int lfs_bd_sync(const struct lfs_config *c)
{
    (void)c;
    return LFS_ERR_OK;
}

/* -------------------------------------------------------------------------
 * LittleFS 配置结构体
 * ---------------------------------------------------------------------- */
static const struct lfs_config s_lfs_cfg = {
    /* 块设备操作 */
    .read  = lfs_bd_read,
    .prog  = lfs_bd_prog,
    .erase = lfs_bd_erase,
    .sync  = lfs_bd_sync,

    /* Flash 几何参数 */
    .read_size      = 1,              /* 最小读单元（字节） */
    .prog_size      = LFS_PAGE_SIZE,  /* 最小写单元 = 1 page = 256B */
    .block_size     = LFS_SECTOR_SIZE,/* 擦除单元   = 1 sector = 4KB */
    .block_count    = LFS_BLOCK_COUNT,/* 16 个 block */
    .cache_size     = LFS_PAGE_SIZE,  /* 256B 读写缓存（减小 RAM 占用） */
    .lookahead_size = 16,             /* lookahead 缓冲（16B = 128 bit） */
    .block_cycles   = 500,            /* 磨损均衡：500 次擦写后自动迁移 */
};

/* -------------------------------------------------------------------------
 * 公共 API
 * ---------------------------------------------------------------------- */

/**
 * @brief  初始化 spiflash 并挂载 LittleFS；若无有效文件系统则自动格式化
 */
int lfs_port_mount(void)
{
    /* 句柄已由 SystemInit() 初始化完毕，直接使用，不能再 init */

    /* 打印 Flash 信息 */
    csi_spiflash_info_t info;
    if (csi_spiflash_get_flash_info(&g_spiflash, &info) == CSI_OK) {
        printf("[lfs] flash: %s, size=%uKB, sector=%uB, page=%uB\r\n",
               info.flash_name ? info.flash_name : "unknown",
               (unsigned)(info.flash_size / 1024),
               (unsigned)info.sector_size,
               (unsigned)info.page_size);
    }

    /* 尝试挂载 */
    int ret = lfs_mount(&s_lfs, &s_lfs_cfg);
    if (ret != LFS_ERR_OK) {
        printf("[lfs] mount failed (%d), formatting...\r\n", ret);
        ret = lfs_format(&s_lfs, &s_lfs_cfg);
        if (ret != LFS_ERR_OK) {
            printf("[lfs] format failed (%d)\r\n", ret);
            return -1;
        }
        ret = lfs_mount(&s_lfs, &s_lfs_cfg);
        if (ret != LFS_ERR_OK) {
            printf("[lfs] mount after format failed (%d)\r\n", ret);
            return -1;
        }
    }

    s_mounted = 1;
    printf("[lfs] mounted OK (part offset=0x%08x, %u blocks x %uKB)\r\n",
           (unsigned)LFS_PART_OFFSET,
           (unsigned)LFS_BLOCK_COUNT,
           (unsigned)(LFS_SECTOR_SIZE / 1024));
    return 0;
}

/**
 * @brief  卸载 LittleFS
 */
void lfs_port_unmount(void)
{
    if (s_mounted) {
        lfs_unmount(&s_lfs);
        s_mounted = 0;
    }
}

/**
 * @brief  返回全局 lfs_t 指针
 */
lfs_t *lfs_port_get(void)
{
    return s_mounted ? &s_lfs : NULL;
}

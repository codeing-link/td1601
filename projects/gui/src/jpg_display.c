/******************************************************************************
 * @file     jpg_display.c
 * @brief    JPEG 解码 + 显示：从 LittleFS 读取 jpg，流式解码后全屏显示
 *
 * 流程：
 *   1. lfs_port_mount() 挂载（失败时自动格式化再挂载）
 *   2. 检查目标文件是否存在且大小与编译进来的原始数据一致：
 *        - 一致 → 直接读取解码
 *        - 不一致/不存在 → 删旧 + 写入 img_1_jpg[] → 读取解码
 *   3. 流式解码（TJpgDec）：input_func 从 LFS 文件逐块读，
 *      output_func 每个 MCU 块直接调 LCD_addWindow 刷屏，无需帧缓冲
 *
 * XIP 安全：erase/program 已由芯片 SDK 标注 ATTRIBUTE_DATA(.ram.code)，
 *           LittleFS 的 lfs_format/lfs_file_write 调用链全在 RAM 中执行，
 *           不需要额外关中断。
 *
 * @version  V1.0
 * @date     2026-06-25
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "jpg_display.h"
#include "lfs_port.h"
#include "ST77916.h"
#include "TJpgDec/tjpgd.h"
#include "img_1.h"

/* ---- 文件系统路径 -------------------------------------------------------- */
#define JPG_DIR      "/img"
#define JPG_PATH     "/img/1.jpg"

/* ---- TJpgDec 工作内存（在 BSS 段，静态分配避免占用栈空间）-------------- */
/* MCU 最大 16×16×3(RGB888)=768B workbuf + huffman/量化表约 2KB = 需 ~3KB
 * 取 3500B 留足余量；若 jd_prepare 返回 JDR_MEM1 则需继续增大            */
#define JDEC_POOL_SIZE  3500
static uint8_t s_jdec_pool[JDEC_POOL_SIZE];

/* ---- 输出行缓冲（MCU 最大宽度 16 像素，RGB565 = 2字节/像素）------------ */
/* TJpgDec 输出的 bitmap 是当前 rect 区域的像素，最大 16×16 块            */
/* LCD_addWindow 需要完整一行的连续像素，此处用静态缓冲传递                */

/* =========================================================================
 * LFS 文件读取上下文（input_func 使用）
 * ====================================================================== */
typedef struct {
    lfs_file_t *file;   /* 已打开的 LFS 文件句柄 */
} jpg_io_t;

/* =========================================================================
 * TJpgDec 输入回调：从 LFS 文件读字节
 * ====================================================================== */
static size_t input_func(JDEC *jd, uint8_t *buf, size_t ndata)
{
    jpg_io_t *io = (jpg_io_t *)jd->device;

    if (buf) {
        /* 读取数据 */
        lfs_ssize_t n = lfs_file_read(lfs_port_get(), io->file, buf, (lfs_size_t)ndata);
        return (n > 0) ? (size_t)n : 0;
    } else {
        /* 跳过（seek） */
        lfs_file_seek(lfs_port_get(), io->file,
                      (lfs_soff_t)ndata, LFS_SEEK_CUR);
        return ndata;
    }
}

/* =========================================================================
 * TJpgDec 输出回调：每个 MCU 块直接刷到 LCD，无需整帧缓冲
 *
 * JD_FORMAT = 1（RGB565），bitmap 是行优先的 uint16_t 数组。
 * rect 给出该块在图像坐标系中的范围，直接对应屏幕坐标。
 * 边界截断块（如最后一行/列）rx/ry 由 TJpgDec 自动裁剪，
 * bitmap 中只包含 rx*ry 个有效像素，LCD_addWindow 尺寸匹配。
 * ====================================================================== */
static int output_func(JDEC *jd, void *bitmap, JRECT *rect)
{
    (void)jd;
    LCD_addWindow(rect->left, rect->top, rect->right, rect->bottom,
                  (uint16_t *)bitmap);
    return 1;   /* 继续解码 */
}

/* =========================================================================
 * 内部：把 img_1_jpg[] 写入 LFS（覆盖写）
 * ====================================================================== */
static int write_jpg_to_lfs(void)
{
    lfs_t *lfs = lfs_port_get();

    /* 创建目录（已存在时忽略） */
    int err = lfs_mkdir(lfs, JPG_DIR);
    if (err != LFS_ERR_OK && err != LFS_ERR_EXIST) {
        printf("[jpg] mkdir '%s' failed (%d)\r\n", JPG_DIR, err);
        return -1;
    }

    /* 删除旧文件（不存在时忽略） */
    lfs_remove(lfs, JPG_PATH);

    /* 写入 */
    lfs_file_t file;
    err = lfs_file_open(lfs, &file, JPG_PATH,
                        LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err != LFS_ERR_OK) {
        printf("[jpg] open for write failed (%d)\r\n", err);
        return -1;
    }

    /* 分块写入，每次 256 字节，避免占用过大栈 */
    const uint8_t *src = img_1_jpg;
    lfs_size_t remain = (lfs_size_t)img_1_jpg_len;
    while (remain > 0) {
        lfs_size_t chunk = (remain > 256) ? 256 : remain;
        lfs_ssize_t written = lfs_file_write(lfs, &file, src, chunk);
        if (written != (lfs_ssize_t)chunk) {
            printf("[jpg] write failed (%d)\r\n", (int)written);
            lfs_file_close(lfs, &file);
            return -1;
        }
        src    += chunk;
        remain -= chunk;
    }

    lfs_file_close(lfs, &file);
    printf("[jpg] written %u bytes to '%s'\r\n",
           (unsigned)img_1_jpg_len, JPG_PATH);
    return 0;
}

/* =========================================================================
 * 内部：比较 LFS 中的 jpg 是否与当前编译进来的 img_1_jpg[] 完全一致
 * ====================================================================== */
static int jpg_file_matches_embedded(lfs_t *lfs)
{
    lfs_file_t file;
    uint8_t buf[256];
    const uint8_t *src = img_1_jpg;
    lfs_size_t remain = (lfs_size_t)img_1_jpg_len;

    int err = lfs_file_open(lfs, &file, JPG_PATH, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        return 0;
    }

    while (remain > 0) {
        lfs_size_t chunk = (remain > sizeof(buf)) ? sizeof(buf) : remain;
        lfs_ssize_t n = lfs_file_read(lfs, &file, buf, chunk);
        if (n != (lfs_ssize_t)chunk || memcmp(buf, src, chunk) != 0) {
            lfs_file_close(lfs, &file);
            return 0;
        }
        src += chunk;
        remain -= chunk;
    }

    lfs_file_close(lfs, &file);
    return 1;
}

/* =========================================================================
 * 内部：从 LFS 读取 jpg 并流式解码显示
 * ====================================================================== */
static int decode_and_display(void)
{
    lfs_t *lfs = lfs_port_get();

    lfs_file_t file;
    int err = lfs_file_open(lfs, &file, JPG_PATH, LFS_O_RDONLY);
    if (err != LFS_ERR_OK) {
        printf("[jpg] open for read failed (%d)\r\n", err);
        return -1;
    }

    /* 初始化 TJpgDec */
    JDEC jdec;
    jpg_io_t io = { .file = &file };

    JRESULT jres = jd_prepare(&jdec, input_func, s_jdec_pool,
                               JDEC_POOL_SIZE, &io);
    if (jres != JDR_OK) {
        printf("[jpg] jd_prepare failed (%d)\r\n", (int)jres);
        lfs_file_close(lfs, &file);
        return -1;
    }

    printf("[jpg] image: %ux%u, decoding...\r\n",
           (unsigned)jdec.width, (unsigned)jdec.height);

    /* 流式解码并显示，scale=0（1:1 原始尺寸） */
    jres = jd_decomp(&jdec, output_func, 0);
    if (jres != JDR_OK) {
        printf("[jpg] jd_decomp failed (%d)\r\n", (int)jres);
        lfs_file_close(lfs, &file);
        return -1;
    }

    lfs_file_close(lfs, &file);
    printf("[jpg] display done\r\n");
    return 0;
}

/* =========================================================================
 * 对外接口
 * ====================================================================== */
void jpg_display_run(void)
{
    printf("\r\n======== JPG Display ========\r\n");

    /* 1. 挂载 LittleFS（挂载失败自动格式化） */
    if (lfs_port_mount() != 0) {
        printf("[jpg] FAIL: lfs mount\r\n");
        return;
    }

    lfs_t *lfs = lfs_port_get();

    /* 诊断：打印当前文件系统已用块数和剩余空间 */
    lfs_ssize_t used_blocks = lfs_fs_size(lfs);
    printf("[jpg] LFS: %d/%u blocks used (~%u bytes free)\r\n",
           (int)used_blocks,
           (unsigned)LFS_BLOCK_COUNT,
           (unsigned)((LFS_BLOCK_COUNT - (used_blocks > 0 ? used_blocks : 0))
                      * LFS_SECTOR_SIZE));

    /* 2. 检查文件是否存在且内容正确 */
    struct lfs_info fi;
    int need_write = 1;
    if (lfs_stat(lfs, JPG_PATH, &fi) == LFS_ERR_OK) {
        if (fi.type == LFS_TYPE_REG &&
            fi.size == (lfs_size_t)img_1_jpg_len) {
            if (jpg_file_matches_embedded(lfs)) {
                printf("[jpg] file exists (%u bytes), skip write\r\n",
                       (unsigned)fi.size);
                need_write = 0;
            } else {
                printf("[jpg] file content changed, rewrite\r\n");
            }
        } else {
            printf("[jpg] file size mismatch (got %u, expect %u), rewrite\r\n",
                   (unsigned)fi.size, (unsigned)img_1_jpg_len);
        }
    } else {
        printf("[jpg] file not found, writing...\r\n");
    }

    /* 3. 按需写入 */
    if (need_write) {
        if (write_jpg_to_lfs() != 0) {
            /* 写入失败（最常见原因：旧数据占满空间）→ 格式化后重试 */
            printf("[jpg] write failed, formatting LFS and retrying...\r\n");
            lfs_port_unmount();

            /* 强制格式化：清空所有旧数据 */
            if (lfs_port_format() != 0) {
                printf("[jpg] FAIL: format\r\n");
                return;
            }
            if (lfs_port_mount() != 0) {
                printf("[jpg] FAIL: remount after format\r\n");
                return;
            }
            lfs = lfs_port_get();

            /* 格式化后再次打印空间状态 */
            used_blocks = lfs_fs_size(lfs);
            printf("[jpg] after format: %d/%u blocks used\r\n",
                   (int)used_blocks, (unsigned)LFS_BLOCK_COUNT);

            if (write_jpg_to_lfs() != 0) {
                printf("[jpg] FAIL: write after format\r\n");
                lfs_port_unmount();
                return;
            }
        }
    }

    /* 4. 流式解码 + 全屏显示 */
    if (decode_and_display() != 0) {
        printf("[jpg] FAIL: decode/display\r\n");
    }

    lfs_port_unmount();
    printf("======== JPG Display Done ========\r\n\r\n");
}

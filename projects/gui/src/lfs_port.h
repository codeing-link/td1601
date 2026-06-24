/******************************************************************************
 * @file     lfs_port.h
 * @brief    LittleFS 移植层：基于 TD1601 SIP 内置 SPI Flash 的底层适配
 *
 * Flash 布局（512 KB SIP Flash）：
 *   0x18000000 ~ 0x1806FFFF  代码 + rodata（448 KB）
 *   0x18070000 ~ 0x1807FFFF  LittleFS 分区（64 KB，共 16 个 4KB sector）
 *
 * @version  V1.0
 * @date     2026-06-24
 ******************************************************************************/
#ifndef _LFS_PORT_H_
#define _LFS_PORT_H_

#include <stdint.h>
#include "littlefs/lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Flash 分区参数（和链接脚本保持一致）-------------------------------- */
/* SIP 512KB Flash 起始 XIP 地址 */
#define LFS_FLASH_BASE          0x18000000U
/* LittleFS 分区偏移（从 Flash 起始算，448KB = 0x70000） */
#define LFS_PART_OFFSET         0x00070000U
/* LittleFS 分区大小（64 KB） */
#define LFS_PART_SIZE           0x00010000U
/* Flash Sector 大小（4 KB） */
#define LFS_SECTOR_SIZE         4096U
/* Flash Page 大小（256 B） */
#define LFS_PAGE_SIZE           256U
/* Block 数量 = 64KB / 4KB = 16 */
#define LFS_BLOCK_COUNT         (LFS_PART_SIZE / LFS_SECTOR_SIZE)

/**
 * @brief  初始化 LittleFS：挂载已有文件系统，若格式化检测失败则自动格式化
 * @return 0 成功，负值失败
 */
int lfs_port_mount(void);

/**
 * @brief  卸载 LittleFS
 */
void lfs_port_unmount(void);

/**
 * @brief  获取全局 lfs 实例指针（挂载成功后有效）
 */
lfs_t *lfs_port_get(void);

#ifdef __cplusplus
}
#endif

#endif /* _LFS_PORT_H_ */

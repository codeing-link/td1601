/******************************************************************************
 * @file     lfs_port.h
 * @brief    LittleFS 移植层：基于 TD1601 SIP 内置 SPI Flash 的底层适配
 *
 * Flash 布局（512 KB SIP Flash）：
 *   0x18000000 ~ 0x1805FFFF  代码 + rodata（384 KB）
 *   0x18060000 ~ 0x1807FFFF  LittleFS 分区（128 KB，共 32 个 4KB sector）
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
/* LittleFS 分区偏移（从 Flash 起始算，384KB = 0x60000） */
#define LFS_PART_OFFSET         0x00060000U
/* LittleFS 分区大小（128 KB） */
#define LFS_PART_SIZE           0x00020000U
/* Flash Sector 大小（4 KB） */
#define LFS_SECTOR_SIZE         4096U
/* Flash Page 大小（256 B） */
#define LFS_PAGE_SIZE           256U
/* Block 数量 = 128KB / 4KB = 32 */
#define LFS_BLOCK_COUNT         (LFS_PART_SIZE / LFS_SECTOR_SIZE)

/**
 * @brief  挂载 LittleFS
 */
int lfs_port_mount(void);

/**
 * @brief  强制格式化 LittleFS（清空所有数据，下次 mount 前必须调用）
 * @return 0 成功，负值失败
 */
int lfs_port_format(void);

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

/******************************************************************************
 * @file     lfs_test.h
 * @brief    LittleFS 测试程序接口
 * @version  V1.0
 * @date     2026-06-24
 ******************************************************************************/
#ifndef _LFS_TEST_H_
#define _LFS_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  LittleFS 完整测试入口
 *         流程：挂载 -> 写彩条图片 -> 读回并显示到 LCD -> 数据校验
 */
void lfs_test_run(void);

#ifdef __cplusplus
}
#endif

#endif /* _LFS_TEST_H_ */

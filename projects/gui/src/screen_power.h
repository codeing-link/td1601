/******************************************************************************
 * @file     screen_power.h
 * @brief    息屏/唤醒接口声明
 ******************************************************************************/

#ifndef _SCREEN_POWER_H_
#define _SCREEN_POWER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 查询当前是否息屏（1=息屏，0=亮屏） */
uint8_t Screen_Is_Off(void);

/** 唤醒屏幕（恢复背光） */
void Screen_Wake(void);

#ifdef __cplusplus
}
#endif

#endif /* _SCREEN_POWER_H_ */

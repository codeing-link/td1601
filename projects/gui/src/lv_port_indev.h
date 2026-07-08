/******************************************************************************
 * @file     lv_port_indev.h
 * @brief    LVGL 输入设备适配层（桥接 CST816 电容触摸驱动）
 *
 * @version  V1.0
 * @date     2026-07-07
 ******************************************************************************/

#ifndef _LV_PORT_INDEV_H_
#define _LV_PORT_INDEV_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief      LV_Port_Indev_Init
 *             注册 LVGL 触摸输入设备，绑定 read 回调到 CST816 驱动。
 *             调用前须已完成 lv_init() 和 Touch_Init()。
 */
void LV_Port_Indev_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* _LV_PORT_INDEV_H_ */

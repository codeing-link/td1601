/******************************************************************************
 * @file     lv_port_disp.h
 * @brief    LVGL 显示适配层（桥接 ST77916 OSPI LCD 驱动）
 *
 * @version  V1.0
 * @date     2026-07-07
 ******************************************************************************/

#ifndef _LV_PORT_DISP_H_
#define _LV_PORT_DISP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief      LV_Port_Disp_Init
 *             注册 LVGL 显示设备，绑定 flush 回调到 ST77916 驱动。
 *             调用前须已完成 lv_init() 和 LCD_Init()。
 */
void LV_Port_Disp_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* _LV_PORT_DISP_H_ */

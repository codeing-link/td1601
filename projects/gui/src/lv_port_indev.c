/******************************************************************************
 * @file     lv_port_indev.c
 * @brief    LVGL 输入设备适配层（桥接 CST816 电容触摸驱动）
 *
 *  职责：
 *    - 创建 LVGL pointer 类型输入设备
 *    - 将 LVGL read 回调映射到 Touch_Poll
 *
 * @version  V1.0
 * @date     2026-07-07
 ******************************************************************************/

#include "lv_port_indev.h"
#include "lvgl.h"
#include "CST816.h"
#include "screen_power.h"

/* ===========================================================================
 * 触摸状态缓存（保持上次坐标，松手后仍上报最后位置）
 * =========================================================================*/
static int16_t s_last_x = 0;
static int16_t s_last_y = 0;

/* ===========================================================================
 * Read 回调
 * =========================================================================*/

/**
 * \brief      indev_read_cb
 *             LVGL 定期调用此函数获取触摸状态。
 * \param[in]  indev   输入设备句柄
 * \param[out] data    填入坐标和按下/释放状态
 */
static void indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    cst816_data_t tp;
    if (Touch_Poll(&tp)) {
        /* 息屏状态下，触摸唤醒屏幕，但不传递本次坐标给 UI */
        if (Screen_Is_Off()) {
            Screen_Wake();
            data->state = LV_INDEV_STATE_RELEASED;
            data->point.x = s_last_x;
            data->point.y = s_last_y;
            return;
        }
        s_last_x = (int16_t)tp.x;
        s_last_y = (int16_t)tp.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = s_last_x;
    data->point.y = s_last_y;
}

/* ===========================================================================
 * 对外接口
 * =========================================================================*/
void LV_Port_Indev_Init(void)
{
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, indev_read_cb);
}

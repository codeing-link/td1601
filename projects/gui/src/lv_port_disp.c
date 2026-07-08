/******************************************************************************
 * @file     lv_port_disp.c
 * @brief    LVGL 显示适配层（桥接 ST77916 OSPI LCD 驱动）
 *
 *  职责：
 *    - 创建 LVGL display 对象（360x360 RGB565）
 *    - 分配 draw buffer（40行 partial 模式）
 *    - 将 LVGL flush 回调映射到 LCD_addWindow
 *
 * @version  V1.0
 * @date     2026-07-07
 ******************************************************************************/

#include "lv_port_disp.h"
#include "lvgl.h"
#include "ST77916.h"

/* ===========================================================================
 * Draw Buffer（40 行 × 360 像素 × 2 字节/像素 = 28800 字节）
 * =========================================================================*/
#define DISP_BUF_LINES  (40)
#define DISP_BUF_SIZE   (EXAMPLE_LCD_WIDTH * DISP_BUF_LINES * sizeof(uint16_t))

static uint8_t s_disp_buf[DISP_BUF_SIZE];

/* ===========================================================================
 * Flush 回调
 * =========================================================================*/

/**
 * \brief      disp_flush_cb
 *             LVGL 渲染完成后调用此函数，将像素数据推送到 LCD 硬件。
 * \param[in]  disp    显示设备句柄
 * \param[in]  area    本次刷新的矩形区域
 * \param[in]  px_map  像素数据指针（RGB565）
 */
static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    LCD_addWindow((uint16_t)area->x1, (uint16_t)area->y1,
                  (uint16_t)area->x2, (uint16_t)area->y2,
                  (uint16_t *)px_map);
    lv_display_flush_ready(disp);
}

/* ===========================================================================
 * 对外接口
 * =========================================================================*/
void LV_Port_Disp_Init(void)
{
    lv_display_t *disp = lv_display_create(EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT);
    lv_display_set_buffers(disp, s_disp_buf, NULL, DISP_BUF_SIZE,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, disp_flush_cb);
}

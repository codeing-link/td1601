/******************************************************************************
 * @file     main.c
 * @brief    GUI 工程入口（LVGL v9 + ST77916 + CST816 + EEZ UI）
 *
 *  初始化顺序：board → LCD → Touch → LVGL → 移植层 → EEZ UI → 主循环
 *
 * @version  V3.0
 * @date     2026-07-07
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <drv/tick.h>
#include "board_config.h"
#include "board_init.h"
#include "ST77916.h"
#include "CST816.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui.h"
#include "cold_anim.h"

/* ===========================================================================
 * LVGL Tick 回调（映射到 CSI 系统 tick）
 * =========================================================================*/
static uint32_t my_tick_get(void)
{
    return (uint32_t)csi_tick_get_ms();
}

/* ===========================================================================
 * 主函数
 * =========================================================================*/
int main(void)
{
    board_init();

    printf("[main] LCD init...\n");
    LCD_Init();

    printf("[main] Touch init...\n");
    Touch_Init();

    printf("[main] LVGL init...\n");
    lv_init();
    lv_tick_set_cb(my_tick_get);

    LV_Port_Disp_Init();
    LV_Port_Indev_Init();

    printf("[main] EEZ UI init...\n");
    ui_init();
    Cold_Anim_Init();

    printf("[main] GUI running.\n");

    while (1) {
        ui_tick();
        lv_timer_handler();
        mdelay(5);
    }

    return 0;
}

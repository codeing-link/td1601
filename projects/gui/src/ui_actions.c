/******************************************************************************
 * @file     ui_actions.c
 * @brief    EEZ Studio Native Action 实现 + 息屏/唤醒管理
 *
 *  实现 EEZ UI 中绑定的 Native 类型用户动作回调。
 *  提供息屏标志供触摸输入层查询，实现触摸唤醒。
 *
 * @version  V1.1
 * @date     2026-07-07
 ******************************************************************************/

#include "eez/gui/src/ui/actions.h"
#include "ST77916.h"
#include "screen_power.h"

/* 息屏状态标志：0=亮屏，1=息屏 */
static volatile uint8_t s_screen_off = 0;

/**
 * \brief  查询当前是否处于息屏状态
 */
uint8_t Screen_Is_Off(void)
{
    return s_screen_off;
}

/**
 * \brief  唤醒屏幕（恢复背光）
 */
void Screen_Wake(void)
{
    if (s_screen_off) {
        s_screen_off = 0;
        Set_Backlight(LCD_Backlight ? LCD_Backlight : 70);
    }
}

/**
 * \brief      action_action_screen_off
 *             息屏动作：关闭 LCD 背光
 * \param[in]  e    LVGL 事件对象（未使用）
 */
void action_action_screen_off(lv_event_t *e)
{
    (void)e;
    s_screen_off = 1;
    Set_Backlight(0);
}

/**
 * \brief      action_light_add
 *             增加背光亮度，步进 10，上限 100
 * \param[in]  e    LVGL 事件对象（未使用）
 */
void action_light_add(lv_event_t *e)
{
    (void)e;
    uint8_t val = LCD_Backlight;
    if (val <= Backlight_MAX - 10) {
        val += 10;
    } else {
        val = Backlight_MAX;
    }
    Set_Backlight(val);
}

/**
 * \brief      action_light_dec
 *             降低背光亮度，步进 10，下限 10（不灭屏）
 * \param[in]  e    LVGL 事件对象（未使用）
 */
void action_light_dec(lv_event_t *e)
{
    (void)e;
    uint8_t val = LCD_Backlight;
    if (val >= 20) {
        val -= 10;
    } else {
        val = 10;
    }
    Set_Backlight(val);
}

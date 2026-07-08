/******************************************************************************
 * @file     cold_anim.c
 * @brief    制冷模式屏幕飞入动画实现
 *
 *  5 个按钮从屏幕底部中心以抛物线弧线飞入到各自最终位置：
 *    - 制冷 (50,140)、返回 (37,220)、加湿 (96,89)、速热 (160,55)、制热 (227,75)
 *  利用 LVGL lv_anim API，在 LV_EVENT_SCREEN_LOADED 时触发。
 *
 * @version  V1.0
 * @date     2026-07-07
 ******************************************************************************/

#include "cold_anim.h"
#include "lvgl.h"
#include "screens.h"

/* ===========================================================================
 * 配置参数
 * =========================================================================*/

#define COLD_BTN_COUNT   5
#define START_X          180     /* 起始 X：屏幕水平中心 */
#define START_Y          400     /* 起始 Y：屏幕底部外侧 */
#define ARC_HEIGHT       50      /* 弧线向上偏移峰值（像素） */
#define ANIM_DURATION    500     /* 单个按钮动画时长 ms */
#define ANIM_DELAY_STEP  80      /* 按钮间延迟递增 ms */

/* ===========================================================================
 * 内部数据
 * =========================================================================*/

/* 每个按钮的目标坐标 */
static const struct {
    int16_t x;
    int16_t y;
} s_targets[COLD_BTN_COUNT] = {
    {50,  140},   /* 制冷 */
    {37,  220},   /* 返回 */
    {96,   89},   /* 加湿 */
    {160,  55},   /* 速热 */
    {227,  75},   /* 制热 */
};

/* 动画上下文：记录每个按钮的对象指针和目标 */
typedef struct {
    lv_obj_t *obj;
    int16_t   target_x;
    int16_t   target_y;
} anim_ctx_t;

static anim_ctx_t s_ctx[COLD_BTN_COUNT];

/* ===========================================================================
 * 动画执行回调
 * =========================================================================*/

/**
 * \brief      anim_exec_cb
 *             根据动画进度值（0~ANIM_DURATION）计算抛物线位置并设置坐标
 * \param[in]  obj   控件指针（lv_anim 传入的 var）
 * \param[in]  v     当前进度值 0 ~ ANIM_DURATION
 */
static void anim_exec_cb(void *var, int32_t v)
{
    anim_ctx_t *ctx = (anim_ctx_t *)var;

    /* 归一化进度 t: 0.0 ~ 1.0（用定点 0~1024） */
    int32_t t = (v * 1024) / ANIM_DURATION;

    /* X：线性插值 */
    int32_t x = START_X + ((ctx->target_x - START_X) * t) / 1024;

    /* Y：线性插值 + 抛物线弧度偏移 */
    int32_t y_linear = START_Y + ((ctx->target_y - START_Y) * t) / 1024;
    /* arc = ARC_HEIGHT * 4 * t * (1-t)，峰值在 t=0.5 时为 ARC_HEIGHT */
    int32_t arc = (ARC_HEIGHT * 4 * t * (1024 - t)) / (1024 * 1024);
    int32_t y = y_linear - arc;

    lv_obj_set_pos(ctx->obj, (int16_t)x, (int16_t)y);
}

/* ===========================================================================
 * 屏幕加载事件回调
 * =========================================================================*/

static void cold_screen_loaded_cb(lv_event_t *e)
{
    (void)e;
    lv_obj_t *scr = objects.cold;

    /* 获取 5 个子控件引用 */
    for (int i = 0; i < COLD_BTN_COUNT; i++) {
        s_ctx[i].obj      = lv_obj_get_child(scr, i);
        s_ctx[i].target_x = s_targets[i].x;
        s_ctx[i].target_y = s_targets[i].y;

        /* 先将按钮移到起始位置（屏幕外） */
        lv_obj_set_pos(s_ctx[i].obj, START_X, START_Y);
    }

    /* 为每个按钮启动飞入动画 */
    for (int i = 0; i < COLD_BTN_COUNT; i++) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, &s_ctx[i]);
        lv_anim_set_exec_cb(&a, anim_exec_cb);
        lv_anim_set_values(&a, 0, ANIM_DURATION);
        lv_anim_set_duration(&a, ANIM_DURATION);
        lv_anim_set_delay(&a, i * ANIM_DELAY_STEP);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_start(&a);
    }
}

/* ===========================================================================
 * 公共接口
 * =========================================================================*/

void Cold_Anim_Init(void)
{
    lv_obj_add_event_cb(objects.cold, cold_screen_loaded_cb,
                        LV_EVENT_SCREEN_LOADED, NULL);
}

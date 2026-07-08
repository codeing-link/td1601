/******************************************************************************
 * @file     cold_anim.h
 * @brief    制冷模式屏幕飞入动画接口
 *
 *  提供 Cold_Anim_Init()，在 ui_init() 之后调用，注册屏幕加载事件，
 *  每次进入 cold 屏幕时 5 个按钮从底部以抛物线弧线飞入。
 *
 * @version  V1.0
 * @date     2026-07-07
 ******************************************************************************/

#ifndef COLD_ANIM_H
#define COLD_ANIM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief      Cold_Anim_Init
 *             初始化制冷屏幕飞入动画（注册 SCREEN_LOADED 事件）
 *             必须在 ui_init() 之后调用
 */
void Cold_Anim_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* COLD_ANIM_H */

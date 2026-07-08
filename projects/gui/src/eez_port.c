/******************************************************************************
 * @file     eez_port.c
 * @brief    EEZ Framework 平台适配
 *
 *  提供 eez-flow 运行时所需的平台相关函数实现：
 *    - osKernelGetTickCount → 映射到 CSI tick
 *
 * @version  V1.0
 * @date     2026-07-07
 ******************************************************************************/

#include <stdint.h>
#include <drv/tick.h>

/**
 * \brief      osKernelGetTickCount
 *             EEZ flow 引擎获取系统毫秒时间戳
 * \return     当前 tick 毫秒值
 */
uint32_t osKernelGetTickCount(void)
{
    return (uint32_t)csi_tick_get_ms();
}

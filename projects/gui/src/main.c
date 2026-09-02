

/******************************************************************************
 * @file     main.c
 * @brief    GUI 主程序：初始化 ST77916 QSPI 屏 + CST816 触摸，主循环处理图片更新和手势切换
 * @version  V1.0
 * @date     2026-06-17
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <drv/uart.h>
#include <drv/tick.h>
#include "board_config.h"
#include "board_init.h"
#include "ST77916.h"
#include "CST816.h"
#include "jpg_display.h"
#include "gui_uart.h"
#include "app_image_update.h"
#include "transport_uart.h"

int main(void)
{
    board_init();

    printf("Hello World!\n");

    /* 1. 初始化 LCD：复位 + QSPI + 命令表 + 背光 + 彩条自测 */
    LCD_Init();

    /* 2. 初始化触摸：I2C + 复位 + 读 ID + INT 中断 */
    Touch_Init();

#if GUI_UART_MODE == GUI_UART_MODE_DATA
    /* 3. DATA 模式：挂载 LittleFS，显示上次图片。下载模式由宏切换。 */
    (void)image_update_init(&g_uart_transport);
#else
    /* 3. LOG 模式：挂载 LFS → 按需写入内置 jpg → 流式解码全屏显示 */
    jpg_display_run();
#endif

    /* 4. 主循环：DATA 模式跑图片传输协议和上下滑切图；LOG 模式保留触摸坐标打印 */
#if GUI_UART_MODE == GUI_UART_MODE_DATA
#else
    cst816_data_t touch;
#endif

    while (1) {
#if GUI_UART_MODE == GUI_UART_MODE_DATA
        image_update_poll();
        mdelay(1);
#else
        if (Touch_Poll(&touch)) {
            printf("touch: x=%d y=%d points=%d\r\n", touch.x, touch.y, touch.points);
        }
        mdelay(10);
#endif
    }

    return 0;
}

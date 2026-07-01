

/******************************************************************************
 * @file     main.c
 * @brief    GUI 主程序：初始化 ST77916 QSPI 屏 + CST816 触摸，主循环读坐标
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

int main(void)
{
    board_init();

    printf("Hello World!\n");

    /* 1. 初始化 LCD：复位 + QSPI + 命令表 + 背光 + 彩条自测 */
    LCD_Init();

    /* 2. 初始化触摸：I2C + 复位 + 读 ID + INT 中断 */
    Touch_Init();

    /* 3. JPEG 解码显示：挂载 LFS → 按需写入 jpg → 流式解码全屏显示 */
    jpg_display_run();

    /* 4. 主循环：DATA 模式做串口回环测试；LOG 模式保留触摸坐标打印 */
#if GUI_UART_MODE == GUI_UART_MODE_DATA
    uint8_t uart_echo_buf[128];
#else
    cst816_data_t touch;
#endif

    while (1) {
#if GUI_UART_MODE == GUI_UART_MODE_DATA
        /* 上位机下发的数据已经由中断放入环形缓冲区，这里读出后原样回发。 */
        uint32_t rx_len = gui_uart_read(uart_echo_buf, sizeof(uart_echo_buf));

        if (rx_len > 0U) {
            (void)gui_uart_send(uart_echo_buf, rx_len);
        }

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

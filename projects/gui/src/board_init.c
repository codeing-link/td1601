

/******************************************************************************
 * @file     main.c
 * @brief    hello world
 * @version  V1.0
 * @date     17. Jan 2018
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <drv/uart.h>
#include "board_config.h"
#include "console/sys_console.h"
#include "gui_uart.h"

#if GUI_UART_MODE == GUI_UART_MODE_LOG
sys_console_t console;
#endif

extern void __ChipInitHandler(void);

void board_init(void)
{
    __ChipInitHandler();

#if GUI_UART_MODE == GUI_UART_MODE_LOG
    /* 默认日志模式：初始化 console 组件，printf 通过 UART0 输出调试日志。 */
    console.uart_id = (uint32_t)CONSOLE_IDX;
    console.baudrate = 921600;
    console.tx.port = CONSOLE_TXD_PORT;
    console.tx.pin = CONSOLE_TXD;
    console.tx.func = CONSOLE_TXD_FUNC;
    console.rx.port = CONSOLE_RXD_PORT;
    console.rx.pin = CONSOLE_RXD;
    console.rx.func = CONSOLE_RXD_FUNC;
    console.uart = NULL;

    console_init(&console);
#elif GUI_UART_MODE == GUI_UART_MODE_DATA
    /*
     * 普通串口模式：board_init 只做芯片初始化。
     * UART0 由 transport_uart 在 image_update_init() 中初始化，便于未来切换 BLE transport。
     */
#else
#error "Unsupported GUI_UART_MODE"
#endif
}

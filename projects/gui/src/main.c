

/******************************************************************************
 * @file     main.c
 * @brief    hello world
 * @version  V1.0
 * @date     03. April 2020
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <drv/uart.h>
#include "board_config.h"
#include "board_init.h"

/* GPIO 翻转示例（定义在 gpio_toggle.c），内部为死循环，不会返回 */
extern void gpio_toggle(void);

int main(void)
{
    board_init();

    printf("Hello World!\n");

    /* 启动 PA24 GPIO 翻转，驱动 LED 闪烁 */
    gpio_toggle();

    return 0;
}

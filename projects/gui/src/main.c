

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

int main(void)
{
    board_init();

    printf("Hello World!\n");

    /* 1. 初始化 LCD：复位 + QSPI + 命令表 + 背光 + 彩条自测 */
    LCD_Init();

    /* 2. 初始化触摸：I2C + 复位 + 读 ID + INT 中断 */
    Touch_Init();

    /* 3. 主循环：有触摸事件则读坐标并打印 */
    cst816_data_t touch;
    while (1) {
        if (Touch_Poll(&touch)) {
            printf("touch: x=%d y=%d points=%d\r\n", touch.x, touch.y, touch.points);
        }
        mdelay(10);
    }

    return 0;
}

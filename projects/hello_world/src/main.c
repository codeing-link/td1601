

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

int main(void)
{
    board_init();

    printf("Hello World!\n");
    CSI_EXAMPLE_RESULT(1);

    return 0;
}

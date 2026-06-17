


/******************************************************************************
 * @file     main.c
 * @brief    CSI Source File for main
 * @version  V1.0
 * @date     12. June 2018
 ******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include "drv/tick.h"
#include <csi_kernel.h>
#include "board_init.h"

#define EXAMPLE_PRIO    5
#define EXAMPLE_TASK_STK_SIZE 1024

k_task_handle_t example_main_task;

void example_main(void)
{
    board_init();

    printf("Hello World!\n");
    CSI_EXAMPLE_RESULT(0);
	csi_kernel_task_del(csi_kernel_task_get_cur());

}

int main(void)
{
    csi_tick_init();

    csi_kernel_init();

    csi_kernel_task_new((k_task_entry_t)example_main, "example_main",
                        0, EXAMPLE_PRIO, 0, 0, EXAMPLE_TASK_STK_SIZE, &example_main_task);

    csi_kernel_start();

    return 0;
}




/******************************************************************************
 * @file     main.c
 * @brief    CSI Source File for main
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <csi_config.h>
#include <stdint.h>
#include <drv/tick.h>
#include <board_init.h>

extern void benchmark_dhry_main(void);

int benchmark_test(void)
{
    benchmark_dhry_main();
    return 0;
}

int main(void)
{
    uint32_t ret = 0;
    board_init();
    ret = benchmark_test();

    return ret;
}

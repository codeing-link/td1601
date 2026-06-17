


/******************************************************************************
 * @file     example_efuse.c
 * @brief    the main function for the efuse driver
 * @version  V1.0
 * @date     22. Mar 2019
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <csi_config.h>
#include <soc.h>
#include <drv/efuse.h>
#include "board_init.h"

static csi_efuse_t efuse;
static void example_srand(void)
{
    csi_efuse_info_t info;
    uint32_t write_fail = 0;
    uint32_t dest_data = 0;
    csi_error_t ret;

    ret = csi_efuse_init(&efuse, 0);
    if (ret != CSI_OK) {
        printf("csi_efuse_init failed [%d] \n", ret);
        return;
    }

    csi_efuse_get_info(&efuse, &info);

    ret = csi_efuse_read(&efuse, 0, &dest_data, 2);
    if (ret != 2) {
        printf("csi_efuse_read failed [%d] \n", ret);
        return;
    }
    csi_efuse_uninit(&efuse);
    srand((dest_data & 0xFFFF) | ((csi_tick_get_ms() & 0xFFFF)<< 16));
    printf("rand num %x %x %x\n", rand(), rand(), rand());
}


int main(void)
{
    board_init();
    example_srand();
    return 0;
}

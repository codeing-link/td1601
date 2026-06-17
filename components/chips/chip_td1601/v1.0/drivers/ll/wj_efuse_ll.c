/*
 * Copyright (C) 2017-2019 NStar-Chip Holding Limited
 */

/******************************************************************************
 * @file     wj_efuse_ll.c
 * @brief    wj efuse ll driver
 * @version  V1.0
 * @date     17. July 2020
 ******************************************************************************/
#include <stdint.h>
#include <drv/tick.h>
#include "wj_efuse_ll.h"

int32_t wj_efuse_program(wj_efuse_regs_t *efuse_base, uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint8_t efuse_data = *data;
    uint32_t time_count = 0;
    efuse_base->WP_A = EFUSEC_WP_A_MAGIC;
    efuse_base->WP_B = EFUSEC_WP_B_MAGIC;
    efuse_base->AVD_PAD = 0xe09;

    for (i = 0; i < size; i++) {
        for (int j = 0; j < 8; j++) {
            time_count = 0;

            while (!(efuse_base->IDLE & EFUSEC_STA_BUSY)) {
                time_count++;

                if (time_count > 0x100000) {
                    return -1;
                }
            }

            if (efuse_data & 0x1) {
                efuse_base->ADDR = (j * 128 + addr);
                efuse_base->WEN = EFUSEC_CTRL_PROGRAM_ENABLE;
                time_count = 0;

                while (!(efuse_base->IDLE & EFUSEC_STA_BUSY)) {
                    time_count++;

                    if (time_count > 0x100000) {
                        return -1;
                    }
                }
            }

            efuse_data = efuse_data >> 1;
        }

        data++;
        efuse_data = *data;
        addr += 1;
    }

    efuse_base->WP_CLR = 1;
    efuse_base->AVD_PAD = 0x0;
    return i;
}

int32_t wj_efuse_read(wj_efuse_regs_t *efuse_base, uint32_t addr, uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t time_count = 0;

    for (i = 0; i < size; i++) {
        time_count = 0;

        while (!(efuse_base->IDLE & 0x1)) {
            time_count++;

            if (time_count > 0x100000) {
                return -1;
            }
        }

        time_count = 0;
        efuse_base->ADDR = addr;
        efuse_base->REN = EFUSEC_CTRL_READ_ENABLE;

        while (!(efuse_base->IDLE & 0x1)) {
            time_count++;

            if (time_count > 0x100000) {
                return -1;
            }
        }

        *data = efuse_base->RDATA;
        data++;
        addr += 1;
    }

    return i;
}




/******************************************************************************
 * @file     dw_ospi_ll.c
 * @brief
 * @version
 * @date     2020-02-10
 ******************************************************************************/

#include "dw_ospi_ll.h"

void dw_ospi_config_sclk_clock(dw_ospi_regs_t *ospi_base, uint32_t clock_in, uint32_t clock_out)
{
    uint32_t div;

    div = (clock_in * 10U) / clock_out;
    if ((div % 10U) >= 5U) {
        div = (div / 10U) + 1U;
    } else {
        div = div / 10U;
    }
    div = (div < 2) ? 2U : div;
    div = (div > 65534U) ? 65534U : div;
    ospi_base->BAUDR &= DW_OSPI_BAUDR_SCKDV_Msk;
    ospi_base->BAUDR = div;
}

uint32_t dw_ospi_get_sclk_clock_div(dw_ospi_regs_t *ospi_base)
{
    return ospi_base->BAUDR;
}

uint32_t dw_ospi_get_data_frame_len(dw_ospi_regs_t *ospi_base)
{
    uint32_t len = ospi_base->CTRLR0 & DW_OSPI_CTRLR0_DFS_Msk;
    len >>= DW_OSPI_CTRLR0_DFS_Pos;
    len++;
    return len;
}

void dw_ospi_config_data_frame_len(dw_ospi_regs_t *ospi_base, uint32_t size)
{
    uint32_t temp;

    if ((size >= 4U) & (size <= 16U)) {
        temp = ospi_base->CTRLR0;
        temp &= ~DW_OSPI_CTRLR0_DFS_Msk;
        temp |= ((size - 1U) << DW_OSPI_CTRLR0_DFS_Pos);
        ospi_base->CTRLR0 = temp;
    }
}

void dw_ospi_reset_regs(dw_ospi_regs_t *ospi_base)
{
    ospi_base->CTRLR0   = 7U;
    ospi_base->CTRLR1   = 0U;
    ospi_base->SSIENR   = 0U;
    ospi_base->SER      = 0U;
    ospi_base->BAUDR    = 0U;
    ospi_base->TXFTLR   = 0U;
    ospi_base->RXFTLR   = 0U;
    ospi_base->IMR      = 0U;
}

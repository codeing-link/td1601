

/******************************************************************************
 * @file     dw_ospi.c
 * @brief
 * @version
 * @date     2020-02-11
 ******************************************************************************/

#include <string.h>

#include <drv/ospi.h>
#include <drv/irq.h>
#include <drv/tick.h>
#include <drv/porting.h>

#include "dw_ospi_ll.h"

#define DW_MAX_OSPI_TXFIFO_LV       0x20U
#define DW_MAX_OSPI_RXFIFO_LV       0x20U
#define DW_DEFAULT_OSPI_TXFIFO_LV   0x8U
#define DW_DEFAULT_OSPI_RXFIFO_LV   0x10U

#define DW_DEFAULT_TRANSCATION_TIMEOUT 200U

#define IS_8BIT_FRAME_LEN(spi_base)   (dw_ospi_get_data_frame_len(spi_base) <= 8U)
#define IS_16BIT_FRAME_LEN(spi_base)  (( dw_ospi_get_data_frame_len(spi_base) > 8U ) && ( dw_ospi_get_data_frame_len(spi_base) <= 16U ))
#define IS_32BIT_FRAME_LEN(spi_base)  (( dw_ospi_get_data_frame_len(spi_base) > 16U ) && ( dw_ospi_get_data_frame_len(spi_base) <= 32U ))

#define DW_OSPI_HS_OFFSET               (2U)

#define DW_OSPI_SET_REG_IDX_MASTER(ospi) (ospi->priv = (void *)0U)
#define DW_OSPI_SET_REG_IDX_SLAVE(ospi)  (ospi->priv = (void *)1U)
#define DW_OSPI_SET_REG_IDX_COMMON(ospi) (ospi->priv = (void *)1U)
#define DW_OSPI_GET_REG_IDX(ospi)        ((unsigned long)ospi->priv)
#define IS_DW_OSPI_IDX_MASTER(ospi)      ((unsigned long)ospi->priv == 0U)
#define IS_DW_OSPI_IDX_SLAVE(ospi)       ((unsigned long)ospi->priv == 1U)

extern uint16_t spi_tx_hs_num[];
extern uint16_t spi_rx_hs_num[];

static csi_error_t dw_ospi_send_intr(csi_ospi_t *ospi, const void *data, uint32_t size);
static csi_error_t dw_ospi_receive_intr(csi_ospi_t *ospi, void *data, uint32_t size);
static csi_error_t dw_ospi_send_receive_intr(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t num);

static dw_ospi_regs_t *dw_get_reg_base(csi_ospi_t *ospi)
{
    dw_ospi_regs_t *spi_base;
    unsigned long reg_base;
    uint32_t idx = 0U;

    idx = DW_OSPI_GET_REG_IDX(ospi);
    reg_base = HANDLE_REG_BASE(ospi) + (idx * (uint32_t)0x100U);
    spi_base = (dw_ospi_regs_t *)reg_base;
    return spi_base;
}

static uint16_t dw_ospi_get_hs_num(csi_ospi_t *ospi, uint16_t *hs_num)
{
    uint16_t num = 0U;

    if (IS_DW_OSPI_IDX_MASTER(ospi)) {
        num = hs_num[( ospi->dev.idx * DW_OSPI_HS_OFFSET )];
    } else {
        num = hs_num[( ospi->dev.idx * DW_OSPI_HS_OFFSET ) + 1U];
    }

    return num;
}

static uint8_t find_max_prime_num(uint32_t num, uint32_t limit)
{
    uint32_t i, min;

    min = (num > limit) ? limit : num;
    i = min;

    while (i > 0U) {
        if (!(num % i)) {
            break;
        }

        i--;
    }

    if (i == 0U) {
        i = min;
    }

    return (uint8_t)i;
}

static uint8_t find_group_len(uint32_t size, uint8_t width)
{
    uint32_t prime_num;
    uint32_t limit;

    limit = 8U;

    do {
        prime_num = find_max_prime_num(size, limit);
        limit = prime_num - 1U;
    } while ((prime_num % width) != 0U);

    return (uint8_t)prime_num;
}


static csi_error_t wait_ready_until_timeout(csi_ospi_t *ospi, uint32_t timeout)
{
    uint32_t timestart = 0U;
    csi_error_t    ret = CSI_OK;
    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    timestart = csi_tick_get_ms();

    while (dw_ospi_get_status(spi_base) & DW_OSPI_SR_BUSY) {
        if ((csi_tick_get_ms() - timestart) > timeout) {
            ret = CSI_TIMEOUT;
            break;
        }
    }

    return ret;
}

static void process_end_transcation(csi_ospi_t *ospi)
{
    uint32_t mode;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);
    mode = dw_ospi_get_transfer_mode(spi_base);

    /* process end of transmit */
    if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) ==  DW_OSPI_CTRLR0_TMOD_TX) {
        if (ospi->tx_size == 0U) {
            if ((dw_ospi_get_status(spi_base) & DW_OSPI_SR_BUSY) == 0U) {
                dw_ospi_disable_tx_empty_irq(spi_base);
                dw_ospi_config_tx_fifo_threshold(spi_base, 0U);
                ospi->state.writeable = 1U;

                if (ospi->callback) {
                    ospi->callback(ospi, OSPI_EVENT_SEND_COMPLETE, ospi->arg);
                }

            }
        }
    }

    /* process end of receive */
    else if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) == DW_OSPI_CTRLR0_TMOD_RX) {
        if (ospi->rx_size == 0U) {
            dw_ospi_disable_rx_fifo_full_irq(spi_base);
            dw_ospi_config_rx_data_len(spi_base, 0U);
            dw_ospi_config_rx_fifo_threshold(spi_base, 0U);

            if (ospi->callback) {
                ospi->callback(ospi, OSPI_EVENT_RECEIVE_COMPLETE, ospi->arg);
            }

            ospi->state.readable = 1U;
        }
    }

    /* process end of transmit & receive */
    else if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) == DW_OSPI_CTRLR0_TMOD_TX_RX) {
        if ((ospi->rx_size == 0U) && (ospi->tx_size == 0U)) {
            dw_ospi_disable_tx_empty_irq(spi_base);
            dw_ospi_disable_rx_fifo_full_irq(spi_base);
            dw_ospi_config_tx_fifo_threshold(spi_base, 0U);
            dw_ospi_config_rx_fifo_threshold(spi_base, 0U);
            ospi->state.readable  = 1U;
            ospi->state.writeable = 1U;

            if (ospi->callback) {
                ospi->callback(ospi, OSPI_EVENT_SEND_RECEIVE_COMPLETE, ospi->arg);
            }
        } else if (ospi->tx_size == 0U) {
            // reduce interrupt times
            dw_ospi_disable_tx_empty_irq(spi_base);
        }
    }
}

static void spi_intr_tx_fifo_empty(csi_ospi_t *ospi)
{
    uint32_t remain_fifo;
    uint32_t value;
    uint32_t frame_len;
    uint8_t  *tx_data;
    uint32_t tx_size;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    remain_fifo = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(spi_base);
    frame_len   = dw_ospi_get_data_frame_len(spi_base);

    /* process end of transcation */
    process_end_transcation(ospi);

    /* transfer loop */
    tx_data = ospi->tx_data;
    tx_size = ospi->tx_size;

    if (frame_len <= 8U) {
        while (tx_size && remain_fifo) {
            /* process 4~8bit frame len */
            value = (uint32_t)(*(uint8_t *)tx_data);
            tx_data += sizeof(uint8_t);
            dw_ospi_transmit_data(spi_base, value);
            remain_fifo--;
            tx_size--;
        }
    } else if ((frame_len > 8U) && (frame_len <= 16U)) {
        while (tx_size && remain_fifo) {
            /* process 8~16bit frame len */
            value = (uint32_t)(*(uint16_t *)tx_data);
            tx_data += sizeof(uint16_t);
            dw_ospi_transmit_data(spi_base, value);
            remain_fifo--;
            tx_size--;
        }
    } else if ((frame_len > 16U) && (frame_len <= 32U)) {
        while (tx_size && remain_fifo) {
            /* process 8~16bit frame len */
            value = (uint32_t)(*(uint32_t *)tx_data);
            tx_data += sizeof(uint32_t);
            dw_ospi_transmit_data(spi_base, value);
            remain_fifo--;
            tx_size--;
        }
    }

    ospi->tx_data = tx_data;
    ospi->tx_size = tx_size;
}

static void spi_intr_rx_fifo_full(csi_ospi_t *ospi)
{
    uint32_t fifo_size;
    uint32_t frame_len;
    uint8_t  *rx_data;
    uint32_t rx_size;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);
    fifo_size = dw_ospi_get_rx_fifo_level(spi_base);
    frame_len = dw_ospi_get_data_frame_len(spi_base);
    rx_data = ospi->rx_data;
    rx_size = ospi->rx_size;

    /* transfer loop */
    if (frame_len <= 8U) {
        while (rx_size && fifo_size) {
            *(uint8_t *)rx_data = (uint8_t)dw_ospi_receive_data(spi_base);
            rx_data += sizeof(uint8_t);
            fifo_size--;
            rx_size--;
        }
    } else if ((frame_len > 8U) && (frame_len <= 16U)) {
        while (rx_size && fifo_size) {
            *(uint16_t *)rx_data = (uint16_t)dw_ospi_receive_data(spi_base);
            rx_data += sizeof(uint16_t);
            fifo_size--;
            rx_size--;
        }
    } else if ((frame_len > 16U) && (frame_len <= 32U)) {
        while (rx_size && fifo_size) {
            *(uint32_t *)rx_data = (uint32_t)dw_ospi_receive_data(spi_base);
            rx_data += sizeof(uint32_t);
            fifo_size--;
            rx_size--;
        }
    }
    /* update rx fifo threshold when remain size less then default threshold*/
    if ((rx_size < (DW_DEFAULT_OSPI_RXFIFO_LV + 1U)) && (rx_size > 0U)) {
        dw_ospi_config_rx_fifo_threshold(spi_base, rx_size - 1U);
    }

    ospi->rx_data = rx_data;
    ospi->rx_size = rx_size;

    /* process end of transcation */
    process_end_transcation(ospi);

}

static void dw_ospi_irqhandler(void *args)
{
    uint32_t status;
    csi_ospi_t *ospi = (csi_ospi_t *)args;
    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    status = dw_ospi_get_interrupt_status(spi_base);

    /* process receive fifo full interrupt */
    if (status & DW_OSPI_ISR_RXFIS) {
        spi_intr_rx_fifo_full(ospi);
    }


    /* process transmit fifo empty interrupt */
    if (status & DW_OSPI_ISR_TXEIS) {
        spi_intr_tx_fifo_empty(ospi);
    }

    /* process Multi-Master contention interrupt */
    if (status & DW_OSPI_ISR_MSTIS) {
        dw_ospi_clr_multi_master_irq(spi_base);

        if (ospi->callback) {
            ospi->callback(ospi, OSPI_EVENT_ERROR, ospi->arg);
        }
    }

    /* process receive fifo overflow interrupt */
    if (status & DW_OSPI_ISR_RXOIS) {
        dw_ospi_clr_rx_fifo_overflow_irq(spi_base);

        if (ospi->callback) {
            ospi->callback(ospi, OSPI_EVENT_ERROR_OVERFLOW, ospi->arg);
        }
    }

    /* process transmit fifo overflow interrupt */
    if (status & DW_OSPI_ISR_TXOIS) {
        dw_ospi_clr_tx_fifo_overflow_irq(spi_base);

        if (ospi->callback) {
            ospi->callback(ospi, OSPI_EVENT_ERROR_OVERFLOW, ospi->arg);
        }
    }

    /* process receive fifo underflow interrupt */
    if (status & DW_OSPI_ISR_RXUIS) {
        dw_ospi_clr_rx_fifo_underflow_irq(spi_base);

        if (ospi->callback) {
            ospi->callback(ospi, OSPI_EVENT_ERROR_UNDERFLOW, ospi->arg);
        }
    }
}

static void dw_ospi_dma_event_cb(csi_dma_ch_t *dma, csi_dma_event_t event, void *arg)
{
    dw_ospi_regs_t *spi_base;
    csi_ospi_t *ospi = (csi_ospi_t *)dma->parent;
    uint32_t mode;

    spi_base = dw_get_reg_base(ospi);
    mode = dw_ospi_get_transfer_mode(spi_base);

    if (event == DMA_EVENT_TRANSFER_DONE) {
        /* process end of transmit */
        if ((ospi->tx_dma != NULL) && (ospi->tx_dma->ch_id == dma->ch_id)) {
            csi_dma_ch_stop(dma);
            dw_ospi_disable_tx_dma(spi_base);

            if (wait_ready_until_timeout(ospi, DW_DEFAULT_TRANSCATION_TIMEOUT) == CSI_OK) {

                ospi->state.writeable = 1U;
                ospi->tx_size = 0U;

                if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) == DW_OSPI_CTRLR0_TMOD_TX) {
                    dw_ospi_config_dma_tx_data_level(spi_base, 0U);

                    if (ospi->callback) {
                        ospi->callback(ospi, OSPI_EVENT_SEND_COMPLETE, ospi->arg);
                    }
                } else {
                    if (ospi->state.readable == 1U) {
                        ospi->callback(ospi, OSPI_EVENT_SEND_RECEIVE_COMPLETE, ospi->arg);
                    }
                }
            }
        } else if ((ospi->rx_dma != NULL) && (ospi->rx_dma->ch_id == dma->ch_id)) {
            csi_dma_ch_stop(dma);
            dw_ospi_disable_rx_dma(spi_base);
            dw_ospi_config_dma_rx_data_level(spi_base, 0U);
            dw_ospi_config_rx_data_len(spi_base, 0U);

            ospi->state.readable = 1U;
            ospi->rx_size = 0U;

            if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) == DW_OSPI_CTRLR0_TMOD_RX) {
                if (ospi->callback) {
                    ospi->callback(ospi, OSPI_EVENT_RECEIVE_COMPLETE, ospi->arg);
                }
            } else {
                if (ospi->state.writeable == 1U) {
                    ospi->callback(ospi, OSPI_EVENT_SEND_RECEIVE_COMPLETE, ospi->arg);
                }
            }
        }
    }
}

csi_error_t csi_ospi_init(csi_ospi_t *ospi, uint32_t idx)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    dw_ospi_regs_t *spi_base;
    csi_error_t ret = CSI_OK;

    if (target_get(DEV_DW_OSPI_TAG, idx, &ospi->dev) != CSI_OK) {
        ret = CSI_ERROR;
    } else {
        ospi->state.writeable = 1U;
        ospi->state.readable  = 1U;
        ospi->state.error     = 0U;
        ospi->send            = NULL;
        ospi->receive         = NULL;
        ospi->send_receive    = NULL;
        ospi->rx_dma          = NULL;
        ospi->tx_dma          = NULL;
        ospi->rx_data         = NULL;
        ospi->tx_data         = NULL;
        ospi->callback        = NULL;
        ospi->arg             = NULL;
        ospi->priv            = 0;

        DW_OSPI_SET_REG_IDX_MASTER(ospi);
        spi_base = dw_get_reg_base(ospi);
        dw_ospi_disable_all_irq(spi_base);
        dw_ospi_disable(spi_base);
        dw_ospi_disable_slave_select_toggle(spi_base);

        DW_OSPI_SET_REG_IDX_SLAVE(ospi);
        spi_base = dw_get_reg_base(ospi);
        dw_ospi_disable_all_irq(spi_base);
        dw_ospi_disable(spi_base);
        dw_ospi_disable_slave_select_toggle(spi_base);
    }

    return ret;
}

void csi_ospi_uninit(csi_ospi_t *ospi)
{
    CSI_PARAM_CHK_NORETVAL(ospi);

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    /* reset all registers */
    DW_OSPI_SET_REG_IDX_MASTER(ospi);
    spi_base = dw_get_reg_base(ospi);
    dw_ospi_reset_regs(spi_base);

    DW_OSPI_SET_REG_IDX_SLAVE(ospi);
    spi_base = dw_get_reg_base(ospi);
    dw_ospi_reset_regs(spi_base);

    /* unregister irq */
    csi_irq_disable((uint32_t)ospi->dev.irq_num);
    csi_irq_detach((uint32_t)ospi->dev.irq_num);
}

csi_error_t csi_ospi_attach_callback(csi_ospi_t *ospi, void *callback, void *arg)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);
    CSI_PARAM_CHK(callback, CSI_ERROR);

    ospi->callback     = callback;
    ospi->arg          = arg;
    ospi->send         = NULL;
    ospi->receive      = NULL;
    ospi->send_receive = NULL;

    return CSI_OK;
}


void csi_ospi_detach_callback(csi_ospi_t *ospi)
{
    CSI_PARAM_CHK_NORETVAL(ospi);

    ospi->callback     = NULL;
    ospi->arg          = NULL;
    ospi->send         = NULL;
    ospi->receive      = NULL;
    ospi->send_receive = NULL;
}


csi_error_t csi_ospi_mode(csi_ospi_t *ospi, csi_ospi_mode_t mode)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);
    csi_error_t   ret = CSI_OK;
    return ret;
}

csi_error_t csi_ospi_line_mode(csi_ospi_t *ospi, csi_ospi_line_mode_t mode)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    dw_ospi_regs_t *spi_base;
    csi_error_t   ret = CSI_OK;

    spi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(spi_base);
    /* configure ospi mode */
    switch (mode) {
        case OSPI_LINE_SINGLE:
            dw_ospi_set_spi_frame_format_std_mode(spi_base);
            dw_ospi_spi_ctrl0_set_trans_type_inst_addr(spi_base);
            break;
        case OSPI_LINE_DUAL:
            dw_ospi_set_spi_frame_format_dual_mode(spi_base);
            dw_ospi_spi_ctrl0_set_trans_type_spifrf(spi_base);
            break;
        case OSPI_LINE_QUAD:
            dw_ospi_set_spi_frame_format_quad_mode(spi_base);
            dw_ospi_spi_ctrl0_set_trans_type_spifrf(spi_base);
            break;
        case OSPI_LINE_OCTAL:
            dw_ospi_set_spi_frame_format_octal_mode(spi_base);
            dw_ospi_spi_ctrl0_set_trans_type_spifrf(spi_base);
            break;

        default:
            ret = CSI_ERROR;
            break;
    }

    return ret;
}

csi_error_t csi_ospi_transfer_mode(csi_ospi_t *ospi, csi_ospi_transfer_mode_t mode)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    dw_ospi_regs_t *spi_base;
    csi_error_t   ret = CSI_OK;

    spi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(spi_base);
    /* configure ospi mode */
    switch (mode) {
        case OSPI_TRANSFER_SEND_RECEIVE:
            dw_ospi_set_tx_rx_mode(spi_base);
            break;
        case OSPI_TRANSFER_SEND_ONLY:
            dw_ospi_set_tx_mode(spi_base);
            break;
        case OSPI_TRANSFER_RECEIVE_ONLY:
            dw_ospi_set_rx_mode(spi_base);
            break;
        case OSPI_TRANSFER_EEPROM_READ:
            dw_ospi_set_eeprom_mode(spi_base);
            break;

        default:
            ret = CSI_ERROR;
            break;
    }

    return ret;
}

csi_error_t csi_ospi_cp_format(csi_ospi_t *ospi, csi_ospi_cp_format_t format)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    dw_ospi_regs_t *spi_base;
    csi_error_t   ret = CSI_OK;

    spi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(spi_base);
    /* configure ospi format */
    switch (format) {
        case OSPI_FORMAT_CPOL0_CPHA0:
            dw_ospi_set_cpol0(spi_base);
            dw_ospi_set_cpha0(spi_base);
            break;

        case OSPI_FORMAT_CPOL0_CPHA1:
            dw_ospi_set_cpol0(spi_base);
            dw_ospi_set_cpha1(spi_base);
            break;

        case OSPI_FORMAT_CPOL1_CPHA0:
            dw_ospi_set_cpol1(spi_base);
            dw_ospi_set_cpha0(spi_base);
            break;

        case OSPI_FORMAT_CPOL1_CPHA1:
            dw_ospi_set_cpol1(spi_base);
            dw_ospi_set_cpha1(spi_base);
            break;

        default:
            ret = CSI_ERROR;
            break;
    }

    return ret;
}

uint32_t csi_ospi_baud(csi_ospi_t *ospi, uint32_t baud)
{
    CSI_PARAM_CHK(ospi,  CSI_ERROR);
    CSI_PARAM_CHK(baud, CSI_ERROR);

    dw_ospi_regs_t *spi_base;
    uint32_t div;
    uint32_t freq = 0U;

    spi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(spi_base);
    dw_ospi_config_sclk_clock(spi_base, soc_get_spi_freq((uint32_t)ospi->dev.idx), baud);

    div = dw_ospi_get_sclk_clock_div(spi_base);

    if (div > 0U) {
        freq =  soc_get_spi_freq((uint32_t)ospi->dev.idx) / div;
    }

    return freq;

}

csi_error_t csi_ospi_frame_len(csi_ospi_t *ospi, csi_ospi_frame_len_t length)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    dw_ospi_regs_t *spi_base;
    csi_error_t ret = CSI_OK;

    if ((length < OSPI_FRAME_LEN_4) || (length > OSPI_FRAME_LEN_32)) {
        ret = CSI_ERROR;
    } else {
        spi_base = dw_get_reg_base(ospi);
        dw_ospi_disable(spi_base);
        /* configura data frame width*/
        dw_ospi_config_data_frame_len(spi_base, (uint32_t)length);
    }

    return ret;
}

int32_t csi_ospi_send(csi_ospi_t *ospi, const void *data, uint32_t size, uint32_t timeout)
{
    CSI_PARAM_CHK(ospi,  CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    uint32_t value;
    uint32_t timestart;
    uint32_t count = 0U;
    uint8_t *tx_data;
    uint32_t current_size;
    int32_t  ret   = CSI_OK;
    dw_ospi_regs_t *spi_base;

    spi_base  = dw_get_reg_base(ospi);

    do {
        if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
            ret = CSI_BUSY;
            break;
        }

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            if (size % sizeof(uint32_t)) {
                ret = CSI_ERROR;
                break;
            }
        }

        if (IS_16BIT_FRAME_LEN(spi_base)) {
            if (size % sizeof(uint16_t)) {
                ret = CSI_ERROR;
                break;
            }
        }

        timestart = csi_tick_get_ms();
        ospi->state.writeable = 0U;
        tx_data = (uint8_t *)data;

        // Convert byte to nums
        if (IS_32BIT_FRAME_LEN(spi_base)) {
            size /= 4U;
        }

        if (IS_16BIT_FRAME_LEN(spi_base)) {
            size /= 2U;
        }

        /* set tx mode */
        dw_ospi_disable(spi_base);
        dw_ospi_set_tx_mode(spi_base);
        dw_ospi_config_tx_fifo_threshold(spi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
        dw_ospi_enable(spi_base);

        /* transfer loop */
        if (IS_8BIT_FRAME_LEN(spi_base)) {
            while (size > 0U) {
                current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(spi_base);

                if (current_size > size) {
                    current_size = size;

                }

                while (current_size--) {
                    value = (uint32_t)(*(uint8_t *)tx_data);
                    dw_ospi_transmit_data(spi_base, value);
                    tx_data += 1;
                    count += 1U;
                    size--;
                }

                if ((csi_tick_get_ms() - timestart) > timeout) {
                    break;
                }
            }
        }

        if (IS_16BIT_FRAME_LEN(spi_base)) {
            while (size > 0U) {
                current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(spi_base);

                if (current_size > size) {
                    current_size = size;

                }

                while (current_size--) {
                    value = (uint32_t)(*(uint16_t *)tx_data);
                    dw_ospi_transmit_data(spi_base, value);
                    tx_data += 2;
                    count += 2U;
                    size--;
                }

                if ((csi_tick_get_ms() - timestart) > timeout) {
                    break;
                }
            }
        }

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            while (size > 0U) {
                current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(spi_base);

                if (current_size > size) {
                    current_size = size;

                }

                while (current_size--) {
                    value = (uint32_t)(*(uint32_t *)tx_data);
                    dw_ospi_transmit_data(spi_base, value);
                    tx_data += 4;
                    count += 4U;
                    size--;
                }

                if ((csi_tick_get_ms() - timestart) > timeout) {
                    break;
                }
            }
        }

        // Check SR.TFE is necessary when tx size = 1, because SR.BUSY has some delay before be vaild
        while (!(dw_ospi_get_status(spi_base) & DW_OSPI_SR_TFE)) {
            if ((csi_tick_get_ms() - timestart) > timeout) {
                break;
            }
        }

        while ((dw_ospi_get_status(spi_base) & DW_OSPI_SR_BUSY)) {
            if ((csi_tick_get_ms() - timestart) > timeout) {
                break;
            }
        }
    } while (0);

    /* close ospi */
    dw_ospi_config_tx_fifo_threshold(spi_base, 0U);
    ospi->state.writeable = 1U;

    if (ret >= 0) {
        ret = (int32_t)count;
    }

    return ret;
}

csi_error_t csi_ospi_send_async(csi_ospi_t *ospi, const void *data, uint32_t size)
{
    CSI_PARAM_CHK(ospi,  CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    csi_error_t ret = CSI_OK;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
        ret = CSI_BUSY;
    }

    if (IS_32BIT_FRAME_LEN(spi_base)) {
        if (size % sizeof(uint32_t)) {
            ret = CSI_ERROR;
        }
    }

    if (IS_16BIT_FRAME_LEN(spi_base)) {
        if (size % sizeof(uint16_t)) {
            ret = CSI_ERROR;
        }
    }

    if ((ret == CSI_OK) && (ospi->callback != NULL)) {
        if (ospi->send) {
            ospi->state.writeable = 0U;
            ret = ospi->send(ospi, data, size);
        } else {
            ospi->state.writeable = 0U;
            csi_irq_attach((uint32_t)ospi->dev.irq_num, &dw_ospi_irqhandler, &ospi->dev);
            csi_irq_enable((uint32_t)ospi->dev.irq_num);
            ret = dw_ospi_send_intr(ospi, data, size);
        }
    } else {
        ret = CSI_ERROR;
    }

    return ret;
}

static csi_error_t dw_ospi_send_intr(csi_ospi_t *ospi, const void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;
    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);
    ospi->tx_data = (uint8_t *)data;

    do {
        // Convert byte to nums
        if (IS_32BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size / 4U;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size / 2U;
        } else if (IS_8BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size;
        } else {
            ret = CSI_ERROR;
            break;
        }

        /* set tx mode*/
        dw_ospi_disable(spi_base);
        dw_ospi_set_tx_mode(spi_base);
        dw_ospi_config_tx_fifo_threshold(spi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
        dw_ospi_enable(spi_base);
        dw_ospi_enable_tx_empty_irq(spi_base);
    } while (0);

    return ret;
}

static csi_error_t dw_ospi_send_dma(csi_ospi_t *ospi, const void *data, uint32_t size)
{
    CSI_PARAM_CHK(ospi,  CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    csi_dma_ch_config_t config;
    dw_ospi_regs_t       *spi_base;
    csi_dma_ch_t        *dma_ch;
    csi_error_t         ret = CSI_OK;

    spi_base = dw_get_reg_base(ospi);
    dma_ch   = (csi_dma_ch_t *)ospi->tx_dma;
    ospi->tx_data = (uint8_t *)data;
    memset(&config, 0, sizeof(csi_dma_ch_config_t));

    do {
        /* configure dma channel */
        if (IS_32BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size / 4U;
            config.src_tw = DMA_DATA_WIDTH_32_BITS;
            config.dst_tw = DMA_DATA_WIDTH_32_BITS;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size / 2U;
            config.src_tw = DMA_DATA_WIDTH_16_BITS;
            config.dst_tw = DMA_DATA_WIDTH_16_BITS;
        } else if (IS_8BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size;
            config.src_tw = DMA_DATA_WIDTH_8_BITS;
            config.dst_tw = DMA_DATA_WIDTH_8_BITS;
        } else {
            ret = CSI_ERROR;
            break;
        }

        config.src_inc = DMA_ADDR_INC;
        config.dst_inc = DMA_ADDR_CONSTANT;
        config.group_len = find_group_len(size, 1U << (uint8_t)config.src_tw);
        config.trans_dir = DMA_MEM2PERH;
        config.handshake = dw_ospi_get_hs_num(ospi, spi_tx_hs_num);
        csi_dma_ch_config(dma_ch, &config);

        /* set tx mode*/
        dw_ospi_disable(spi_base);
        dw_ospi_set_tx_mode(spi_base);
        dw_ospi_enable_tx_dma(spi_base);
        dw_ospi_enable(spi_base);
        soc_dcache_clean_invalid_range((unsigned long)ospi->tx_data, size);
        csi_dma_ch_start(ospi->tx_dma, ospi->tx_data, (void *) & (spi_base->DR), size);

    } while (0);

    return ret;
}

int32_t csi_ospi_receive(csi_ospi_t *ospi, void *data, uint32_t size, uint32_t timeout)
{
    CSI_PARAM_CHK(ospi,  CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    uint32_t timestart;
    uint32_t count = 0U;
    int32_t  ret = CSI_OK;
    uint8_t *rx_data;
    uint32_t current_size;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    do {
        if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
            ret = CSI_BUSY;
            break;
        }

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            if (size % sizeof(uint32_t)) {
                ret = CSI_ERROR;
                break;
            }
        }

        if (IS_16BIT_FRAME_LEN(spi_base)) {
            if (size % sizeof(uint16_t)) {
                ret = CSI_ERROR;
                break;
            }
        }

        timestart = csi_tick_get_ms();
        ospi->state.readable = 0U;
        ospi->rx_data = (uint8_t *)data;

        // Convert byte to nums
        if (IS_32BIT_FRAME_LEN(spi_base)) {
            size = size / 4U;
        }

        if (IS_16BIT_FRAME_LEN(spi_base)) {
            size = size / 2U;
        }

        rx_data = (uint8_t *)data;

        /* set rx mode*/
        dw_ospi_disable(spi_base);
        dw_ospi_set_rx_mode(spi_base);
        dw_ospi_config_rx_data_len(spi_base, size - 1U);
        dw_ospi_enable(spi_base);

        if (IS_DW_OSPI_IDX_MASTER(ospi)) {
            dw_ospi_transmit_data(spi_base, 0U);
        }

        /* transfer loop */
        if (IS_8BIT_FRAME_LEN(spi_base)) {
            while (size > 0U) {
                current_size = dw_ospi_get_rx_fifo_level(spi_base);

                if (current_size > size) {
                    current_size = size;

                }

                while (current_size--) {
                    *(uint8_t *)rx_data = (uint8_t)dw_ospi_receive_data(spi_base);
                    rx_data += 1;
                    size--;
                    count++;
                }
            }

            if ((csi_tick_get_ms() - timestart) > timeout) {
                break;
            }
        }

        if (IS_16BIT_FRAME_LEN(spi_base)) {
            while (size > 0U) {
                current_size = dw_ospi_get_rx_fifo_level(spi_base);

                if (current_size > size) {
                    current_size = size;

                }

                while (current_size--) {
                    *(uint16_t *)rx_data = (uint16_t)dw_ospi_receive_data(spi_base);
                    rx_data += 2;
                    size--;
                    count += 2U;
                }
            }

            if ((csi_tick_get_ms() - timestart) > timeout) {
                break;
            }
        }

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            while (size > 0U) {
                current_size = dw_ospi_get_rx_fifo_level(spi_base);

                if (current_size > size) {
                    current_size = size;

                }

                while (current_size--) {
                    *(uint32_t *)rx_data = (uint32_t)dw_ospi_receive_data(spi_base);
                    rx_data += 4;
                    size--;
                    count += 4U;
                }
            }

            if ((csi_tick_get_ms() - timestart) > timeout) {
                break;
            }
        }
        /* wait end of transcation */
        while ((dw_ospi_get_status(spi_base) & DW_OSPI_SR_BUSY)) {
            if ((csi_tick_get_ms() - timestart) > timeout) {
                break;
            }
        }
    } while (0);

    /* close ospi */
    dw_ospi_config_rx_data_len(spi_base, 0U);
    dw_ospi_config_rx_fifo_threshold(spi_base, 0U);
    ospi->state.readable = 1U;

    if (ret >= 0) {
        ret = (int32_t)count;
    }

    return ret;
}


csi_error_t csi_ospi_receive_async(csi_ospi_t *ospi, void *data, uint32_t size)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    csi_error_t ret = CSI_OK;

    if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
        ret = CSI_BUSY;
    } else {
        dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            if (size % sizeof(uint32_t)) {
                ret = CSI_ERROR;
            }
        }

        if (IS_16BIT_FRAME_LEN(spi_base)) {
            if (size % sizeof(uint16_t)) {
                ret = CSI_ERROR;
            }
        }

        if ((ret == CSI_OK) && (ospi->callback != NULL)) {
            if (ospi->receive) {
                ospi->state.readable = 0U;
                ret = ospi->receive(ospi, data, size);
            } else {
                ospi->state.readable = 0U;
                csi_irq_attach((uint32_t)ospi->dev.irq_num, &dw_ospi_irqhandler, &ospi->dev);
                csi_irq_enable((uint32_t)ospi->dev.irq_num);
                ret = dw_ospi_receive_intr(ospi, data, size);
            }
        } else {
            ret = CSI_ERROR;
        }
    }

    return ret;
}

static csi_error_t dw_ospi_receive_intr(csi_ospi_t *ospi, void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;
    uint32_t rx_fifo_lv;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    ospi->rx_data = (uint8_t *)data;

    do {
        // Convert byte to nums
        if (IS_32BIT_FRAME_LEN(spi_base)) {
            ospi->rx_size = size / 4U;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            ospi->rx_size = size / 2U;
        } else if (IS_8BIT_FRAME_LEN(spi_base)) {
            ospi->rx_size = size;
        } else {
            ret = CSI_ERROR;
            break;
        }

        /* set rx mode*/
        dw_ospi_disable(spi_base);
        dw_ospi_set_rx_mode(spi_base);
        dw_ospi_config_rx_data_len(spi_base, ospi->rx_size - 1U);
        rx_fifo_lv = (ospi->rx_size < DW_DEFAULT_OSPI_RXFIFO_LV) ? (ospi->rx_size - 1U) : DW_DEFAULT_OSPI_RXFIFO_LV;
        dw_ospi_config_rx_fifo_threshold(spi_base, rx_fifo_lv);
        dw_ospi_enable(spi_base);
        dw_ospi_enable_rx_fifo_full_irq(spi_base);

        if (IS_DW_OSPI_IDX_MASTER(ospi)) {
            dw_ospi_transmit_data(spi_base, 0xffU);
        }
    } while (0);

    return ret;
}

static csi_error_t dw_ospi_receive_dma(csi_ospi_t *ospi, void *data, uint32_t size)
{
    csi_dma_ch_config_t config;
    dw_ospi_regs_t       *spi_base;
    csi_dma_ch_t        *dma_ch;
    csi_error_t         ret = CSI_OK;

    spi_base = dw_get_reg_base(ospi);
    dma_ch   = (csi_dma_ch_t *)ospi->rx_dma;
    ospi->rx_data = (uint8_t *)data;
    memset(&config, 0, sizeof(csi_dma_ch_config_t));

    do {
        /* configure dma channel */
        if (IS_32BIT_FRAME_LEN(spi_base)) {
            ospi->rx_size = size / 4U;
            config.src_tw = DMA_DATA_WIDTH_32_BITS;
            config.dst_tw = DMA_DATA_WIDTH_32_BITS;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            ospi->rx_size = size / 2U;
            config.src_tw = DMA_DATA_WIDTH_16_BITS;
            config.dst_tw = DMA_DATA_WIDTH_16_BITS;
        } else if (IS_8BIT_FRAME_LEN(spi_base)) {
            ospi->rx_size = size;
            config.src_tw = DMA_DATA_WIDTH_8_BITS;
            config.dst_tw = DMA_DATA_WIDTH_8_BITS;
        } else {
            ret = CSI_ERROR;
            break;
        }

        config.src_inc = DMA_ADDR_CONSTANT;
        config.dst_inc = DMA_ADDR_INC;
        if (IS_32BIT_FRAME_LEN(spi_base)) {
            config.group_len = 4;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            config.group_len = 2;
        } else if (IS_8BIT_FRAME_LEN(spi_base)) {
            config.group_len = 1;
        }
        config.trans_dir = DMA_PERH2MEM;
        config.handshake = dw_ospi_get_hs_num(ospi, spi_rx_hs_num);;
        csi_dma_ch_config(dma_ch, &config);

        /* set rx mode*/
        dw_ospi_disable(spi_base);
        dw_ospi_set_rx_mode(spi_base);
        dw_ospi_config_rx_data_len(spi_base, ospi->rx_size - 1U);
        dw_ospi_config_dma_rx_data_level(spi_base, ((uint32_t)config.group_len / ((uint32_t)1U << (uint32_t)config.src_tw)) - 1U);
        dw_ospi_enable_rx_dma(spi_base);

        soc_dcache_clean_invalid_range((unsigned long)ospi->rx_data, size);
        csi_dma_ch_start(ospi->rx_dma, (void *) & (spi_base->DR), ospi->rx_data, size);

        dw_ospi_enable(spi_base);

        if (IS_DW_OSPI_IDX_MASTER(ospi)) {
            dw_ospi_transmit_data(spi_base, 0xffU);
        }
    } while (0);

    return ret;
}

int32_t csi_ospi_send_receive(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size, uint32_t timeout)
{
    CSI_PARAM_CHK(ospi,      CSI_ERROR);
    CSI_PARAM_CHK(data_out, CSI_ERROR);
    CSI_PARAM_CHK(data_in,  CSI_ERROR);
    CSI_PARAM_CHK(size,     CSI_ERROR);

    uint32_t value;
    uint32_t timestart;
    uint32_t count = 0U;
    int32_t  ret   = CSI_OK;
    uint32_t tx_size, rx_size;
    uint8_t  *tx_data, *rx_data;
    uint32_t current_size;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    do {

        if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
            ret = CSI_BUSY;
            break;
        }

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            if (size % sizeof(uint32_t)) {
                ret = CSI_ERROR;
                break;
            }
        }

        if (IS_16BIT_FRAME_LEN(spi_base)) {
            if (size % sizeof(uint16_t)) {
                ret = CSI_ERROR;
                break;
            }
        }

        timestart = csi_tick_get_ms();
        ospi->state.writeable = 0U;
        ospi->state.readable  = 0U;
        tx_data = (uint8_t *)data_out;

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            tx_size = size / 4U;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            tx_size = size / 2U;
        } else {
            tx_size = size;
        }

        rx_data = (uint8_t *)data_in;

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            rx_size = size / 4U;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            rx_size = size / 2U;
        } else {
            rx_size = size;
        }

        /* set tx rx mode*/
        dw_ospi_disable(spi_base);
        dw_ospi_set_tx_rx_mode(spi_base);
        dw_ospi_config_tx_fifo_threshold(spi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
        dw_ospi_config_rx_fifo_threshold(spi_base, DW_DEFAULT_OSPI_RXFIFO_LV);
        dw_ospi_enable(spi_base);

        /* transfer loop */
        if (IS_8BIT_FRAME_LEN(spi_base)) {
            while ((tx_size > 0U) || (rx_size > 0U)) {
                /* process tx fifo empty */
                if (tx_size > 0U) {
                    current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(spi_base);

                    if (current_size > tx_size) {
                        current_size = tx_size;

                    }

                    while (current_size--) {
                        value = (uint32_t)(*(uint8_t *)tx_data);
                        dw_ospi_transmit_data(spi_base, value);
                        tx_data += 1;
                        count += 1U;
                        tx_size--;
                    }
                }

                /* process rx fifo not empty */
                if (rx_size > 0U) {
                    current_size = dw_ospi_get_rx_fifo_level(spi_base);

                    if (current_size > rx_size) {
                        current_size = rx_size;

                    }

                    while (current_size--) {
                        *(uint8_t *)rx_data = (uint8_t)dw_ospi_receive_data(spi_base);
                        rx_data += 1;
                        rx_size--;
                    }
                }

                if ((csi_tick_get_ms() - timestart) > timeout) {
                    break;
                }
            }
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            while ((tx_size > 0U) || (rx_size > 0U)) {
                /* process tx fifo empty */
                if (tx_size > 0U) {
                    current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(spi_base);

                    if (current_size > tx_size) {
                        current_size = tx_size;

                    }

                    while (current_size--) {
                        value = (uint32_t)(*(uint16_t *)tx_data);
                        dw_ospi_transmit_data(spi_base, value);
                        tx_data += 2;
                        count += 2U;
                        tx_size--;
                    }
                }

                /* process rx fifo not empty */
                if (rx_size > 0U) {
                    current_size = dw_ospi_get_rx_fifo_level(spi_base);

                    if (current_size > rx_size) {
                        current_size = rx_size;

                    }

                    while (current_size--) {
                        *(uint16_t *)rx_data = (uint16_t)dw_ospi_receive_data(spi_base);
                        rx_data += 2;
                        rx_size--;
                    }
                }

                if ((csi_tick_get_ms() - timestart) > timeout) {
                    break;
                }

            }
        } else if (IS_32BIT_FRAME_LEN(spi_base)) {
            while ((tx_size > 0U) || (rx_size > 0U)) {
                /* process tx fifo empty */
                if (tx_size > 0U) {
                    current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(spi_base);

                    if (current_size > tx_size) {
                        current_size = tx_size;

                    }

                    while (current_size--) {
                        value = (uint32_t)(*(uint32_t *)tx_data);
                        dw_ospi_transmit_data(spi_base, value);
                        tx_data += 4;
                        count += 4U;
                        tx_size--;
                    }
                }

                /* process rx fifo not empty */
                if (rx_size > 0U) {
                    current_size = dw_ospi_get_rx_fifo_level(spi_base);

                    if (current_size > rx_size) {
                        current_size = rx_size;

                    }

                    while (current_size--) {
                        *(uint32_t *)rx_data = (uint32_t)dw_ospi_receive_data(spi_base);
                        rx_data += 4;
                        rx_size--;
                    }
                }

                if ((csi_tick_get_ms() - timestart) > timeout) {
                    break;
                }

            }
        }

        /* wait end of transcation */
        while (dw_ospi_get_status(spi_base) & DW_OSPI_SR_BUSY) {
            if ((csi_tick_get_ms() - timestart) > timeout) {
                break;
            }
        }
    } while (0);

    /* close ospi */
    ospi->state.writeable = 1U;
    ospi->state.readable  = 1U;

    if (ret >= 0) {
        ret = (int32_t)count;
    }

    return ret;
}


csi_error_t csi_ospi_send_receive_async(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);
    CSI_PARAM_CHK(data_out, CSI_ERROR);
    CSI_PARAM_CHK(data_in, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    csi_error_t ret = CSI_OK;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
        ret = CSI_BUSY;
    }

    if (IS_32BIT_FRAME_LEN(spi_base)) {
        if (size % sizeof(uint32_t)) {
            ret = CSI_ERROR;
        }
    }

    if (IS_16BIT_FRAME_LEN(spi_base)) {
        if (size % sizeof(uint16_t)) {
            ret = CSI_ERROR;
        }
    }

    if ((ret == CSI_OK) && (ospi->callback != NULL)) {
        if (ospi->send_receive) {
            ospi->state.readable  = 0U;
            ospi->state.writeable = 0U;
            ret = ospi->send_receive(ospi, data_out, data_in, size);
        } else {
            ospi->state.readable  = 0U;
            ospi->state.writeable = 0U;
            csi_irq_attach((uint32_t)ospi->dev.irq_num, &dw_ospi_irqhandler, &ospi->dev);
            csi_irq_enable((uint32_t)ospi->dev.irq_num);
            ret = dw_ospi_send_receive_intr(ospi, data_out, data_in, size);
        }
    } else {
        ret = CSI_ERROR;
    }

    return ret;
}

static csi_error_t dw_ospi_send_receive_intr(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size)
{
    csi_error_t ret = CSI_OK;
    uint32_t rx_fifo_lv;

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);

    ospi->tx_data = (uint8_t *)data_out;
    ospi->rx_data = (uint8_t *)data_in;

    do {
        if (IS_32BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size / 4U;
            ospi->rx_size = size / 4U;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size / 2U;
            ospi->rx_size = size / 2U;
        } else if (IS_8BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size;
            ospi->rx_size = size;
        } else {
            ret = CSI_ERROR;
            break;
        }

        /* set tx rx mode*/
        dw_ospi_disable(spi_base);
        dw_ospi_set_tx_rx_mode(spi_base);
        dw_ospi_config_rx_data_len(spi_base, ospi->rx_size - 1U);
        dw_ospi_config_tx_fifo_threshold(spi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
        rx_fifo_lv = (ospi->rx_size < (DW_DEFAULT_OSPI_RXFIFO_LV + 1U)) ? (ospi->rx_size - 1U) : DW_DEFAULT_OSPI_RXFIFO_LV;
        dw_ospi_config_rx_fifo_threshold(spi_base, rx_fifo_lv);
        dw_ospi_enable(spi_base);
        dw_ospi_enable_rx_fifo_full_irq(spi_base);
        dw_ospi_enable_tx_empty_irq(spi_base);

    } while (0);

    return ret;
}

static csi_error_t dw_ospi_send_receive_dma(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size)
{
    csi_dma_ch_config_t config;
    dw_ospi_regs_t       *spi_base;
    csi_dma_ch_t        *dma_ch;
    csi_error_t         ret = CSI_OK;

    spi_base = dw_get_reg_base(ospi);
    ospi->tx_data = (uint8_t *)data_out;
    memset(&config, 0, sizeof(csi_dma_ch_config_t));

    do {

        if (IS_32BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size / 4U;
            ospi->rx_size = size / 4U;
            config.src_tw = DMA_DATA_WIDTH_32_BITS;
            config.dst_tw = DMA_DATA_WIDTH_32_BITS;
        } else if (IS_16BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size / 2U;
            ospi->rx_size = size / 2U;
            config.src_tw = DMA_DATA_WIDTH_16_BITS;
            config.dst_tw = DMA_DATA_WIDTH_16_BITS;
        } else if (IS_8BIT_FRAME_LEN(spi_base)) {
            ospi->tx_size = size;
            ospi->rx_size = size;
            config.src_tw = DMA_DATA_WIDTH_8_BITS;
            config.dst_tw = DMA_DATA_WIDTH_8_BITS;
        } else {
            ret = CSI_ERROR;
            break;
        }

        ospi->rx_data = (uint8_t *)data_in;

        /* configure tx dma channel */
        dma_ch   = (csi_dma_ch_t *)ospi->tx_dma;
        config.src_inc = DMA_ADDR_INC;
        config.dst_inc = DMA_ADDR_CONSTANT;
        config.group_len = DW_DEFAULT_OSPI_TXFIFO_LV;
        config.group_len = find_group_len(size, 1U << (uint8_t)config.src_tw);
        config.trans_dir = DMA_MEM2PERH;
        config.handshake = dw_ospi_get_hs_num(ospi, spi_tx_hs_num);
        csi_dma_ch_config(dma_ch, &config);

        /* configure dma rx channel */
        dma_ch   = (csi_dma_ch_t *)ospi->rx_dma;
        config.src_inc = DMA_ADDR_CONSTANT;
        config.dst_inc = DMA_ADDR_INC;
        config.group_len = find_group_len(size, 1U << (uint8_t)config.src_tw);
        config.trans_dir = DMA_PERH2MEM;
        config.handshake = dw_ospi_get_hs_num(ospi, spi_rx_hs_num);
        csi_dma_ch_config(dma_ch, &config);

        /* set tx_rx mode*/
        dw_ospi_disable(spi_base);
        dw_ospi_set_tx_rx_mode(spi_base);
        dw_ospi_config_rx_data_len(spi_base, ospi->rx_size - 1U);
        dw_ospi_config_dma_rx_data_level(spi_base, ((uint32_t)config.group_len / ((uint32_t)1U << (uint32_t)config.src_tw)) - 1U);
        dw_ospi_enable_rx_dma(spi_base);
        dw_ospi_enable_tx_dma(spi_base);

        soc_dcache_clean_invalid_range((unsigned long)ospi->tx_data, size);
        soc_dcache_clean_invalid_range((unsigned long)ospi->rx_data, size);
        dw_ospi_enable(spi_base);
        csi_dma_ch_start(ospi->rx_dma, (void *) & (spi_base->DR), ospi->rx_data, size);
        csi_dma_ch_start(ospi->tx_dma, ospi->tx_data, (void *) & (spi_base->DR), size);
    } while (0);

    return ret;
}

csi_error_t csi_ospi_get_state(csi_ospi_t *ospi, csi_state_t *state)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);
    CSI_PARAM_CHK(state, CSI_ERROR);

    *state = ospi->state;
    return CSI_OK;
}

csi_error_t csi_ospi_link_dma(csi_ospi_t *ospi, csi_dma_ch_t *tx_dma, csi_dma_ch_t *rx_dma)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    csi_error_t ret = CSI_OK;

    if (tx_dma != NULL) {
        tx_dma->parent = ospi;
        ret = csi_dma_ch_alloc(tx_dma, -1, -1);

        if (ret == CSI_OK) {
            csi_dma_ch_attach_callback(tx_dma, dw_ospi_dma_event_cb, NULL);
            ospi->tx_dma = tx_dma;
            ospi->send = dw_ospi_send_dma;
        } else {
            tx_dma->parent = NULL;
        }
    } else {
        if (ospi->tx_dma) {
            csi_dma_ch_detach_callback(ospi->tx_dma);
            csi_dma_ch_free(ospi->tx_dma);
            ospi->tx_dma = NULL;
        }

        ospi->send = NULL;
    }

    if (ret == CSI_OK) {
        if (rx_dma != NULL) {
            rx_dma->parent = ospi;
            ret = csi_dma_ch_alloc(rx_dma, -1, -1);

            if (ret == CSI_OK) {
                csi_dma_ch_attach_callback(rx_dma, dw_ospi_dma_event_cb, NULL);
                ospi->rx_dma = rx_dma;
                ospi->receive = dw_ospi_receive_dma;
            } else {
                rx_dma->parent = NULL;
            }
        } else {
            if (ospi->rx_dma) {
                csi_dma_ch_detach_callback(ospi->rx_dma);
                csi_dma_ch_free(ospi->rx_dma);
                ospi->rx_dma = NULL;
            }

            ospi->receive = NULL;
        }
    }


    if (ret == CSI_OK) {
        if ((tx_dma != NULL) && (rx_dma != NULL)) {
            ospi->send_receive =  dw_ospi_send_receive_dma;
        } else {
            ospi->send_receive = NULL;
        }
    }

    return ret;
}

void csi_ospi_select_slave(csi_ospi_t *ospi, uint32_t slave_num)
{
    CSI_PARAM_CHK_NORETVAL(ospi);

    dw_ospi_regs_t *spi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(spi_base);
    dw_ospi_enable_slave(spi_base, slave_num);

    dw_ospi_disable(spi_base);
    dw_ospi_set_tx_mode(spi_base);
    dw_ospi_config_tx_fifo_threshold(spi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
    dw_ospi_enable(spi_base);
}

#ifdef CONFIG_PM
csi_error_t dw_ospi_pm_action(csi_dev_t *dev, csi_pm_dev_action_t action)
{
    CSI_PARAM_CHK(dev, CSI_ERROR);

    csi_error_t ret = CSI_OK;
    csi_pm_dev_t *pm_dev = &dev->pm_dev;

    switch (action) {
        case PM_DEV_SUSPEND:
            csi_pm_dev_save_regs(pm_dev->reten_mem, (uint32_t *)dev->reg_base, 7U);
            csi_pm_dev_save_regs(pm_dev->reten_mem + 7U, (uint32_t *)(dev->reg_base + 44U), 1U);
            csi_pm_dev_save_regs(pm_dev->reten_mem + 8U, (uint32_t *)(dev->reg_base + 76U), 3U);
            csi_pm_dev_save_regs(pm_dev->reten_mem + 11U, (uint32_t *)(dev->reg_base + 160U), 1U);
            break;

        case PM_DEV_RESUME:
            csi_pm_dev_restore_regs(pm_dev->reten_mem, (uint32_t *)dev->reg_base, 2U);
            csi_pm_dev_restore_regs(pm_dev->reten_mem + 3U, (uint32_t *)dev->reg_base + 12U, 4U);
            csi_pm_dev_restore_regs(pm_dev->reten_mem + 7U, (uint32_t *)(dev->reg_base + 44U), 1U);
            csi_pm_dev_restore_regs(pm_dev->reten_mem + 8U, (uint32_t *)(dev->reg_base + 76U), 3U);
            csi_pm_dev_restore_regs(pm_dev->reten_mem + 11U, (uint32_t *)(dev->reg_base + 160U), 1U);
            csi_pm_dev_restore_regs(pm_dev->reten_mem + 2U, (uint32_t *)(dev->reg_base + 8U), 1U);
            break;

        default:
            ret = CSI_ERROR;
            break;
    }

    return ret;
}

csi_error_t csi_ospi_enable_pm(csi_ospi_t *ospi)
{
    return csi_pm_dev_register(&ospi->dev, dw_ospi_pm_action, 36U, 0U);
}

void csi_ospi_disable_pm(csi_ospi_t *ospi)
{
    csi_pm_dev_unregister(&ospi->dev);
}
#endif

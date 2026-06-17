

/******************************************************************************
 * @file     tdm.c
 * @brief
 * @version
 * @date     2020-10-20
 ******************************************************************************/

#include <string.h>
#include <csi_config.h>
#include <board_config.h>
#include <sys_clk.h>
#include <drv/tdm.h>
#include <drv/irq.h>
#include <drv/tick.h>
#include <drv/porting.h>
#include <drv/list.h>
#include <drv/pin.h>
#include <sys_clk.h>
#include "wj_tdm_ll.h"

extern uint16_t tdm_rx_hs_num[];
extern uint16_t tdm_tx_hs_num[];
uint32_t g_tx_irq_count=0;
#ifdef CONFIG_XIP
#define TDM_CODE_IN_RAM __attribute__((section(".ram.code")))
#else
#define TDM_CODE_IN_RAM
#endif
static csi_error_t wj_tdm_send_intr(csi_tdm_t *spi, const void *data, uint32_t size);
static csi_error_t wj_tdm_receive_intr(csi_tdm_t *spi, void *data, uint32_t size);
static int32_t tdm_send_polling(csi_tdm_t *tdm, const void *data, uint32_t num)
{
    wj_tdm_regs_t *tdm_base;
    tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    int32_t sent_num = (int32_t)num;
    uint32_t val = wj_tdm_get_slot_width_mode(tdm_base);
    if (val <= 1) {
        uint8_t *send_data = (uint8_t *)data;
        while (num--) {
            while((wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Msk));
            wj_tdm_write_data(tdm_base, *(send_data++));
        }
    } else if (val <= 3) {
        uint16_t *send_data = (uint16_t *)data;
        num = num / 2;
        while (num--) {
            while((wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Msk));
            wj_tdm_write_data(tdm_base, *(send_data++));
        }
    } else {
        uint32_t *send_data = (uint32_t *)data;
        num = num /4;
        while (num--) {
            while((wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Msk));
            wj_tdm_write_data(tdm_base, *(send_data++));
        }
    }
    while(!(wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_EMPTY_Msk));
    // wj_tdm_disable(tdm_base);
    // wj_ip_ctrl_tdm_disable(PMU_REG_BASE);
    // wj_tdm_enable(tdm_base);
    // wj_ip_ctrl_tdm_enable(PMU_REG_BASE);

    // while (num--) {
    //     while((wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Msk));
    //     wj_tdm_write_data(tdm_base, *(send_data++));
    // }

    return sent_num;
}

static int32_t tdm_receive_polling(csi_tdm_t *tdm, void *data, uint32_t num)
{
    wj_tdm_regs_t *tdm_base;
    tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    int32_t received_size = (int32_t)num;

        /* clear fifo*/
    // wj_tdm_flush_rxfifo(tdm_base);

    uint32_t val = wj_tdm_get_slot_width_mode(tdm_base);
    if (val <= 1) {
        uint8_t *readdata = (uint8_t *)data;
        while (num--) {
            while(wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_RXFIFO_EMPTY_Msk);
            *(readdata++) = wj_tdm_get_data(tdm_base);
        }
    } else if (val <= 3) {
        uint16_t *readdata = (uint16_t *)data;
        num = num / 2;
        while (num--) {
            while(wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_RXFIFO_EMPTY_Msk);
            *(readdata++) = wj_tdm_get_data(tdm_base);
        }
    } else {
        uint32_t *readdata = (uint32_t *)data;
        num = num /4;
        while (num--) {
            while(wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_RXFIFO_EMPTY_Msk);
            *(readdata++) = wj_tdm_get_data(tdm_base);
        }
    }
    while(!(wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_RXFIFO_EMPTY_Msk)) {
        wj_tdm_get_data(tdm_base);
    }
    // wj_tdm_disable(tdm_base);
    // wj_ip_ctrl_tdm_disable(PMU_REG_BASE);
    // wj_tdm_enable(tdm_base);
    // wj_ip_ctrl_tdm_enable(PMU_REG_BASE);


    // while (num--) {
    //     while(wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_RXFIFO_EMPTY_Msk);
    //     *(readdata++) = wj_tdm_get_data(tdm_base);
    // }

    return received_size;
}

static void tdm_intr_tx_fifo_empty(csi_tdm_t *tdm)
{
    uint32_t remain_fifo;
    uint32_t value;
    uint32_t frame_len;
    uint8_t  *tx_data;
    uint32_t tx_size;

    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);

    remain_fifo = WJ_MAX_TDM_FIFO_LV - wj_tdm_get_txdma_data_level(tdm_base);
    frame_len   = wj_tdm_get_slot_width_mode(tdm_base);

    /* transfer loop */
    tx_data = tdm->tx_data;
    tx_size = tdm->tx_size;

    if (frame_len <= 1U) {
        while (tx_size && remain_fifo) {
            /* process 4~8bit frame len */
            value = (uint32_t)(*(uint8_t *)tx_data);
            tx_data += sizeof(uint8_t);
            wj_tdm_write_data(tdm_base, value);
            remain_fifo--;
            tx_size--;
        }
    } else if (frame_len <= 3U) {
        while (tx_size && remain_fifo) {
            /* process 8~16bit frame len */
            value = (uint32_t)(*(uint16_t *)tx_data);
            tx_data += sizeof(uint16_t);
            wj_tdm_write_data(tdm_base, value);
            // printf("%x %x\n", (uint32_t)tx_data, value);
            remain_fifo--;
            tx_size--;
            // if (wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_AFULL_Msk) {
            //     printf("%x %x\n", (uint32_t)tx_data, value);
            //     break;
            // }
        }
    }

    tdm->tx_data = tx_data;
    tdm->tx_size = tx_size;
    wj_tdm_clr_tx_fifo_dataready_irq(tdm_base);
    if (tdm->tx_size == 0U) {
        tdm->state.writeable = 1;
        // wj_tdm_disable_tx_empty_irq(tdm_base);
        // wj_tdm_config_tx_fifo_threshold(tdm_base, 0U);
        while(!(wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_EMPTY_Msk));
        if (tdm->callback) {
            tdm->callback(tdm, TDM_EVENT_SEND_COMPLETE, tdm->arg);
        }
    }
    // *(volatile uint32_t *)0x60044020 |= 0x10;
    // *(volatile uint32_t *)0x60044020 &= ~0x10;
}

static void tdm_intr_rx_fifo_full(csi_tdm_t *tdm)
{
    uint32_t fifo_size;
    uint32_t frame_len;
    uint8_t  *rx_data;
    uint32_t rx_size;

    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    fifo_size = wj_tdm_get_rxdma_data_level(tdm_base);
    frame_len = wj_tdm_get_slot_width_mode(tdm_base);
    rx_data = tdm->rx_data;
    rx_size = tdm->rx_size;

    /* transfer loop */
    if (frame_len <= 1U) {
        while (rx_size && fifo_size) {
            *(uint8_t *)rx_data = (uint8_t)wj_tdm_get_data(tdm_base);
            rx_data += sizeof(uint8_t);
            fifo_size--;
            rx_size--;
        }
        if (rx_size < fifo_size) {
            while (rx_size) {
                *(uint8_t *)rx_data = (uint8_t)wj_tdm_get_data(tdm_base);
                rx_data += sizeof(uint8_t);
                rx_size--;
            }            
        }
    } else if (frame_len <= 3U) {
        while (rx_size && fifo_size) {
            *(uint16_t *)rx_data = (uint16_t)wj_tdm_get_data(tdm_base);
            // printf("%x %x", rx_data, *(uint16_t *)rx_data);
            rx_data += sizeof(uint16_t);
            fifo_size--;
            rx_size--;
        }
        if (rx_size < (fifo_size)) {
            while (rx_size) {
                *(uint16_t *)rx_data = (uint16_t)wj_tdm_get_data(tdm_base);
                rx_data += sizeof(uint16_t);
                rx_size--;
            }
        }
    }

    // /* update rx fifo threshold when remain size less then default threshold*/
    // if ((rx_size < (WJ_DEFAULT_TDM_RXFIFO_LV + 1U)) && (rx_size > 0U)) {
    //     wj_tdm_config_rx_fifo_threshold(tdm_base, rx_size - 1U);
    // }
    wj_tdm_clr_rx_fifo_dataready_irq(tdm_base);
    tdm->rx_data = rx_data;
    tdm->rx_size = rx_size;
    if (tdm->rx_size == 0U) {
        tdm->state.readable = 1;
        // wj_tdm_disable_rx_fifo_full_irq(tdm_base);
        // wj_tdm_config_rx_fifo_threshold(tdm_base, 0U);
        if (tdm->callback) {
            tdm->callback(tdm, TDM_EVENT_RECEIVE_COMPLETE, tdm->arg);
        }
    } else {
        g_tx_irq_count++;
    }
}

TDM_CODE_IN_RAM static void wj_tdm_irqhandler(void *args)
{
    uint32_t status = 0;
    uint32_t int_status = 0;
    uint32_t fifo_status = 0;
    csi_tdm_t *tdm = (csi_tdm_t *)args;
    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);

    status = wj_tdm_get_irq_status(tdm_base);
    int_status = wj_tdm_get_int_enable_status(tdm_base);
    fifo_status = wj_tdm_get_fifo_status(tdm_base);

    /* process receive fifo full interrupt */
    if (status & WJ_TDM_INT_RXFIFO_DATA_READY_Msk) {
        tdm_intr_rx_fifo_full(tdm);
    }

    /* process transmit fifo empty interrupt */
    // if (status & WJ_TDM_INT_TXFIFO_DATA_READY_Msk) {
    if ((status & WJ_TDM_INT_TXFIFO_DATA_READY_Msk) || ((int_status & WJ_TDM_INT_EN_TXFIFO_DATA_READY_EN_Msk) && (fifo_status & WJ_TDM_FIFO_STATUS_TXFIFO_TXLVL_TRIG_Msk))) {
        tdm_intr_tx_fifo_empty(tdm);
    }
    /* process receive fifo overflow interrupt */
    if ((status & WJ_TDM_INT_RXFIFO_OVERFLOW_Msk)) {
        wj_tdm_clr_rx_fifo_overflow_irq(tdm_base);

        if (tdm->callback) {
            tdm->callback(tdm, TDM_EVENT_ERROR_OVERFLOW, tdm->arg);
        }
    }

    /* process transmit fifo overflow interrupt */
    if ((status & WJ_TDM_INT_TXFIFO_OVERFLOW_Msk)) {
        wj_tdm_clr_tx_fifo_overflow_irq(tdm_base);

        if (tdm->callback) {
            tdm->callback(tdm, TDM_EVENT_ERROR_OVERFLOW, tdm->arg);
        }
    }

    /* process receive fifo underflow interrupt */
    if (status & WJ_TDM_INT_RXFIFO_UNDERFLOW_Msk) {
        wj_tdm_clr_rx_fifo_underflow_irq(tdm_base);

        if (tdm->callback) {
            tdm->callback(tdm, TDM_EVENT_ERROR_UNDERFLOW, tdm->arg);
        }
    }

    if (status & WJ_TDM_INT_TXFIFO_UNDERFLOW_Msk) {
        wj_tdm_clr_tx_fifo_underflow_irq(tdm_base);

        if (tdm->callback) {
            tdm->callback(tdm, TDM_EVENT_ERROR_UNDERFLOW, tdm->arg);
        }
    }
}

TDM_CODE_IN_RAM void wj_tdm_dma_event_cb(csi_dma_ch_t *dma, csi_dma_event_t event, void *arg)
{
    CSI_PARAM_CHK_NORETVAL(dma);
    csi_tdm_t *tdm = (csi_tdm_t *)dma->parent;
    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    if (event == DMA_EVENT_TRANSFER_ERROR) { /* DMA transfer ERROR */
        if (tdm->rx_dma->ch_id == dma->ch_id) {
            /* to do rx error action */
            tdm->state.error = 1U;

            if (tdm->callback) {
                tdm->callback(tdm, TDM_EVENT_ERROR, tdm->arg);
            }
        } else if (tdm->tx_dma->ch_id == dma->ch_id) {
            tdm->state.error = 1U;

            if (tdm->callback) {
                tdm->callback(tdm, TDM_EVENT_ERROR, tdm->arg);
            }
        }

    } else if (event == DMA_EVENT_TRANSFER_DONE) {
        if ((tdm->tx_dma != NULL) && (tdm->tx_dma->ch_id == dma->ch_id)) {
            uint32_t read_len = tdm->tx_period;

            if (tdm->tx_buf->data_len <= tdm->tx_period) {
                if (tdm->callback) {
                    tdm->callback(tdm, TDM_EVENT_TX_BUFFER_EMPTY, tdm->arg);
                }
            } else {
                tdm->tx_buf->read = (tdm->tx_buf->read + read_len) % tdm->tx_buf->size;
                tdm->tx_buf->data_len -= read_len;

                uint32_t tx_num = 0;
    #if 0
                uint32_t TX_FIFO[10];
                uint32_t send_fifo_Level = 0;
                uint32_t emptyfifo = 0;
                send_fifo_Level = wj_tdm_get_transmit_fifo_level((wj_tdm_regs_t *)tdm->dev.reg_base);
                emptyfifo = (10 > (TDM_MAX_FIFO - send_fifo_Level)) ? TDM_MAX_FIFO - send_fifo_Level : 10;

                memcpy(TX_FIFO, tdm->tx_buf->buffer + tdm->tx_buf->read, emptyfifo * 4);

                for (tx_num = 0; tx_num < (emptyfifo); tx_num ++) {
                    wj_tdm_transmit_data((wj_tdm_regs_t *)tdm->dev.reg_base, TX_FIFO[tx_num]);
                }
    #endif
                csi_dma_ch_start(tdm->tx_dma, tdm->tx_buf->buffer + tdm->tx_buf->read  + (tx_num << 2), (void *) & (tdm_base->TDM_TXFIFO_DATA), tdm->tx_period - (tx_num << 2));

                if (tdm->callback) {
                    tdm->callback(tdm, TDM_EVENT_SEND_COMPLETE, tdm->arg);
                }
            }
        } else {
            /* to do rx action */
            uint32_t write_len = tdm->rx_period;
            tdm->rx_buf->write = (tdm->rx_buf->write + write_len) % tdm->rx_buf->size;
            tdm->rx_buf->data_len += write_len;
            tdm->rx_buf->data_len = tdm->rx_buf->data_len % tdm->rx_buf->size;

            if (tdm->callback && tdm->rx_buf->data_len != 0) {
                uint32_t rx_num = 0U;
    #if 0
                uint32_t RX_FIFO[16];
                uint32_t receive_fifo_level = 0U;
                uint32_t emptyfifo = 0U;
                receive_fifo_level = wj_tdm_get_receive_fifo_level((wj_tdm_regs_t *)tdm->dev.reg_base);

                if (receive_fifo_level > 24U) {
                    emptyfifo = (16U > (TDM_MAX_FIFO - receive_fifo_level)) ? (TDM_MAX_FIFO - receive_fifo_level) : 16U;

                    for (rx_num = 0U; rx_num < emptyfifo; rx_num ++) {
                        RX_FIFO[rx_num] = wj_tdm_receive_data((wj_tdm_regs_t *)tdm->dev.reg_base);
                    }

                    memcpy(tdm->rx_buf->buffer + tdm->rx_buf->write, RX_FIFO, emptyfifo << 2U);
    #endif
                    soc_dcache_clean_invalid_range((unsigned long)(tdm->rx_buf->buffer + tdm->rx_buf->write + (rx_num << 2U)), (tdm->rx_period));
                    csi_dma_ch_start(tdm->rx_dma, (void *) & (tdm_base->TDM_RXFIFO_DATA), tdm->rx_buf->buffer + tdm->rx_buf->write + (rx_num << 2U), (tdm->rx_period));
                // } else {
                //     soc_dcache_clean_invalid_range((unsigned long)(tdm->rx_buf->buffer + tdm->rx_buf->write), tdm->rx_period);
                //     csi_dma_ch_start(tdm->rx_dma, (void *) & (tdm_base->TDM_RXFIFO_DATA), tdm->rx_buf->buffer + tdm->rx_buf->write, tdm->rx_period);
                // }
                tdm->callback(tdm, TDM_EVENT_RECEIVE_COMPLETE, tdm->arg);
            } else {
                tdm->callback(tdm, TDM_EVENT_RX_BUFFER_FULL, tdm->arg);
            }
        }

    }
}

csi_error_t csi_tdm_init(csi_tdm_t *tdm, uint32_t idx)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    ret = target_get(DEV_WJ_TDM_TAG, idx, &tdm->dev);

    tdm->rx_dma = NULL;
    tdm->tx_dma = NULL;
    tdm->rx_buf = NULL;
    tdm->tx_buf = NULL;
    tdm->rx_data         = NULL;
    tdm->tx_data         = NULL;
    tdm->send            = NULL;
    tdm->receive         = NULL;
    tdm->state.error = 0U;
    tdm->state.readable = 1U;
    tdm->state.writeable = 1U;
    tdm->priv = (void *)0U;
    tdm->callback = NULL;
    csi_pin_set_mux(PA10,   PA10_I2S0_MCLK);
    csi_pin_set_mux(PA6,   PA6_I2S0_SCLK);
    csi_pin_set_mux(PA7,  PA7_I2S0_WSCLK);
#if 1
    *(volatile uint32_t *)0x50040008=0x100;
    *(volatile uint32_t *)0x50040090=0x7;
    *(volatile uint32_t *)0x50040098=0xf;
    *(volatile uint32_t *)0x50040000=0x1;
#endif
    wj_ip_ctrl_tdm_disable(PMU_REG_BASE);
    wj_ip_ctrl_tdm_i2s_mono_disable(PMU_REG_BASE);

    wj_tdm_set_i2s_chtx((wj_tdm_regs_t *)tdm->dev.reg_base, 0);
    // wj_tdm_set_i2s_chtx((wj_tdm_regs_t *)tdm->dev.reg_base, 1);
    wj_tdm_set_i2s_chrx((wj_tdm_regs_t *)tdm->dev.reg_base, 2);
    // wj_tdm_set_i2s_chrx((wj_tdm_regs_t *)tdm->dev.reg_base, 3);
    wj_tdm_set_i2s_chen((wj_tdm_regs_t *)tdm->dev.reg_base, 0);
    // wj_tdm_set_i2s_chen((wj_tdm_regs_t *)tdm->dev.reg_base, 1);
    wj_tdm_set_i2s_chen((wj_tdm_regs_t *)tdm->dev.reg_base, 2);
    // wj_tdm_set_i2s_chen((wj_tdm_regs_t *)tdm->dev.reg_base, 3);
    return ret;
}

void csi_tdm_uninit(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK_NORETVAL(tdm);
    wj_tdm_regs_t *tdm_base;
    tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    tdm->rx_dma = NULL;
    tdm->tx_dma = NULL;
    tdm->rx_buf = NULL;
    tdm->tx_buf = NULL;
    tdm->state.error = 0U;
    tdm->state.readable = 0U;
    tdm->state.writeable = 0U;
    wj_tdm_disable(tdm_base);
    wj_ip_ctrl_tdm_disable(PMU_REG_BASE);
    csi_irq_disable((uint32_t)tdm->dev.irq_num);
    csi_irq_detach((uint32_t)tdm->dev.irq_num);
}

void csi_tdm_enable(csi_tdm_t *tdm, bool enable)
{
    CSI_PARAM_CHK_NORETVAL(tdm);
    wj_tdm_regs_t *tdm_base;
    tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);

    if (enable) {
        wj_tdm_enable(tdm_base);
        wj_ip_ctrl_tdm_enable(PMU_REG_BASE);
        wj_tdm_sclk_enable(tdm_base);
    } else {
        wj_tdm_disable(tdm_base);
        wj_ip_ctrl_tdm_disable(PMU_REG_BASE);
        wj_tdm_sclk_disable(tdm_base);
    }
}

csi_error_t csi_tdm_format(csi_tdm_t *tdm, csi_tdm_format_t *format)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    CSI_PARAM_CHK(format, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    wj_tdm_regs_t *tdm_base;
    tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);

    wj_tdm_disable(tdm_base);
    wj_ip_ctrl_tdm_disable(PMU_REG_BASE);
    switch (format->mode) {
        case TDM_MODE_MASTER:
            wj_tdm_master_mode(tdm_base);
            break;

        case TDM_MODE_SLAVE:
            wj_tdm_slave_mode(tdm_base);
            break;

        default:
            ret = CSI_ERROR;
            break;
    }

    // wj_tdm_valid_data_edge(tdm_base, (uint32_t)format->sample_edge);
    switch (format->sample_edge) {
        case TDM_SAMPLE_RISING_EDGE:
            wj_tdm_sclk_polarity_negative_dege(tdm_base);
            break;
        case TDM_SAMPLE_FALL_EDGE:
            wj_tdm_sclk_polarity_posedge_dege(tdm_base);
            break;
        default:
            ret = CSI_UNSUPPORTED;
            break;
    }

    switch (format->protocol) {
        case TDM_PROTOCOL_STANDARD:
            wj_tdm_ws_format_standard(tdm_base);
            break;

        case TDM_PROTOCOL_LSB_JUSTIFIED:
            wj_tdm_ws_format_left_justified(tdm_base);
            break;

        case TDM_PROTOCOL_MSB_JUSTIFIED:
            wj_tdm_ws_format_right_justified(tdm_base);
            break;

        default:
            ret = CSI_UNSUPPORTED;
            break;
    }

    switch (format->width) {
        case TDM_SAMPLE_WIDTH_16BIT:
            wj_tdm_set_slot_width_16bits_mode(tdm_base);
            break;

        case TDM_SAMPLE_WIDTH_24BIT:
            wj_tdm_set_slot_width_24bits_mode(tdm_base);
            break;

        case TDM_SAMPLE_WIDTH_32BIT:
            wj_tdm_set_slot_width_32bits_mode(tdm_base);
            break;

        default:
            ret = CSI_UNSUPPORTED;
            break;
    }

    if (format->polarity) {
        wj_tdm_ws_polarity_i2s_left_posetive(tdm_base);
    } else {
        wj_tdm_ws_polarity_i2s_left_negative(tdm_base);
    }

    if (format->work_mode == TDM_WORKING_MODE_TDM) {
        wj_tdm_tdm_mode(tdm_base);
    } else {
        wj_tdm_i2s_mode(tdm_base);
    }
    // wj_tdm_div0_level(tdm_base, soc_get_audio_clk(0U) / (format->rate * format->sclk_nfs));

    return ret;
}

csi_error_t csi_tdm_rx_link_dma(csi_tdm_t *tdm, csi_dma_ch_t *rx_dma)
{
    CSI_PARAM_CHK(ch, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if (rx_dma != NULL) {
        rx_dma->parent = tdm;
        ret = csi_dma_ch_alloc(rx_dma, -1, -1);

        if (ret == CSI_OK) {
            csi_dma_ch_attach_callback(rx_dma, wj_tdm_dma_event_cb, NULL);
            tdm->rx_dma = rx_dma;
        } else {
            rx_dma->parent = NULL;
            ret = CSI_ERROR;
        }
    } else {
        if (tdm->rx_dma) {
            csi_dma_ch_detach_callback(tdm->rx_dma);
            csi_dma_ch_free(tdm->rx_dma);
            tdm->rx_dma = NULL;
        } else {
            ret = CSI_ERROR;
        }
    }

    return ret;
}

csi_error_t csi_tdm_tx_link_dma(csi_tdm_t *tdm, csi_dma_ch_t *tx_dma)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if (tx_dma != NULL) {
        tx_dma->parent = tdm;
        ret = csi_dma_ch_alloc(tx_dma, -1, -1);

        if (ret == CSI_OK) {
            csi_dma_ch_attach_callback(tx_dma, wj_tdm_dma_event_cb, NULL);
            tdm->tx_dma = tx_dma;
        } else {
            tx_dma->parent = NULL;
            ret = CSI_ERROR;
        }
    } else {
        if (tdm->tx_dma) {
            csi_dma_ch_detach_callback(tdm->tx_dma);
            csi_dma_ch_free(tdm->tx_dma);
            tdm->tx_dma = NULL;
        } else {
            ret = CSI_ERROR;
        }
    }

    return ret;
}

void csi_tdm_rx_set_buffer(csi_tdm_t *tdm, csi_ringbuffer_t *buffer)
{
    CSI_PARAM_CHK_NORETVAL(tdm);
    CSI_PARAM_CHK_NORETVAL(buffer);
    tdm->rx_buf = buffer;
}

void csi_tdm_tx_set_buffer(csi_tdm_t *tdm, csi_ringbuffer_t *buffer)
{
    CSI_PARAM_CHK_NORETVAL(tdm);
    CSI_PARAM_CHK_NORETVAL(buffer);
    tdm->tx_buf = buffer;
}

csi_error_t csi_tdm_rx_set_period(csi_tdm_t *tdm, uint32_t period)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if (period == 0U) {
        ret = CSI_ERROR;
    } else {
        if ((tdm->rx_buf->size % period) != 0U) {
            ret = CSI_ERROR;
        } else {
            tdm->rx_period = period;
        }
    }

    return ret;   
}

csi_error_t csi_tdm_tx_set_period(csi_tdm_t *tdm, uint32_t period)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if (period == 0U) {
        ret = CSI_ERROR;
    } else {
        if ((tdm->tx_buf->size % period) != 0U) {
            ret = CSI_ERROR;
        } else {
            tdm->tx_period = period;
        }
    }

    return ret;
}

uint32_t csi_tdm_rx_buffer_avail(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, 0U);
    return csi_ringbuffer_avail(tdm->rx_buf);
}

uint32_t csi_tdm_rx_buffer_remain(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, 0U);
    return csi_ringbuffer_len(tdm->rx_buf);
}

csi_error_t csi_tdm_rx_buffer_reset(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, NULL);
    csi_error_t ret = CSI_ERROR;
    csi_ringbuffer_reset(tdm->rx_buf);
    if (tdm->rx_buf->buffer != NULL) {
        memset(tdm->rx_buf->buffer, 0, tdm->rx_buf->size);
    }

    if ((tdm->rx_buf->read == 0U) && (tdm->rx_buf->write == 0U)) {
        ret = CSI_OK;
    }
    return ret;
}

uint32_t csi_tdm_tx_buffer_avail(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, 0U);
    return csi_ringbuffer_avail(tdm->tx_buf);   
}

uint32_t csi_tdm_tx_buffer_remain(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, 0U);
    return csi_ringbuffer_len(tdm->tx_buf);
}

csi_error_t csi_tdm_tx_buffer_reset(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, NULL);
    csi_error_t ret = CSI_ERROR;
    csi_ringbuffer_reset(tdm->tx_buf);
    if (tdm->tx_buf->buffer != NULL) {
        memset(tdm->tx_buf->buffer, 0, tdm->tx_buf->size);
    }

    if ((tdm->tx_buf->read == 0U) && (tdm->tx_buf->write == 0U)) {
        ret = CSI_OK;
    }
    return ret;
}

int32_t csi_tdm_receive(csi_tdm_t *tdm, void *data, uint32_t size)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    int32_t received_size = 0;
    uint8_t *read_data = (void *)data;
    int32_t read_size = 0;

    if (tdm->rx_dma == NULL) {
        received_size = tdm_receive_polling(tdm, read_data, size);

        if (received_size == (int32_t)size) {
            read_size = (int32_t)size;
        } else {
            read_size = CSI_ERROR;
        }

    } else {

        while (1) {
            read_size += (int32_t)csi_ringbuffer_out(tdm->rx_buf, (void *)(read_data + (uint32_t)read_size), (size - (uint32_t)read_size));

            if ((size - (uint32_t)read_size) <= 0U) {
                break;
            }
        }

    }


    return read_size;
}

uint32_t csi_tdm_send_dma(csi_tdm_t *tdm, const void *data, uint32_t size)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    uint32_t write_len;

    uint32_t result = csi_irq_save();
    write_len = csi_ringbuffer_in(tdm->tx_buf, data, size);
    csi_irq_restore(result);

    if ((uint8_t *)tdm->priv) { ///< if dma is stop, then start it
        wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
        uint32_t tx_num = 0U;
#if 0
        uint32_t TX_FIFO[16];
        uint32_t send_fifo_level = 0U;
        uint32_t emptyfifo = 0U;
        send_fifo_level = wj_tdm_get_transmit_fifo_level((wj_tdm_regs_t *)tdm->dev.reg_base);
        emptyfifo = (16U > (TDM_MAX_FIFO - send_fifo_level)) ? (TDM_MAX_FIFO - send_fifo_level) : 16U;
        memcpy(TX_FIFO, tdm->tx_buf->buffer + tdm->tx_buf->read, emptyfifo << 2U);

        for (tx_num = 0U; tx_num < emptyfifo; tx_num ++) {
            wj_tdm_transmit_data((wj_tdm_regs_t *)tdm->dev.reg_base, TX_FIFO[tx_num]);
        }

        wj_tdm_get_transmit_fifo_level((wj_tdm_regs_t *)tdm->dev.reg_base);
#endif
        soc_dcache_clean_invalid_range((unsigned long)(tdm->tx_buf->buffer + tdm->tx_buf->read + (tx_num << 2U)), tdm->tx_period - (tx_num << 2U));
        csi_dma_ch_start(tdm->tx_dma, tdm->tx_buf->buffer + tdm->tx_buf->read + (tx_num << 2U), (void *) & (tdm_base->TDM_TXFIFO_DATA), tdm->tx_period - (tx_num << 2U));
        tdm->priv = (void *)0U;
    }

    return write_len;
}

csi_error_t csi_tdm_send_async(csi_tdm_t *tdm, const void *data, uint32_t size)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    csi_error_t ret = CSI_OK;
    // uint32_t write_len;
    // uint32_t result = csi_irq_save();
    // write_len = csi_ringbuffer_in(tdm->tx_buf, data, size);
    // csi_irq_restore(result);

    if ((tdm->state.writeable == 0U) || (tdm->state.readable == 0U)) {
        ret = CSI_BUSY;
    }
    if (tdm->callback != NULL) {
        if (tdm->send) {
            tdm->state.writeable = 0U;
            ret = tdm->send(tdm, data, size);
        } else {
            tdm->state.writeable = 0U;
            csi_irq_attach((uint32_t)tdm->dev.irq_num, &wj_tdm_irqhandler, &tdm->dev);
            csi_irq_enable((uint32_t)tdm->dev.irq_num);
            ret = wj_tdm_send_intr(tdm, data, size);
        }
    } else {
        ret = CSI_ERROR;
    }

    return ret;
}
#define WJ_DEFAULT_TDM_TXFIFO_LV    8U
static csi_error_t wj_tdm_send_intr(csi_tdm_t *tdm, const void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;
    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    tdm->tx_data = (uint8_t *)data;
    uint32_t tx_fifo_lv = 0;
    uint32_t tx_fifo_num = 0;
    uint32_t value = 0;
    uint32_t frame_len   = wj_tdm_get_slot_width_mode(tdm_base);
    if (frame_len <= 1) {
        tdm->tx_size = size;
    } else if (frame_len <= 3) {
        tdm->tx_size = size / 2U;
    } else {
        ret = CSI_ERROR;
        return ret;
    }
    /* set tx mode*/
    tx_fifo_lv = (tdm->tx_size < WJ_DEFAULT_TDM_TXFIFO_LV) ? (tdm->tx_size) : WJ_DEFAULT_TDM_TXFIFO_LV;
    wj_tdm_set_txdma_data_level(tdm_base, tx_fifo_lv);
#if 1
    tx_fifo_num = (tdm->tx_size <= (WJ_DEFAULT_TDM_TXFIFO_LV + 4)) ? (tdm->tx_size) : (WJ_DEFAULT_TDM_TXFIFO_LV + 4);
    if (frame_len <= 1U) {
        while ((tdm->tx_size) && tx_fifo_num) {
            /* process 4~8bit frame len */
            while((wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Msk));
            value = (uint32_t)(*(uint8_t *)(tdm->tx_data));
            tdm->tx_data += sizeof(uint8_t);
            wj_tdm_write_data(tdm_base, value);
            tx_fifo_num--;
            (tdm->tx_size)--;
        }
    } else if (frame_len <= 3U) {
        while ((tdm->tx_size) && tx_fifo_num) {
            /* process 8~16bit frame len */
            while((wj_tdm_get_fifo_status(tdm_base) & WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Msk));
            value = (uint32_t)(*(uint16_t *)(tdm->tx_data));
            tdm->tx_data += sizeof(uint16_t);
            wj_tdm_write_data(tdm_base, value);
            tx_fifo_num--;
            (tdm->tx_size)--;
        }
    }
    wj_tdm_clr_tx_fifo_dataready_irq(tdm_base);
#endif
    if (tdm->tx_size) {
        wj_tdm_enable_tx_data_ready_irq(tdm_base);
    }
    return ret;
}

int32_t csi_tdm_send(csi_tdm_t *tdm, const void *data, uint32_t size)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    int32_t sent_size = 0;
    int32_t write_size = 0;
    uint8_t *send_data = (void *)data;

    if (tdm->tx_dma == NULL) {
        sent_size = tdm_send_polling(tdm, send_data, size);

        if (sent_size == (int32_t)size) {
            write_size = sent_size;
        } else {
            write_size = CSI_ERROR;
        }

    } else {
        while (1) {
            write_size += (int32_t)csi_ringbuffer_in(tdm->tx_buf, (void *)(send_data + (uint32_t)write_size), (size - (uint32_t)write_size));

            if ((uint8_t *)tdm->priv) { ///< if dma is stop, then start it
                wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
                uint32_t tx_num = 0U;
#if 0
                uint32_t TX_FIFO[10];
                uint32_t send_fifo_level = 0U;
                uint32_t emptyfifo = 0U;
                send_fifo_level = wj_tdm_get_transmit_fifo_level((wj_tdm_regs_t *)tdm->dev.reg_base);
                emptyfifo = (10U > (TDM_MAX_FIFO - send_fifo_level)) ? (TDM_MAX_FIFO - send_fifo_level) : 10U;
                memcpy(TX_FIFO, tdm->tx_buf->buffer + tdm->tx_buf->read, emptyfifo << 2U);

                for (tx_num = 0U; tx_num < emptyfifo; tx_num ++) {
                    wj_tdm_transmit_data((wj_tdm_regs_t *)tdm->dev.reg_base, TX_FIFO[tx_num]);
                }

                wj_tdm_get_transmit_fifo_level((wj_tdm_regs_t *)tdm->dev.reg_base);
#endif
                soc_dcache_clean_invalid_range((unsigned long)(tdm->tx_buf->buffer + tdm->tx_buf->read + (tx_num << 2U)), tdm->tx_period - (tx_num << 2U));
                csi_dma_ch_start(tdm->tx_dma, tdm->tx_buf->buffer + tdm->tx_buf->read + (tx_num << 2U), (void *) & (tdm_base->TDM_TXFIFO_DATA), tdm->tx_period - (tx_num << 2U));
                tdm->priv = (void *)0U;
            }

            if ((size - (uint32_t)write_size) <= 0U) {
                break;
            }
        }

        while (!csi_ringbuffer_is_empty(tdm->tx_buf));
    }

    return write_size;
}

uint32_t csi_tdm_receive_dma(csi_tdm_t *tdm, void *data, uint32_t size)
{
    CSI_PARAM_CHK(tdm, 0U);
    CSI_PARAM_CHK(data, 0U);
    uint32_t read_len;

    uint32_t result = csi_irq_save();
    read_len = csi_ringbuffer_out(tdm->rx_buf, (void *)data, size);
    csi_irq_restore(result);
    return read_len;
}

csi_error_t csi_tdm_receive_async(csi_tdm_t *tdm, void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;

    if ((tdm->state.writeable == 0U) || (tdm->state.readable == 0U)) {
        ret = CSI_BUSY;
    } else {

        if ((ret == CSI_OK) && (tdm->callback != NULL)) {
            if (tdm->receive) {
                tdm->state.readable = 0U;
                ret = tdm->receive(tdm, data, size);
            } else {
                tdm->state.readable = 0U;
                csi_irq_attach((uint32_t)tdm->dev.irq_num, &wj_tdm_irqhandler, &tdm->dev);
                csi_irq_enable((uint32_t)tdm->dev.irq_num);
                ret = wj_tdm_receive_intr(tdm, data, size);
            }
        } else {
            ret = CSI_ERROR;
        }
    }
    return ret;

}

static csi_error_t wj_tdm_receive_intr(csi_tdm_t *tdm, void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;
    uint32_t rx_fifo_lv;

    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);

    tdm->rx_data = (uint8_t *)data;

    do {
        // Convert byte to nums
        uint32_t frame_len   = wj_tdm_get_slot_width_mode(tdm_base);
        if (frame_len <= 1) {
            tdm->rx_size = size;
        } else if (frame_len <= 3) {
            tdm->rx_size = size / 2U;
        } else {
            ret = CSI_ERROR;
            break;
        }

        /* set rx mode*/
        rx_fifo_lv = (tdm->rx_size < WJ_DEFAULT_TDM_RXFIFO_LV) ? (tdm->rx_size - 1U) : WJ_DEFAULT_TDM_RXFIFO_LV;
        // wj_tdm_flush_rxfifo(tdm_base);
        wj_tdm_set_rxdma_data_level(tdm_base, rx_fifo_lv);
        wj_tdm_enable_rx_data_ready_irq(tdm_base);
    } while (0);

    return ret;
}

csi_error_t csi_tdm_send_pause(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    csi_dma_ch_stop(tdm->tx_dma);
    tdm->state.writeable = 0U;
    return ret;
}

csi_error_t csi_tdm_send_resume(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    uint32_t tx_num = 0U;
#if 0
    uint32_t TX_FIFO[16];
    uint32_t send_fifo_level = 0U;
    uint32_t emptyfifo = 0U;
    send_fifo_level = wj_tdm_get_transmit_fifo_level((wj_tdm_regs_t *)tdm->dev.reg_base);

    if (send_fifo_level < 9U) {
        emptyfifo = (16U > (TDM_MAX_FIFO - send_fifo_level)) ? (TDM_MAX_FIFO - send_fifo_level) : 16U;
        memcpy(TX_FIFO, tdm->tx_buf->buffer + tdm->tx_buf->read, emptyfifo << 2U);

        for (tx_num = 0U; tx_num < emptyfifo; tx_num ++) {
            wj_tdm_transmit_data((wj_tdm_regs_t *)tdm->dev.reg_base, TX_FIFO[tx_num]);
        }
#endif
        soc_dcache_clean_invalid_range((unsigned long)(tdm->tx_buf->buffer + tdm->tx_buf->read + (tx_num << 2U)), tdm->tx_period - (tx_num << 2U));
        csi_dma_ch_start(tdm->tx_dma, tdm->tx_buf->buffer + tdm->tx_buf->read + (tx_num << 2U), (void *) & (tdm_base->TDM_TXFIFO_DATA), tdm->tx_period - (tx_num << 2U));
    // } else {
    //     soc_dcache_clean_invalid_range((unsigned long)(tdm->tx_buf->buffer + tdm->tx_buf->read), tdm->tx_period);
    //     csi_dma_ch_start(tdm->tx_dma, (tdm->tx_buf->buffer + tdm->tx_buf->read), (void *) & (tdm_base->TDM_TXFIFO_DATA), tdm->tx_period);
    //     wj_tdm_get_transmit_fifo_level(tdm_base);
    // }

    tdm->state.writeable = 1U;
    return ret;
}

csi_error_t csi_tdm_send_start(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    csi_dma_ch_config_t config;
    memset(&config, 0, sizeof(csi_dma_ch_config_t));
    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    csi_dma_ch_t *dma_ch = (csi_dma_ch_t *)tdm->tx_dma;

    wj_tdm_disable(tdm_base);
    wj_ip_ctrl_tdm_disable(PMU_REG_BASE);
    wj_tdm_set_txdma_data_level(tdm_base, 0U);
    wj_tdm_txdma_enable(tdm_base);

    config.src_inc = DMA_ADDR_INC;
    config.dst_inc = DMA_ADDR_CONSTANT;
    config.src_tw = DMA_DATA_WIDTH_16_BITS;
    config.dst_tw = DMA_DATA_WIDTH_16_BITS;
    /* config for wj_dma */
    config.group_len = 1U;
    config.trans_dir = DMA_MEM2PERH;
    config.src_reload_en = 0U;
    config.dst_reload_en = 0U;
    config.half_int_en = 0U;
    /* config for etb */
    config.handshake = tdm_tx_hs_num[tdm->dev.idx];

    wj_tdm_enable(tdm_base);
    wj_ip_ctrl_tdm_enable(PMU_REG_BASE);
    wj_tdm_sclk_enable(tdm_base);
    // wj_tdm_flush_txfifo(tdm_base);
    // memset(tdm->tx_buf->buffer, 0, tdm->tx_buf->size);
    ret = csi_dma_ch_config(dma_ch, &config);
    soc_dcache_clean_invalid_range((unsigned long)(tdm->tx_buf->buffer + tdm->tx_buf->read), (tdm->tx_period));
    csi_dma_ch_start(tdm->tx_dma, tdm->tx_buf->buffer + tdm->tx_buf->read, (void *) & (tdm_base->TDM_TXFIFO_DATA), (tdm->tx_period));
    tdm->state.writeable = 1U;

    return ret;
}

csi_error_t csi_tdm_receive_start(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    csi_dma_ch_config_t config;
    memset(&config, 0U, sizeof(csi_dma_ch_config_t));
    wj_tdm_regs_t *tdm_base;
    tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    csi_dma_ch_t *dma_ch = (csi_dma_ch_t *)tdm->rx_dma;

    wj_tdm_disable(tdm_base);
    wj_ip_ctrl_tdm_disable(PMU_REG_BASE);
    wj_tdm_set_rxdma_data_level(tdm_base, 0U);
    wj_tdm_rxdma_enable(tdm_base);

    config.src_inc = DMA_ADDR_CONSTANT;
    config.dst_inc = DMA_ADDR_INC;
    config.src_tw = DMA_DATA_WIDTH_8_BITS;
    config.dst_tw = DMA_DATA_WIDTH_8_BITS;
    /* config for wj_dma */
    config.group_len = 1U;
    config.trans_dir = DMA_PERH2MEM;
    config.src_reload_en = 0U;
    config.dst_reload_en = 0U;
    config.half_int_en = 0U;
    /* config for etb */
    config.handshake = tdm_rx_hs_num[tdm->dev.idx];

    wj_tdm_enable(tdm_base);
    wj_ip_ctrl_tdm_enable(PMU_REG_BASE);
    wj_tdm_sclk_enable(tdm_base);
    // memset(tdm->rx_buf->buffer, 0, tdm->rx_buf->size);
    ret = csi_dma_ch_config(dma_ch, &config);

    // wj_tdm_flush_rxfifo(tdm_base);
    soc_dcache_clean_invalid_range((unsigned long)(tdm->rx_buf->buffer + tdm->rx_buf->write), (tdm->rx_period));
    csi_dma_ch_start(tdm->rx_dma, (void *) & (tdm_base->TDM_RXFIFO_DATA), tdm->rx_buf->buffer + tdm->rx_buf->write, (tdm->rx_period));
    tdm->state.readable = 1U;
    return ret;
}

// csi_error_t csi_tdm_all_ch_receive_start(csi_tdm_t *tdm)
// {
//     CSI_PARAM_CHK(tdm, CSI_ERROR);
//     csi_error_t ret = CSI_OK;
//     csi_tdm_ch_t *alloced_ch;
//     slist_for_each_entry(&tdm->head, alloced_ch, csi_tdm_ch_t, next) {
//         csi_dma_ch_config_t config;
//         memset(&config, 0U, sizeof(csi_dma_ch_config_t));
//         wj_tdm_regs_t *tdm_base;
//         tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(alloced_ch->tdm);
//         csi_dma_ch_t *dma_ch = (csi_dma_ch_t *)alloced_ch->rx_dma;

//         wj_tdm_set_dma_data_level(tdm_base, 4U);

//         config.src_inc = DMA_ADDR_CONSTANT;
//         config.dst_inc = DMA_ADDR_INC;
//         config.src_tw = DMA_DATA_WIDTH_32_BITS;
//         config.dst_tw = DMA_DATA_WIDTH_32_BITS;
//         /* config for wj_dma */
//         config.group_len = 1U;
//         config.trans_dir = DMA_PERH2MEM;
//         config.src_reload_en = 0U;
//         config.dst_reload_en = 0U;
//         config.half_int_en = 0U;
//         /* config for etb */
//         config.handshake = tdm_rx_hs_num[alloced_ch->tdm->dev.idx];

//         /* config for dma cyclic mode*/
//         config.cyclic_en = 1U;
//         config.cyclic_config.buffer = alloced_ch->rx_buf->buffer;
//         config.cyclic_config.buffer_bytes = alloced_ch->rx_buf->size;
//         config.cyclic_config.period_bytes = alloced_ch->rx_period;

//         switch (alloced_ch->ch_idx) {
//             case 0U:
//                 config.cyclic_config.lli_buf = tdm_ch0_receive_buf;
//                 config.cyclic_config.srcaddr = (void *) & (tdm_base->LDR1);
//                 break;

//             case 1U:
//                 config.cyclic_config.lli_buf = tdm_ch1_receive_buf;
//                 config.cyclic_config.srcaddr = (void *) & (tdm_base->RDR1);
//                 break;

//             case 2U:
//                 config.cyclic_config.lli_buf = tdm_ch2_receive_buf;
//                 config.cyclic_config.srcaddr = (void *) & (tdm_base->LDR2);
//                 break;

//             case 3U:
//                 config.cyclic_config.lli_buf = tdm_ch3_receive_buf;
//                 config.cyclic_config.srcaddr = (void *) & (tdm_base->RDR2);
//                 break;

//             case 4U:
//                 config.cyclic_config.lli_buf = tdm_ch4_receive_buf;
//                 config.cyclic_config.srcaddr = (void *) & (tdm_base->LDR3);
//                 break;

//             case 5U:
//                 config.cyclic_config.lli_buf = tdm_ch5_receive_buf;
//                 config.cyclic_config.srcaddr = (void *) & (tdm_base->RDR3);
//                 break;

//             case 6U:
//                 config.cyclic_config.lli_buf = tdm_ch6_receive_buf;
//                 config.cyclic_config.srcaddr = (void *) & (tdm_base->LDR4);
//                 break;

//             case 7U:
//                 config.cyclic_config.lli_buf = tdm_ch7_receive_buf;
//                 config.cyclic_config.srcaddr = (void *) & (tdm_base->RDR4);
//                 break;

//             default:
//                 ret = CSI_ERROR;
//                 break;
//         }

//         if (ret != CSI_OK) {
//             break;
//         }

//         ret = csi_dma_ch_config(dma_ch, &config);

//         if (ret != CSI_OK) {
//             break;
//         }

//         soc_dcache_clean_invalid_range((unsigned long)(alloced_ch->rx_buf->buffer), alloced_ch->rx_buf->size);
//         csi_dma_ch_start(alloced_ch->rx_dma, NULL, NULL, 0U);
//         alloced_ch->state.readable = 1U;
//     }
//     return ret;
// }


void csi_tdm_send_stop(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK_NORETVAL(tdm);
    wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    csi_dma_ch_stop(tdm->tx_dma);
    csi_ringbuffer_reset(tdm->tx_buf);
    memset(tdm->tx_buf->buffer, 0, tdm->tx_buf->size);

    wj_tdm_flush_txfifo(tdm_base);
    tdm->state.writeable = 0U;
}

void csi_tdm_receive_stop(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK_NORETVAL(tdm);
    wj_tdm_regs_t *tdm_base;
    tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
    csi_dma_ch_stop(tdm->rx_dma);
    csi_ringbuffer_reset(tdm->rx_buf);
    memset(tdm->rx_buf->buffer, 0, tdm->rx_buf->size);
    wj_tdm_flush_rxfifo(tdm_base);
    tdm->state.readable = 0U;    
}

// void csi_tdm_all_ch_receive_stop(csi_tdm_t *tdm)
// {
//     CSI_PARAM_CHK_NORETVAL(tdm);
//     csi_tdm_ch_t *alloced_ch;
//     slist_for_each_entry(&tdm->head, alloced_ch, csi_tdm_ch_t, next) {
//         csi_dma_ch_stop(alloced_ch->rx_dma);
//         csi_ringbuffer_reset(alloced_ch->rx_buf);
//         alloced_ch->state.readable = 0U;
//     }    
// }

csi_error_t csi_tdm_attach_callback(csi_tdm_t *tdm, void *callback, void *arg)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    tdm->callback = callback;
    tdm->arg = arg;
    tdm->send         = NULL;
    tdm->receive      = NULL;
    return ret;
}

void csi_tdm_detach_callback(csi_tdm_t *tdm)
{
    CSI_PARAM_CHK_NORETVAL(tdm);
    tdm->callback = NULL;
    tdm->arg = NULL;
    tdm->send         = NULL;
    tdm->receive      = NULL;
}

// csi_error_t csi_tdm_ch_alloc(csi_tdm_t *tdm, csi_tdm_ch_t *ch, uint32_t ch_idx)
// {
//     CSI_PARAM_CHK(tdm, CSI_ERROR);
//     CSI_PARAM_CHK(ch, CSI_ERROR);
//     csi_error_t ret = CSI_OK;
//     csi_tdm_ch_t *alloced_ch;
//     slist_for_each_entry(&tdm->head, alloced_ch, csi_tdm_ch_t, next) {
//         if (alloced_ch->ch_idx == ch_idx) {
//             ret = CSI_ERROR;
//             break;
//         }
//     }

//     if (ret == CSI_OK) {
//         slist_add(&ch->next, &tdm->head);
//         ch->ch_idx = ch_idx;
//         ch->tdm = tdm;
//         ch->rx_dma = NULL;
//         ch->rx_buf = NULL;
//         ch->state.error = 0U;
//         ch->state.readable = 0U;
//         ch->priv = (void *)0U;
//         ch->callback = NULL;

//         wj_tdm_regs_t *tdm_base = (wj_tdm_regs_t *)HANDLE_REG_BASE(tdm);
//         wj_tdm_disable(tdm_base);

//         switch (ch_idx / 2U) {
//             case 0U:
//                 wj_tdm_set_ch_num(tdm_base, 0U);
//                 break;

//             case 1U:
//                 wj_tdm_set_ch_num(tdm_base, 1U);
//                 break;

//             case 2U:
//                 wj_tdm_set_ch_num(tdm_base, 2U);
//                 break;

//             case 3U:
//                 wj_tdm_set_ch_num(tdm_base, 3U);
//                 break;
            
//             default:
//                 ret = CSI_ERROR;
//             break;
//         }

//     }

//     return ret;   
// }

// csi_error_t csi_tdm_ch_free(csi_tdm_t *tdm, csi_tdm_ch_t *ch)
// {
//     CSI_PARAM_CHK(tdm, CSI_ERROR);
//     CSI_PARAM_CHK(ch, CSI_ERROR);
//     csi_error_t ret = CSI_ERROR;
//     csi_tdm_ch_t *alloced_ch;
//     slist_for_each_entry(&tdm->head, alloced_ch, csi_tdm_ch_t, next) {
//         if (alloced_ch->ch_idx == ch->ch_idx) {
//             ch->rx_dma = NULL;
//             ch->rx_buf = NULL;
//             ch->state.error = 0U;
//             ch->state.readable = 0U;
//             ch->priv = (void *)0U;
//             ch->callback = NULL;
//             slist_remove(&ch->next, &tdm->head);
//             ret = CSI_OK;
//             break;
//         }
//     }
//     return ret;    
// }


csi_error_t csi_tdm_get_state(csi_tdm_t *tdm, csi_state_t *state)
{
    CSI_PARAM_CHK(tdm, CSI_ERROR);
    CSI_PARAM_CHK(state, CSI_ERROR);
    *state = tdm->state;
    return CSI_OK;
}

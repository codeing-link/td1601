/******************************************************************************
 * @file     dw_ospi.c
 * @brief
 * @version
 * @date     2026-04-24
 ******************************************************************************/
/**
 * @brief Data Frame Size (DFS) encoding for DW OSPI controller
 * 
 * DFS field in CTRLR0 register maps to actual data frame length:
 * 
 * +------------+------------------+-------------------------+
 * | DFS Value  | Frame Length     | Typical Use Case        |
 * +------------+------------------+-------------------------+
 * | 0x00-0x02  | Reserved         | DO NOT USE              |
 * | 0x03       | 4 bits           | 4-bit SPI devices       |
 * | 0x04       | 5 bits           | Special protocols       |
 * | 0x05       | 6 bits           | Special protocols       |
 * | 0x06       | 7 bits           | Legacy SPI (e.g., 7-bit addr) |
 * | 0x07       | 8 bits (default) | Standard SPI byte transfer |
 * | 0x08       | 9 bits           | 9-bit SPI DACs/ADCs     |
 * | 0x09       | 10 bits          | 10-bit devices          |
 * | 0x0A       | 11 bits          | 11-bit devices          |
 * | 0x0B       | 12 bits          | 12-bit ADCs             |
 * | 0x0C       | 13 bits          | 13-bit devices          |
 * | 0x0D       | 14 bits          | 14-bit devices          |
 * | 0x0E       | 15 bits          | 15-bit devices          |
 * | 0x0F       | 16 bits          | 16-bit SPI devices      |
 * | 0x10       | 17 bits          | Special protocols       |
 * | 0x11       | 18 bits          | Special protocols       |
 * | 0x12       | 19 bits          | Special protocols       |
 * | 0x13       | 20 bits          | Special protocols       |
 * | 0x14       | 21 bits          | Special protocols       |
 * | 0x15       | 22 bits          | Special protocols       |
 * | 0x16       | 23 bits          | Special protocols       |
 * | 0x17       | 24 bits          | 24-bit (e.g., 3-byte address) |
 * | 0x18       | 25 bits          | Special protocols       |
 * | 0x19       | 26 bits          | Special protocols       |
 * | 0x1A       | 27 bits          | Special protocols       |
 * | 0x1B       | 28 bits          | Special protocols       |
 * | 0x1C       | 29 bits          | Special protocols       |
 * | 0x1D       | 30 bits          | Special protocols       |
 * | 0x1E       | 31 bits          | Special protocols       |
 * | 0x1F       | 32 bits          | 32-bit transfers        |
 * +------------+------------------+-------------------------+
 * 
 * @note When using Dual/Quad/Octal modes, DFS must meet alignment requirements:
 *       - Dual mode:   DFS % 2 == 0
 *       - Quad mode:   DFS % 4 == 0
 *       - Octal mode:  DFS % 8 == 0
 * 
 * @note Common DFS values:
 *       - 0x07 (8 bits):  Standard SPI, most common
 *       - 0x0F (16 bits): 16-bit devices, or when 2-byte alignment needed
 *       - 0x1F (32 bits): 32-bit devices, or best performance on 32-bit bus
 * 
 * @note When DFS < 32 bits:
 *       - TX data must be right-justified before writing to FIFO
 *       - RX data is automatically right-justified, upper bits zero-padded
 */

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

static inline bool is_8bit_frame(uint32_t frame_len)  { return frame_len <= 8;  }
static inline bool is_16bit_frame(uint32_t frame_len) { return frame_len > 8  && frame_len <= 16; }
static inline bool is_32bit_frame(uint32_t frame_len) { return frame_len > 16 && frame_len <= 32; }

#define DW_OSPI_HS_OFFSET               (2U)

#define DW_OSPI_SET_REG_IDX_MASTER(ospi) (ospi->priv = (void *)0U)
#define DW_OSPI_SET_REG_IDX_SLAVE(ospi)  (ospi->priv = (void *)1U)
#define DW_OSPI_SET_REG_IDX_COMMON(ospi) (ospi->priv = (void *)1U)
#define DW_OSPI_GET_REG_IDX(ospi)        ((unsigned long)ospi->priv)
#define IS_DW_OSPI_IDX_MASTER(ospi)      ((unsigned long)ospi->priv == 0U)
#define IS_DW_OSPI_IDX_SLAVE(ospi)       ((unsigned long)ospi->priv == 1U)

extern uint16_t ospi_tx_hs_num[];
extern uint16_t ospi_rx_hs_num[];

static csi_error_t dw_ospi_send_intr(csi_ospi_t *ospi, const void *data, uint32_t size);
static csi_error_t dw_ospi_receive_intr(csi_ospi_t *ospi, void *data, uint32_t size);
static csi_error_t dw_ospi_send_receive_intr(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t num);

static dw_ospi_regs_t *dw_get_reg_base(csi_ospi_t *ospi)
{
    dw_ospi_regs_t *ospi_base;
    unsigned long reg_base;
    uint32_t idx = 0U;

    idx = DW_OSPI_GET_REG_IDX(ospi);
    reg_base = HANDLE_REG_BASE(ospi) + (idx * (uint32_t)0x100U);
    ospi_base = (dw_ospi_regs_t *)reg_base;
    return ospi_base;
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
    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);

    timestart = csi_tick_get_ms();

    while (dw_ospi_get_status(ospi_base) & DW_OSPI_SR_BUSY) {
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

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);
    mode = dw_ospi_get_transfer_mode(ospi_base);

    /* process end of transmit */
    if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) ==  DW_OSPI_CTRLR0_TMOD_TX) {
        if (ospi->tx_size == 0U) {
            if ((dw_ospi_get_status(ospi_base) & DW_OSPI_SR_BUSY) == 0U) {
                dw_ospi_disable_tx_empty_irq(ospi_base);
                dw_ospi_config_tx_fifo_threshold(ospi_base, 0U);
                ospi->state.writeable = 1U;

                if (ospi->callback) {
                    ospi->callback(ospi, OSPI_EVENT_SEND_COMPLETE, ospi->arg);
                }
				ospi->cmd_sent = 0U;
            }
        }
    }

    /* process end of receive */
    else if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) == DW_OSPI_CTRLR0_TMOD_RX) {
        if (ospi->rx_size == 0U) {
            dw_ospi_disable_rx_fifo_full_irq(ospi_base);
            dw_ospi_config_rx_data_len(ospi_base, 0U);
            dw_ospi_config_rx_fifo_threshold(ospi_base, 0U);

            if (ospi->callback) {
                ospi->callback(ospi, OSPI_EVENT_RECEIVE_COMPLETE, ospi->arg);
            }

            ospi->state.readable = 1U;
			ospi->cmd_sent = 0U;
        }
    }

    /* process end of transmit & receive */
    else if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) == DW_OSPI_CTRLR0_TMOD_TX_RX) {
        if ((ospi->rx_size == 0U) && (ospi->tx_size == 0U)) {
            dw_ospi_disable_tx_empty_irq(ospi_base);
            dw_ospi_disable_rx_fifo_full_irq(ospi_base);
            dw_ospi_config_tx_fifo_threshold(ospi_base, 0U);
            dw_ospi_config_rx_fifo_threshold(ospi_base, 0U);
            ospi->state.readable  = 1U;
            ospi->state.writeable = 1U;

            if (ospi->callback) {
                ospi->callback(ospi, OSPI_EVENT_SEND_RECEIVE_COMPLETE, ospi->arg);
            }
			ospi->cmd_sent = 0U;
        } else if (ospi->tx_size == 0U) {
            // reduce interrupt times
            dw_ospi_disable_tx_empty_irq(ospi_base);
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

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);

    remain_fifo = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(ospi_base);
    frame_len   = dw_ospi_get_data_frame_len(ospi_base);

    process_end_transcation(ospi);
    if (ospi->tx_size == 0U) {
      return;
    }
    /* transfer loop */
    tx_data = ospi->tx_data;
    tx_size = ospi->tx_size;
	
    if (!ospi->cmd_sent) {

        if (!ospi->cmd->instruction.disabled) {
            if (remain_fifo >= 1U) {                     
                dw_ospi_transmit_data(ospi_base, ospi->cmd->instruction.value);
                remain_fifo -= 1U;                     
            }
        }

        if (!ospi->cmd->address.disabled) {
            if (remain_fifo >= 1U) {                   
                dw_ospi_transmit_data(ospi_base, ospi->cmd->address.value);
                remain_fifo -= 1U;                   
            }
        }

        ospi->cmd_sent = 1U;
    }
	
    if (frame_len <= 8U) {
        while (tx_size && remain_fifo) {
            /* process 4~8bit frame len */
            value = (uint32_t)(*(uint8_t *)tx_data);
            tx_data += sizeof(uint8_t);
            dw_ospi_transmit_data(ospi_base, value);
            remain_fifo--;
            tx_size--;
        }
    } else if ((frame_len > 8U) && (frame_len <= 16U)) {
        while (tx_size && remain_fifo) {
            /* process 8~16bit frame len */
            value = (uint32_t)(*(uint16_t *)tx_data);
            tx_data += sizeof(uint16_t);
            dw_ospi_transmit_data(ospi_base, value);
            remain_fifo--;
            tx_size--;
        }
    } else if ((frame_len > 16U) && (frame_len <= 32U)) {
        while (tx_size && remain_fifo) {
            /* process 8~16bit frame len */
            value = (uint32_t)(*(uint32_t *)tx_data);
            tx_data += sizeof(uint32_t);
            dw_ospi_transmit_data(ospi_base, value);
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

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);
    fifo_size = dw_ospi_get_rx_fifo_level(ospi_base);
    frame_len = dw_ospi_get_data_frame_len(ospi_base);
    rx_data = ospi->rx_data;
    rx_size = ospi->rx_size;

    /* transfer loop */
    if (frame_len <= 8U) {
        while (rx_size && fifo_size) {
            *(uint8_t *)rx_data = (uint8_t)dw_ospi_receive_data(ospi_base);
            rx_data += sizeof(uint8_t);
            fifo_size--;
            rx_size--;
        }
    } else if ((frame_len > 8U) && (frame_len <= 16U)) {
        while (rx_size && fifo_size) {
            *(uint16_t *)rx_data = (uint16_t)dw_ospi_receive_data(ospi_base);
            rx_data += sizeof(uint16_t);
            fifo_size--;
            rx_size--;
        }
    } else if ((frame_len > 16U) && (frame_len <= 32U)) {
        while (rx_size && fifo_size) {
            *(uint32_t *)rx_data = (uint32_t)dw_ospi_receive_data(ospi_base);
            rx_data += sizeof(uint32_t);
            fifo_size--;
            rx_size--;
        }
    }
    /* update rx fifo threshold when remain size less then default threshold*/
    if ((rx_size < (DW_DEFAULT_OSPI_RXFIFO_LV + 1U)) && (rx_size > 0U)) {
        dw_ospi_config_rx_fifo_threshold(ospi_base, rx_size - 1U);
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
    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);

    status = dw_ospi_get_interrupt_status(ospi_base);

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
        dw_ospi_clr_multi_master_irq(ospi_base);

        if (ospi->callback) {
            ospi->callback(ospi, OSPI_EVENT_ERROR, ospi->arg);
        }
    }

    /* process receive fifo overflow interrupt */
    if (status & DW_OSPI_ISR_RXOIS) {
        dw_ospi_clr_rx_fifo_overflow_irq(ospi_base);

        if (ospi->callback) {
            ospi->callback(ospi, OSPI_EVENT_ERROR_OVERFLOW, ospi->arg);
        }
    }

    /* process transmit fifo overflow interrupt */
    if (status & DW_OSPI_ISR_TXOIS) {
        dw_ospi_clr_tx_fifo_overflow_irq(ospi_base);

        if (ospi->callback) {
            ospi->callback(ospi, OSPI_EVENT_ERROR_OVERFLOW, ospi->arg);
        }
    }

    /* process receive fifo underflow interrupt */
    if (status & DW_OSPI_ISR_RXUIS) {
        dw_ospi_clr_rx_fifo_underflow_irq(ospi_base);

        if (ospi->callback) {
            ospi->callback(ospi, OSPI_EVENT_ERROR_UNDERFLOW, ospi->arg);
        }
    }
}

static void dw_ospi_dma_event_cb(csi_dma_ch_t *dma, csi_dma_event_t event, void *arg)
{
    dw_ospi_regs_t *ospi_base;
    csi_ospi_t *ospi = (csi_ospi_t *)dma->parent;
    uint32_t mode;

    ospi_base = dw_get_reg_base(ospi);
    mode = dw_ospi_get_transfer_mode(ospi_base);

    if (event == DMA_EVENT_TRANSFER_DONE) {
        /* process end of transmit */
        if ((ospi->tx_dma != NULL) && (ospi->tx_dma->ch_id == dma->ch_id)) {
            csi_dma_ch_stop(dma);
            dw_ospi_disable_tx_dma(ospi_base);

            if (wait_ready_until_timeout(ospi, DW_DEFAULT_TRANSCATION_TIMEOUT) == CSI_OK) {

                ospi->state.writeable = 1U;
                ospi->tx_size = 0U;

                if ((mode & DW_OSPI_CTRLR0_TMOD_Msk) == DW_OSPI_CTRLR0_TMOD_TX) {
                    dw_ospi_config_dma_tx_data_level(ospi_base, 0U);

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
            dw_ospi_disable_rx_dma(ospi_base);
            dw_ospi_config_dma_rx_data_level(ospi_base, 0U);
            dw_ospi_config_rx_data_len(ospi_base, 0U);
            soc_dcache_clean_invalid_range((unsigned long)ospi->rx_data,ospi->rx_size);
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

static csi_error_t _ospi_check_command(csi_ospi_t *ospi, csi_ospi_command_t* cmd)
{
    CSI_PARAM_CHK(cmd, CSI_ERROR);
    CSI_PARAM_CHK(ospi,CSI_ERROR);

    dw_ospi_regs_t *ospi_base;
    csi_error_t   ret = CSI_OK;
    ospi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(ospi_base);

    if(!cmd->instruction.disabled)
    {
        if(cmd->instruction.size >= 0 && cmd->instruction.size <= 3)
        {
            dw_ospi_spi_ctrl0_set_inst_l(ospi_base, cmd->instruction.size);
        }
        else
        {
            ret = CSI_ERROR;
        }
    }
    else
    {
        dw_ospi_spi_ctrl0_set_inst_l(ospi_base, 0x0);
    }

    if(!cmd->address.disabled)
    {
        switch (cmd->address.size)
        {
        case OSPI_ADDRESS_0_BITS:
            dw_ospi_spi_ctrl0_set_addr_l(ospi_base, 0x0);
            break;
        case OSPI_ADDRESS_8_BITS:
            dw_ospi_spi_ctrl0_set_addr_l(ospi_base, 0x2);
            break;
        case OSPI_ADDRESS_16_BITS:
            dw_ospi_spi_ctrl0_set_addr_l(ospi_base, 0x4);
            break;
        case OSPI_ADDRESS_24_BITS:
            dw_ospi_spi_ctrl0_set_addr_l(ospi_base, 0x6);
            break;
        case OSPI_ADDRESS_32_BITS:
            dw_ospi_spi_ctrl0_set_addr_l(ospi_base, 0x8);
            break;
        default:
            return CSI_ERROR;
        }
    }
    else
    {
        dw_ospi_spi_ctrl0_set_addr_l(ospi_base, 0x0);
    }    

    if(!cmd->alt.disabled)
    {
        return CSI_ERROR;
    }

    if(!cmd->data.disabled)
    {
        switch (cmd->data.transfer_mode)
        {
        case OSPI_TRANSFER_SEND_RECEIVE:
            dw_ospi_set_tx_rx_mode(ospi_base);
            break;
        case OSPI_TRANSFER_SEND_ONLY:
            dw_ospi_set_tx_mode(ospi_base);
            break;
        case OSPI_TRANSFER_RECEIVE_ONLY:
            dw_ospi_set_rx_mode(ospi_base);
            break;
        case OSPI_TRANSFER_EEPROM_READ:
            dw_ospi_set_eeprom_mode(ospi_base);
            break;
        default:
            break;
        }
        if(cmd->data.frame_len >= 4)
        {
            dw_ospi_config_ctl_frame_len(ospi_base, cmd->data.frame_len - 1);
        }

        switch (cmd->data.bus_width)
        {
        case OSPI_LINE_SINGLE:
            dw_ospi_set_spi_frame_format_std_mode(ospi_base);
            break;
        case OSPI_LINE_DUAL:
            dw_ospi_set_spi_frame_format_dual_mode(ospi_base);
            break;
        case OSPI_LINE_QUAD:
            dw_ospi_set_spi_frame_format_quad_mode(ospi_base);
            break;
        case OSPI_LINE_OCTAL:
            dw_ospi_set_spi_frame_format_octal_mode(ospi_base);
            break;
        default:
            break;
        }
    }

    if((!cmd->address.disabled) || (!cmd->instruction.disabled))
    {
        if((cmd->instruction.bus_width != OSPI_LINE_SINGLE) && (cmd->address.bus_width != OSPI_LINE_SINGLE))
        {
            dw_ospi_spi_ctrl0_set_trans_type_frf_inst_addr(ospi_base);
        }
        else if((cmd->instruction.bus_width == OSPI_LINE_SINGLE) && (cmd->address.bus_width != OSPI_LINE_SINGLE))
        {
            dw_ospi_spi_ctrl0_set_trans_type_inst_std_addr_frf(ospi_base);
        }
        else
        {
            dw_ospi_spi_ctrl0_set_trans_type_std_inst_addr(ospi_base);
        }
    }
    dw_ospi_spi_ctrl0_set_wait_cycles(ospi_base, cmd->dummy_count);

    if (cmd->ddr_enable != 0U) 
    {
        return CSI_ERROR;
    }
    dw_ospi_enable_slave(ospi_base);
    return ret;
}

csi_error_t csi_ospi_init(csi_ospi_t *ospi, uint32_t idx)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    dw_ospi_regs_t *ospi_base;
    csi_error_t ret = CSI_OK;

    if (target_get(DEV_DW_OSPI_TAG, idx, &ospi->dev) != CSI_OK) 
    {
        ret = CSI_ERROR;
    } 
    else 
    {
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
        ospi->cmd             = NULL;
        ospi->priv            = 0;
		ospi->cmd_sent        = 0U;
        DW_OSPI_SET_REG_IDX_MASTER(ospi);
        ospi_base = dw_get_reg_base(ospi);
        dw_ospi_disable(ospi_base);
        dw_ospi_disable_all_irq(ospi_base);
        dw_ospi_disable_slave_select_toggle(ospi_base);
    }

    return ret;
}

void csi_ospi_uninit(csi_ospi_t *ospi)
{
    CSI_PARAM_CHK_NORETVAL(ospi);

    DW_OSPI_SET_REG_IDX_MASTER(ospi);

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);

    dw_ospi_reset_regs(ospi_base);

    csi_irq_disable((uint32_t)ospi->dev.irq_num);
    csi_irq_detach((uint32_t)ospi->dev.irq_num);
}

csi_error_t csi_ospi_config(csi_ospi_t *ospi, csi_ospi_command_t *cmd)
{
    csi_error_t ret = _ospi_check_command(ospi, cmd);

    if (ret == CSI_OK) {
        ospi->cmd = cmd;
    }

    return ret;
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

csi_error_t csi_ospi_cp_format(csi_ospi_t *ospi, csi_ospi_cp_format_t format)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    dw_ospi_regs_t *ospi_base;
    csi_error_t   ret = CSI_OK;

    ospi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(ospi_base);
    /* configure ospi format */
    switch (format) {
        case OSPI_FORMAT_CPOL0_CPHA0:
            dw_ospi_set_cpol0(ospi_base);
            dw_ospi_set_cpha0(ospi_base);
            break;

        case OSPI_FORMAT_CPOL0_CPHA1:
            dw_ospi_set_cpol0(ospi_base);
            dw_ospi_set_cpha1(ospi_base);
            break;

        case OSPI_FORMAT_CPOL1_CPHA0:
            dw_ospi_set_cpol1(ospi_base);
            dw_ospi_set_cpha0(ospi_base);
            break;

        case OSPI_FORMAT_CPOL1_CPHA1:
            dw_ospi_set_cpol1(ospi_base);
            dw_ospi_set_cpha1(ospi_base);
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

    dw_ospi_regs_t *ospi_base;
    uint32_t div;
    uint32_t freq = 0U;

    ospi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(ospi_base);
    dw_ospi_config_sclk_clock(ospi_base, soc_get_spi_freq((uint32_t)ospi->dev.idx), baud);

    div = dw_ospi_get_sclk_clock_div(ospi_base);

    if (div > 0U) {
        freq =  soc_get_spi_freq((uint32_t)ospi->dev.idx) / div;
    }

    return freq;

}

int32_t csi_ospi_send(csi_ospi_t *ospi, const void *data,
                         uint32_t size, uint32_t timeout)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);

    if (size > 0U) {
        CSI_PARAM_CHK(data, CSI_ERROR);
    }

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);
    uint8_t  *tx_data    = (uint8_t *)data;
    uint32_t  count      = 0U;
    uint32_t  timestart;
    int32_t   ret        = CSI_OK;
    bool      timed_out  = false;

    /* ---- 前置检查 ---- */
    if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
        return CSI_BUSY;
    }

    /* ---- 帧数换算 ---- */
    if (is_32bit_frame(ospi->cmd->data.frame_len)) {
        size = size / 4U;
    } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
        size = size / 2U;
    }
    /* else: 8-bit, size 不变 */

    timestart = csi_tick_get_ms();
    ospi->state.writeable = 0U;

    /* ---- 配置硬件 ---- */
    dw_ospi_disable(ospi_base);
    dw_ospi_set_tx_mode(ospi_base);
    dw_ospi_config_tx_fifo_threshold(ospi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
    dw_ospi_enable(ospi_base);

    /* ---- 写入 instruction / address ---- */
    if(ospi->cmd->data.bus_width != OSPI_LINE_SINGLE){
        if (!ospi->cmd->instruction.disabled) {
        dw_ospi_transmit_data(ospi_base, ospi->cmd->instruction.value);
    }
        if (!ospi->cmd->address.disabled) {
        dw_ospi_transmit_data(ospi_base, ospi->cmd->address.value);
    }
    }

    /* ---- 数据传输循环（size==0 时直接跳过） ---- */
    while (size > 0U && !timed_out) {

        uint32_t fifo_free = DW_MAX_OSPI_TXFIFO_LV
                           - dw_ospi_get_tx_fifo_level(ospi_base);
        uint32_t burst = (fifo_free < size) ? fifo_free : size;

        while (burst--) {
            if (is_32bit_frame(ospi->cmd->data.frame_len)) {
                dw_ospi_transmit_data(ospi_base, *(uint32_t *)tx_data);
                tx_data += 4;  count += 4U;
            } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
                dw_ospi_transmit_data(ospi_base, *(uint16_t *)tx_data);
                tx_data += 2;  count += 2U;
            } else {
                dw_ospi_transmit_data(ospi_base, *(uint8_t *)tx_data);
                tx_data += 1;  count += 1U;
            }
            size--;
        }

        if ((csi_tick_get_ms() - timestart) > timeout) {
            timed_out = true;
        }
    }

    /* ---- 等待 FIFO 排空 + 总线空闲 ---- */
    while (!timed_out) {
        if ((dw_ospi_get_status(ospi_base) & DW_OSPI_SR_TFE) &&
            !(dw_ospi_get_status(ospi_base) & DW_OSPI_SR_BUSY)) {
            break;
        }
        if ((csi_tick_get_ms() - timestart) > timeout) {
            timed_out = true;
            break;
        }
    }

    /* ---- 清理 ---- */
    dw_ospi_config_tx_fifo_threshold(ospi_base, 0U);
    ospi->state.writeable = 1U;

    /* ---- 返回值 ---- */
    if (timed_out) {
        ret = (count > 0U) ? (int32_t)count : CSI_TIMEOUT;
    } else {
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

    if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
        ret = CSI_BUSY;
    }

    if (is_32bit_frame(ospi->cmd->data.frame_len)) {
        if (size % sizeof(uint32_t)) {
            ret = CSI_ERROR;
        }
    }

    if (is_16bit_frame(ospi->cmd->data.frame_len)) {
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
    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);

    ospi->tx_data = (uint8_t *)data;

    uint32_t frame_len = ospi->cmd->data.frame_len;

    if (is_32bit_frame(frame_len)) {
        ospi->tx_size = size / 4U;
    } else if (is_16bit_frame(frame_len)) {
        ospi->tx_size = size / 2U;
    } else if (is_8bit_frame(frame_len)) {
        ospi->tx_size = size;
    } else {
        return CSI_ERROR;
    }
    __asm__ volatile("" ::: "memory");
    dw_ospi_disable(ospi_base);
    dw_ospi_set_tx_mode(ospi_base);
    dw_ospi_config_tx_fifo_threshold(ospi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
    dw_ospi_enable(ospi_base);
	dw_ospi_enable_tx_empty_irq(ospi_base);
    return ret;
}

static csi_error_t dw_ospi_send_dma(csi_ospi_t *ospi, const void *data, uint32_t size)
{
    CSI_PARAM_CHK(ospi,  CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    csi_dma_ch_config_t     config;
    dw_ospi_regs_t          *ospi_base;
    csi_dma_ch_t            *dma_ch;
    csi_error_t             ret = CSI_OK;
    bool                    timed_out = false;
    uint32_t                timestart;
    uint32_t                timeout = 1000;
    ospi_base = dw_get_reg_base(ospi);
    dma_ch   = (csi_dma_ch_t *)ospi->tx_dma;
    ospi->tx_data = (uint8_t *)data;
    memset(&config, 0, sizeof(csi_dma_ch_config_t));

    do {
        /* configure dma channel */
        if (is_32bit_frame(ospi->cmd->data.frame_len)) {
            ospi->tx_size = size / 4U;
            config.src_tw = DMA_DATA_WIDTH_32_BITS;
            config.dst_tw = DMA_DATA_WIDTH_32_BITS;
        } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
            ospi->tx_size = size / 2U;
            config.src_tw = DMA_DATA_WIDTH_16_BITS;
            config.dst_tw = DMA_DATA_WIDTH_16_BITS;
        } else if (is_8bit_frame(ospi->cmd->data.frame_len)) {
            ospi->tx_size = size;
            config.src_tw = DMA_DATA_WIDTH_8_BITS;
            config.dst_tw = DMA_DATA_WIDTH_8_BITS;
        } else {
            ret = CSI_ERROR;
            break;
        }
        //__asm__ volatile("" ::: "memory");
        config.src_inc = DMA_ADDR_INC;
        config.dst_inc = DMA_ADDR_CONSTANT;
        config.group_len = find_group_len(size, 1U << (uint8_t)config.src_tw);
        config.trans_dir = DMA_MEM2PERH;
        config.handshake = ospi_tx_hs_num[ospi->dev.idx];
        csi_dma_ch_config(dma_ch, &config);

        /* set tx mode*/
        dw_ospi_disable(ospi_base);
        dw_ospi_set_tx_mode(ospi_base);
        dw_ospi_enable(ospi_base);
        if(!ospi->cmd->instruction.disabled)
        {
            dw_ospi_transmit_data(ospi_base, ospi->cmd->instruction.value);
        }
        if(!ospi->cmd->address.disabled)
        {
        dw_ospi_transmit_data(ospi_base, ospi->cmd->address.value);
        }
        timestart = csi_tick_get_ms();
        while (!timed_out) {
        if (dw_ospi_get_status(ospi_base) & DW_OSPI_SR_TFE) {
            break;
        }
        if ((csi_tick_get_ms() - timestart) > timeout) {
            timed_out = true;
            break;
        }
        }
        if (timed_out) {
            ret = CSI_ERROR;
            break;
        }
        dw_ospi_enable_tx_dma(ospi_base);
        soc_dcache_clean_invalid_range((unsigned long)ospi->tx_data, size);
        csi_dma_ch_start(ospi->tx_dma, ospi->tx_data, (void *) & (ospi_base->DR), size);

    } while (0);

    return ret;
}

int32_t csi_ospi_receive(csi_ospi_t *ospi,void *data, uint32_t size, uint32_t timeout)
{
    CSI_PARAM_CHK(ospi, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(size, CSI_ERROR);

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);
    uint8_t  *rx_data   = (uint8_t *)data;
    uint32_t  rx_frames = size;
    uint32_t  count     = 0U;
    uint32_t  timestart;
    int32_t   ret       = CSI_OK;
    bool      timed_out = false;

    /* ---- 前置检查 ---- */
    if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
        return CSI_BUSY;
    }

    /* ---- 计算帧数 ---- */
    if (is_32bit_frame(ospi->cmd->data.frame_len)) {
        if (size % sizeof(uint32_t)) { return CSI_ERROR; }
        rx_frames = size / 4U;
    } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
        if (size % sizeof(uint16_t)) { return CSI_ERROR; }
        rx_frames = size / 2U;
    }
    /* else: 8-bit, rx_frames = size */

    timestart = csi_tick_get_ms();
    ospi->state.readable = 0U;

    /* ---- 写入 instruction / address / dummy ---- */
    if((ospi->cmd->data.bus_width != OSPI_LINE_SINGLE) && (!ospi->cmd->data.disabled)){

        dw_ospi_disable(ospi_base);
        dw_ospi_set_rx_mode(ospi_base);
        dw_ospi_config_rx_data_len(ospi_base, rx_frames - 1U);
        dw_ospi_enable(ospi_base);

        if (!ospi->cmd->instruction.disabled) {
        dw_ospi_transmit_data(ospi_base, ospi->cmd->instruction.value);
    }
        if (!ospi->cmd->address.disabled) {
        dw_ospi_transmit_data(ospi_base, ospi->cmd->address.value);
    }
    }else{

        dw_ospi_disable(ospi_base);
        dw_ospi_set_tx_mode(ospi_base);
        dw_ospi_enable(ospi_base);

        if (!ospi->cmd->instruction.disabled) {
        dw_ospi_transmit_data(ospi_base, ospi->cmd->instruction.value);
    }
        if (!ospi->cmd->address.disabled) {
        dw_ospi_transmit_data(ospi_base, ospi->cmd->address.value);
    }

        while (!timed_out) {
            if ((dw_ospi_get_status(ospi_base) & DW_OSPI_SR_TFE) &&
                !(dw_ospi_get_status(ospi_base) & DW_OSPI_SR_BUSY)) {
                break;
            }
            if ((csi_tick_get_ms() - timestart) > timeout) {
                timed_out = true;
                break;
            }
        }

        if ((!timed_out) && (!ospi->cmd->data.disabled)) {
            dw_ospi_disable(ospi_base);
            dw_ospi_set_rx_mode(ospi_base);
            dw_ospi_config_rx_data_len(ospi_base, rx_frames - 1U);
            dw_ospi_enable(ospi_base);

            dw_ospi_transmit_data(ospi_base, 0);
        }

    }

    /* ---- 数据接收循环（超时检查在循环内部） ---- */
    while (rx_frames > 0U && !timed_out) {

        uint32_t fifo_avail = dw_ospi_get_rx_fifo_level(ospi_base);

        if (fifo_avail == 0U) {
            /* FIFO 为空，检查超时后继续等，避免 tight spin */
            if ((csi_tick_get_ms() - timestart) > timeout) {
                timed_out = true;
            }
            continue;   /* 回到 while 头部重新检查 */
        }

        /* 限制本次读取不超过剩余帧数 */
        uint32_t burst = (fifo_avail < rx_frames) ? fifo_avail : rx_frames;

        while (burst--) {
            uint32_t val = dw_ospi_receive_data(ospi_base);

            if (is_32bit_frame(ospi->cmd->data.frame_len)) {
                *(uint32_t *)rx_data = val;
                rx_data += 4;  count += 4U;
            } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
                *(uint16_t *)rx_data = (uint16_t)val;
                rx_data += 2;  count += 2U;
            } else {
                *(uint8_t *)rx_data = (uint8_t)val;
                rx_data += 1;  count += 1U;
            }
            rx_frames--;
        }

        /* 每次 burst 后也检查超时 */
        if ((csi_tick_get_ms() - timestart) > timeout) {
            timed_out = true;
        }
    }

    /* ---- 等待总线空闲（仅在未超时时） ---- */
    if (!timed_out) {
        while (dw_ospi_get_status(ospi_base) & DW_OSPI_SR_BUSY) {
            if ((csi_tick_get_ms() - timestart) > timeout) {
                timed_out = true;
                break;
            }
        }
    }

    /* ---- 清理 ---- */
    dw_ospi_config_rx_data_len(ospi_base, 0U);
    dw_ospi_config_rx_fifo_threshold(ospi_base, 0U);
    ospi->state.readable = 1U;

    /* ---- 返回值 ---- */
    if (timed_out) {
        ret = (count > 0U) ? (int32_t)count : CSI_TIMEOUT;
    } else {
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

        if (is_32bit_frame(ospi->cmd->data.frame_len)) {
            if (size % sizeof(uint32_t)) {
                ret = CSI_ERROR;
            }
        }

        if (is_16bit_frame(ospi->cmd->data.frame_len)) {
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

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);

    ospi->rx_data = (uint8_t *)data;

    do {
        // Convert byte to nums
        if (is_32bit_frame(ospi->cmd->data.frame_len)) {
            ospi->rx_size = size / 4U;
        } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
            ospi->rx_size = size / 2U;
        } else if (is_8bit_frame(ospi->cmd->data.frame_len)) {
            ospi->rx_size = size;
        } else {
            ret = CSI_ERROR;
            break;
        }
        __asm__ volatile("" ::: "memory");
        if ((ospi->cmd->data.bus_width != OSPI_LINE_SINGLE) && (!ospi->cmd->data.disabled)) {

            dw_ospi_disable(ospi_base);
            dw_ospi_set_rx_mode(ospi_base);
            dw_ospi_config_rx_data_len(ospi_base, ospi->rx_size - 1U);

            rx_fifo_lv = (ospi->rx_size < DW_DEFAULT_OSPI_RXFIFO_LV) ? (ospi->rx_size - 1U) : DW_DEFAULT_OSPI_RXFIFO_LV;
            dw_ospi_config_rx_fifo_threshold(ospi_base, rx_fifo_lv);
            dw_ospi_enable_rx_fifo_full_irq(ospi_base);
            dw_ospi_enable(ospi_base);


            if (!ospi->cmd->instruction.disabled) {
                dw_ospi_transmit_data(ospi_base, ospi->cmd->instruction.value);
            }
            if (!ospi->cmd->address.disabled) {
                dw_ospi_transmit_data(ospi_base, ospi->cmd->address.value);
            }
        } else {

            dw_ospi_disable(ospi_base);
            dw_ospi_set_tx_mode(ospi_base);
            dw_ospi_enable(ospi_base);

            if (!ospi->cmd->instruction.disabled) {
                dw_ospi_transmit_data(ospi_base, ospi->cmd->instruction.value);
            }
            if (!ospi->cmd->address.disabled) {
                dw_ospi_transmit_data(ospi_base, ospi->cmd->address.value);
            }

            // 等待发送完成
            uint32_t timeout = 200;
            uint32_t start = csi_tick_get_ms();
            while (1) {
                if ((dw_ospi_get_status(ospi_base) & DW_OSPI_SR_TFE) && !(dw_ospi_get_status(ospi_base) & DW_OSPI_SR_BUSY)) {
                    break;
                }
                if (csi_tick_get_ms() - start > timeout) {
                    ret = CSI_TIMEOUT;
                    break;
                }
            }

            if (ret != CSI_OK) break;

            dw_ospi_disable(ospi_base);
            dw_ospi_set_rx_mode(ospi_base);
            dw_ospi_config_rx_data_len(ospi_base, ospi->rx_size - 1U);

            rx_fifo_lv = (ospi->rx_size < DW_DEFAULT_OSPI_RXFIFO_LV) ? (ospi->rx_size - 1U) : DW_DEFAULT_OSPI_RXFIFO_LV;
            dw_ospi_config_rx_fifo_threshold(ospi_base, rx_fifo_lv);
            dw_ospi_enable_rx_fifo_full_irq(ospi_base);
            dw_ospi_enable(ospi_base);

            dw_ospi_transmit_data(ospi_base, 0);
        }
    } while (0);

    return ret;
}

static csi_error_t dw_ospi_receive_dma(csi_ospi_t *ospi, void *data, uint32_t size)
{
    csi_dma_ch_config_t     config;
    dw_ospi_regs_t       *ospi_base;
    csi_dma_ch_t            *dma_ch;
    csi_error_t         ret = CSI_OK;

    ospi_base = dw_get_reg_base(ospi);
    dma_ch   = (csi_dma_ch_t *)ospi->rx_dma;
    ospi->rx_data = (uint8_t *)data;
    memset(&config, 0, sizeof(csi_dma_ch_config_t));

    do {
        /* configure dma channel */
        if (is_32bit_frame(ospi->cmd->data.frame_len)) {
            ospi->rx_size = size / 4U;
            config.src_tw = DMA_DATA_WIDTH_32_BITS;
            config.dst_tw = DMA_DATA_WIDTH_32_BITS;
        } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
            ospi->rx_size = size / 2U;
            config.src_tw = DMA_DATA_WIDTH_16_BITS;
            config.dst_tw = DMA_DATA_WIDTH_16_BITS;
        } else if (is_8bit_frame(ospi->cmd->data.frame_len)) {
            ospi->rx_size = size;
            config.src_tw = DMA_DATA_WIDTH_8_BITS;
            config.dst_tw = DMA_DATA_WIDTH_8_BITS;
        } else {
            ret = CSI_ERROR;
            break;
        }
        config.src_inc = DMA_ADDR_CONSTANT;
        config.dst_inc = DMA_ADDR_INC;
        if (is_32bit_frame(ospi->cmd->data.frame_len)) {
            config.group_len = 4;
        } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
            config.group_len = 2;
        } else if (is_8bit_frame(ospi->cmd->data.frame_len)) {
            config.group_len = 1;
        }
        config.trans_dir = DMA_PERH2MEM;
        config.handshake = ospi_rx_hs_num[ospi->dev.idx];
        csi_dma_ch_config(dma_ch, &config);

        /* set rx mode*/
        dw_ospi_disable(ospi_base);
        dw_ospi_set_rx_mode(ospi_base);
        dw_ospi_config_rx_data_len(ospi_base, ospi->rx_size - 1U);
        dw_ospi_config_dma_rx_data_level(ospi_base, ((uint32_t)config.group_len / ((uint32_t)1U << (uint32_t)config.src_tw)) - 1U);
        dw_ospi_enable_rx_dma(ospi_base);

        soc_dcache_clean_invalid_range((unsigned long)ospi->rx_data, size);
        csi_dma_ch_start(ospi->rx_dma, (void *) & (ospi_base->DR), ospi->rx_data, size);

        dw_ospi_enable(ospi_base);

        if(!ospi->cmd->instruction.disabled)
        {
            dw_ospi_transmit_data(ospi_base, ospi->cmd->instruction.value);
        }
        if(!ospi->cmd->address.disabled)
        {
            dw_ospi_transmit_data(ospi_base, ospi->cmd->address.value);
        }
        if(ospi->cmd->data.bus_width == OSPI_LINE_SINGLE)
        {
            dw_ospi_transmit_data(ospi_base, 0);
        }
    } while (0);

    return ret;
}

int32_t csi_ospi_send_receive(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size, uint32_t timeout)
{
    CSI_PARAM_CHK(ospi,     CSI_ERROR);
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

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);

    do {

        if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
            ret = CSI_BUSY;
            break;
        }

        if (is_32bit_frame(ospi->cmd->data.frame_len)) {
            if (size % sizeof(uint32_t)) {
                ret = CSI_ERROR;
                break;
            }
        }

        if (is_16bit_frame(ospi->cmd->data.frame_len)) {
            if (size % sizeof(uint16_t)) {
                ret = CSI_ERROR;
                break;
            }
        }

        timestart = csi_tick_get_ms();
        ospi->state.writeable = 0U;
        ospi->state.readable  = 0U;
        tx_data = (uint8_t *)data_out;
        tx_size = (is_32bit_frame(ospi->cmd->data.frame_len)) ? size / 4U :
          (is_16bit_frame(ospi->cmd->data.frame_len)) ? size / 2U : size;
        rx_data = (uint8_t *)data_in;
        rx_size = (is_32bit_frame(ospi->cmd->data.frame_len)) ? size / 4U :
          (is_16bit_frame(ospi->cmd->data.frame_len)) ? size / 2U : size;

        /* set tx rx mode*/
        dw_ospi_disable(ospi_base);
        dw_ospi_set_tx_rx_mode(ospi_base);
        dw_ospi_config_tx_fifo_threshold(ospi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
        dw_ospi_config_rx_fifo_threshold(ospi_base, DW_DEFAULT_OSPI_RXFIFO_LV);
        dw_ospi_enable(ospi_base);

        /* transfer loop */
        if (is_8bit_frame(ospi->cmd->data.frame_len)) {
            while ((tx_size > 0U) || (rx_size > 0U)) {
                /* process tx fifo empty */
                if (tx_size > 0U) {
                    current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(ospi_base);

                    if (current_size > tx_size) {
                        current_size = tx_size;

                    }

                    while (current_size--) {
                        value = (uint32_t)(*(uint8_t *)tx_data);
                        dw_ospi_transmit_data(ospi_base, value);
                        tx_data += 1;
                        count += 1U;
                        tx_size--;
                    }
                }

                /* process rx fifo not empty */
                if (rx_size > 0U) {
                    current_size = dw_ospi_get_rx_fifo_level(ospi_base);

                    if (current_size > rx_size) {
                        current_size = rx_size;

                    }

                    while (current_size--) {
                        *(uint8_t *)rx_data = (uint8_t)dw_ospi_receive_data(ospi_base);
                        rx_data += 1;
                        rx_size--;
                    }
                }

                if ((csi_tick_get_ms() - timestart) > timeout) {
                    break;
                }
            }
        } else if (is_16bit_frame(ospi->cmd->data.frame_len)) {
            while ((tx_size > 0U) || (rx_size > 0U)) {
                /* process tx fifo empty */
                if (tx_size > 0U) {
                    current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(ospi_base);

                    if (current_size > tx_size) {
                        current_size = tx_size;

                    }

                    while (current_size--) {
                        value = (uint32_t)(*(uint16_t *)tx_data);
                        dw_ospi_transmit_data(ospi_base, value);
                        tx_data += 2;
                        count += 2U;
                        tx_size--;
                    }
                }

                /* process rx fifo not empty */
                if (rx_size > 0U) {
                    current_size = dw_ospi_get_rx_fifo_level(ospi_base);

                    if (current_size > rx_size) {
                        current_size = rx_size;

                    }

                    while (current_size--) {
                        *(uint16_t *)rx_data = (uint16_t)dw_ospi_receive_data(ospi_base);
                        rx_data += 2;
                        rx_size--;
                    }
                }

                if ((csi_tick_get_ms() - timestart) > timeout) {
                    break;
                }

            }
        } else if (is_32bit_frame(ospi->cmd->data.frame_len)) {
            while ((tx_size > 0U) || (rx_size > 0U)) {
                /* process tx fifo empty */
                if (tx_size > 0U) {
                    current_size = DW_MAX_OSPI_TXFIFO_LV - dw_ospi_get_tx_fifo_level(ospi_base);

                    if (current_size > tx_size) {
                        current_size = tx_size;

                    }

                    while (current_size--) {
                        value = (uint32_t)(*(uint32_t *)tx_data);
                        dw_ospi_transmit_data(ospi_base, value);
                        tx_data += 4;
                        count += 4U;
                        tx_size--;
                    }
                }

                /* process rx fifo not empty */
                if (rx_size > 0U) {
                    current_size = dw_ospi_get_rx_fifo_level(ospi_base);

                    if (current_size > rx_size) {
                        current_size = rx_size;

                    }

                    while (current_size--) {
                        *(uint32_t *)rx_data = (uint32_t)dw_ospi_receive_data(ospi_base);
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
        while (dw_ospi_get_status(ospi_base) & DW_OSPI_SR_BUSY) {
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

    if ((ospi->state.writeable == 0U) || (ospi->state.readable == 0U)) {
        ret = CSI_BUSY;
    }

    if (is_32bit_frame(ospi->cmd->data.frame_len)) {
        if (size % sizeof(uint32_t)) {
            ret = CSI_ERROR;
        }
    }

    if (is_16bit_frame(ospi->cmd->data.frame_len)) {
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

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);
    uint32_t frame_len = dw_ospi_get_data_frame_len(ospi_base);

    ospi->tx_data = (uint8_t *)data_out;
    ospi->rx_data = (uint8_t *)data_in;

    do {
       
        if (is_32bit_frame(frame_len)) {
            ospi->tx_size = size / 4U;
            ospi->rx_size = size / 4U;
        } else if (is_16bit_frame(frame_len)) {
            ospi->tx_size = size / 2U;
            ospi->rx_size = size / 2U;
        } else if (is_8bit_frame(frame_len)) {
            ospi->tx_size = size;
            ospi->rx_size = size;
        } else {
            ret = CSI_ERROR;
            break;
        }

        /* set tx rx mode*/
        dw_ospi_disable(ospi_base);
        dw_ospi_set_tx_rx_mode(ospi_base);
        dw_ospi_config_rx_data_len(ospi_base, ospi->rx_size - 1U);
        dw_ospi_config_tx_fifo_threshold(ospi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
        rx_fifo_lv = (ospi->rx_size < (DW_DEFAULT_OSPI_RXFIFO_LV + 1U)) ? (ospi->rx_size - 1U) : DW_DEFAULT_OSPI_RXFIFO_LV;
        dw_ospi_config_rx_fifo_threshold(ospi_base, rx_fifo_lv);
        dw_ospi_enable(ospi_base);
        dw_ospi_enable_rx_fifo_full_irq(ospi_base);
        dw_ospi_enable_tx_empty_irq(ospi_base);

    } while (0);

    return ret;
}

static csi_error_t dw_ospi_send_receive_dma(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size)
{
    csi_dma_ch_config_t config;
    dw_ospi_regs_t       *ospi_base;
    csi_dma_ch_t        *dma_ch;
    csi_error_t         ret = CSI_OK;
    uint32_t frame_len;

    ospi_base = dw_get_reg_base(ospi);
    frame_len = dw_ospi_get_data_frame_len(ospi_base);

    ospi->tx_data = (uint8_t *)data_out;
    memset(&config, 0, sizeof(csi_dma_ch_config_t));

    do {
        
        if (is_32bit_frame(frame_len)) {
            ospi->tx_size = size / 4U;
            ospi->rx_size = size / 4U;
            config.src_tw = DMA_DATA_WIDTH_32_BITS;
            config.dst_tw = DMA_DATA_WIDTH_32_BITS;
        } else if (is_16bit_frame(frame_len)) {
            ospi->tx_size = size / 2U;
            ospi->rx_size = size / 2U;
            config.src_tw = DMA_DATA_WIDTH_16_BITS;
            config.dst_tw = DMA_DATA_WIDTH_16_BITS;
        } else if (is_8bit_frame(frame_len)) {
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
        config.group_len = find_group_len(size, 1U << (uint8_t)config.src_tw);
        config.trans_dir = DMA_MEM2PERH;
        config.handshake = ospi_tx_hs_num[ospi->dev.idx];
        csi_dma_ch_config(dma_ch, &config);

        /* configure dma rx channel */
        dma_ch   = (csi_dma_ch_t *)ospi->rx_dma;
        config.src_inc = DMA_ADDR_CONSTANT;
        config.dst_inc = DMA_ADDR_INC;
        config.group_len = find_group_len(size, 1U << (uint8_t)config.src_tw);
        config.trans_dir = DMA_PERH2MEM;
        config.handshake = ospi_rx_hs_num[ospi->dev.idx];
        csi_dma_ch_config(dma_ch, &config);

        /* set tx_rx mode*/
        dw_ospi_disable(ospi_base);
        dw_ospi_set_tx_rx_mode(ospi_base);
        dw_ospi_config_rx_data_len(ospi_base, ospi->rx_size - 1U);
        dw_ospi_config_dma_rx_data_level(ospi_base, ((uint32_t)config.group_len / ((uint32_t)1U << (uint32_t)config.src_tw)) - 1U);
        dw_ospi_enable_rx_dma(ospi_base);
        dw_ospi_enable_tx_dma(ospi_base);

        soc_dcache_clean_invalid_range((unsigned long)ospi->tx_data, size);
        soc_dcache_clean_invalid_range((unsigned long)ospi->rx_data, size);
        dw_ospi_enable(ospi_base);
        csi_dma_ch_start(ospi->rx_dma, (void *)&(ospi_base->DR), ospi->rx_data, size);
        csi_dma_ch_start(ospi->tx_dma, ospi->tx_data, (void *)&(ospi_base->DR), size);
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

    dw_ospi_regs_t *ospi_base = dw_get_reg_base(ospi);
    dw_ospi_disable(ospi_base);
    dw_ospi_enable_slave(ospi_base);

    dw_ospi_disable(ospi_base);
    dw_ospi_set_tx_mode(ospi_base);
    dw_ospi_config_tx_fifo_threshold(ospi_base, DW_DEFAULT_OSPI_TXFIFO_LV);
    dw_ospi_enable(ospi_base);
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



/******************************************************************************
 * @file     wj_tdm_ll.h
 * @brief    header file for wj tdm ll driver
 * @version  V1.0
 * @date     20. Oct 2020
 ******************************************************************************/

#ifndef _WJ_TDM_LL_H_
#define _WJ_TDM_LL_H_

#include <soc.h>
#include <csi_core.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WJ_MAX_TDM_FIFO_LV        0x20U

#define WJ_DEFAULT_TDM_RXFIFO_LV  0x8U
/*
 * TDM register bit definitions
 */

/* TMD_TRANS_EN , offset: 0x00 */
#define WJ_TMD_TRANS_EN_Pos                             (0U)
#define WJ_TMD_TRANS_EN_Msk                             (0x1U << WJ_TMD_TRANS_EN_Pos)
#define WJ_TMD_TRANS_EN                                 WJ_TMD_TRANS_EN_Msk

/* TDM_CONFIG , offset: 0x04 */
#define WJ_TDM_CONFIG_MODE_Pos                       (0U)
#define WJ_TDM_CONFIG_MODE_Msk                       (0x1U << WJ_TDM_CONFIG_MODE_Pos)
#define WJ_TDM_CONFIG_MODE_TDM                       (0x0U << WJ_TDM_CONFIG_MODE_Pos)
#define WJ_TDM_CONFIG_MODE_I2S                       WJ_TDM_CONFIG_MODE_Msk

#define WJ_TDM_CONFIG_SLOT_WIDTH_Pos                 (4U)
#define WJ_TDM_CONFIG_SLOT_WIDTH_Msk                 (0x7U << WJ_TDM_CONFIG_SLOT_WIDTH_Pos)
#define WJ_TDM_CONFIG_SLOT_WIDTH_8BITS               (0x1U << WJ_TDM_CONFIG_SLOT_WIDTH_Pos)
#define WJ_TDM_CONFIG_SLOT_WIDTH_12BITS              (0x2U << WJ_TDM_CONFIG_SLOT_WIDTH_Pos)
#define WJ_TDM_CONFIG_SLOT_WIDTH_16BITS              (0x3U << WJ_TDM_CONFIG_SLOT_WIDTH_Pos)
#define WJ_TDM_CONFIG_SLOT_WIDTH_20BITS              (0x4U << WJ_TDM_CONFIG_SLOT_WIDTH_Pos)
#define WJ_TDM_CONFIG_SLOT_WIDTH_24BITS              (0x5U << WJ_TDM_CONFIG_SLOT_WIDTH_Pos)
#define WJ_TDM_CONFIG_SLOT_WIDTH_28BITS              (0x6U << WJ_TDM_CONFIG_SLOT_WIDTH_Pos)
#define WJ_TDM_CONFIG_SLOT_WIDTH_32BITS              (0x7U << WJ_TDM_CONFIG_SLOT_WIDTH_Pos)

#define WJ_TDM_CONFIG_SLOT_NUM_Pos                   (8U)
#define WJ_TDM_CONFIG_SLOT_NUM_Msk                   (0xFU << WJ_TDM_CONFIG_SLOT_NUM_Pos)

#define WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos            (12U)
#define WJ_TDM_CONFIG_SLOT_RESOLUTION_Msk            (0x7U << WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos)
#define WJ_TDM_CONFIG_SLOT_RESOLUTION_8BITS          (0x1U << WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos)
#define WJ_TDM_CONFIG_SLOT_RESOLUTION_12BITS         (0x2U << WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos)
#define WJ_TDM_CONFIG_SLOT_RESOLUTION_16BITS         (0x3U << WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos)
#define WJ_TDM_CONFIG_SLOT_RESOLUTION_20BITS         (0x4U << WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos)
#define WJ_TDM_CONFIG_SLOT_RESOLUTION_24BITS         (0x5U << WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos)
#define WJ_TDM_CONFIG_SLOT_RESOLUTION_28BITS         (0x6U << WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos)
#define WJ_TDM_CONFIG_SLOT_RESOLUTION_32BITS         (0x7U << WJ_TDM_CONFIG_SLOT_RESOLUTION_Pos)

#define WJ_TDM_CONFIG_I2S_CHEN_Pos                 (16U)
#define WJ_TDM_CONFIG_I2S_CHEN_Msk                 (0xFU << WJ_TDM_CONFIG_I2S_CHEN_Pos)

#define WJ_TDM_CONFIG_I2S_CHTXRX_Pos               (20U)
#define WJ_TDM_CONFIG_I2S_CHTXRX_Msk               (0xFU << WJ_TDM_CONFIG_I2S_CHTXRX_Pos)

#define WJ_TDM_CONFIG_DATA_EXTENTION_Pos           (24U)
#define WJ_TDM_CONFIG_DATA_EXTENTION_Msk           (0x1U << WJ_TDM_CONFIG_DATA_EXTENTION_Pos)

#define WJ_TDM_CONFIG_EDGE_TRANSFER_Pos            (25U)
#define WJ_TDM_CONFIG_EDGE_TRANSFER_Msk            (0x1U << WJ_TDM_CONFIG_EDGE_TRANSFER_Pos)

#define WJ_TDM_CONFIG_LOOP_TEST_Pos                (26U)
#define WJ_TDM_CONFIG_LOOP_TEST_Msk                (0x1U << WJ_TDM_CONFIG_LOOP_TEST_Pos)

#define WJ_TDM_CONFIG_OUT_MUTE_Pos                 (27U)
#define WJ_TDM_CONFIG_OUT_MUTE_Msk                 (0x1U << WJ_TDM_CONFIG_OUT_MUTE_Pos)

#define WJ_TDM_CONFIG_TDM_TXEN_Pos                 (28U)
#define WJ_TDM_CONFIG_TDM_TXEN_Msk                 (0x1U << WJ_TDM_CONFIG_TDM_TXEN_Pos)

#define WJ_TDM_CONFIG_TDM_RXEN_Pos                 (29U)
#define WJ_TDM_CONFIG_TDM_RXEN_Msk                 (0x1U << WJ_TDM_CONFIG_TDM_RXEN_Pos)

/* TDM_CLOCK_GEN , offset: 0x08 */
#define WJ_TDM_CLOCK_GEN_MASTEREN_Pos                       (0U)
#define WJ_TDM_CLOCK_GEN_MASTEREN_Msk                       (0x1U << WJ_TDM_CLOCK_GEN_MASTEREN_Pos)

#define WJ_TDM_CLOCK_GEN_SCLK_POLARITY_Pos                       (1U)
#define WJ_TDM_CLOCK_GEN_SCLK_POLARITY_Msk                       (0x1U << WJ_TDM_CLOCK_GEN_SCLK_POLARITY_Pos)

#define WJ_TDM_CLOCK_GEN_WS_FORMAT_Pos                       (2U)
#define WJ_TDM_CLOCK_GEN_WS_FORMAT_Msk                       (0x3U << WJ_TDM_CLOCK_GEN_WS_FORMAT_Pos)
#define WJ_TDM_CLOCK_GEN_WS_FORMAT_TDM_WS_LEFT_JUSTIFIED        (0x1U << WJ_TDM_CLOCK_GEN_WS_FORMAT_Pos)
#define WJ_TDM_CLOCK_GEN_WS_FORMAT_TDM_WS_STANDARD_DSP          (0x0U << WJ_TDM_CLOCK_GEN_WS_FORMAT_Pos)
#define WJ_TDM_CLOCK_GEN_WS_FORMAT_I2S_WS_RIGHT_JUSTIFIED       (0x2U << WJ_TDM_CLOCK_GEN_WS_FORMAT_Pos)
#define WJ_TDM_CLOCK_GEN_WS_FORMAT_I2S_WS_LEFT_JUSTIFIED        (0x1U << WJ_TDM_CLOCK_GEN_WS_FORMAT_Pos)
#define WJ_TDM_CLOCK_GEN_WS_FORMAT_I2S_WS_STANDARD_DSP          (0x0U << WJ_TDM_CLOCK_GEN_WS_FORMAT_Pos)

#define WJ_TDM_CLOCK_GEN_WS_POLARITY_I2S_Pos                       (4U)
#define WJ_TDM_CLOCK_GEN_WS_POLARITY_I2S_Msk                       (0x1U << WJ_TDM_CLOCK_GEN_WS_POLARITY_I2S_Pos)

#define WJ_TDM_CLOCK_GEN_WS_WIDTH_TDM_Pos                       (5U)
#define WJ_TDM_CLOCK_GEN_WS_WIDTH_TDM_Msk                       (0x1U << WJ_TDM_CLOCK_GEN_WS_WIDTH_TDM_Pos)

#define WJ_TDM_CLOCK_GEN_WS_SCLK_ENABLE_Pos                       (8U)
#define WJ_TDM_CLOCK_GEN_WS_SCLK_ENABLE_Msk                       (0x1U << WJ_TDM_CLOCK_GEN_WS_SCLK_ENABLE_Pos)

#define WJ_TDM_CLOCK_GEN_WS_MCLK_DIV_Pos                       (9U)
#define WJ_TDM_CLOCK_GEN_WS_MCLK_DIV_Msk                       (0x2U << WJ_TDM_CLOCK_GEN_WS_MCLK_DIV_Pos)

/* TDM_FIFO_STATUS , offset: 0x14 */
#define WJ_TDM_FIFO_STATUS_RXFIFO_AEMPTY_Pos                       (0U)
#define WJ_TDM_FIFO_STATUS_RXFIFO_AEMPTY_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_RXFIFO_AEMPTY_Pos)

#define WJ_TDM_FIFO_STATUS_RXFIFO_EMPTY_Pos                       (1U)
#define WJ_TDM_FIFO_STATUS_RXFIFO_EMPTY_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_RXFIFO_EMPTY_Pos)

#define WJ_TDM_FIFO_STATUS_RXFIFO_AFULL_Pos                       (2U)
#define WJ_TDM_FIFO_STATUS_RXFIFO_AFULL_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_RXFIFO_AFULL_Pos)

#define WJ_TDM_FIFO_STATUS_RXFIFO_FULL_Pos                       (3U)
#define WJ_TDM_FIFO_STATUS_RXFIFO_FULL_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_RXFIFO_FULL_Pos)

#define WJ_TDM_FIFO_STATUS_RXFIFO_RXLVL_TRIG_Pos                       (4U)
#define WJ_TDM_FIFO_STATUS_RXFIFO_RXLVL_TRIG_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_RXFIFO_RXLVL_TRIG_Pos)

#define WJ_TDM_FIFO_STATUS_RXFIFO_POP_ERROR_Pos                       (5U)
#define WJ_TDM_FIFO_STATUS_RXFIFO_POP_ERROR_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_RXFIFO_POP_ERROR_Pos)

#define WJ_TDM_FIFO_STATUS_RXFIFO_PUSH_ERROR_Pos                       (6U)
#define WJ_TDM_FIFO_STATUS_RXFIFO_PUSH_ERROR_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_RXFIFO_PUSH_ERROR_Pos)
#define WJ_TDM_FIFO_STATUS_RXFIFO_FLUSH_Pos                       (7U)
#define WJ_TDM_FIFO_STATUS_RXFIFO_FLUSH_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_RXFIFO_FLUSH_Pos)
#define WJ_TDM_FIFO_STATUS_TXFIFO_AEMPTY_Pos                       (8U)
#define WJ_TDM_FIFO_STATUS_TXFIFO_AEMPTY_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_TXFIFO_AEMPTY_Pos)
#define WJ_TDM_FIFO_STATUS_TXFIFO_EMPTY_Pos                       (9U)
#define WJ_TDM_FIFO_STATUS_TXFIFO_EMPTY_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_TXFIFO_EMPTY_Pos)
#define WJ_TDM_FIFO_STATUS_TXFIFO_AFULL_Pos                       (10U)
#define WJ_TDM_FIFO_STATUS_TXFIFO_AFULL_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_TXFIFO_AFULL_Pos)
#define WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Pos                       (11U)
#define WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_TXFIFO_FULL_Pos)
#define WJ_TDM_FIFO_STATUS_TXFIFO_TXLVL_TRIG_Pos                       (12U)
#define WJ_TDM_FIFO_STATUS_TXFIFO_TXLVL_TRIG_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_TXFIFO_TXLVL_TRIG_Pos)
#define WJ_TDM_FIFO_STATUS_TXFIFO_POP_ERROR_Pos                       (13U)
#define WJ_TDM_FIFO_STATUS_TXFIFO_POP_ERROR_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_TXFIFO_POP_ERROR_Pos)
#define WJ_TDM_FIFO_STATUS_TXFIFO_PUSH_ERROR_Pos                       (14U)
#define WJ_TDM_FIFO_STATUS_TXFIFO_PUSH_ERROR_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_TXFIFO_PUSH_ERROR_Pos)
#define WJ_TDM_FIFO_STATUS_TXFIFO_FLUSH_Pos                       (15U)
#define WJ_TDM_FIFO_STATUS_TXFIFO_FLUSH_Msk                       (0x1U << WJ_TDM_FIFO_STATUS_TXFIFO_FLUSH_Pos)
/* TDM_DMA_CONFIG , offset: 0x18 */
#define WJ_TDM_DMA_CONFIG_DMA_RXLVL_Pos                    (0U)
#define WJ_TDM_DMA_CONFIG_DMA_RXLVL_Msk                    (0xFFU << WJ_TDM_DMA_CONFIG_DMA_RXLVL_Pos)

#define WJ_TDM_DMA_CONFIG_DMA_TXLVL_Pos                    (8U)
#define WJ_TDM_DMA_CONFIG_DMA_TXLVL_Msk                    (0xFFU << WJ_TDM_DMA_CONFIG_DMA_TXLVL_Pos)

#define WJ_TDM_DMA_CONFIG_DMA_TX_EN_Pos                    (17U)
#define WJ_TDM_DMA_CONFIG_DMA_TX_EN_Msk                    (0x1U << WJ_TDM_DMA_CONFIG_DMA_TX_EN_Pos)

#define WJ_TDM_DMA_CONFIG_DMA_RX_EN_Pos                    (16U)
#define WJ_TDM_DMA_CONFIG_DMA_RX_EN_Msk                    (0x1U << WJ_TDM_DMA_CONFIG_DMA_RX_EN_Pos)

/* TDM_INT , offset: 0x20 */
#define WJ_TDM_INT_RXFIFO_DATA_READY_Pos                    (0U)
#define WJ_TDM_INT_RXFIFO_DATA_READY_Msk                    (0x1U << WJ_TDM_INT_RXFIFO_DATA_READY_Pos)

#define WJ_TDM_INT_RXFIFO_OVERFLOW_Pos                    (1U)
#define WJ_TDM_INT_RXFIFO_OVERFLOW_Msk                    (0x1U << WJ_TDM_INT_RXFIFO_OVERFLOW_Pos)

#define WJ_TDM_INT_RXFIFO_UNDERFLOW_Pos                    (2U)
#define WJ_TDM_INT_RXFIFO_UNDERFLOW_Msk                    (0x1U << WJ_TDM_INT_RXFIFO_UNDERFLOW_Pos)

#define WJ_TDM_INT_TXFIFO_DATA_READY_Pos                    (4U)
#define WJ_TDM_INT_TXFIFO_DATA_READY_Msk                    (0x1U << WJ_TDM_INT_TXFIFO_DATA_READY_Pos)

#define WJ_TDM_INT_TXFIFO_OVERFLOW_Pos                    (5U)
#define WJ_TDM_INT_TXFIFO_OVERFLOW_Msk                    (0x1U << WJ_TDM_INT_TXFIFO_OVERFLOW_Pos)

#define WJ_TDM_INT_TXFIFO_UNDERFLOW_Pos                    (6U)
#define WJ_TDM_INT_TXFIFO_UNDERFLOW_Msk                    (0x1U << WJ_TDM_INT_TXFIFO_UNDERFLOW_Pos)

/* TDM_INT_RAW , offset: 0x24 */
#define WJ_TDM_INT_RAW_RXFIFO_DATA_READY_Pos                    (0U)
#define WJ_TDM_INT_RAW_RXFIFO_DATA_READY_Msk                    (0x1U << WJ_TDM_INT_RAW_RXFIFO_DATA_READY_Pos)

#define WJ_TDM_INT_RAW_RXFIFO_OVERFLOW_Pos                    (1U)
#define WJ_TDM_INT_RAW_RXFIFO_OVERFLOW_Msk                    (0x1U << WJ_TDM_INT_RAW_RXFIFO_OVERFLOW_Pos)

#define WJ_TDM_INT_RAW_RXFIFO_UNDERFLOW_Pos                    (2U)
#define WJ_TDM_INT_RAW_RXFIFO_UNDERFLOW_Msk                    (0x1U << WJ_TDM_INT_RAW_RXFIFO_UNDERFLOW_Pos)

#define WJ_TDM_INT_RAW_TXFIFO_DATA_READY_Pos                    (4U)
#define WJ_TDM_INT_RAW_TXFIFO_DATA_READY_Msk                    (0x1U << WJ_TDM_INT_RAW_TXFIFO_DATA_READY_Pos)

#define WJ_TDM_INT_RAW_TXFIFO_OVERFLOW_Pos                    (5U)
#define WJ_TDM_INT_RAW_TXFIFO_OVERFLOW_Msk                    (0x1U << WJ_TDM_INT_RAW_TXFIFO_OVERFLOW_Pos)

#define WJ_TDM_INT_RAW_TXFIFO_UNDERFLOW_Pos                    (6U)
#define WJ_TDM_INT_RAW_TXFIFO_UNDERFLOW_Msk                    (0x1U << WJ_TDM_INT_RAW_TXFIFO_UNDERFLOW_Pos)

/* TDM_INT_EN , offset: 0x2C */
#define WJ_TDM_INT_EN_RXFIFO_DATA_READY_EN_Pos                    (0U)
#define WJ_TDM_INT_EN_RXFIFO_DATA_READY_EN_Msk                    (0x1U << WJ_TDM_INT_EN_RXFIFO_DATA_READY_EN_Pos)

#define WJ_TDM_INT_EN_RXFIFO_OVERFLOW_EN_Pos                    (1U)
#define WJ_TDM_INT_EN_RXFIFO_OVERFLOW_EN_Msk                    (0x1U << WJ_TDM_INT_EN_RXFIFO_OVERFLOW_EN_Pos)

#define WJ_TDM_INT_EN_RXFIFO_UNDERFLOW_EN_Pos                    (2U)
#define WJ_TDM_INT_EN_RXFIFO_UNDERFLOW_EN_Msk                    (0x1U << WJ_TDM_INT_EN_RXFIFO_UNDERFLOW_EN_Pos)

#define WJ_TDM_INT_EN_TXFIFO_DATA_READY_EN_Pos                    (4U)
#define WJ_TDM_INT_EN_TXFIFO_DATA_READY_EN_Msk                    (0x1U << WJ_TDM_INT_EN_TXFIFO_DATA_READY_EN_Pos)

#define WJ_TDM_INT_EN_TXFIFO_OVERFLOW_EN_Pos                    (5U)
#define WJ_TDM_INT_EN_TXFIFO_OVERFLOW_EN_Msk                    (0x1U << WJ_TDM_INT_EN_TXFIFO_OVERFLOW_EN_Pos)

#define WJ_TDM_INT_EN_TXFIFO_UNDERFLOW_EN_Pos                    (6U)
#define WJ_TDM_INT_EN_TXFIFO_UNDERFLOW_EN_Msk                    (0x1U << WJ_TDM_INT_EN_TXFIFO_UNDERFLOW_EN_Pos)

typedef struct {
    __IOM uint32_t TDM_TRANS_EN;
    __IOM uint32_t TDM_CONFIG;
    __IOM uint32_t TDM_CLOCK_GEN;
    __IOM uint32_t TDM_RXFIFO_DATA;
    __IOM uint32_t TDM_TXFIFO_DATA;
    __IOM uint32_t TDM_FIFO_STATUS;
    __IOM uint32_t TDM_DMA_CONFIG;
    uint32_t RESERVED0;
    __IOM uint32_t TDM_INT;
    __IOM uint32_t TDM_INT_RAW;
    uint32_t RESERVED1;
    __IOM uint32_t TDM_INT_EN;
} wj_tdm_regs_t;

static inline void wj_tdm_enable(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_TRANS_EN |= WJ_TMD_TRANS_EN;
}

static inline void wj_tdm_disable(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_TRANS_EN &= ~WJ_TMD_TRANS_EN;
}

static inline void wj_tdm_i2s_mode(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_CONFIG |= WJ_TDM_CONFIG_MODE_I2S;
}

static inline void wj_tdm_tdm_mode(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_CONFIG &= ~WJ_TDM_CONFIG_MODE_I2S;
}

static inline uint32_t wj_tdm_get_slot_width_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= WJ_TDM_CONFIG_SLOT_WIDTH_Msk;
    val = val >> WJ_TDM_CONFIG_SLOT_WIDTH_Pos;
    return val;
}

static inline void wj_tdm_set_slot_width_8bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_WIDTH_Msk;
    val |= WJ_TDM_CONFIG_SLOT_WIDTH_8BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_width_12bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_WIDTH_Msk;
    val |= WJ_TDM_CONFIG_SLOT_WIDTH_12BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_width_16bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_WIDTH_Msk;
    val |= WJ_TDM_CONFIG_SLOT_WIDTH_16BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_width_20bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_WIDTH_Msk;
    val |= WJ_TDM_CONFIG_SLOT_WIDTH_20BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_width_24bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_WIDTH_Msk;
    val |= WJ_TDM_CONFIG_SLOT_WIDTH_24BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_width_28bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_WIDTH_Msk;
    val |= WJ_TDM_CONFIG_SLOT_WIDTH_28BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_width_32bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_WIDTH_Msk;
    val |= WJ_TDM_CONFIG_SLOT_WIDTH_32BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_num(wj_tdm_regs_t *tdm_base, uint32_t slot_num)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_NUM_Msk;
    val |= slot_num << WJ_TDM_CONFIG_SLOT_NUM_Pos;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_resolution_8bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_RESOLUTION_Msk;
    val |= WJ_TDM_CONFIG_SLOT_RESOLUTION_8BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_resolution_12bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_RESOLUTION_Msk;
    val |= WJ_TDM_CONFIG_SLOT_RESOLUTION_12BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_resolution_16bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_RESOLUTION_Msk;
    val |= WJ_TDM_CONFIG_SLOT_RESOLUTION_16BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_resolution_20bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_RESOLUTION_Msk;
    val |= WJ_TDM_CONFIG_SLOT_RESOLUTION_20BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_resolution_24bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_RESOLUTION_Msk;
    val |= WJ_TDM_CONFIG_SLOT_RESOLUTION_24BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_resolution_28bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_RESOLUTION_Msk;
    val |= WJ_TDM_CONFIG_SLOT_RESOLUTION_28BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_slot_resolution_32bits_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_SLOT_RESOLUTION_Msk;
    val |= WJ_TDM_CONFIG_SLOT_RESOLUTION_32BITS;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_i2s_chen(wj_tdm_regs_t *tdm_base, uint32_t ch_num)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val |= ((1 << ch_num) << WJ_TDM_CONFIG_I2S_CHEN_Pos);
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_i2s_chdis(wj_tdm_regs_t *tdm_base, uint32_t ch_num)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~((1 << ch_num) << WJ_TDM_CONFIG_I2S_CHEN_Pos);
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_i2s_chtx(wj_tdm_regs_t *tdm_base, uint32_t ch_num)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~((1 << ch_num) << WJ_TDM_CONFIG_I2S_CHTXRX_Pos);
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_i2s_chrx(wj_tdm_regs_t *tdm_base, uint32_t ch_num)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val |= ((1 << ch_num) << WJ_TDM_CONFIG_I2S_CHTXRX_Pos);
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_data_extention_paddingzero(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_DATA_EXTENTION_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_set_data_extention_sign_msb(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val |= WJ_TDM_CONFIG_DATA_EXTENTION_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_loop_test(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val |= WJ_TDM_CONFIG_LOOP_TEST_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_config_edge_transfer_same_edge(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_EDGE_TRANSFER_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_config_edge_transfer_different_edge(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val |= WJ_TDM_CONFIG_EDGE_TRANSFER_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_mute_enable(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val |= WJ_TDM_CONFIG_OUT_MUTE_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_mute_disable(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_OUT_MUTE_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_transmit_ch_enable(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val |= WJ_TDM_CONFIG_TDM_TXEN_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_transmit_ch_disable(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_TDM_TXEN_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_receive_ch_disable(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val &= ~WJ_TDM_CONFIG_TDM_RXEN_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_receive_ch_enable(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CONFIG;
    val |= WJ_TDM_CONFIG_TDM_RXEN_Msk;
    tdm_base->TDM_CONFIG = val;
}

static inline void wj_tdm_master_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val |= WJ_TDM_CLOCK_GEN_MASTEREN_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_slave_mode(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_MASTEREN_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_sclk_polarity_negative_dege(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_SCLK_POLARITY_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_sclk_polarity_posedge_dege(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val |= WJ_TDM_CLOCK_GEN_SCLK_POLARITY_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_ws_format_standard(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_WS_FORMAT_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_ws_format_left_justified(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_WS_FORMAT_Msk;
    val |= WJ_TDM_CLOCK_GEN_WS_FORMAT_TDM_WS_LEFT_JUSTIFIED;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_ws_format_right_justified(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_WS_FORMAT_Msk;
    val |= WJ_TDM_CLOCK_GEN_WS_FORMAT_I2S_WS_RIGHT_JUSTIFIED;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_ws_polarity_i2s_left_negative(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_WS_POLARITY_I2S_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_ws_polarity_i2s_left_posetive(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val |= WJ_TDM_CLOCK_GEN_WS_POLARITY_I2S_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_ws_width_tdm_1cycle(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_WS_WIDTH_TDM_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_ws_width_tdm_2cycle(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val |= WJ_TDM_CLOCK_GEN_WS_WIDTH_TDM_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_sclk_enable(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val |= WJ_TDM_CLOCK_GEN_WS_SCLK_ENABLE_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_sclk_disable(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_WS_SCLK_ENABLE_Msk;
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline void wj_tdm_mclk_div(wj_tdm_regs_t *tdm_base, uint8_t div)
{
    uint32_t val = 0;
    val = tdm_base->TDM_CLOCK_GEN;
    val &= ~WJ_TDM_CLOCK_GEN_WS_MCLK_DIV_Msk;
    val |= (div << WJ_TDM_CLOCK_GEN_WS_MCLK_DIV_Pos);
    tdm_base->TDM_CLOCK_GEN = val;
}

static inline uint32_t wj_tdm_get_data(wj_tdm_regs_t *tdm_base)
{
    return tdm_base->TDM_RXFIFO_DATA;
}

static inline void wj_tdm_write_data(wj_tdm_regs_t *tdm_base, uint32_t data)
{
    tdm_base->TDM_TXFIFO_DATA = data;
}

static inline void wj_tdm_flush_txfifo(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_FIFO_STATUS |= WJ_TDM_FIFO_STATUS_TXFIFO_FLUSH_Msk;
    tdm_base->TDM_FIFO_STATUS &= ~WJ_TDM_FIFO_STATUS_TXFIFO_FLUSH_Msk;
}

static inline void wj_tdm_flush_rxfifo(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_FIFO_STATUS |= WJ_TDM_FIFO_STATUS_RXFIFO_FLUSH_Msk;
    tdm_base->TDM_FIFO_STATUS &= ~WJ_TDM_FIFO_STATUS_RXFIFO_FLUSH_Msk;
}

static inline uint32_t wj_tdm_get_fifo_status(wj_tdm_regs_t *tdm_base)
{
    return  tdm_base->TDM_FIFO_STATUS;
}

static inline uint32_t wj_tdm_get_irq_status(wj_tdm_regs_t *tdm_base)
{
    return tdm_base->TDM_INT;
}

static inline void wj_tdm_clr_tx_fifo_overflow_irq(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_INT |= WJ_TDM_INT_TXFIFO_OVERFLOW_Msk;
}

static inline void wj_tdm_clr_rx_fifo_overflow_irq(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_INT |= WJ_TDM_INT_RXFIFO_OVERFLOW_Msk;
}

static inline void wj_tdm_clr_tx_fifo_underflow_irq(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_INT |= WJ_TDM_INT_TXFIFO_UNDERFLOW_Msk;
}

static inline void wj_tdm_clr_rx_fifo_underflow_irq(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_INT |= WJ_TDM_INT_RXFIFO_UNDERFLOW_Msk;
}

static inline void wj_tdm_clr_rx_fifo_dataready_irq(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_INT |= WJ_TDM_INT_RXFIFO_DATA_READY_Msk;
}

static inline void wj_tdm_clr_tx_fifo_dataready_irq(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_INT |= WJ_TDM_INT_TXFIFO_DATA_READY_Msk;
}

static inline uint32_t wj_tdm_get_irq_raw_status(wj_tdm_regs_t *tdm_base)
{
    return tdm_base->TDM_INT_RAW;
}

static inline void wj_tdm_txdma_enable(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_DMA_CONFIG |= WJ_TDM_DMA_CONFIG_DMA_TX_EN_Msk;
}

static inline void wj_tdm_txdma_disable(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_DMA_CONFIG &= ~WJ_TDM_DMA_CONFIG_DMA_TX_EN_Msk;
}

static inline void wj_tdm_rxdma_enable(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_DMA_CONFIG |= WJ_TDM_DMA_CONFIG_DMA_RX_EN_Msk;
}

static inline void wj_tdm_rxdma_disable(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_DMA_CONFIG &= ~WJ_TDM_DMA_CONFIG_DMA_RX_EN_Msk;
}

static inline void wj_tdm_set_txdma_data_level(wj_tdm_regs_t *tdm_base, uint32_t level)
{
    uint32_t val = 0;
    val = tdm_base->TDM_DMA_CONFIG;
    val &= ~WJ_TDM_DMA_CONFIG_DMA_TXLVL_Msk;
    val |= (level << WJ_TDM_DMA_CONFIG_DMA_TXLVL_Pos);
    tdm_base->TDM_DMA_CONFIG = val;
}

static inline uint32_t wj_tdm_get_txdma_data_level(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_DMA_CONFIG;
    val &= WJ_TDM_DMA_CONFIG_DMA_TXLVL_Msk;
    val = val >> WJ_TDM_DMA_CONFIG_DMA_TXLVL_Pos;
    return val;
}

static inline uint32_t wj_tdm_get_rxdma_data_level(wj_tdm_regs_t *tdm_base)
{
    uint32_t val = 0;
    val = tdm_base->TDM_DMA_CONFIG;
    val &= WJ_TDM_DMA_CONFIG_DMA_RXLVL_Msk;
    val = val >> WJ_TDM_DMA_CONFIG_DMA_RXLVL_Pos;
    return val;
}

static inline void wj_tdm_set_rxdma_data_level(wj_tdm_regs_t *tdm_base, uint32_t level)
{
    uint32_t val = 0;
    val = tdm_base->TDM_DMA_CONFIG;
    val &= ~WJ_TDM_DMA_CONFIG_DMA_RXLVL_Msk;
    val |= (level << WJ_TDM_DMA_CONFIG_DMA_RXLVL_Pos);
    tdm_base->TDM_DMA_CONFIG = val;
}

static inline void wj_tdm_enable_tx_data_ready_irq(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_INT_EN |= WJ_TDM_INT_EN_TXFIFO_DATA_READY_EN_Msk;
}

static inline void wj_tdm_enable_rx_data_ready_irq(wj_tdm_regs_t *tdm_base)
{
    tdm_base->TDM_INT_EN |= WJ_TDM_INT_EN_RXFIFO_DATA_READY_EN_Msk;
}

static inline uint32_t wj_tdm_get_int_enable_status(wj_tdm_regs_t *tdm_base)
{
    return tdm_base->TDM_INT_EN;
}


#ifdef __cplusplus
}
#endif

#endif /* _WJ_TDM_LL_H_ */
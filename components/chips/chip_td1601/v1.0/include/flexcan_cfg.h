/*
 * @flexcan_cfg.h
 *
 *  Created on: 2023年8月25日
 *      Author: Dell
 */

#ifndef SDRV_FLEXCAN_CFG_H_
#define SDRV_FLEXCAN_CFG_H_

#include "sdrv_flexcan.h"

#define NXP_CSI_FLEXCAN_DEBUG_SW    (1)
#define NXP_CSI_FLEXCAN_XFER_POLL   (0)

#if (defined(NXP_CSI_FLEXCAN_XFER_POLL) && !NXP_CSI_FLEXCAN_XFER_POLL)
#define NXP_CSI_FLEXCAN_XFER_INT    (1)
#endif

#if (defined(NXP_CSI_FLEXCAN_XFER_INT) && NXP_CSI_FLEXCAN_XFER_INT)
#define NXP_FLEXCAN_RX_FIFO         (0)
#define NXP_FLEXCAN_WAKINT          (0)

#if (defined(NXP_FLEXCAN_RX_FIFO) && !NXP_FLEXCAN_RX_FIFO)
//NXP_FLEXCAN_BUSOFF
#define NXP_FLEXCAN_BUSOFF          (0)
#if (defined(NXP_FLEXCAN_BUSOFF) && NXP_FLEXCAN_BUSOFF)
#define NXP_FLEXCAN_BUSOFF_DONE     (1)
#endif
#define NXP_FLEXCAN_PNET            (0)
#endif

#if (defined(NXP_FLEXCAN_PNET) && NXP_FLEXCAN_PNET)
//#define FLEXCAN_HAS_GLITCH_FILTER   1
#define NXP_FLEXCAN_PNET_WAKE_TO    (0)
#endif

#define NXP_CSI_FLEXCAN_ERROR_AND_WARNING_INT    (0)

#endif  //end of (defined(NXP_CSI_FLEXCAN_XFER_INT) && NXP_CSI_FLEXCAN_XFER_INT)

/* timer inter */
#define NXP_FLEXCAN_HIGH_TIMER_WRAP_INT          (0)

#define TX_PADDING_VAL (0xA5)
#define NXP_FLEXCAN_SUPERVISOR_ACCESS       (1<<0)
#define NXP_FLEXCAN_TEST_ACCESS             (1<<1)
#define NXP_FLEXCAN_DEBUG_ACCESS            (1<<2)
#define NXP_FLEXCAN_DOZE_ACCESS             (1<<3)
#define NXP_FLEXCAN_STOP_ACCESS             (1<<4)
#define NXP_FLEXCAN_SOFT_RESET_ACCESS       (1<<8)
#define NXP_FLEXCAN_HR_TIMER_EN_ACCESS      (1<<12)

#if 1
/* for 40MHz. */
#define BAUDRATE_1M_5M                                                         \
    .nominalBitTiming =                                                        \
        {/* 1Mbps, sample point 80% */                                         \
         .preDivider = 1U,                                                     \
         .rJumpwidth = 5U,                                                     \
         .propSeg = 10U,                                                       \
         /*.phaseSeg1 = 21U,         for 40M */                                \
         /*.phaseSeg1 = 29U,   */ /* for 48M */                                \
         .phaseSeg1 = 21U,                                                     \
         .phaseSeg2 = 8U},                                                     \
    .dataBitTiming =                                                           \
        {/* 5Mbps, sample point 75% */                                         \
         .preDivider =                                                         \
             1U, /* Should be the same as nominalBitTiming.preDivider. */      \
         .rJumpwidth = 1U,                                                     \
         .propSeg = 2U,                                                        \
         /*.phaseSeg1 = 3U,         for 40M  */                                \
         /*.phaseSeg1 = 5U,   */ /* for 48M  */                                \
         .phaseSeg1 = 3U,                                                      \
         .phaseSeg2 = 2U}

/* for 40MHz. */
#define BAUDRATE_500K_1M                                                       \
    .nominalBitTiming =                                                        \
        {/* 500kbps, sample point 75% */                                       \
         .preDivider = 4U,                                                     \
         .rJumpwidth = 2U,                                                     \
         .propSeg = 6U,                                                        \
         /*Ori for 40M .phaseSeg1 = 8U,*/                                      \
         /*for 48M .phaseSeg1 = 12U,   */                                      \
         .phaseSeg1 = 8U,                                                     \
         .phaseSeg2 = 5U},                                                     \
    .dataBitTiming =                                                           \
        {/* 1Mbps, sample point 80% */                                         \
         .preDivider =                                                         \
             4U, /* Should be the same as nominalBitTiming.preDivider. */      \
         .rJumpwidth = 2U,                                                     \
         .propSeg = 3U,                                                        \
         /*Ori for 40M.phaseSeg1 = 4U, */                                      \
         /*for 48M.phaseSeg1 = 6U, */                                          \
         .phaseSeg1 = 4U,                                                      \
         .phaseSeg2 = 2U}
#else
/* for 48MHz. */
#define BAUDRATE_1M_5M                                                         \
    .nominalBitTiming =                                                        \
        {/* 1Mbps, sample point 80% */                                         \
         .preDivider = 1U,                                                     \
         .rJumpwidth = 5U,                                                     \
         .propSeg = 10U,                                                       \
         /*.phaseSeg1 = 21U,         for 40M */                                \
         /*.phaseSeg1 = 29U,   */ /* for 48M */                                \
         .phaseSeg1 = 29U,                                                     \
         .phaseSeg2 = 8U},                                                     \
    .dataBitTiming =                                                           \
        {/* 5Mbps, sample point 75% */                                         \
         .preDivider =                                                         \
             1U, /* Should be the same as nominalBitTiming.preDivider. */      \
         .rJumpwidth = 1U,                                                     \
         .propSeg = 2U,                                                        \
         /*.phaseSeg1 = 3U,         for 40M  */                                \
         /*.phaseSeg1 = 5U,   */ /* for 48M  */                                \
         .phaseSeg1 = 5U,                                                      \
         .phaseSeg2 = 2U}

/* for 40MHz. */
#define BAUDRATE_500K_1M                                                       \
    .nominalBitTiming =                                                        \
        {/* 500kbps, sample point 75% */                                       \
         .preDivider = 4U,                                                     \
         .rJumpwidth = 2U,                                                     \
         .propSeg = 6U,                                                        \
         /*Ori for 40M .phaseSeg1 = 8U,*/                                      \
         /*for 48M .phaseSeg1 = 12U,   */                                      \
         .phaseSeg1 = 12U,                                                     \
         .phaseSeg2 = 5U},                                                     \
    .dataBitTiming =                                                           \
        {/* 1Mbps, sample point 80% */                                         \
         .preDivider =                                                         \
             4U, /* Should be the same as nominalBitTiming.preDivider. */      \
         .rJumpwidth = 2U,                                                     \
         .propSeg = 3U,                                                        \
         /*Ori for 40M.phaseSeg1 = 4U, */                                      \
         /*for 48M.phaseSeg1 = 6U, */                                          \
         .phaseSeg1 = 6U,                                                      \
         .phaseSeg2 = 2U}
#endif
#define RX_MAILBOX_NUM 6
#define TX_MAILBOX_NUM 7

#if (defined(NXP_FLEXCAN_RX_FIFO) && NXP_FLEXCAN_RX_FIFO)
#define RX_FIFO_ID_FILTER_NUM 		32
#define USED_MB_FOR_FIFO 			14
extern flexcan_rx_fifo_config_t flexcan_fifo_cfg;
#endif

extern const flexcan_config_t g_flexcan_config;
extern flexcan_rx_mb_config_t rxmbcfg[RX_MAILBOX_NUM];

/**/
extern int canfd_pmu_cfg_default(flexcan_handle_t *handle);
extern int canfd_pmu_cfg_doze_en(flexcan_handle_t *handle);
extern int canfd_pmu_cfg_soft_reset(flexcan_handle_t *handle);

#endif /* SDRV_FLEXCAN_CFG_H_ */
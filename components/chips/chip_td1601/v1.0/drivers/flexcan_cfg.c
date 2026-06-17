/*
 * flexcan_cfg.c
 *
 *  Created on: 2023年8月25日
 *      Author: Dell
 */

#include "flexcan_cfg.h"
#include "sdrv_flexcan.h"

#if (defined(MCT_FLEXCAN_RX_FIFO) && MCT_FLEXCAN_RX_FIFO)
const flexcan_config_t g_flexcan_config = {
    .clkSrc = FLEXCAN_ClkSrcOsc, /* 40MHz */
    //.clkSrc = FLEXCAN_ClkSrcPeri, /* 48MHz */
    .maxMbNum = 64U,
    .enableSelfWakeup = true,
    .enableIndividMask = true,
    .enableCANFD = false,
    BAUDRATE_500K_1M,
    //BAUDRATE_1M_5M,
    .enableLBUFTransmittedFirst = true
};

static flexcan_rx_fifo_filter_table_t flexcan_tableID[RX_FIFO_ID_FILTER_NUM] = {
    {
        MAKE_TYPE_A_FILTER(0x327, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x337, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x347, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x357, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x367, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x377, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x387, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x397, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x3a7, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x3b7, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x3c7, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {  /* LSB can be 0 or 1 */
        MAKE_TYPE_A_FILTER(0x3d7, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FE, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x3e7, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    },
    {  /* LSB can be 0 or 1 */
        MAKE_TYPE_A_FILTER(0x3f7, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFE, EXTENDED_DATA_ID)
    },
    /* remaining elements can only be affected by RXFGMASK. */
    {
        MAKE_TYPE_A_FILTER(0x507, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x517, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x527, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x537, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x547, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x557, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x567, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x577, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FF, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x587, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FE, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x597, STANDARD_DATA_ID),
        MAKE_TYPE_A_FILTER(0x7FE, STANDARD_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x5a7, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x5b7, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x5c7, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x5d7, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x5e7, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x5f7, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x607, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    },
    {
        MAKE_TYPE_A_FILTER(0x617, EXTENDED_DATA_ID),
        MAKE_TYPE_A_FILTER(0x1FFFFFFF, EXTENDED_DATA_ID)
    }
};

flexcan_rx_fifo_config_t flexcan_fifo_cfg = {
    .priority = FLEXCAN_RxFifoPrioHigh,
    .idFilterType = FLEXCAN_RxFifoFilterTypeA,
    .idFilterNum = RX_FIFO_ID_FILTER_NUM,
    .filter_tab = &flexcan_tableID[0]
};

#else
const flexcan_config_t g_flexcan_config = {
    .clkSrc = FLEXCAN_ClkSrcOsc, /* 40MHz */
//	.clkSrc = FLEXCAN_ClkSrcPeri, /* 48MHz */
    .maxMbNum = 14U,
#if (defined(MCT_FLEXCAN_PNET) && MCT_FLEXCAN_PNET)
    .enableSelfWakeup = false,
#else
    .enableSelfWakeup = true,
#endif
    .enableIndividMask = true,
    .enableCANFD = true,
//	.enableCANFD = false,
//	BAUDRATE_500K_1M,
    BAUDRATE_1M_5M,
    ///for test
//    .enableLoopBack = true,
    .enableLBUFTransmittedFirst = true,
    .can_fd_cfg = {.enableISOCANFD = true,
                   .enableBRS = true,
                   .enableTDC = true,
                   .TDCOffset = 8U,
                   .r0_mb_data_size = CAN_FD_64BYTES_PER_MB,
                   .r1_mb_data_size = CAN_FD_64BYTES_PER_MB},
#if (defined(MCT_FLEXCAN_PNET) && MCT_FLEXCAN_PNET)
    .enableCANFD = false,
    .enablePretendedNetworking = true,
    .enableSelfWakeup = false,
    .enableDoze = true,
    .pnet_cfg = {
        .wakeUpMatch = true,
        .numMatches = 25,
#if (defined(MCT_FLEXCAN_PNET_WAKE_TO) && MCT_FLEXCAN_PNET_WAKE_TO )   //for wake to , timeout
        .wakeUpTimeout = true,
        .matchTimeout = 0x100,
#endif
        .filterComb = FLEXCAN_FILTER_ID_NTIMES,
        .idFilter1 = {.id = 0x12},
        .idFilter2 = {.id = 0x45},
        .idFilterType = FLEXCAN_FILTER_MATCH_RANGE,
    }
#endif              
};

flexcan_rx_mb_config_t rxmbcfg[RX_MAILBOX_NUM] = {
#if (defined(MCT_FLEXCAN_PNET) && MCT_FLEXCAN_PNET)
    {.id = 0x12,
     .format = FLEXCAN_STANDARD_FRAME,
     .type = FLEXCAN_FrameTypeData},
    {.id = 0x13,
     .format = FLEXCAN_STANDARD_FRAME,
     .type = FLEXCAN_FrameTypeData},
#endif
    {.id = 0x123,
     .format = FLEXCAN_STANDARD_FRAME,
     .type = FLEXCAN_FrameTypeData},
    {.id = 0x249,
     .format = FLEXCAN_STANDARD_FRAME,
     .type = FLEXCAN_FrameTypeData},
    {.id = 0x208,
     .format = FLEXCAN_STANDARD_FRAME,
     .type = FLEXCAN_FrameTypeData},
    {.id = 0x200,
     .format = FLEXCAN_STANDARD_FRAME,
     .type = FLEXCAN_FrameTypeData},
    {.id = 0x230,
     .format = FLEXCAN_STANDARD_FRAME,
     .type = FLEXCAN_FrameTypeData},
};
#endif


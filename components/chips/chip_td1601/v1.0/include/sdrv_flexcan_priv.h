/**
 * @file sdrv_flexcan_priv.h
 *
 *  Created on: 2023年8月25日
 *      Author: Dell
 */

#ifndef SDRV_FLEXCAN_PRIV_H_
#define SDRV_FLEXCAN_PRIV_H_

#include <stdint.h>

#ifdef __cplusplus
#define __I volatile /**< Defines 'read only' permissions */
#else
#define __I volatile const /**< Defines 'read only' permissions */
#endif
#define __O volatile  /**< Defines 'write only' permissions */
#define __IO volatile /**< Defines 'read / write' permissions */

/** CAN - Register Layout Typedef */
typedef struct {
    __IO uint32_t MCR;   /**< Module Configuration Register, offset: 0x0 */
    __IO uint32_t CTRL1; /**< Control 1 register, offset: 0x4 */
    __IO uint32_t TIMER; /**< Free Running Timer, offset: 0x8 */
    uint8_t RESERVED_0[4];
    __IO uint32_t
        RXMGMASK; /**< Rx Mailboxes Global Mask Register, offset: 0x10 */
    __IO uint32_t RX14MASK; /**< Rx 14 Mask register, offset: 0x14 */
    __IO uint32_t RX15MASK; /**< Rx 15 Mask register, offset: 0x18 */
    __IO uint32_t ECR;      /**< Error Counter, offset: 0x1C */
    __IO uint32_t ESR1;     /**< Error and Status 1 register, offset: 0x20 */
    __IO uint32_t IMASK2;   /**< Interrupt Masks 2 register, offset: 0x24 */
    __IO uint32_t IMASK1;   /**< Interrupt Masks 1 register, offset: 0x28 */
    __IO uint32_t IFLAG2;   /**< Interrupt Flags 2 register, offset: 0x2C */
    __IO uint32_t IFLAG1;   /**< Interrupt Flags 1 register, offset: 0x30 */
    __IO uint32_t CTRL2;    /**< Control 2 register, offset: 0x34 */
    __IO uint32_t ESR2;      /**< Error and Status 2 register, offset: 0x38 */
    uint8_t RESERVED_1[8];
    __I uint32_t CRCR;      /**< CRC Register, offset: 0x44 */
    __IO uint32_t RXFGMASK; /**< Rx FIFO Global Mask register, offset: 0x48 */
    __I uint32_t RXFIR;     /**< Rx FIFO Information Register, offset: 0x4C */
    __IO uint32_t CBT;      /**< CAN Bit Timing Register, offset: 0x50 */
    uint8_t RESERVED_2[20];
    __IO uint32_t IMASK4; /**< Interrupt Masks 4 register, offset: 0x68 */
    __IO uint32_t IMASK3; /**< Interrupt Masks 3 register, offset: 0x6C */
    __IO uint32_t IFLAG4; /**< Interrupt Flags 4 register, offset: 0x70 */
    __IO uint32_t IFLAG3; /**< Interrupt Flags 3 register, offset: 0x74 */
    uint8_t RESERVED_3[8];
    struct { /* offset: 0x80, array step: 0x10 */
        __IO uint32_t
            CS; /**< Message Buffer 0 CS Register..Message Buffer 127 CS
                   Register, array offset: 0x80, array step: 0x10 */
        __IO uint32_t
            ID; /**< Message Buffer 0 ID Register..Message Buffer 127 ID
                   Register, array offset: 0x84, array step: 0x10 */
        __IO uint32_t
            WORD0; /**< Message Buffer 0 WORD0 Register..Message Buffer 127
                      WORD0 Register, array offset: 0x88, array step: 0x10 */
        __IO uint32_t
            WORD1; /**< Message Buffer 0 WORD1 Register..Message Buffer 127
                      WORD1 Register, array offset: 0x8C, array step: 0x10 */
    } MB[128];
    __IO uint32_t RXIMR[128]; /**< Rx Individual Mask Registers, array offset:
                                 0x880, array step: 0x4 */
    uint8_t RESERVED_4[96];
    __IO uint32_t MECR; /**< Memory Error Control Register, offset: 0xAE0 */
    __IO uint32_t
        ERRIAR; /**< Error Injection Address Register, offset: 0xAE4 */
    __IO uint32_t
        ERRIDPR; /**< Error Injection Data Pattern Register, offset: 0xAE8 */
    __IO uint32_t
        ERRIPPR; /**< Error Injection Parity Pattern Register, offset: 0xAEC */
    __I uint32_t RERRAR;   /**< Error Report Address Register, offset: 0xAF0 */
    __I uint32_t RERRDR;   /**< Error Report Data Register, offset: 0xAF4 */
    __I uint32_t RERRSYNR; /**< Error Report Syndrome Register, offset: 0xAF8 */
    __IO uint32_t ERRSR;   /**< Error Status Register, offset: 0xAFC */
    __IO uint32_t
        CTRL1_PN; /**< Pretended Networking Control 1 Register, offset: 0xB00 */
    __IO uint32_t
        CTRL2_PN; /**< Pretended Networking Control 2 Register, offset: 0xB04 */
    __IO uint32_t WU_MTC;  /**< Pretended Networking Wake Up Match Register,
                              offset: 0xB08 */
    __IO uint32_t FLT_ID1; /**< Pretended Networking ID Filter 1 Register,
                              offset: 0xB0C */
    __IO uint32_t
        FLT_DLC; /**< Pretended Networking DLC Filter Register, offset: 0xB10 */
    __IO uint32_t PL1_LO; /**< Pretended Networking Payload Low Filter 1
                             Register, offset: 0xB14 */
    __IO uint32_t PL1_HI; /**< Pretended Networking Payload High Filter 1
                             Register, offset: 0xB18 */
    __IO uint32_t
        FLT_ID2_IDMASK; /**< Pretended Networking ID Filter 2 Register/ ID Mask
                           Register, offset: 0xB1C */
    __IO uint32_t
        PL2_PLMASK_LO; /**< Pretended Networking Payload Low Filter 2 Register/
                          Payload Low Mask Register, offset: 0xB20 */
    __IO uint32_t PL2_PLMASK_HI; /**< Pretended Networking Payload High Filter 2
                                    High Order Bits/ Payload High Mask Register,
                                    offset: 0xB24 */
    uint8_t RESERVED_5[24];
    struct {                  /* offset: 0xB40, array step: 0x10 */
        __I uint32_t WMB_CS;  /**< Wake Up Message Buffer Register for C/S,
                                 offset: 0xB40, array step: 0x10 */
        __I uint32_t WMB_ID;  /**< Wake Up Message Buffer Register for ID,
                                 offset: 0xB44, array step: 0x10 */
        __I uint32_t WMB_D03; /**< Wake Up Message Buffer Register for Data 0-3,
                                 offset: 0xB48, array step: 0x10 */
        __I uint32_t WMB_D47; /**< Wake Up Message Buffer Register for Data 4-7,
                                 offset: 0xB4C, array step: 0x10 */
    } WMB[4];
    uint8_t RESERVED_6[128];
    __IO uint32_t FDCTRL; /**< CAN FD Control Register, offset: 0xC00 */
    __IO uint32_t FDCBT;  /**< CAN FD Bit Timing Register, offset: 0xC04 */
    __I uint32_t FDCRC;   /**< CAN FD CRC Register, offset: 0xC08 */
    __IO uint32_t ERFCR;  /**< Enhanced Rx FIFO Control Register, offset: 0xC0C */
    __IO uint32_t ERFIER; /**< Enhanced Rx FIFO Interrupt Enable Register, offset: 0xC10 */
    __IO uint32_t ERFSR;  /**< Enhanced Rx FIFO Status Register, offset: 0xC14 */
    __IO uint32_t HR_TIME_STAMP[128];  /**< High Resolution Time Stamp Registers, offset: 0xC30–E2C */
    uint8_t RESERVED_7[8660];
    __IO uint32_t ERFFEL[128];  /**< Enhanced Rx FIFO Filter Element Registers, offset: 0x3000–31FC */
} CAN_Type;

/* ----------------------------------------------------------------------------
   -- CAN Register Masks
   ----------------------------------------------------------------------------
 */

/**
 * @addtogroup CAN_Register_Masks CAN Register Masks
 * @{
 */

/** @name MCR - Module Configuration Register */
#define CAN_MCR_MAXMB_MASK (0x7FU)
#define CAN_MCR_MAXMB_SHIFT (0U)
#define CAN_MCR_MAXMB(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_MAXMB_SHIFT)) & CAN_MCR_MAXMB_MASK)
#define CAN_MCR_IDAM_MASK (0x300U)
#define CAN_MCR_IDAM_SHIFT (8U)
#define CAN_MCR_IDAM(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_IDAM_SHIFT)) & CAN_MCR_IDAM_MASK)
#define CAN_MCR_FDEN_MASK (0x800U)
#define CAN_MCR_FDEN_SHIFT (11U)
#define CAN_MCR_FDEN(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_FDEN_SHIFT)) & CAN_MCR_FDEN_MASK)
#define CAN_MCR_AEN_MASK (0x1000U)
#define CAN_MCR_AEN_SHIFT (12U)
#define CAN_MCR_AEN(x)                                                         \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_AEN_SHIFT)) & CAN_MCR_AEN_MASK)
#define CAN_MCR_LPRIOEN_MASK (0x2000U)
#define CAN_MCR_LPRIOEN_SHIFT (13U)
#define CAN_MCR_LPRIOEN(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_LPRIOEN_SHIFT)) &                  \
     CAN_MCR_LPRIOEN_MASK)
#define CAN_MCR_PNET_EN_MASK (0x4000U)
#define CAN_MCR_PNET_EN_SHIFT (14U)
#define CAN_MCR_PNET_EN(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_PNET_EN_SHIFT)) &                  \
     CAN_MCR_PNET_EN_MASK)
#define CAN_MCR_DMA_MASK (0x8000U)
#define CAN_MCR_DMA_SHIFT (15U)
#define CAN_MCR_DMA(x)                                                         \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_DMA_SHIFT)) & CAN_MCR_DMA_MASK)
#define CAN_MCR_IRMQ_MASK (0x10000U)
#define CAN_MCR_IRMQ_SHIFT (16U)
#define CAN_MCR_IRMQ(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_IRMQ_SHIFT)) & CAN_MCR_IRMQ_MASK)
#define CAN_MCR_SRXDIS_MASK (0x20000U)
#define CAN_MCR_SRXDIS_SHIFT (17U)
#define CAN_MCR_SRXDIS(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_SRXDIS_SHIFT)) &                   \
     CAN_MCR_SRXDIS_MASK)
#define CAN_MCR_DOZE_MASK (0x40000U)
#define CAN_MCR_DOZE_SHIFT (18U)
#define CAN_MCR_DOZE(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_DOZE_SHIFT)) & CAN_MCR_DOZE_MASK)
#define CAN_MCR_WAKSRC_MASK (0x80000U)
#define CAN_MCR_WAKSRC_SHIFT (19U)
#define CAN_MCR_WAKSRC(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_WAKSRC_SHIFT)) &                   \
     CAN_MCR_WAKSRC_MASK)
#define CAN_MCR_LPMACK_MASK (0x100000U)
#define CAN_MCR_LPMACK_SHIFT (20U)
#define CAN_MCR_LPMACK(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_LPMACK_SHIFT)) &                   \
     CAN_MCR_LPMACK_MASK)
#define CAN_MCR_WRNEN_MASK (0x200000U)
#define CAN_MCR_WRNEN_SHIFT (21U)
#define CAN_MCR_WRNEN(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_WRNEN_SHIFT)) & CAN_MCR_WRNEN_MASK)
#define CAN_MCR_SLFWAK_MASK (0x400000U)
#define CAN_MCR_SLFWAK_SHIFT (22U)
#define CAN_MCR_SLFWAK(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_SLFWAK_SHIFT)) &                   \
     CAN_MCR_SLFWAK_MASK)
#define CAN_MCR_SUPV_MASK (0x800000U)
#define CAN_MCR_SUPV_SHIFT (23U)
#define CAN_MCR_SUPV(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_SUPV_SHIFT)) & CAN_MCR_SUPV_MASK)
#define CAN_MCR_FRZACK_MASK (0x1000000U)
#define CAN_MCR_FRZACK_SHIFT (24U)
#define CAN_MCR_FRZACK(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_FRZACK_SHIFT)) &                   \
     CAN_MCR_FRZACK_MASK)
#define CAN_MCR_SOFTRST_MASK (0x2000000U)
#define CAN_MCR_SOFTRST_SHIFT (25U)
#define CAN_MCR_SOFTRST(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_SOFTRST_SHIFT)) &                  \
     CAN_MCR_SOFTRST_MASK)
#define CAN_MCR_WAKMSK_MASK (0x4000000U)
#define CAN_MCR_WAKMSK_SHIFT (26U)
#define CAN_MCR_WAKMSK(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_WAKMSK_SHIFT)) &                   \
     CAN_MCR_WAKMSK_MASK)
#define CAN_MCR_NOTRDY_MASK (0x8000000U)
#define CAN_MCR_NOTRDY_SHIFT (27U)
#define CAN_MCR_NOTRDY(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_NOTRDY_SHIFT)) &                   \
     CAN_MCR_NOTRDY_MASK)
#define CAN_MCR_HALT_MASK (0x10000000U)
#define CAN_MCR_HALT_SHIFT (28U)
#define CAN_MCR_HALT(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_HALT_SHIFT)) & CAN_MCR_HALT_MASK)
#define CAN_MCR_RFEN_MASK (0x20000000U)
#define CAN_MCR_RFEN_SHIFT (29U)
#define CAN_MCR_RFEN(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_RFEN_SHIFT)) & CAN_MCR_RFEN_MASK)
#define CAN_MCR_FRZ_MASK (0x40000000U)
#define CAN_MCR_FRZ_SHIFT (30U)
#define CAN_MCR_FRZ(x)                                                         \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_FRZ_SHIFT)) & CAN_MCR_FRZ_MASK)
#define CAN_MCR_MDIS_MASK (0x80000000U)
#define CAN_MCR_MDIS_SHIFT (31U)
#define CAN_MCR_MDIS(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_MCR_MDIS_SHIFT)) & CAN_MCR_MDIS_MASK)

/** @name CTRL1 - Control 1 register */
#define CAN_CTRL1_PROPSEG_MASK (0x7U)
#define CAN_CTRL1_PROPSEG_SHIFT (0U)
#define CAN_CTRL1_PROPSEG(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PROPSEG_SHIFT)) &                \
     CAN_CTRL1_PROPSEG_MASK)
#define CAN_CTRL1_LOM_MASK (0x8U)
#define CAN_CTRL1_LOM_SHIFT (3U)
#define CAN_CTRL1_LOM(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_LOM_SHIFT)) & CAN_CTRL1_LOM_MASK)
#define CAN_CTRL1_LBUF_MASK (0x10U)
#define CAN_CTRL1_LBUF_SHIFT (4U)
#define CAN_CTRL1_LBUF(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_LBUF_SHIFT)) &                   \
     CAN_CTRL1_LBUF_MASK)
#define CAN_CTRL1_TSYN_MASK (0x20U)
#define CAN_CTRL1_TSYN_SHIFT (5U)
#define CAN_CTRL1_TSYN(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_TSYN_SHIFT)) &                   \
     CAN_CTRL1_TSYN_MASK)
#define CAN_CTRL1_BOFFREC_MASK (0x40U)
#define CAN_CTRL1_BOFFREC_SHIFT (6U)
#define CAN_CTRL1_BOFFREC(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_BOFFREC_SHIFT)) &                \
     CAN_CTRL1_BOFFREC_MASK)
#define CAN_CTRL1_SMP_MASK (0x80U)
#define CAN_CTRL1_SMP_SHIFT (7U)
#define CAN_CTRL1_SMP(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_SMP_SHIFT)) & CAN_CTRL1_SMP_MASK)
#define CAN_CTRL1_RWRNMSK_MASK (0x400U)
#define CAN_CTRL1_RWRNMSK_SHIFT (10U)
#define CAN_CTRL1_RWRNMSK(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_RWRNMSK_SHIFT)) &                \
     CAN_CTRL1_RWRNMSK_MASK)
#define CAN_CTRL1_TWRNMSK_MASK (0x800U)
#define CAN_CTRL1_TWRNMSK_SHIFT (11U)
#define CAN_CTRL1_TWRNMSK(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_TWRNMSK_SHIFT)) &                \
     CAN_CTRL1_TWRNMSK_MASK)
#define CAN_CTRL1_LPB_MASK (0x1000U)
#define CAN_CTRL1_LPB_SHIFT (12U)
#define CAN_CTRL1_LPB(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_LPB_SHIFT)) & CAN_CTRL1_LPB_MASK)
#define CAN_CTRL1_CLKSRC_MASK (0x2000U)
#define CAN_CTRL1_CLKSRC_SHIFT (13U)
#define CAN_CTRL1_CLKSRC(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_CLKSRC_SHIFT)) &                 \
     CAN_CTRL1_CLKSRC_MASK)
#define CAN_CTRL1_ERRMSK_MASK (0x4000U)
#define CAN_CTRL1_ERRMSK_SHIFT (14U)
#define CAN_CTRL1_ERRMSK(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_ERRMSK_SHIFT)) &                 \
     CAN_CTRL1_ERRMSK_MASK)
#define CAN_CTRL1_BOFFMSK_MASK (0x8000U)
#define CAN_CTRL1_BOFFMSK_SHIFT (15U)
#define CAN_CTRL1_BOFFMSK(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_BOFFMSK_SHIFT)) &                \
     CAN_CTRL1_BOFFMSK_MASK)
#define CAN_CTRL1_PSEG2_MASK (0x70000U)
#define CAN_CTRL1_PSEG2_SHIFT (16U)
#define CAN_CTRL1_PSEG2(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PSEG2_SHIFT)) &                  \
     CAN_CTRL1_PSEG2_MASK)
#define CAN_CTRL1_PSEG1_MASK (0x380000U)
#define CAN_CTRL1_PSEG1_SHIFT (19U)
#define CAN_CTRL1_PSEG1(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PSEG1_SHIFT)) &                  \
     CAN_CTRL1_PSEG1_MASK)
#define CAN_CTRL1_RJW_MASK (0xC00000U)
#define CAN_CTRL1_RJW_SHIFT (22U)
#define CAN_CTRL1_RJW(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_RJW_SHIFT)) & CAN_CTRL1_RJW_MASK)
#define CAN_CTRL1_PRESDIV_MASK (0xFF000000U)
#define CAN_CTRL1_PRESDIV_SHIFT (24U)
#define CAN_CTRL1_PRESDIV(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PRESDIV_SHIFT)) &                \
     CAN_CTRL1_PRESDIV_MASK)

/** @name TIMER - Free Running Timer */
#define CAN_TIMER_TIMER_MASK (0xFFFFU)
#define CAN_TIMER_TIMER_SHIFT (0U)
#define CAN_TIMER_TIMER(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_TIMER_TIMER_SHIFT)) &                  \
     CAN_TIMER_TIMER_MASK)

/** @name RXMGMASK - Rx Mailboxes Global Mask Register */
#define CAN_RXMGMASK_MG_MASK (0xFFFFFFFFU)
#define CAN_RXMGMASK_MG_SHIFT (0U)
#define CAN_RXMGMASK_MG(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_RXMGMASK_MG_SHIFT)) &                  \
     CAN_RXMGMASK_MG_MASK)

/** @name RX14MASK - Rx 14 Mask register */
#define CAN_RX14MASK_RX14M_MASK (0xFFFFFFFFU)
#define CAN_RX14MASK_RX14M_SHIFT (0U)
#define CAN_RX14MASK_RX14M(x)                                                  \
    (((uint32_t)(((uint32_t)(x)) << CAN_RX14MASK_RX14M_SHIFT)) &               \
     CAN_RX14MASK_RX14M_MASK)

/** @name RX15MASK - Rx 15 Mask register */
#define CAN_RX15MASK_RX15M_MASK (0xFFFFFFFFU)
#define CAN_RX15MASK_RX15M_SHIFT (0U)
#define CAN_RX15MASK_RX15M(x)                                                  \
    (((uint32_t)(((uint32_t)(x)) << CAN_RX15MASK_RX15M_SHIFT)) &               \
     CAN_RX15MASK_RX15M_MASK)

/** @name ECR - Error Counter */
#define CAN_ECR_TXERRCNT_MASK (0xFFU)
#define CAN_ECR_TXERRCNT_SHIFT (0U)
#define CAN_ECR_TXERRCNT(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_ECR_TXERRCNT_SHIFT)) &                 \
     CAN_ECR_TXERRCNT_MASK)
#define CAN_ECR_RXERRCNT_MASK (0xFF00U)
#define CAN_ECR_RXERRCNT_SHIFT (8U)
#define CAN_ECR_RXERRCNT(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_ECR_RXERRCNT_SHIFT)) &                 \
     CAN_ECR_RXERRCNT_MASK)

/** @name ESR1 - Error and Status 1 register */
#define CAN_ESR1_WAKINT_MASK (0x1U)
#define CAN_ESR1_WAKINT_SHIFT (0U)
#define CAN_ESR1_WAKINT(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_WAKINT_SHIFT)) &                  \
     CAN_ESR1_WAKINT_MASK)
#define CAN_ESR1_ERRINT_MASK (0x2U)
#define CAN_ESR1_ERRINT_SHIFT (1U)
#define CAN_ESR1_ERRINT(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_ERRINT_SHIFT)) &                  \
     CAN_ESR1_ERRINT_MASK)
#define CAN_ESR1_BOFFINT_MASK (0x4U)
#define CAN_ESR1_BOFFINT_SHIFT (2U)
#define CAN_ESR1_BOFFINT(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BOFFINT_SHIFT)) &                 \
     CAN_ESR1_BOFFINT_MASK)
#define CAN_ESR1_RX_MASK (0x8U)
#define CAN_ESR1_RX_SHIFT (3U)
#define CAN_ESR1_RX(x)                                                         \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_RX_SHIFT)) & CAN_ESR1_RX_MASK)
#define CAN_ESR1_FLTCONF_MASK (0x30U)
#define CAN_ESR1_FLTCONF_SHIFT (4U)
#define CAN_ESR1_FLTCONF(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_FLTCONF_SHIFT)) &                 \
     CAN_ESR1_FLTCONF_MASK)
#define CAN_ESR1_TX_MASK (0x40U)
#define CAN_ESR1_TX_SHIFT (6U)
#define CAN_ESR1_TX(x)                                                         \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_TX_SHIFT)) & CAN_ESR1_TX_MASK)
#define CAN_ESR1_IDLE_MASK (0x80U)
#define CAN_ESR1_IDLE_SHIFT (7U)
#define CAN_ESR1_IDLE(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_IDLE_SHIFT)) & CAN_ESR1_IDLE_MASK)
#define CAN_ESR1_RXWRN_MASK (0x100U)
#define CAN_ESR1_RXWRN_SHIFT (8U)
#define CAN_ESR1_RXWRN(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_RXWRN_SHIFT)) &                   \
     CAN_ESR1_RXWRN_MASK)
#define CAN_ESR1_TXWRN_MASK (0x200U)
#define CAN_ESR1_TXWRN_SHIFT (9U)
#define CAN_ESR1_TXWRN(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_TXWRN_SHIFT)) &                   \
     CAN_ESR1_TXWRN_MASK)
#define CAN_ESR1_STFERR_MASK (0x400U)
#define CAN_ESR1_STFERR_SHIFT (10U)
#define CAN_ESR1_STFERR(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_STFERR_SHIFT)) &                  \
     CAN_ESR1_STFERR_MASK)
#define CAN_ESR1_FRMERR_MASK (0x800U)
#define CAN_ESR1_FRMERR_SHIFT (11U)
#define CAN_ESR1_FRMERR(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_FRMERR_SHIFT)) &                  \
     CAN_ESR1_FRMERR_MASK)
#define CAN_ESR1_CRCERR_MASK (0x1000U)
#define CAN_ESR1_CRCERR_SHIFT (12U)
#define CAN_ESR1_CRCERR(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_CRCERR_SHIFT)) &                  \
     CAN_ESR1_CRCERR_MASK)
#define CAN_ESR1_ACKERR_MASK (0x2000U)
#define CAN_ESR1_ACKERR_SHIFT (13U)
#define CAN_ESR1_ACKERR(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_ACKERR_SHIFT)) &                  \
     CAN_ESR1_ACKERR_MASK)
#define CAN_ESR1_BIT0ERR_MASK (0x4000U)
#define CAN_ESR1_BIT0ERR_SHIFT (14U)
#define CAN_ESR1_BIT0ERR(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BIT0ERR_SHIFT)) &                 \
     CAN_ESR1_BIT0ERR_MASK)
#define CAN_ESR1_BIT1ERR_MASK (0x8000U)
#define CAN_ESR1_BIT1ERR_SHIFT (15U)
#define CAN_ESR1_BIT1ERR(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BIT1ERR_SHIFT)) &                 \
     CAN_ESR1_BIT1ERR_MASK)
#define CAN_ESR1_RWRNINT_MASK (0x10000U)
#define CAN_ESR1_RWRNINT_SHIFT (16U)
#define CAN_ESR1_RWRNINT(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_RWRNINT_SHIFT)) &                 \
     CAN_ESR1_RWRNINT_MASK)
#define CAN_ESR1_TWRNINT_MASK (0x20000U)
#define CAN_ESR1_TWRNINT_SHIFT (17U)
#define CAN_ESR1_TWRNINT(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_TWRNINT_SHIFT)) &                 \
     CAN_ESR1_TWRNINT_MASK)
#define CAN_ESR1_SYNCH_MASK (0x40000U)
#define CAN_ESR1_SYNCH_SHIFT (18U)
#define CAN_ESR1_SYNCH(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_SYNCH_SHIFT)) &                   \
     CAN_ESR1_SYNCH_MASK)
#define CAN_ESR1_BOFFDONEINT_MASK (0x80000U)
#define CAN_ESR1_BOFFDONEINT_SHIFT (19U)
#define CAN_ESR1_BOFFDONEINT(x)                                                \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_BOFFDONEINT_SHIFT)) &             \
     CAN_ESR1_BOFFDONEINT_MASK)
#define CAN_ESR1_ERRINT_FAST_MASK (0x100000U)
#define CAN_ESR1_ERRINT_FAST_SHIFT (20U)
#define CAN_ESR1_ERRINT_FAST(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_ERRINT_FAST_SHIFT)) &                  \
     CAN_ESR1_ERRINT_FAST_MASK)
#define CAN_ESR1_ERROVR_MASK (0x200000U)
#define CAN_ESR1_ERROVR_SHIFT (21U)
#define CAN_ESR1_ERROVR(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR1_ERROVR_SHIFT)) &                  \
     CAN_ESR1_ERROVR_MASK)

/** @name IMASK1 - Interrupt Masks 1 register */
#define CAN_IMASK1_BUF31TO0M_MASK (0xFFFFFFFFU)
#define CAN_IMASK1_BUF31TO0M_SHIFT (0U)
#define CAN_IMASK1_BUF31TO0M(x)                                                \
    (((uint32_t)(((uint32_t)(x)) << CAN_IMASK1_BUF31TO0M_SHIFT)) &             \
     CAN_IMASK1_BUF31TO0M_MASK)

/** @name IFLAG1 - Interrupt Flags 1 register */
#define CAN_IFLAG1_BUF0I_MASK (0x1U)
#define CAN_IFLAG1_BUF0I_SHIFT (0U)
#define CAN_IFLAG1_BUF0I(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF0I_SHIFT)) &                 \
     CAN_IFLAG1_BUF0I_MASK)
#define CAN_IFLAG1_BUF4TO1I_MASK (0x1EU)
#define CAN_IFLAG1_BUF4TO1I_SHIFT (1U)
#define CAN_IFLAG1_BUF4TO1I(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF4TO1I_SHIFT)) &              \
     CAN_IFLAG1_BUF4TO1I_MASK)
#define CAN_IFLAG1_BUF5I_MASK (0x20U)
#define CAN_IFLAG1_BUF5I_SHIFT (5U)
#define CAN_IFLAG1_BUF5I(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF5I_SHIFT)) &                 \
     CAN_IFLAG1_BUF5I_MASK)
#define CAN_IFLAG1_BUF6I_MASK (0x40U)
#define CAN_IFLAG1_BUF6I_SHIFT (6U)
#define CAN_IFLAG1_BUF6I(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF6I_SHIFT)) &                 \
     CAN_IFLAG1_BUF6I_MASK)
#define CAN_IFLAG1_BUF7I_MASK (0x80U)
#define CAN_IFLAG1_BUF7I_SHIFT (7U)
#define CAN_IFLAG1_BUF7I(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF7I_SHIFT)) &                 \
     CAN_IFLAG1_BUF7I_MASK)
#define CAN_IFLAG1_BUF31TO8I_MASK (0xFFFFFF00U)
#define CAN_IFLAG1_BUF31TO8I_SHIFT (8U)
#define CAN_IFLAG1_BUF31TO8I(x)                                                \
    (((uint32_t)(((uint32_t)(x)) << CAN_IFLAG1_BUF31TO8I_SHIFT)) &             \
     CAN_IFLAG1_BUF31TO8I_MASK)

/** @name CTRL2 - Control 2 register */
#define CAN_CTRL2_EDFLTDIS_MASK (0x800U)
#define CAN_CTRL2_EDFLTDIS_SHIFT (11U)
#define CAN_CTRL2_EDFLTDIS(x)                                                  \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_EDFLTDIS_SHIFT)) &               \
     CAN_CTRL2_EDFLTDIS_MASK)
#define CAN_CTRL2_ISOCANFDEN_MASK (0x1000U)
#define CAN_CTRL2_ISOCANFDEN_SHIFT (12U)
#define CAN_CTRL2_ISOCANFDEN(x)                                                \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_ISOCANFDEN_SHIFT)) &             \
     CAN_CTRL2_ISOCANFDEN_MASK)
#define CAN_CTRL2_PREXCEN_MASK (0x4000U)
#define CAN_CTRL2_PREXCEN_SHIFT (14U)
#define CAN_CTRL2_PREXCEN(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_PREXCEN_SHIFT)) &                \
     CAN_CTRL2_PREXCEN_MASK)
#define CAN_CTRL2_TIMER_SRC_MASK (0x8000U)
#define CAN_CTRL2_TIMER_SRC_SHIFT (15U)
#define CAN_CTRL2_TIMER_SRC(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_TIMER_SRC_SHIFT)) &              \
     CAN_CTRL2_TIMER_SRC_MASK)
#define CAN_CTRL2_EACEN_MASK (0x10000U)
#define CAN_CTRL2_EACEN_SHIFT (16U)
#define CAN_CTRL2_EACEN(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_EACEN_SHIFT)) &                  \
     CAN_CTRL2_EACEN_MASK)
#define CAN_CTRL2_RRS_MASK (0x20000U)
#define CAN_CTRL2_RRS_SHIFT (17U)
#define CAN_CTRL2_RRS(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_RRS_SHIFT)) & CAN_CTRL2_RRS_MASK)
#define CAN_CTRL2_MRP_MASK (0x40000U)
#define CAN_CTRL2_MRP_SHIFT (18U)
#define CAN_CTRL2_MRP(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_MRP_SHIFT)) & CAN_CTRL2_MRP_MASK)
#define CAN_CTRL2_TASD_MASK (0xF80000U)
#define CAN_CTRL2_TASD_SHIFT (19U)
#define CAN_CTRL2_TASD(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_TASD_SHIFT)) &                   \
     CAN_CTRL2_TASD_MASK)
#define CAN_CTRL2_RFFN_MASK (0xF000000U)
#define CAN_CTRL2_RFFN_SHIFT (24U)
#define CAN_CTRL2_RFFN(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_RFFN_SHIFT)) &                   \
     CAN_CTRL2_RFFN_MASK)
#define CAN_CTRL2_WRMFRZ_MASK (0x10000000U)
#define CAN_CTRL2_WRMFRZ_SHIFT (28U)
#define CAN_CTRL2_WRMFRZ(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_WRMFRZ_SHIFT)) &                 \
     CAN_CTRL2_WRMFRZ_MASK)
#define CAN_CTRL2_ECRWRE_MASK (0x20000000U)
#define CAN_CTRL2_ECRWRE_SHIFT (29U)
#define CAN_CTRL2_ECRWRE(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_ECRWRE_SHIFT)) &                 \
     CAN_CTRL2_ECRWRE_MASK)
#define CAN_CTRL2_BOFFDONEMSK_MASK (0x40000000U)
#define CAN_CTRL2_BOFFDONEMSK_SHIFT (30U)
#define CAN_CTRL2_BOFFDONEMSK(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_BOFFDONEMSK_SHIFT)) &            \
     CAN_CTRL2_BOFFDONEMSK_MASK)
#define CAN_CTRL2_ERRMSK_FAST_MASK (0x80000000U)
#define CAN_CTRL2_ERRMSK_FAST_SHIFT (31U)
#define CAN_CTRL2_ERRMSK_FAST(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_ERRMSK_FAST_SHIFT)) &            \
     CAN_CTRL2_ERRMSK_FAST_MASK)

/** @name ESR2 - Error and Status 2 register */
#define CAN_ESR2_IMB_MASK (0x2000U)
#define CAN_ESR2_IMB_SHIFT (13U)
#define CAN_ESR2_IMB(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR2_IMB_SHIFT)) & CAN_ESR2_IMB_MASK)
#define CAN_ESR2_VPS_MASK (0x4000U)
#define CAN_ESR2_VPS_SHIFT (14U)
#define CAN_ESR2_VPS(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR2_VPS_SHIFT)) & CAN_ESR2_VPS_MASK)
#define CAN_ESR2_LPTM_MASK (0x7F0000U)
#define CAN_ESR2_LPTM_SHIFT (16U)
#define CAN_ESR2_LPTM(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR2_LPTM_SHIFT)) & CAN_ESR2_LPTM_MASK)
///
#define CAN_ESR2_HTWAINT_STATE_MASK (0x10000000U)
#define CAN_ESR2_HTWAINT_STATE_SHIFT (28U)
#define CAN_ESR2_HTWAINT_STATE(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR2_HTWAINT_STATE_SHIFT)) & CAN_ESR2_HTWAINT_STATE_MASK)
///
#define CAN_ESR2_HTWAINT_SET_MASK (0x1000U)
#define CAN_ESR2_HTWAINT_SET_SHIFT (12U)
#define CAN_ESR2_HTWAINT_SET(x)                                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_ESR2_HTWAINT_SET_SHIFT)) & CAN_ESR2_HTWAINT_SET_MASK)

/** @name CRCR - CRC Register */
#define CAN_CRCR_TXCRC_MASK (0x7FFFU)
#define CAN_CRCR_TXCRC_SHIFT (0U)
#define CAN_CRCR_TXCRC(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_CRCR_TXCRC_SHIFT)) &                   \
     CAN_CRCR_TXCRC_MASK)
#define CAN_CRCR_MBCRC_MASK (0x7F0000U)
#define CAN_CRCR_MBCRC_SHIFT (16U)
#define CAN_CRCR_MBCRC(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_CRCR_MBCRC_SHIFT)) &                   \
     CAN_CRCR_MBCRC_MASK)

/** @name RXFGMASK - Rx FIFO Global Mask register */
#define CAN_RXFGMASK_FGM_MASK (0xFFFFFFFFU)
#define CAN_RXFGMASK_FGM_SHIFT (0U)
#define CAN_RXFGMASK_FGM(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_RXFGMASK_FGM_SHIFT)) &                 \
     CAN_RXFGMASK_FGM_MASK)

/** @name RXFIR - Rx FIFO Information Register */
#define CAN_RXFIR_IDHIT_MASK (0x1FFU)
#define CAN_RXFIR_IDHIT_SHIFT (0U)
#define CAN_RXFIR_IDHIT(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_RXFIR_IDHIT_SHIFT)) &                  \
     CAN_RXFIR_IDHIT_MASK)

/** @name CBT - CAN Bit Timing Register */
#define CAN_CBT_EPSEG2_MASK (0x1FU)
#define CAN_CBT_EPSEG2_SHIFT (0U)
#define CAN_CBT_EPSEG2(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_CBT_EPSEG2_SHIFT)) &                   \
     CAN_CBT_EPSEG2_MASK)
#define CAN_CBT_EPSEG1_MASK (0x3E0U)
#define CAN_CBT_EPSEG1_SHIFT (5U)
#define CAN_CBT_EPSEG1(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_CBT_EPSEG1_SHIFT)) &                   \
     CAN_CBT_EPSEG1_MASK)
#define CAN_CBT_EPROPSEG_MASK (0xFC00U)
#define CAN_CBT_EPROPSEG_SHIFT (10U)
#define CAN_CBT_EPROPSEG(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_CBT_EPROPSEG_SHIFT)) &                 \
     CAN_CBT_EPROPSEG_MASK)
#define CAN_CBT_ERJW_MASK (0x1F0000U)
#define CAN_CBT_ERJW_SHIFT (16U)
#define CAN_CBT_ERJW(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_CBT_ERJW_SHIFT)) & CAN_CBT_ERJW_MASK)
#define CAN_CBT_EPRESDIV_MASK (0x7FE00000U)
#define CAN_CBT_EPRESDIV_SHIFT (21U)
#define CAN_CBT_EPRESDIV(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_CBT_EPRESDIV_SHIFT)) &                 \
     CAN_CBT_EPRESDIV_MASK)
#define CAN_CBT_BTF_MASK (0x80000000U)
#define CAN_CBT_BTF_SHIFT (31U)
#define CAN_CBT_BTF(x)                                                         \
    (((uint32_t)(((uint32_t)(x)) << CAN_CBT_BTF_SHIFT)) & CAN_CBT_BTF_MASK)

/** @name MECR - Memory Error Control Register */
#define CAN_MECR_NCEFAFRZ_MASK (0x80U)
#define CAN_MECR_NCEFAFRZ_SHIFT (7U)
#define CAN_MECR_NCEFAFRZ(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_NCEFAFRZ_SHIFT)) &                \
     CAN_MECR_NCEFAFRZ_MASK)
#define CAN_MECR_ECCDIS_MASK (0x100U)
#define CAN_MECR_ECCDIS_SHIFT (8U)
#define CAN_MECR_ECCDIS(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_ECCDIS_SHIFT)) &                  \
     CAN_MECR_ECCDIS_MASK)
#define CAN_MECR_RERRDIS_MASK (0x200U)
#define CAN_MECR_RERRDIS_SHIFT (9U)
#define CAN_MECR_RERRDIS(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_RERRDIS_SHIFT)) &                 \
     CAN_MECR_RERRDIS_MASK)
#define CAN_MECR_EXTERRIE_MASK (0x2000U)
#define CAN_MECR_EXTERRIE_SHIFT (13U)
#define CAN_MECR_EXTERRIE(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_EXTERRIE_SHIFT)) &                \
     CAN_MECR_EXTERRIE_MASK)
#define CAN_MECR_FAERRIE_MASK (0x4000U)
#define CAN_MECR_FAERRIE_SHIFT (14U)
#define CAN_MECR_FAERRIE(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_FAERRIE_SHIFT)) &                 \
     CAN_MECR_FAERRIE_MASK)
#define CAN_MECR_HAERRIE_MASK (0x8000U)
#define CAN_MECR_HAERRIE_SHIFT (15U)
#define CAN_MECR_HAERRIE(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_HAERRIE_SHIFT)) &                 \
     CAN_MECR_HAERRIE_MASK)
#define CAN_MECR_CEI_MSK_MASK (0x10000U)
#define CAN_MECR_CEI_MSK_SHIFT (16U)
#define CAN_MECR_CEI_MSK(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_CEI_MSK_SHIFT)) &                 \
     CAN_MECR_CEI_MSK_MASK)
#define CAN_MECR_FANCEI_MSK_MASK (0x40000U)
#define CAN_MECR_FANCEI_MSK_SHIFT (18U)
#define CAN_MECR_FANCEI_MSK(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_FANCEI_MSK_SHIFT)) &              \
     CAN_MECR_FANCEI_MSK_MASK)
#define CAN_MECR_HANCEI_MSK_MASK (0x80000U)
#define CAN_MECR_HANCEI_MSK_SHIFT (19U)
#define CAN_MECR_HANCEI_MSK(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_HANCEI_MSK_SHIFT)) &              \
     CAN_MECR_HANCEI_MSK_MASK)
#define CAN_MECR_ECRWRDIS_MASK (0x80000000U)
#define CAN_MECR_ECRWRDIS_SHIFT (31U)
#define CAN_MECR_ECRWRDIS(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_MECR_ECRWRDIS_SHIFT)) &                \
     CAN_MECR_ECRWRDIS_MASK)

/** @name CS - Message Buffer CS Register */
#define CAN_CS_TIME_STAMP_MASK (0xFFFFU)
#define CAN_CS_TIME_STAMP_SHIFT (0U)
#define CAN_CS_TIME_STAMP(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CS_TIME_STAMP_SHIFT)) &                \
     CAN_CS_TIME_STAMP_MASK)
#define CAN_CS_DLC_MASK (0xF0000U)
#define CAN_CS_DLC_SHIFT (16U)
#define CAN_CS_DLC(x)                                                          \
    (((uint32_t)(((uint32_t)(x)) << CAN_CS_DLC_SHIFT)) & CAN_CS_DLC_MASK)
#define CAN_CS_RTR_MASK (0x100000U)
#define CAN_CS_RTR_SHIFT (20U)
#define CAN_CS_RTR(x)                                                          \
    (((uint32_t)(((uint32_t)(x)) << CAN_CS_RTR_SHIFT)) & CAN_CS_RTR_MASK)
#define CAN_CS_IDE_MASK (0x200000U)
#define CAN_CS_IDE_SHIFT (21U)
#define CAN_CS_IDE(x)                                                          \
    (((uint32_t)(((uint32_t)(x)) << CAN_CS_IDE_SHIFT)) & CAN_CS_IDE_MASK)
#define CAN_CS_SRR_MASK (0x400000U)
#define CAN_CS_SRR_SHIFT (22U)
#define CAN_CS_SRR(x)                                                          \
    (((uint32_t)(((uint32_t)(x)) << CAN_CS_SRR_SHIFT)) & CAN_CS_SRR_MASK)
#define CAN_CS_CODE_MASK (0xF000000U)
#define CAN_CS_CODE_SHIFT (24U)
#define CAN_CS_CODE(x)                                                         \
    (((uint32_t)(((uint32_t)(x)) << CAN_CS_CODE_SHIFT)) & CAN_CS_CODE_MASK)
#define CAN_CS_ESI_MASK (0x20000000U)
#define CAN_CS_ESI_SHIFT (29U)
#define CAN_CS_ESI(x)                            ((uint32_t)(((uint32_t)(x)) << CAN_CS_ESI_SHIFT)) & CAN_CS_ESI_MASK)
#define CAN_CS_BRS_MASK (0x40000000U)
#define CAN_CS_BRS_SHIFT (30U)
#define CAN_CS_BRS(x)                            ((uint32_t)(((uint32_t)(x)) << CAN_CS_BRS_SHIFT)) & CAN_CS_BRS_MASK)
#define CAN_CS_EDL_MASK (0x80000000U)
#define CAN_CS_EDL_SHIFT (31U)
#define CAN_CS_EDL(x)                            ((uint32_t)(((uint32_t)(x)) << CAN_CS_EDL_SHIFT)) & CAN_CS_EDL_MASK)

/* The count of CAN_CS */
#define CAN_CS_COUNT (128U)

/** @name ID - Message Buffer 0 ID Register..Message Buffer 15 ID Register */
#define CAN_ID_EXT_MASK (0x3FFFFU)
#define CAN_ID_EXT_SHIFT (0U)
#define CAN_ID_EXT(x)                                                          \
    (((uint32_t)(((uint32_t)(x)) << CAN_ID_EXT_SHIFT)) & CAN_ID_EXT_MASK)
#define CAN_ID_STD_MASK (0x1FFC0000U)
#define CAN_ID_STD_SHIFT (18U)
#define CAN_ID_STD(x)                                                          \
    (((uint32_t)(((uint32_t)(x)) << CAN_ID_STD_SHIFT)) & CAN_ID_STD_MASK)
#define CAN_ID_PRIO_MASK (0xE0000000U)
#define CAN_ID_PRIO_SHIFT (29U)
#define CAN_ID_PRIO(x)                                                         \
    (((uint32_t)(((uint32_t)(x)) << CAN_ID_PRIO_SHIFT)) & CAN_ID_PRIO_MASK)

/* The count of CAN_ID */
#define CAN_ID_COUNT (128U)

/** @name WORD0 - Message Buffer 0 WORD0 Register..Message Buffer 15 WORD0
 * Register */
#define CAN_WORD0_DATA_BYTE_3_MASK (0xFFU)
#define CAN_WORD0_DATA_BYTE_3_SHIFT (0U)
#define CAN_WORD0_DATA_BYTE_3(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_WORD0_DATA_BYTE_3_SHIFT)) &            \
     CAN_WORD0_DATA_BYTE_3_MASK)
#define CAN_WORD0_DATA_BYTE_2_MASK (0xFF00U)
#define CAN_WORD0_DATA_BYTE_2_SHIFT (8U)
#define CAN_WORD0_DATA_BYTE_2(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_WORD0_DATA_BYTE_2_SHIFT)) &            \
     CAN_WORD0_DATA_BYTE_2_MASK)
#define CAN_WORD0_DATA_BYTE_1_MASK (0xFF0000U)
#define CAN_WORD0_DATA_BYTE_1_SHIFT (16U)
#define CAN_WORD0_DATA_BYTE_1(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_WORD0_DATA_BYTE_1_SHIFT)) &            \
     CAN_WORD0_DATA_BYTE_1_MASK)
#define CAN_WORD0_DATA_BYTE_0_MASK (0xFF000000U)
#define CAN_WORD0_DATA_BYTE_0_SHIFT (24U)
#define CAN_WORD0_DATA_BYTE_0(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_WORD0_DATA_BYTE_0_SHIFT)) &            \
     CAN_WORD0_DATA_BYTE_0_MASK)

/* The count of CAN_WORD0 */
#define CAN_WORD0_COUNT (128U)

/** @name WORD1 - Message Buffer 0 WORD1 Register..Message Buffer 15 WORD1
 * Register */
#define CAN_WORD1_DATA_BYTE_7_MASK (0xFFU)
#define CAN_WORD1_DATA_BYTE_7_SHIFT (0U)
#define CAN_WORD1_DATA_BYTE_7(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_WORD1_DATA_BYTE_7_SHIFT)) &            \
     CAN_WORD1_DATA_BYTE_7_MASK)
#define CAN_WORD1_DATA_BYTE_6_MASK (0xFF00U)
#define CAN_WORD1_DATA_BYTE_6_SHIFT (8U)
#define CAN_WORD1_DATA_BYTE_6(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_WORD1_DATA_BYTE_6_SHIFT)) &            \
     CAN_WORD1_DATA_BYTE_6_MASK)
#define CAN_WORD1_DATA_BYTE_5_MASK (0xFF0000U)
#define CAN_WORD1_DATA_BYTE_5_SHIFT (16U)
#define CAN_WORD1_DATA_BYTE_5(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_WORD1_DATA_BYTE_5_SHIFT)) &            \
     CAN_WORD1_DATA_BYTE_5_MASK)
#define CAN_WORD1_DATA_BYTE_4_MASK (0xFF000000U)
#define CAN_WORD1_DATA_BYTE_4_SHIFT (24U)
#define CAN_WORD1_DATA_BYTE_4(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_WORD1_DATA_BYTE_4_SHIFT)) &            \
     CAN_WORD1_DATA_BYTE_4_MASK)

/* The count of CAN_WORD1 */
#define CAN_WORD1_COUNT (128U)

/** @name RXIMR - Rx Individual Mask Registers */
#define CAN_RXIMR_MI_MASK (0xFFFFFFFFU)
#define CAN_RXIMR_MI_SHIFT (0U)
#define CAN_RXIMR_MI(x)                                                        \
    (((uint32_t)(((uint32_t)(x)) << CAN_RXIMR_MI_SHIFT)) & CAN_RXIMR_MI_MASK)

/* The count of CAN_RXIMR */
#define CAN_RXIMR_COUNT (128U)

/** @name CTRL1_PN - Pretended Networking Control 1 Register */
#define CAN_CTRL1_PN_FCS_MASK (0x3U)
#define CAN_CTRL1_PN_FCS_SHIFT (0U)
#define CAN_CTRL1_PN_FCS(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PN_FCS_SHIFT)) &                 \
     CAN_CTRL1_PN_FCS_MASK)
#define CAN_CTRL1_PN_IDFS_MASK (0xCU)
#define CAN_CTRL1_PN_IDFS_SHIFT (2U)
#define CAN_CTRL1_PN_IDFS(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PN_IDFS_SHIFT)) &                \
     CAN_CTRL1_PN_IDFS_MASK)
#define CAN_CTRL1_PN_PLFS_MASK (0x30U)
#define CAN_CTRL1_PN_PLFS_SHIFT (4U)
#define CAN_CTRL1_PN_PLFS(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PN_PLFS_SHIFT)) &                \
     CAN_CTRL1_PN_PLFS_MASK)
#define CAN_CTRL1_PN_NMATCH_MASK (0xFF00U)
#define CAN_CTRL1_PN_NMATCH_SHIFT (8U)
#define CAN_CTRL1_PN_NMATCH(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PN_NMATCH_SHIFT)) &              \
     CAN_CTRL1_PN_NMATCH_MASK)
#define CAN_CTRL1_PN_WUMF_MSK_MASK (0x10000U)
#define CAN_CTRL1_PN_WUMF_MSK_SHIFT (16U)
#define CAN_CTRL1_PN_WUMF_MSK(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PN_WUMF_MSK_SHIFT)) &            \
     CAN_CTRL1_PN_WUMF_MSK_MASK)
#define CAN_CTRL1_PN_WTOF_MSK_MASK (0x20000U)
#define CAN_CTRL1_PN_WTOF_MSK_SHIFT (17U)
#define CAN_CTRL1_PN_WTOF_MSK(x)                                               \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL1_PN_WTOF_MSK_SHIFT)) &            \
     CAN_CTRL1_PN_WTOF_MSK_MASK)

/** @name CTRL2_PN - Pretended Networking Control 2 Register */
#define CAN_CTRL2_PN_MATCHTO_MASK (0xFFFFU)
#define CAN_CTRL2_PN_MATCHTO_SHIFT (0U)
#define CAN_CTRL2_PN_MATCHTO(x)                                                \
    (((uint32_t)(((uint32_t)(x)) << CAN_CTRL2_PN_MATCHTO_SHIFT)) &             \
     CAN_CTRL2_PN_MATCHTO_MASK)

/** @name WU_MTC - Pretended Networking Wake Up Match Register */
#define CAN_WU_MTC_MCOUNTER_MASK (0xFF00U)
#define CAN_WU_MTC_MCOUNTER_SHIFT (8U)
#define CAN_WU_MTC_MCOUNTER(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_WU_MTC_MCOUNTER_SHIFT)) &              \
     CAN_WU_MTC_MCOUNTER_MASK)
#define CAN_WU_MTC_WUMF_MASK (0x10000U)
#define CAN_WU_MTC_WUMF_SHIFT (16U)
#define CAN_WU_MTC_WUMF(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_WU_MTC_WUMF_SHIFT)) &                  \
     CAN_WU_MTC_WUMF_MASK)
#define CAN_WU_MTC_WTOF_MASK (0x20000U)
#define CAN_WU_MTC_WTOF_SHIFT (17U)
#define CAN_WU_MTC_WTOF(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_WU_MTC_WTOF_SHIFT)) &                  \
     CAN_WU_MTC_WTOF_MASK)

/** @name FLT_ID1 - Pretended Networking ID Filter 1 Register */
#define CAN_FLT_ID1_FLT_ID1_MASK (0x1FFFFFFFU)
#define CAN_FLT_ID1_FLT_ID1_SHIFT (0U)
#define CAN_FLT_ID1_FLT_ID1(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_FLT_ID1_FLT_ID1_SHIFT)) &              \
     CAN_FLT_ID1_FLT_ID1_MASK)
#define CAN_FLT_ID1_FLT_RTR_MASK (0x20000000U)
#define CAN_FLT_ID1_FLT_RTR_SHIFT (29U)
#define CAN_FLT_ID1_FLT_RTR(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_FLT_ID1_FLT_RTR_SHIFT)) &              \
     CAN_FLT_ID1_FLT_RTR_MASK)
#define CAN_FLT_ID1_FLT_IDE_MASK (0x40000000U)
#define CAN_FLT_ID1_FLT_IDE_SHIFT (30U)
#define CAN_FLT_ID1_FLT_IDE(x)                                                 \
    (((uint32_t)(((uint32_t)(x)) << CAN_FLT_ID1_FLT_IDE_SHIFT)) &              \
     CAN_FLT_ID1_FLT_IDE_MASK)

/** @name FLT_DLC - Pretended Networking DLC Filter Register */
#define CAN_FLT_DLC_FLT_DLC_HI_MASK (0xFU)
#define CAN_FLT_DLC_FLT_DLC_HI_SHIFT (0U)
#define CAN_FLT_DLC_FLT_DLC_HI(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_FLT_DLC_FLT_DLC_HI_SHIFT)) &           \
     CAN_FLT_DLC_FLT_DLC_HI_MASK)
#define CAN_FLT_DLC_FLT_DLC_LO_MASK (0xF0000U)
#define CAN_FLT_DLC_FLT_DLC_LO_SHIFT (16U)
#define CAN_FLT_DLC_FLT_DLC_LO(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_FLT_DLC_FLT_DLC_LO_SHIFT)) &           \
     CAN_FLT_DLC_FLT_DLC_LO_MASK)

/** @name PL1_LO - Pretended Networking Payload Low Filter 1 Register */
#define CAN_PL1_LO_Data_byte_3_MASK (0xFFU)
#define CAN_PL1_LO_Data_byte_3_SHIFT (0U)
#define CAN_PL1_LO_Data_byte_3(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL1_LO_Data_byte_3_SHIFT)) &           \
     CAN_PL1_LO_Data_byte_3_MASK)
#define CAN_PL1_LO_Data_byte_2_MASK (0xFF00U)
#define CAN_PL1_LO_Data_byte_2_SHIFT (8U)
#define CAN_PL1_LO_Data_byte_2(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL1_LO_Data_byte_2_SHIFT)) &           \
     CAN_PL1_LO_Data_byte_2_MASK)
#define CAN_PL1_LO_Data_byte_1_MASK (0xFF0000U)
#define CAN_PL1_LO_Data_byte_1_SHIFT (16U)
#define CAN_PL1_LO_Data_byte_1(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL1_LO_Data_byte_1_SHIFT)) &           \
     CAN_PL1_LO_Data_byte_1_MASK)
#define CAN_PL1_LO_Data_byte_0_MASK (0xFF000000U)
#define CAN_PL1_LO_Data_byte_0_SHIFT (24U)
#define CAN_PL1_LO_Data_byte_0(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL1_LO_Data_byte_0_SHIFT)) &           \
     CAN_PL1_LO_Data_byte_0_MASK)

/** @name PL1_HI - Pretended Networking Payload High Filter 1 Register */
#define CAN_PL1_HI_Data_byte_7_MASK (0xFFU)
#define CAN_PL1_HI_Data_byte_7_SHIFT (0U)
#define CAN_PL1_HI_Data_byte_7(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL1_HI_Data_byte_7_SHIFT)) &           \
     CAN_PL1_HI_Data_byte_7_MASK)
#define CAN_PL1_HI_Data_byte_6_MASK (0xFF00U)
#define CAN_PL1_HI_Data_byte_6_SHIFT (8U)
#define CAN_PL1_HI_Data_byte_6(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL1_HI_Data_byte_6_SHIFT)) &           \
     CAN_PL1_HI_Data_byte_6_MASK)
#define CAN_PL1_HI_Data_byte_5_MASK (0xFF0000U)
#define CAN_PL1_HI_Data_byte_5_SHIFT (16U)
#define CAN_PL1_HI_Data_byte_5(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL1_HI_Data_byte_5_SHIFT)) &           \
     CAN_PL1_HI_Data_byte_5_MASK)
#define CAN_PL1_HI_Data_byte_4_MASK (0xFF000000U)
#define CAN_PL1_HI_Data_byte_4_SHIFT (24U)
#define CAN_PL1_HI_Data_byte_4(x)                                              \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL1_HI_Data_byte_4_SHIFT)) &           \
     CAN_PL1_HI_Data_byte_4_MASK)

/** @name FLT_ID2_IDMASK - Pretended Networking ID Filter 2 Register / ID Mask
 * Register */
#define CAN_FLT_ID2_IDMASK_FLT_ID2_IDMASK_MASK (0x1FFFFFFFU)
#define CAN_FLT_ID2_IDMASK_FLT_ID2_IDMASK_SHIFT (0U)
#define CAN_FLT_ID2_IDMASK_FLT_ID2_IDMASK(x)                                   \
    (((uint32_t)(((uint32_t)(x))                                               \
                 << CAN_FLT_ID2_IDMASK_FLT_ID2_IDMASK_SHIFT)) &                \
     CAN_FLT_ID2_IDMASK_FLT_ID2_IDMASK_MASK)
#define CAN_FLT_ID2_IDMASK_RTR_MSK_MASK (0x20000000U)
#define CAN_FLT_ID2_IDMASK_RTR_MSK_SHIFT (29U)
#define CAN_FLT_ID2_IDMASK_RTR_MSK(x)                                          \
    (((uint32_t)(((uint32_t)(x)) << CAN_FLT_ID2_IDMASK_RTR_MSK_SHIFT)) &       \
     CAN_FLT_ID2_IDMASK_RTR_MSK_MASK)
#define CAN_FLT_ID2_IDMASK_IDE_MSK_MASK (0x40000000U)
#define CAN_FLT_ID2_IDMASK_IDE_MSK_SHIFT (30U)
#define CAN_FLT_ID2_IDMASK_IDE_MSK(x)                                          \
    (((uint32_t)(((uint32_t)(x)) << CAN_FLT_ID2_IDMASK_IDE_MSK_SHIFT)) &       \
     CAN_FLT_ID2_IDMASK_IDE_MSK_MASK)

/** @name PL2_PLMASK_LO - Pretended Networking Payload Low Filter 2 Register /
 * Payload Low Mask Register */
#define CAN_PL2_PLMASK_LO_Data_byte_3_MASK (0xFFU)
#define CAN_PL2_PLMASK_LO_Data_byte_3_SHIFT (0U)
#define CAN_PL2_PLMASK_LO_Data_byte_3(x)                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL2_PLMASK_LO_Data_byte_3_SHIFT)) &    \
     CAN_PL2_PLMASK_LO_Data_byte_3_MASK)
#define CAN_PL2_PLMASK_LO_Data_byte_2_MASK (0xFF00U)
#define CAN_PL2_PLMASK_LO_Data_byte_2_SHIFT (8U)
#define CAN_PL2_PLMASK_LO_Data_byte_2(x)                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL2_PLMASK_LO_Data_byte_2_SHIFT)) &    \
     CAN_PL2_PLMASK_LO_Data_byte_2_MASK)
#define CAN_PL2_PLMASK_LO_Data_byte_1_MASK (0xFF0000U)
#define CAN_PL2_PLMASK_LO_Data_byte_1_SHIFT (16U)
#define CAN_PL2_PLMASK_LO_Data_byte_1(x)                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL2_PLMASK_LO_Data_byte_1_SHIFT)) &    \
     CAN_PL2_PLMASK_LO_Data_byte_1_MASK)
#define CAN_PL2_PLMASK_LO_Data_byte_0_MASK (0xFF000000U)
#define CAN_PL2_PLMASK_LO_Data_byte_0_SHIFT (24U)
#define CAN_PL2_PLMASK_LO_Data_byte_0(x)                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL2_PLMASK_LO_Data_byte_0_SHIFT)) &    \
     CAN_PL2_PLMASK_LO_Data_byte_0_MASK)

/** @name PL2_PLMASK_HI - Pretended Networking Payload High Filter 2 low order
 * bits / Payload High Mask Register */
#define CAN_PL2_PLMASK_HI_Data_byte_7_MASK (0xFFU)
#define CAN_PL2_PLMASK_HI_Data_byte_7_SHIFT (0U)
#define CAN_PL2_PLMASK_HI_Data_byte_7(x)                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL2_PLMASK_HI_Data_byte_7_SHIFT)) &    \
     CAN_PL2_PLMASK_HI_Data_byte_7_MASK)
#define CAN_PL2_PLMASK_HI_Data_byte_6_MASK (0xFF00U)
#define CAN_PL2_PLMASK_HI_Data_byte_6_SHIFT (8U)
#define CAN_PL2_PLMASK_HI_Data_byte_6(x)                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL2_PLMASK_HI_Data_byte_6_SHIFT)) &    \
     CAN_PL2_PLMASK_HI_Data_byte_6_MASK)
#define CAN_PL2_PLMASK_HI_Data_byte_5_MASK (0xFF0000U)
#define CAN_PL2_PLMASK_HI_Data_byte_5_SHIFT (16U)
#define CAN_PL2_PLMASK_HI_Data_byte_5(x)                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL2_PLMASK_HI_Data_byte_5_SHIFT)) &    \
     CAN_PL2_PLMASK_HI_Data_byte_5_MASK)
#define CAN_PL2_PLMASK_HI_Data_byte_4_MASK (0xFF000000U)
#define CAN_PL2_PLMASK_HI_Data_byte_4_SHIFT (24U)
#define CAN_PL2_PLMASK_HI_Data_byte_4(x)                                       \
    (((uint32_t)(((uint32_t)(x)) << CAN_PL2_PLMASK_HI_Data_byte_4_SHIFT)) &    \
     CAN_PL2_PLMASK_HI_Data_byte_4_MASK)

/** @name WMBn_CS - Wake Up Message Buffer Register for C/S */
#define CAN_WMBn_CS_DLC_MASK (0xF0000U)
#define CAN_WMBn_CS_DLC_SHIFT (16U)
#define CAN_WMBn_CS_DLC(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_CS_DLC_SHIFT)) &                  \
     CAN_WMBn_CS_DLC_MASK)
#define CAN_WMBn_CS_RTR_MASK (0x100000U)
#define CAN_WMBn_CS_RTR_SHIFT (20U)
#define CAN_WMBn_CS_RTR(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_CS_RTR_SHIFT)) &                  \
     CAN_WMBn_CS_RTR_MASK)
#define CAN_WMBn_CS_IDE_MASK (0x200000U)
#define CAN_WMBn_CS_IDE_SHIFT (21U)
#define CAN_WMBn_CS_IDE(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_CS_IDE_SHIFT)) &                  \
     CAN_WMBn_CS_IDE_MASK)
#define CAN_WMBn_CS_SRR_MASK (0x400000U)
#define CAN_WMBn_CS_SRR_SHIFT (22U)
#define CAN_WMBn_CS_SRR(x)                                                     \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_CS_SRR_SHIFT)) &                  \
     CAN_WMBn_CS_SRR_MASK)

/** @name WMBn_ID - Wake Up Message Buffer Register for ID */
#define CAN_WMBn_ID_ID_MASK (0x1FFFFFFFU)
#define CAN_WMBn_ID_ID_SHIFT (0U)
#define CAN_WMBn_ID_ID(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_ID_ID_SHIFT)) &                   \
     CAN_WMBn_ID_ID_MASK)

/** @name WMBn_D03 - Wake Up Message Buffer Register for Data 0-3 */
#define CAN_WMBn_D03_Data_byte_3_MASK (0xFFU)
#define CAN_WMBn_D03_Data_byte_3_SHIFT (0U)
#define CAN_WMBn_D03_Data_byte_3(x)                                            \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_D03_Data_byte_3_SHIFT)) &         \
     CAN_WMBn_D03_Data_byte_3_MASK)
#define CAN_WMBn_D03_Data_byte_2_MASK (0xFF00U)
#define CAN_WMBn_D03_Data_byte_2_SHIFT (8U)
#define CAN_WMBn_D03_Data_byte_2(x)                                            \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_D03_Data_byte_2_SHIFT)) &         \
     CAN_WMBn_D03_Data_byte_2_MASK)
#define CAN_WMBn_D03_Data_byte_1_MASK (0xFF0000U)
#define CAN_WMBn_D03_Data_byte_1_SHIFT (16U)
#define CAN_WMBn_D03_Data_byte_1(x)                                            \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_D03_Data_byte_1_SHIFT)) &         \
     CAN_WMBn_D03_Data_byte_1_MASK)
#define CAN_WMBn_D03_Data_byte_0_MASK (0xFF000000U)
#define CAN_WMBn_D03_Data_byte_0_SHIFT (24U)
#define CAN_WMBn_D03_Data_byte_0(x)                                            \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_D03_Data_byte_0_SHIFT)) &         \
     CAN_WMBn_D03_Data_byte_0_MASK)

/** @name WMBn_D47 - Wake Up Message Buffer Register Data 4-7 */
#define CAN_WMBn_D47_Data_byte_7_MASK (0xFFU)
#define CAN_WMBn_D47_Data_byte_7_SHIFT (0U)
#define CAN_WMBn_D47_Data_byte_7(x)                                            \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_D47_Data_byte_7_SHIFT)) &         \
     CAN_WMBn_D47_Data_byte_7_MASK)
#define CAN_WMBn_D47_Data_byte_6_MASK (0xFF00U)
#define CAN_WMBn_D47_Data_byte_6_SHIFT (8U)
#define CAN_WMBn_D47_Data_byte_6(x)                                            \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_D47_Data_byte_6_SHIFT)) &         \
     CAN_WMBn_D47_Data_byte_6_MASK)
#define CAN_WMBn_D47_Data_byte_5_MASK (0xFF0000U)
#define CAN_WMBn_D47_Data_byte_5_SHIFT (16U)
#define CAN_WMBn_D47_Data_byte_5(x)                                            \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_D47_Data_byte_5_SHIFT)) &         \
     CAN_WMBn_D47_Data_byte_5_MASK)
#define CAN_WMBn_D47_Data_byte_4_MASK (0xFF000000U)
#define CAN_WMBn_D47_Data_byte_4_SHIFT (24U)
#define CAN_WMBn_D47_Data_byte_4(x)                                            \
    (((uint32_t)(((uint32_t)(x)) << CAN_WMBn_D47_Data_byte_4_SHIFT)) &         \
     CAN_WMBn_D47_Data_byte_4_MASK)

/** @name FDCTRL - CAN FD Control Register */
#define CAN_FDCTRL_TDCVAL_MASK (0x3FU)
#define CAN_FDCTRL_TDCVAL_SHIFT (0U)
#define CAN_FDCTRL_TDCOFF_MASK (0x1F00U)
#define CAN_FDCTRL_TDCOFF_SHIFT (8U)
#define CAN_FDCTRL_TDCOFF(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_TDCOFF_SHIFT)) &                \
     CAN_FDCTRL_TDCOFF_MASK)
#define CAN_FDCTRL_TDCFAIL_MASK (0x4000U)
#define CAN_FDCTRL_TDCFAIL_SHIFT (14U)
#define CAN_FDCTRL_TDCFAIL(x)                                                  \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_TDCFAIL_SHIFT)) &               \
     CAN_FDCTRL_TDCFAIL_MASK)
#define CAN_FDCTRL_TDCEN_MASK (0x8000U)
#define CAN_FDCTRL_TDCEN_SHIFT (15U)
#define CAN_FDCTRL_TDCEN(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_TDCEN_SHIFT)) &                 \
     CAN_FDCTRL_TDCEN_MASK)
#define CAN_FDCTRL_MBDSR0_MASK (0x30000U)
#define CAN_FDCTRL_MBDSR0_SHIFT (16U)
#define CAN_FDCTRL_MBDSR0(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_MBDSR0_SHIFT)) &                \
     CAN_FDCTRL_MBDSR0_MASK)
#define CAN_FDCTRL_MBDSR1_MASK (0x180000U)
#define CAN_FDCTRL_MBDSR1_SHIFT (19U)
#define CAN_FDCTRL_MBDSR1(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_MBDSR1_SHIFT)) &                \
     CAN_FDCTRL_MBDSR1_MASK)
#define CAN_FDCTRL_MBDSR2_MASK (0xC00000U)
#define CAN_FDCTRL_MBDSR2_SHIFT (22U)
#define CAN_FDCTRL_MBDSR2(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_MBDSR2_SHIFT)) &                \
     CAN_FDCTRL_MBDSR2_MASK)
#define CAN_FDCTRL_MBDSR3_MASK (0x6000000U)
#define CAN_FDCTRL_MBDSR3_SHIFT (25U)
#define CAN_FDCTRL_MBDSR3(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_MBDSR3_SHIFT)) &                \
     CAN_FDCTRL_MBDSR3_MASK)
#define CAN_FDCTRL_FDRATE_MASK (0x80000000U)
#define CAN_FDCTRL_FDRATE_SHIFT (31U)
#define CAN_FDCTRL_FDRATE(x)                                                   \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCTRL_FDRATE_SHIFT)) &                \
     CAN_FDCTRL_FDRATE_MASK)

/** @name FDCBT - CAN FD Bit Timing Register */
#define CAN_FDCBT_FPSEG2_MASK (7U)
#define CAN_FDCBT_FPSEG2_SHIFT (0U)
#define CAN_FDCBT_FPSEG2(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FPSEG2_SHIFT)) &                 \
     CAN_FDCBT_FPSEG2_MASK)
#define CAN_FDCBT_FPSEG1_MASK (0xE0U)
#define CAN_FDCBT_FPSEG1_SHIFT (5U)
#define CAN_FDCBT_FPSEG1(x)                                                    \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FPSEG1_SHIFT)) &                 \
     CAN_FDCBT_FPSEG1_MASK)
#define CAN_FDCBT_FPROPSEG_MASK (0x7C00U)
#define CAN_FDCBT_FPROPSEG_SHIFT (10U)
#define CAN_FDCBT_FPROPSEG(x)                                                  \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FPROPSEG_SHIFT)) &               \
     CAN_FDCBT_FPROPSEG_MASK)
#define CAN_FDCBT_FRJW_MASK (0x70000U)
#define CAN_FDCBT_FRJW_SHIFT (16U)
#define CAN_FDCBT_FRJW(x)                                                      \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FRJW_SHIFT)) &                   \
     CAN_FDCBT_FRJW_MASK)
#define CAN_FDCBT_FPRESDIV_MASK (0x3FF00000U)
#define CAN_FDCBT_FPRESDIV_SHIFT (20U)
#define CAN_FDCBT_FPRESDIV(x)                                                  \
    (((uint32_t)(((uint32_t)(x)) << CAN_FDCBT_FPRESDIV_SHIFT)) &               \
     CAN_FDCBT_FPRESDIV_MASK)

/** @name ERR_IRQ_STATUS - Function Safety Error Interrupt Status Register */
#define ERR_IRQ_STATUS_PSLVERR_MASK (0x200000U)

/** @name SAFTY_ERR_IRQ_STATUS_EN -Function Safety Error Interrupt Status Enable Register */
#define ERR_IRQ_STATUS_PSLVERR_INT_EN_MASK (0x200000U)
/**
 * @}
 */ /* end of group CAN_Register_Masks */

/** @brief FlexCAN Internal State. */
enum _flexcan_state {
    FLEXCAN_StateIdle = 0x0,     /**< MB is in idle state.*/
    FLEXCAN_StateRxData = 0x1,   /**< MB is in receiving data state.*/
    FLEXCAN_StateRxRemote = 0x2, /**< MB is in receiving remote reply state.*/
    FLEXCAN_StateTxData = 0x3,   /**< MB is in transmit data state.*/
    FLEXCAN_StateTxRemote = 0x4, /**< MB is in transmit remote request state.*/
    FLEXCAN_StateRxFifo = 0x5,   /**< MB is in receiving data from fifo state.*/
};

/** @brief FlexCAN message buffer CODE for Rx buffers */
enum _flexcan_mb_code_rx {
    FLEXCAN_RxMbInactive = 0x0, /**< MB is not active.*/
    FLEXCAN_RxMbFull = 0x2,     /**< MB is full.*/
    FLEXCAN_RxMbEmpty = 0x4,    /**< MB is active and empty.*/
    FLEXCAN_RxMbOverrun = 0x6,  /**< MB is overwritten into a full buffer.*/
    FLEXCAN_RxMbBusy = 0x8, /**< FlexCAN is updating the contents of the MB.*/
    /**  The CPU must not access the MB.*/
    FLEXCAN_RxMbRanswer =
        0xA, /**< A frame was configured to recognize a Remote Request Frame*/
    /**  and transmit a Response Frame in return.*/
    FLEXCAN_RxMbNotUsed = 0xF /**< Not used*/
};

/** @brief FlexCAN message buffer CODE FOR Tx buffers*/
enum _flexcan_mb_code_tx {
    FLEXCAN_TxMbInactive = 0x8, /**< MB is not active.*/
    FLEXCAN_TxMbAbort = 0x9,    /**< MB is aborted.*/
    FLEXCAN_TxMbDataOrRemote =
        0xC, /**< MB is a TX Data Frame(when MB RTR = 0) or*/
    /**< MB is a TX Remote Request Frame (when MB RTR = 1).*/
    FLEXCAN_TxMbTanswer = 0xE, /**< MB is a TX Response Request Frame from.*/
    /**  an incoming Remote Request Frame.*/
    FLEXCAN_TxMbNotUsed = 0xF /**< Not used*/
};

/**
 * @brief FlexCAN status flags.
 *
 * This provides constants for the FlexCAN status flags for use in the FlexCAN
 * functions. Note: The CPU read action clears FlEXCAN_ErrorFlag, therefore user
 * need to read FlEXCAN_ErrorFlag and distinguish which error is occur using
 * _flexcan_error_flags enumerations.
 */
enum _flexcan_flags {
    FLEXCAN_SynchFlag = CAN_ESR1_SYNCH_MASK, /**< CAN Synchronization Status. */
    FLEXCAN_TxWarningIntFlag =
        CAN_ESR1_TWRNINT_MASK, /**< Tx Warning Interrupt Flag. */
    FLEXCAN_RxWarningIntFlag =
        CAN_ESR1_RWRNINT_MASK, /**< Rx Warning Interrupt Flag. */
    FLEXCAN_TxErrorWarningFlag =
        CAN_ESR1_TXWRN_MASK, /**< Tx Error Warning Status. */
    FLEXCAN_RxErrorWarningFlag =
        CAN_ESR1_RXWRN_MASK,               /**< Rx Error Warning Status. */
    FLEXCAN_IdleFlag = CAN_ESR1_IDLE_MASK, /**< CAN IDLE Status Flag. */
    FLEXCAN_FaultConfinementFlag =
        CAN_ESR1_FLTCONF_MASK, /**< Fault Confinement State Flag. */
    FLEXCAN_TransmittingFlag =
        CAN_ESR1_TX_MASK, /**< FlexCAN In Transmission Status. */
    FLEXCAN_ReceivingFlag =
        CAN_ESR1_RX_MASK, /**< FlexCAN In Reception Status. */
    FLEXCAN_BusOffIntFlag =
        CAN_ESR1_BOFFINT_MASK,                   /**< Bus Off Interrupt Flag. */
    /// BUS OFF DONE
    FLEXCAN_BusOffDoneIntFlag = CAN_ESR1_BOFFDONEINT_MASK,
    /// High resolution counte wrap
    FLEXCAN_HighCounterWrapIntFlag = CAN_ESR2_HTWAINT_SET_MASK,
    FLEXCAN_ErrorIntFlag = CAN_ESR1_ERRINT_MASK, /**< Error Interrupt Flag. */
    FLEXCAN_WakeUpIntFlag =
        CAN_ESR1_WAKINT_MASK, /**< Wake-Up Interrupt Flag. */
    FLEXCAN_ErrorFlag =
        CAN_ESR1_BIT1ERR_MASK | /**< All FlexCAN Error Status. */
        CAN_ESR1_BIT0ERR_MASK | CAN_ESR1_ACKERR_MASK | CAN_ESR1_CRCERR_MASK |
        CAN_ESR1_FRMERR_MASK | CAN_ESR1_STFERR_MASK
};

/**
 * @brief FlexCAN Rx FIFO status flags.
 *
 * The FlexCAN Rx FIFO Status enumerations is used to ditermine the status of
 * Rx FIFO. Because Rx FIFO occupy the MB0 ~ MB7(Rx Fifo filter will also occupy
 * more Message Buffer space), Rx FIFO status flags are maped to corresponding
 * Message Buffer status flags.
 */
enum _flexcan_rx_fifo_flags {
    FLEXCAN_RxFifoOverflowFlag =
        CAN_IFLAG1_BUF7I_MASK, /**< Rx FIFO overflow flag. */
    FLEXCAN_RxFifoWarningFlag =
        CAN_IFLAG1_BUF6I_MASK, /**< Rx FIFO almost full flag. */
    FLEXCAN_RxFifoFrameAvlFlag =
        CAN_IFLAG1_BUF5I_MASK, /**< Frames available in Rx FIFO flag. */
};

/* FlexCAN freezing timeout counter. */
#define FLEXCAN_TIMEOUT_COUNTER 50000U

/* CAN FD SMB region. */
#define CAN_FD_SMB_START_ADDR_OFFSET 0xF28U
#define CAN_FD_SMB_END_ADDR_OFFSET 0xFFFU

/* Get message buffer fields. */
#define mb_cs_field(x) (*(x))
#define mb_id_field(x) (*((x) + 1))
#define mb_data_field(x) ((uint32_t *)((x) + 2))

/* Number of message buffers */
#define FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER 64U

/* Number of message buffer region. */
#if FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER <= 32
#define REGION_NUM 1U
#elif FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER <= 64
#define REGION_NUM 2U
#elif FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER <= 96
#define REGION_NUM 3U
#else
#define REGION_NUM 4U
#endif

/* Number of message buffer per region. */
#define REGION_8BYTES_MB_NUM 32U
#define REGION_16BYTES_MB_NUM 21U
#define REGION_32BYTES_MB_NUM 12U
#define REGION_64BYTES_MB_NUM 7U

/* Number of wakeup message buffers */
#define FLEXCAN_HAS_WAKEUP_MESSAGE_BUFFER_NUMBER 4

/* Region size in word. */
#define PER_REGION_SIZE_IN_WORD (4U * 32U)

/* CAN FD extended data length DLC encoding */
#define CAN_DLC_VALUE_12_BYTES 9U
#define CAN_DLC_VALUE_16_BYTES 10U
#define CAN_DLC_VALUE_20_BYTES 11U
#define CAN_DLC_VALUE_24_BYTES 12U
#define CAN_DLC_VALUE_32_BYTES 13U
#define CAN_DLC_VALUE_48_BYTES 14U
#define CAN_DLC_VALUE_64_BYTES 15U

/* Rx FIFO message buffer ID definition. */
#define RX_FIFO_MB_ID 0U

/* Rx FIFO interrupt ID definitions. */
#define RX_FIFO_FRAME_AVL_MB_ID 5U
#define RX_FIFO_ALMOST_FULL_MB_ID 6U
#define RX_FIFO_OVERFLOW_MB_ID 7U

#endif

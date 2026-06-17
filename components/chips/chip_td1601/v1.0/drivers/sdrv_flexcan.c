/**
 * @file sdrv_flexcan.c
 *
 *  Created on: 2023年8月25日
 *      Author: Dell
 */

//#include <bits.h>
//#include <debug.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "drv/irq.h"
#include "sdrv_flexcan.h"
#include "sdrv_flexcan_priv.h"
#include "flexcan_cfg.h"
#include "drv/common.h"
#include <soc.h>

#define ssdk_printf(l, x...)		printf(x)
#define ASSERT(c)   				CSI_ASSERT(c)

#define __BKPT(value)               __asm volatile ("bkpt "#value)

void __assertion_failed(char *failedExpr)
{
    (void)printf("ASSERT ERROR \" %s \n", failedExpr);
    for (;;)
    {
        //__BKPT(0);
    }
}
#define _ASSERT_STR(x) _ASSERT_VAL(x)
#define _ASSERT_VAL(x) #x
#define assert(expr)   \
  ((expr) ? (void)0 :  \
		  __assertion_failed(__FILE__ ":" _ASSERT_STR(__LINE__) " : " #expr))

#define BIT_SET(x, bit) (((x) & (1UL << (bit))) ? 1 : 0)

#define BITMAP_BITS_PER_INT (sizeof(unsigned int) * 8)
#define BITMAP_BIT_IN_INT(x) ((x) & (BITMAP_BITS_PER_INT - 1))
#define BITMAP_INT(x) ((x) / BITMAP_BITS_PER_INT)

/***************Internal function prototypes***************/

static inline flexcan_status_e flexcan_enter_freeze_mode(CAN_Type *base,
                                                         bool allowTimeout);
static inline flexcan_status_e flexcan_exit_freeze_mode(CAN_Type *base,
                                                        bool allowTimeout);
static inline void flexcan_fd_clean_smb_region(CAN_Type *base);
static void flexcan_pnet_init(CAN_Type *base,
                              const flexcan_pn_config_t *config);
static void flexcan_fd_init(CAN_Type *base, const flexcan_fd_config_t *config);
static inline bool flexcan_is_fd_enabled(CAN_Type *base);
static uint32_t *flexcan_get_msg_buf_addr(CAN_Type *base, uint8_t msgBufId);
static void flexcan_enable_mb_int(CAN_Type *base, uint8_t msgBufId);
static void flexcan_disable_mb_int(CAN_Type *base, uint8_t msgBufId);
static uint8_t flexcan_compute_dlc_val(uint8_t payloadSize);
static uint8_t flexcan_compute_payload_len(uint8_t dlc_val);
static inline uint32_t flexcan_get_mb_int_req(CAN_Type *base, uint8_t group);
static uint32_t flexcan_get_mb_int_status_flag(CAN_Type *base,
                                               uint8_t msgBufId);
static inline void flexcan_clear_mb_int_flag(CAN_Type *base, uint8_t msgBufId);
static void flexcan_copy_from_mb(uint32_t *addr, uint8_t *data, uint8_t len);
static void flexcan_copy_to_mb(uint32_t *addr, uint8_t *data, uint8_t len);
#if FLEXCAN_COPY_USE_BURST
static inline void flexcan_copy8(uint32_t *src, uint32_t *dest);
static inline void flexcan_copy16(uint32_t *src, uint32_t *dest);
static inline void flexcan_copy32(uint32_t *src, uint32_t *dest);
#endif
static inline bool flexcan_ext_nominal_bit_timing(
    const flexcan_timing_config_t *nominal_bit_timing);

/***************Function implementation***************/

/* Internal function implementation start */

static inline flexcan_status_e flexcan_enter_freeze_mode(CAN_Type *base,
                                                         bool allowTimeout)
{
    ASSERT(base);

    uint32_t timeoutCnt = FLEXCAN_TIMEOUT_COUNTER;
    flexcan_status_e retVal = FLEXCAN_SUCCESS;

    /* Set Freeze, Halt bits. */
    base->MCR |= CAN_MCR_FRZ_MASK | CAN_MCR_HALT_MASK;

    /* Wait until the FlexCAN Module enter freeze mode. */
    while ((!(base->MCR & CAN_MCR_FRZACK_MASK)) && (timeoutCnt > 0U)) {
        if (allowTimeout) {
            timeoutCnt--;
        }
    }

    /* Return failed status if timeout. */
    if (!(base->MCR & CAN_MCR_FRZACK_MASK)) {
        ssdk_printf(SSDK_ERR, "\r\nEnter freeze mode timeout\r\n");
        retVal = FLEXCAN_FAIL;
    }

    return retVal;
}

static inline flexcan_status_e flexcan_exit_freeze_mode(CAN_Type *base,
                                                        bool allowTimeout)
{
    ASSERT(base);

    uint32_t timeoutCnt = FLEXCAN_TIMEOUT_COUNTER;
    flexcan_status_e retVal = FLEXCAN_SUCCESS;

    /* Clear Freeze, Halt bits. */
    base->MCR &= ~(CAN_MCR_FRZ_MASK | CAN_MCR_HALT_MASK);
    //base->MCR &= ~(CAN_MCR_HALT_MASK);

    /* Wait until the FlexCAN Module exit freeze mode. */
    while ((base->MCR & CAN_MCR_FRZACK_MASK) && (timeoutCnt > 0U)) {
        if (allowTimeout) {
            timeoutCnt--;
        }
    }

    /* Return failed status if timeout. */
    if (base->MCR & CAN_MCR_FRZACK_MASK) {
        ssdk_printf(SSDK_ERR, "\r\nExit freeze mode timeout\r\n");
        retVal = FLEXCAN_FAIL;
    }

    return retVal;
}

static inline void flexcan_fd_clean_smb_region(CAN_Type *base)
{
    ASSERT(base);

    uint16_t start = CAN_FD_SMB_START_ADDR_OFFSET;

    /* Enable unrestricted write access to FlexCAN memory. */
    base->CTRL2 |= CAN_CTRL2_WRMFRZ_MASK;

    /* Clear CAN FD SMB region to avoid non-correctable errors. */
    while (start < CAN_FD_SMB_END_ADDR_OFFSET) {
        *((uint32_t *)((uint8_t *)base + start)) = 0U;
        start += 4U;
    }

    /* Enable write access restriction. */
    base->CTRL2 &= ~CAN_CTRL2_WRMFRZ_MASK;
}

static void flexcan_pnet_init(CAN_Type *base, const flexcan_pn_config_t *config)
{
    ASSERT(base && config);

    /* Configure CTRL1_PN with the filter criteria to
     * be used to receive wakeup messages.
     */
    base->CTRL1_PN &=
        ~(CAN_CTRL1_PN_WTOF_MSK_MASK | CAN_CTRL1_PN_WUMF_MSK_MASK |
          CAN_CTRL1_PN_NMATCH_MASK | CAN_CTRL1_PN_PLFS_MASK |
          CAN_CTRL1_PN_IDFS_MASK | CAN_CTRL1_PN_FCS_MASK);
    base->CTRL1_PN |=
        (CAN_CTRL1_PN_WTOF_MSK(config->wakeUpTimeout ? 1UL : 0UL) |
         CAN_CTRL1_PN_WUMF_MSK(config->wakeUpMatch ? 1UL : 0UL) |
         CAN_CTRL1_PN_NMATCH(config->numMatches) |
         CAN_CTRL1_PN_FCS(config->filterComb) |
         CAN_CTRL1_PN_IDFS(config->idFilterType) |
         CAN_CTRL1_PN_PLFS(config->payloadFilterType));

    /* Set timeout value under PNET. */
    base->CTRL2_PN = (base->CTRL2_PN & ~CAN_CTRL2_PN_MATCHTO_MASK) |
                     CAN_CTRL2_PN_MATCHTO(config->matchTimeout);

    /* Configures values used to filter incoming message ID either
     * for equal to, smaller than, greater than comparisons,
     * or as the lower limit value in an ID range detection.
     */
    base->FLT_ID1 &= ~(CAN_FLT_ID1_FLT_IDE_MASK | CAN_FLT_ID1_FLT_RTR_MASK |
                       CAN_FLT_ID1_FLT_ID1_MASK);
    base->FLT_ID1 |=
        (CAN_FLT_ID1_FLT_IDE(config->idFilter1.extendedId ? 1UL : 0UL) |
         CAN_FLT_ID1_FLT_RTR(config->idFilter1.remoteFrame ? 1UL : 0UL));
    if (config->idFilter1.extendedId) {
        base->FLT_ID1 |= CAN_FLT_ID1_FLT_ID1(config->idFilter1.id);
    } else {
        base->FLT_ID1 |=
            CAN_FLT_ID1_FLT_ID1(config->idFilter1.id << CAN_ID_STD_SHIFT);
    }

    if ((config->idFilterType == FLEXCAN_FILTER_MATCH_EXACT) ||
        (config->idFilterType == FLEXCAN_FILTER_MATCH_RANGE)) {
        /* Configures values either for upper limit value in ID range detection
         * or for ID mask in ID exact match.
         */
        base->FLT_ID2_IDMASK &= ~(CAN_FLT_ID2_IDMASK_IDE_MSK_MASK |
                                  CAN_FLT_ID2_IDMASK_RTR_MSK_MASK |
                                  CAN_FLT_ID2_IDMASK_FLT_ID2_IDMASK_MASK);
        base->FLT_ID2_IDMASK |=
            (CAN_FLT_ID2_IDMASK_IDE_MSK(config->idFilter2.extendedId ? 1UL
                                                                     : 0UL) |
             CAN_FLT_ID2_IDMASK_RTR_MSK(config->idFilter2.remoteFrame ? 1UL
                                                                      : 0UL));

        if (config->idFilter2.extendedId) {
            base->FLT_ID2_IDMASK |=
                CAN_FLT_ID2_IDMASK_FLT_ID2_IDMASK(config->idFilter2.id);
        } else {
            base->FLT_ID2_IDMASK |= CAN_FLT_ID2_IDMASK_FLT_ID2_IDMASK(
                config->idFilter2.id << CAN_ID_STD_SHIFT);
        }
    } else {
        /* need to check only. */
        base->FLT_ID2_IDMASK |=
            CAN_FLT_ID2_IDMASK_IDE_MSK_MASK | CAN_FLT_ID2_IDMASK_RTR_MSK_MASK;
    }

    /* Config criteria if payload filtering selected. */
    if ((config->filterComb == FLEXCAN_FILTER_ID_PAYLOAD) ||
        (config->filterComb == FLEXCAN_FILTER_ID_PAYLOAD_NTIMES)) {
        base->FLT_DLC &=
            ~(CAN_FLT_DLC_FLT_DLC_HI_MASK | CAN_FLT_DLC_FLT_DLC_HI_MASK);
        base->FLT_DLC |=
            (CAN_FLT_DLC_FLT_DLC_HI(config->payloadFilter.dlcLow) |
             CAN_FLT_DLC_FLT_DLC_LO(config->payloadFilter.dlcHigh));

        base->PL1_HI =
            CAN_PL1_HI_Data_byte_4(config->payloadFilter.payload1[4]) |
            CAN_PL1_HI_Data_byte_5(config->payloadFilter.payload1[5]) |
            CAN_PL1_HI_Data_byte_6(config->payloadFilter.payload1[6]) |
            CAN_PL1_HI_Data_byte_7(config->payloadFilter.payload1[7]);

        base->PL1_LO =
            CAN_PL1_LO_Data_byte_0(config->payloadFilter.payload1[0]) |
            CAN_PL1_LO_Data_byte_1(config->payloadFilter.payload1[1]) |
            CAN_PL1_LO_Data_byte_2(config->payloadFilter.payload1[2]) |
            CAN_PL1_LO_Data_byte_3(config->payloadFilter.payload1[3]);
        /* Configures values either for upper limit value in payload range
         * detection or for payload mask in payload exact match.
         */
        if ((config->payloadFilterType == FLEXCAN_FILTER_MATCH_EXACT) ||
            (config->idFilterType == FLEXCAN_FILTER_MATCH_RANGE)) {
            base->PL2_PLMASK_HI = CAN_PL2_PLMASK_HI_Data_byte_4(
                                      config->payloadFilter.payload2[4]) |
                                  CAN_PL2_PLMASK_HI_Data_byte_5(
                                      config->payloadFilter.payload2[5]) |
                                  CAN_PL2_PLMASK_HI_Data_byte_6(
                                      config->payloadFilter.payload2[6]) |
                                  CAN_PL2_PLMASK_HI_Data_byte_7(
                                      config->payloadFilter.payload2[7]);

            base->PL2_PLMASK_LO = CAN_PL2_PLMASK_LO_Data_byte_0(
                                      config->payloadFilter.payload2[0]) |
                                  CAN_PL2_PLMASK_LO_Data_byte_1(
                                      config->payloadFilter.payload2[1]) |
                                  CAN_PL2_PLMASK_LO_Data_byte_2(
                                      config->payloadFilter.payload2[2]) |
                                  CAN_PL2_PLMASK_LO_Data_byte_3(
                                      config->payloadFilter.payload2[3]);
        }
    }
}

static void flexcan_fd_init(CAN_Type *base, const flexcan_fd_config_t *config)
{
    ASSERT(base && config);

    uint32_t fdctrlTemp = base->FDCTRL;

    /* Enable Bit Rate Switch? */
    if (config->enableBRS) {
        fdctrlTemp |= CAN_FDCTRL_FDRATE_MASK;
    } else {
        fdctrlTemp &= ~CAN_FDCTRL_FDRATE_MASK;
    }

    /* Enable Transceiver Delay Compensation? */
    if (config->enableTDC) {
        fdctrlTemp |= CAN_FDCTRL_TDCEN_MASK;
        /* Set TDC offset. */
        fdctrlTemp &= ~CAN_FDCTRL_TDCOFF_MASK;
        fdctrlTemp |= CAN_FDCTRL_TDCOFF(config->TDCOffset);
    } else {
        fdctrlTemp &= ~CAN_FDCTRL_TDCEN_MASK;
    }

    /* Set message buffer data size for region 0. */
    fdctrlTemp &= ~CAN_FDCTRL_MBDSR0_MASK;
    fdctrlTemp |= CAN_FDCTRL_MBDSR0(config->r0_mb_data_size);

#if FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER > 32

    /* Set message buffer data size for region 1. */
    fdctrlTemp &= ~CAN_FDCTRL_MBDSR1_MASK;
    fdctrlTemp |= CAN_FDCTRL_MBDSR1(config->r1_mb_data_size);

#endif

#if FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER > 64

    /* Set message buffer data size for region 2. */
    fdctrlTemp &= ~CAN_FDCTRL_MBDSR2_MASK;
    fdctrlTemp |= CAN_FDCTRL_MBDSR2(config->r2_mb_data_size);

#endif

#if FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER > 96

    /* Set message buffer data size for region 3. */
    fdctrlTemp &= ~CAN_FDCTRL_MBDSR3_MASK;
    fdctrlTemp |= CAN_FDCTRL_MBDSR3(config->r3_mb_data_size);

#endif

    /* Update FDCTRL register. */
    base->FDCTRL = fdctrlTemp;

    /* Enable ISO CAN FD? */
    if (config->enableISOCANFD) {
        base->CTRL2 |= CAN_CTRL2_ISOCANFDEN_MASK;
    }
}

static inline bool flexcan_is_fd_enabled(CAN_Type *base)
{
    ASSERT(base);

    return (((base->MCR & CAN_MCR_FDEN_MASK) >> CAN_MCR_FDEN_SHIFT) != 0U);
}

static uint32_t *flexcan_get_msg_buf_addr(CAN_Type *base, uint8_t msgBufId)
{
    ASSERT(base);

    uint32_t *msgBufAddr;

    if (flexcan_is_fd_enabled(base)) {
        uint8_t msgBufRegionIdx;
        uint8_t fdctrl_mbdsrx;
        uint8_t payloadSize;
        uint8_t msgBufSize;
        uint8_t regionMaxMBNum;

        msgBufAddr = (uint32_t *)base->MB;

        for (msgBufRegionIdx = 0U; msgBufRegionIdx < REGION_NUM;
             msgBufRegionIdx++) {
            /* Get MBDSRx bits from FDCTRL register. */
            fdctrl_mbdsrx =
                (((base->FDCTRL) >> (16U + msgBufRegionIdx * 3U)) & 3U);
            /* Get message buffer data size in bytes. */
            payloadSize = 1U << (fdctrl_mbdsrx + 3U);
            /* Get message buffer size in words. */
            msgBufSize = (payloadSize + 8U) >> 2U;
            /* Maxium MB index in the region. */
            if (fdctrl_mbdsrx == 0U) {
                regionMaxMBNum = REGION_8BYTES_MB_NUM;
            } else if (fdctrl_mbdsrx == 1U) {
                regionMaxMBNum = REGION_16BYTES_MB_NUM;
            } else if (fdctrl_mbdsrx == 2U) {
                regionMaxMBNum = REGION_32BYTES_MB_NUM;
            } else {
                regionMaxMBNum = REGION_64BYTES_MB_NUM;
            }

            if (msgBufId < regionMaxMBNum) {
                msgBufAddr += (msgBufId * msgBufSize);
                break;
            } else {
                msgBufAddr += PER_REGION_SIZE_IN_WORD;
                msgBufId -= regionMaxMBNum;
            }
        }
    } else {
        msgBufAddr = (uint32_t *)&base->MB[msgBufId];
    }

    return msgBufAddr;
}

static inline void flexcan_enable_mb_int(CAN_Type *base, uint8_t msgBufId)
{
    uint8_t regionId = msgBufId / 32U;
    uint8_t bitOffset = msgBufId % 32U;

    switch (regionId) {
    case 0U:
        base->IMASK1 |= (uint32_t)(1U << bitOffset);
        break;

    case 1U:
        base->IMASK2 |= (uint32_t)(1U << bitOffset);
        break;

    case 2U:
        base->IMASK3 |= (uint32_t)(1U << bitOffset);
        break;

    case 3U:
        base->IMASK4 |= (uint32_t)(1U << bitOffset);
        break;

    default:
        break;
    }
}

static inline void flexcan_disable_mb_int(CAN_Type *base, uint8_t msgBufId)
{
    uint8_t regionId = msgBufId / 32U;
    uint8_t bitOffset = msgBufId % 32U;

    switch (regionId) {
    case 0U:
        base->IMASK1 &= (uint32_t)(~(1U << bitOffset));
        break;

    case 1U:
        base->IMASK2 &= (uint32_t)(~(1U << bitOffset));
        break;

    case 2U:
        base->IMASK3 &= (uint32_t)(~(1U << bitOffset));
        break;

    case 3U:
        base->IMASK4 &= (uint32_t)(~(1U << bitOffset));
        break;

    default:
        break;
    }
}

static uint8_t flexcan_compute_dlc_val(uint8_t payloadSize)
{
    uint8_t ret_dlc_val = 0xFFU;
    static const uint8_t payload_code[65] = {
        0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U,
        /* 9 to 12 payload have DLC Code 12 Bytes */
        CAN_DLC_VALUE_12_BYTES, CAN_DLC_VALUE_12_BYTES, CAN_DLC_VALUE_12_BYTES,
        CAN_DLC_VALUE_12_BYTES,
        /* 13 to 16 payload have DLC Code 16 Bytes */
        CAN_DLC_VALUE_16_BYTES, CAN_DLC_VALUE_16_BYTES, CAN_DLC_VALUE_16_BYTES,
        CAN_DLC_VALUE_16_BYTES,
        /* 17 to 20 payload have DLC Code 20 Bytes */
        CAN_DLC_VALUE_20_BYTES, CAN_DLC_VALUE_20_BYTES, CAN_DLC_VALUE_20_BYTES,
        CAN_DLC_VALUE_20_BYTES,
        /* 21 to 24 payload have DLC Code 24 Bytes */
        CAN_DLC_VALUE_24_BYTES, CAN_DLC_VALUE_24_BYTES, CAN_DLC_VALUE_24_BYTES,
        CAN_DLC_VALUE_24_BYTES,
        /* 25 to 32 payload have DLC Code 32 Bytes */
        CAN_DLC_VALUE_32_BYTES, CAN_DLC_VALUE_32_BYTES, CAN_DLC_VALUE_32_BYTES,
        CAN_DLC_VALUE_32_BYTES, CAN_DLC_VALUE_32_BYTES, CAN_DLC_VALUE_32_BYTES,
        CAN_DLC_VALUE_32_BYTES, CAN_DLC_VALUE_32_BYTES,
        /* 33 to 48 payload have DLC Code 48 Bytes */
        CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES,
        CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES,
        CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES,
        CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES,
        CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES, CAN_DLC_VALUE_48_BYTES,
        CAN_DLC_VALUE_48_BYTES,
        /* 49 to 64 payload have DLC Code 64 Bytes */
        CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES,
        CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES,
        CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES,
        CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES,
        CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES, CAN_DLC_VALUE_64_BYTES,
        CAN_DLC_VALUE_64_BYTES};

    if (payloadSize <= 64U) {
        ret_dlc_val = payload_code[payloadSize];
    } else {
        /* The argument is not a valid payload size,
           so return invalid value 0xFF. */
    }

    return ret_dlc_val;
}

static uint8_t flexcan_compute_payload_len(uint8_t dlc_val)
{
    uint8_t ret_payload_len = 0U;

    if (dlc_val <= 8U) {
        ret_payload_len = dlc_val;
    } else if (dlc_val == CAN_DLC_VALUE_12_BYTES) {
        ret_payload_len = 12U;
    } else if (dlc_val == CAN_DLC_VALUE_16_BYTES) {
        ret_payload_len = 16U;
    } else if (dlc_val == CAN_DLC_VALUE_20_BYTES) {
        ret_payload_len = 20U;
    } else if (dlc_val == CAN_DLC_VALUE_24_BYTES) {
        ret_payload_len = 24U;
    } else if (dlc_val == CAN_DLC_VALUE_32_BYTES) {
        ret_payload_len = 32U;
    } else if (dlc_val == CAN_DLC_VALUE_48_BYTES) {
        ret_payload_len = 48U;
    } else if (dlc_val == CAN_DLC_VALUE_64_BYTES) {
        ret_payload_len = 64U;
    } else {
        /* Do nothing. */
    }

    return ret_payload_len;
}

static inline uint32_t flexcan_get_mb_int_req(CAN_Type *base, uint8_t group)
{
    uint32_t int_iflag = 0, int_req = 0;

    switch (group) {
    case 0U:
        int_iflag = base->IFLAG1;
        int_req = int_iflag & base->IMASK1;
        break;

    case 1U:
        int_iflag = base->IFLAG2;
        int_req = int_iflag & base->IMASK2;
        break;

    case 2U:
        int_iflag = base->IFLAG3;
        int_req = int_iflag & base->IMASK3;
        break;

    case 3U:
        int_iflag = base->IFLAG4;
        int_req = int_iflag & base->IMASK4;
        break;

    default:
        break;
    }

    return int_req;
}

static inline uint32_t flexcan_get_mb_int_status_flag(CAN_Type *base,
                                                      uint8_t msgBufId)
{
   uint32_t int_iflag = 0, ret_flag;

    if (msgBufId <= 31U) {
        int_iflag = base->IFLAG1;
        ret_flag = ((int_iflag & base->IMASK1) >> msgBufId) & 1U;
    } else if (msgBufId <= 63U) {
        int_iflag = base->IFLAG2;
        ret_flag = ((int_iflag & base->IMASK2) >> (msgBufId % 32U)) & 1U;
    } else if (msgBufId <= 95U) {
        int_iflag = base->IFLAG3;
        ret_flag = ((int_iflag & base->IMASK3) >> (msgBufId % 32U)) & 1U;
    } else if (msgBufId <= 127U) {
        int_iflag = base->IFLAG4;
        ret_flag = ((int_iflag & base->IMASK4) >> (msgBufId % 32U)) & 1U;
    } else {
        /* Invalid msgBufId value means check if any message buffer
           interrupt is active. */
        int_iflag = base->IFLAG1;
        ret_flag = int_iflag & base->IMASK1;

        int_iflag = base->IFLAG2;
        ret_flag |= int_iflag & base->IMASK2;

        int_iflag = base->IFLAG3;
        ret_flag |= int_iflag & base->IMASK3;

        int_iflag = base->IFLAG4;
        ret_flag |= int_iflag & base->IMASK4;
    }

    return ret_flag;
}

static inline void flexcan_clear_mb_int_flag(CAN_Type *base, uint8_t msgBufId)
{
    uint32_t mask = 1U << (msgBufId % 32U);

    if (msgBufId <= 31U) {
        base->IFLAG1 = mask;
    } else if (msgBufId <= 63U) {
        base->IFLAG2 = mask;
    } else if (msgBufId <= 95U) {
        base->IFLAG3 = mask;
    } else if (msgBufId <= 127U) {
        base->IFLAG4 = mask;
    } else {
        /* Do nothing. */
    }
}

static void flexcan_copy_from_mb(uint32_t *addr, uint8_t *data, uint8_t len)
{
    uint8_t temp_buf[64];

#if FLEXCAN_COPY_USE_BURST
    if (len <= 8U) {
        flexcan_copy8(addr, (uint32_t *)temp_buf);
    } else if (len <= 16U) {
        flexcan_copy16(addr, (uint32_t *)temp_buf);
    } else if (len <= 32U) {
        flexcan_copy32(addr, (uint32_t *)temp_buf);
    } else {
        flexcan_copy32(addr, (uint32_t *)temp_buf);
        flexcan_copy32(addr + 8U, (uint32_t *)(temp_buf + 32U));
    }
#else
    uint8_t *mb_addr = (uint8_t *)addr;
    for (uint8_t Idx = 0u; Idx < len; Idx++) {
        temp_buf[Idx] = mb_addr[IDX_CONVERT(Idx)];
    }
#endif

    memcpy(data, temp_buf, len);
}

static void flexcan_copy_to_mb(uint32_t *addr, uint8_t *data, uint8_t len)
{
#if FLEXCAN_COPY_USE_BURST
    if (len <= 8U) {
        flexcan_copy8((uint32_t *)data, addr);
    } else if (len <= 16U) {
        flexcan_copy16((uint32_t *)data, addr);
    } else if (len <= 32U) {
        flexcan_copy32((uint32_t *)data, addr);
    } else {
        flexcan_copy32((uint32_t *)data, addr);
        flexcan_copy32((uint32_t *)(data + 32U), addr + 8U);
    }
#else
    uint8_t *mb_addr = (uint8_t *)addr;
    #if 0   //Ori
    for (uint8_t Idx = 0u; Idx < len; Idx++) {
        mb_addr[IDX_CONVERT(Idx)] = data[Idx];
    }
    #else
    __attribute__((unused)) uint32_t *mb_addr_4B = (uint32_t *)addr;
    uint32_t temp_buf_4B[64/4]={0};
    
    for (uint8_t Idx = 0u; Idx < len; Idx++) 
    {
        mb_addr[IDX_CONVERT(Idx)] = data[Idx];
        
        temp_buf_4B[Idx/4] |= mb_addr[IDX_CONVERT(Idx)]<<((3-(Idx%4))*8);
    }
    
    memcpy(addr, (uint8_t *)temp_buf_4B, len);

    #endif
#endif
}

#if FLEXCAN_COPY_USE_BURST
#if (defined(__ARM_ARCH) || defined(__aarch64__) || defined(__aarch32__))
static inline void flexcan_copy8(uint32_t *src, uint32_t *dest)
{
    uint32_t word0 = 0U, word1 = 0U;
    __ASM volatile("ldmia %2, {%0, %1} \n"
#if CORE_LITTLE_ENDIAN
                   "rev %0, %0        \n"
                   "rev %1, %1        \n"
#endif
                   "stmia %3, {%0, %1}   "
                   :"+r"(word0), "+r"(word1)
                   : "r"(src), "r"(dest)
                   : );
}

static inline void flexcan_copy16(uint32_t *src, uint32_t *dest)
{
    uint32_t word0 = 0U, word1 = 0U, word2 = 0U, word3 = 0U;
    __ASM volatile("ldmia %4, {%0, %1, %2, %3} \n"
#if CORE_LITTLE_ENDIAN
                   "rev %0, %0        \n"
                   "rev %1, %1        \n"
                   "rev %2, %2        \n"
                   "rev %3, %3        \n"
#endif
                   "stmia %5, {%0, %1, %2, %3}   "
                   : "+r"(word0), "+r"(word1), "+r"(word2), "+r"(word3)
                   : "r"(src), "r"(dest)
                   : );
}

static inline void flexcan_copy32(uint32_t *src, uint32_t *dest)
{
    __ASM volatile("ldmia %0, {r4-r11}\n"
#if CORE_LITTLE_ENDIAN
                    "rev r4, r4        \n"
                    "rev r5, r5        \n"
                    "rev r6, r6        \n"
                    "rev r7, r7        \n"
                    "rev r8, r8        \n"
                    "rev r9, r9        \n"
                    "rev r10, r10      \n"
                    "rev r11, r11      \n"
#endif
                     "stmia %1, {r4-r11}  "
                     :
                     : "r"(src), "r"(dest)
                     : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11",
                       "memory");
}
#endif
#if defined(__riscv)
static inline void flexcan_copy8(uint32_t *src, uint32_t *dest)
{
    uint32_t word0 = 0U, word1 = 0U;
    __asm__ volatile("lw %0, 0(%2) \n"
                     "lw %1, 4(%2) \n"
#if CORE_LITTLE_ENDIAN
                     "rev %0, %0        \n"
                     "rev %1, %1        \n"
#endif
                     "sw %0, 0(%3) \n"
                     "sw %1, 4(%3) \n"
                     :
                     : "r"(word0), "r"(word1), "r"(src), "r"(dest)
                     : "memory");
}

static inline void flexcan_copy16(uint32_t *src, uint32_t *dest)
{
    uint32_t word0 = 0U, word1 = 0U, word2 = 0U, word3 = 0U;
    __asm__ volatile("lw %0, 0(%4) \n"
                     "lw %1, 4(%4) \n"
                     "lw %2, 8(%4) \n"
                     "lw %3, 12(%4) \n"
#if CORE_LITTLE_ENDIAN
                     "rev %0, %0        \n"
                     "rev %1, %1        \n"
                     "rev %2, %2        \n"
                     "rev %3, %3        \n"
#endif
                     "sw %0, 0(%5) \n"
                     "sw %1, 4(%5) \n"
                     "sw %2, 8(%5) \n"
                     "sw %3, 12(%5) \n"
                     :
                     : "r"(word0), "r"(word1), "r"(word2), "r"(word3), "r"(src), "r"(dest)
                     : "memory");
}

static inline void flexcan_copy32(uint32_t *src, uint32_t *dest)
{
	__asm__ volatile("lw x4, 0(%0) \n"
#if CORE_LITTLE_ENDIAN
					"rev x4, x4        \n"
#endif
					"sw x4, 0(%1)   \n"
					"lw x5, 4(%0) \n"
#if CORE_LITTLE_ENDIAN
					"rev x5, x5        \n"
#endif
					"sw x5, 4(%1)   \n"
					"lw x6, 8(%0) \n"
#if CORE_LITTLE_ENDIAN
					"rev x6, x6        \n"
#endif
					"sw x6, 8(%1)   \n"
					"lw x7, 12(%0) \n"
#if CORE_LITTLE_ENDIAN
					"rev x7, x7        \n"
#endif
					"sw x7, 12(%1)   \n"
					"lw x8, 16(%0) \n"
#if CORE_LITTLE_ENDIAN
					"rev x8, x8        \n"
#endif
					"sw x8, 16(%1)   \n"
					"lw x9, 20(%0) \n"
#if CORE_LITTLE_ENDIAN
					"rev x9, x9        \n"
#endif
					"sw x9, 20(%1)   \n"
					"lw x10, 24(%0) \n"
#if CORE_LITTLE_ENDIAN
					"rev x10, x10      \n"
#endif
					"sw x10, 24(%1)   \n"
					"lw x11, 28(%0) \n"
#if CORE_LITTLE_ENDIAN
					"rev x11, x11      \n"
#endif
					"sw x11, 28(%1)   \n"
					:
					: "r"(src), "r"(dest)
					: "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11",
					"memory");
}
#endif	//endof 
#endif	//endof #if FLEXCAN_COPY_USE_BURST

static inline bool flexcan_ext_nominal_bit_timing(
    const flexcan_timing_config_t *nominal_bit_timing)
{
    if ((nominal_bit_timing->preDivider - 1U > 0xFFU) ||
        (nominal_bit_timing->rJumpwidth - 1U > 3U) ||
        (nominal_bit_timing->propSeg - 1U > 7U) ||
        (nominal_bit_timing->phaseSeg1 - 1U > 7U) ||
        (nominal_bit_timing->phaseSeg2 - 1U > 7U)) {
        return true;
    } else {
        return false;
    }
}

int canfd_pmu_cfg_soft_reset(flexcan_handle_t *handle)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);
    
    if((unsigned int)base == NXP_CAN0_BASE)
    {
        /* PMU ON */
        //*(volatile unsigned int *)(0xD4000000+0x604) |= (1<<23);
        
        /* SYS REG ON */
        *(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) = 0x1;
        //*(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) = NXP_FLEXCAN_SUPERVISOR_ACCESS;
        //*(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) |= NXP_FLEXCAN_SUPERVISOR_ACCESS;
        udelay(1);
        
        *(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) |= 0x101;
        //*(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) |= (NXP_FLEXCAN_SUPERVISOR_ACCESS | NXP_FLEXCAN_SOFT_RESET_ACCESS);
        udelay(1);
    }
    
    return 0;
}

int canfd_pmu_cfg_default(flexcan_handle_t *handle)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);
    
    if((unsigned int)base == NXP_CAN0_BASE)
    {
        *(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) = (NXP_FLEXCAN_SUPERVISOR_ACCESS | NXP_FLEXCAN_SOFT_RESET_ACCESS);
        udelay(1);
    }
    
    return 0;
}

int canfd_pmu_cfg_hr_timer_sw(flexcan_handle_t *handle, bool on_off)
{
	ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);
	
	int ret = 0;
	
	if(on_off)
	{
		if((unsigned int)base == NXP_CAN0_BASE)
		{
			*(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) |= 0x100;
			*(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) |= (NXP_FLEXCAN_HR_TIMER_EN_ACCESS);
			udelay(1);
		}
	}
	
	return ret;
}

int canfd_pmu_cfg_doze_en(flexcan_handle_t *handle)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);
    
    if((unsigned int)base == NXP_CAN0_BASE)
    {
        /* SYS REG ON */
        //*(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) = 0x001;
        //*(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) |= 0x101;
        *(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) = NXP_FLEXCAN_SUPERVISOR_ACCESS;
        udelay(1);
        
        *(volatile unsigned int *)(NXP_FLEXCAN_0_SYSREG_BASE) |= ((NXP_FLEXCAN_SUPERVISOR_ACCESS | NXP_FLEXCAN_SOFT_RESET_ACCESS) | NXP_FLEXCAN_DOZE_ACCESS);
        udelay(1);
    
    }
    
    return 0;
}

void canfd_create_handle(flexcan_handle_t *handle, uint8_t canfd_id, flexcan_transfer_callback_t callback)
{
	if(canfd_id <= 0 && canfd_id > MAX_FLEXCAN_CH){
		ssdk_printf(SSDK_ERR, "ERROR: canfd_id not right, it should be 1~%d.\n", MAX_FLEXCAN_CH);
		return;
	}
	uint8_t controller_id = FLEXCAN1 + (canfd_id - 0);
	uint8_t irq_num = NXP_CAN_IRQn + (canfd_id);
    uint8_t soc_info_idx_start = 0, soc_info_idx_end = 0;
    uint8_t soc_info_irq_idx_start = 0, soc_info_irq_idx_end = 0;
	void *reg_base;
    uint32_t i = 0;
    uint32_t ret = 0;
	if(canfd_id == 0){
		irq_num = NXP_CAN_IRQn;
		reg_base = (void *)(NXP_CAN0_BASE);
        soc_info_idx_start = 0x0;
        soc_info_idx_end = 0x0;
        soc_info_irq_idx_start = NXP_CAN_IRQn;
        soc_info_irq_idx_end = NXP_CAN_IRQn;
	}
	flexcan_create_handle(handle, controller_id, reg_base,
			irq_num, callback, NULL);
    for(i=soc_info_idx_start; i<=soc_info_idx_end; i++)   
    {
#if (defined(NXP_FLEXCAN_USE_DEV_ARR_4_IRQ) && !NXP_FLEXCAN_USE_DEV_ARR_4_IRQ)
        ret = target_get(DEV_NXP_FLEXCAN_TAG, i, &handle->csi_dev);
#else
        ret = target_get(DEV_NXP_FLEXCAN_TAG, i, &handle->csi_dev[i]);
#endif
        if (ret) {
            break;
        }
    }

    if (!ret && callback) {
#if (defined(NXP_CSI_FLEXCAN_XFER_POLL) && !NXP_CSI_FLEXCAN_XFER_POLL)
        for(i=soc_info_irq_idx_start; i<=soc_info_irq_idx_end; i++)
        {
#if (defined(NXP_FLEXCAN_USE_DEV_ARR_4_IRQ) && !NXP_FLEXCAN_USE_DEV_ARR_4_IRQ)
            csi_irq_attach(i, &flexcan_irq_handler, &handle->csi_dev);
    #else
            csi_irq_attach(i, &flexcan_irq_handler, &handle->csi_dev[i]);
    #endif
            csi_irq_enable(i);
        }
#endif
    }
}

/* Internal function implementation end */

/***************API Function implementation***************/

/* API function implementation start */

flexcan_status_e flexcan_init(flexcan_handle_t *handle,
                              uint8_t can_id, flexcan_transfer_callback_t callback, const flexcan_config_t *config)
{
    ASSERT(handle);

    canfd_create_handle(handle, can_id, callback);

    canfd_pmu_cfg_soft_reset(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);

    uint32_t mcrTemp;
    uint32_t ctrl1Temp;
    flexcan_status_e retVal = FLEXCAN_SUCCESS;

    /* Assertion. */
    ASSERT(config && base);
    ASSERT((config->maxMbNum > 0) &&
           (config->maxMbNum <= FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER));

    /* operate soft reset of SemidDrive enhancements. */
    ///base->SOFT_RESET = 0x0;
    #if 0
    *(volatile unsigned int *)(0xd4000514) |= (0x60000000);
    volatile uint32_t delay_i=20;
    while(delay_i--);
    *(volatile unsigned int *)(0xd4000514) &= (~(0x60000000));
    #endif

    /* Reset to known status.*/
    (void)flexcan_reset(handle);

    /* Check if FlexCAN Module already Enabled before calling Module_Config. */
    if (!(base->MCR & CAN_MCR_MDIS_MASK)) {
        retVal = flexcan_enable(handle, false);
    }

    if (retVal != FLEXCAN_SUCCESS)
        return retVal;

    /* Protocol-Engine clock source selection, This bit must be set
     * when FlexCAN Module in Disable Mode.
     */
    if (FLEXCAN_ClkSrcOsc == config->clkSrc) {
        base->CTRL1 &= ~CAN_CTRL1_CLKSRC_MASK;
    } else {
        base->CTRL1 |= CAN_CTRL1_CLKSRC_MASK;
    }

    /* Enable FlexCAN Module before configuration. */
    retVal = flexcan_enable(handle, true);
    if (retVal != FLEXCAN_SUCCESS)
        return retVal;

    /* Enter Freeze Mode. */
    retVal = flexcan_enter_freeze_mode(base, true);
    if (retVal != FLEXCAN_SUCCESS)
        return retVal;

    /* Save current MCR value. */
    mcrTemp = base->MCR;
    /* Save current CTRL1 value. */
    ctrl1Temp = base->CTRL1;

    /* Set the maximum number of Message Buffers */
    mcrTemp =
        (mcrTemp & ~CAN_MCR_MAXMB_MASK) | CAN_MCR_MAXMB(config->maxMbNum - 1);

    /* Disable busoff automatic recovering. */
    ctrl1Temp |= CAN_CTRL1_BOFFREC_MASK;

    /* Enable Loop Back Mode? */
    if (config->enableLoopBack) {
        ctrl1Temp &= ~CAN_CTRL1_LOM_MASK;
        ctrl1Temp |= CAN_CTRL1_LPB_MASK;
        mcrTemp &= ~CAN_MCR_SRXDIS_MASK;
    } else {
        ctrl1Temp &= ~CAN_CTRL1_LPB_MASK;
        /* Disable the self reception feature when FlexCAN is not in loopback
         * mode. */
        mcrTemp |= CAN_MCR_SRXDIS_MASK;
        /* Enable Listen-Only Mode? */
        if (config->enableListenOnly) {
            ctrl1Temp |= CAN_CTRL1_LOM_MASK;
        } else {
            ctrl1Temp &= ~CAN_CTRL1_LOM_MASK;
        }
    }

    /* Enable Self Wake Up Mode? */
    if (config->enableSelfWakeup) {
        mcrTemp |= CAN_MCR_SLFWAK_MASK;
    } else {
        mcrTemp &= ~CAN_MCR_SLFWAK_MASK;
    }

    /* Enable Individual Rx Masking? */
    if (config->enableIndividMask) {
        mcrTemp |= CAN_MCR_IRMQ_MASK;
    } else {
        mcrTemp &= ~CAN_MCR_IRMQ_MASK;
    }

#if FLEXCAN_HAS_DOZE_MODE_SUPPORT

    /* Enable Doze Mode? */
    if (config->enableDoze) {
        mcrTemp |= CAN_MCR_DOZE_MASK;
        handle->enableDoze = true;
    } else {
        mcrTemp &= ~CAN_MCR_DOZE_MASK;
        handle->enableDoze = false;
    }

#endif

    /* Enable CAN FD operation? */
    if (config->enableCANFD) {
        mcrTemp |= CAN_MCR_FDEN_MASK;
        flexcan_fd_init(base, &config->can_fd_cfg);
    } else {
        mcrTemp &= ~CAN_MCR_FDEN_MASK;
        ctrl1Temp |= CAN_CTRL1_SMP_MASK;
    }

    /* Enable PNET setting? */
    if (config->enablePretendedNetworking) {
        mcrTemp |= CAN_MCR_PNET_EN_MASK;
        flexcan_pnet_init(base, &config->pnet_cfg);
    } else {
        mcrTemp &= ~CAN_MCR_PNET_EN_MASK;
    }

    /* Select ordering mechanism for mb transmission. */
    if (config->enableLBUFTransmittedFirst) {
        ctrl1Temp |= CAN_CTRL1_LBUF_MASK;
    } else {
        ctrl1Temp &= ~CAN_CTRL1_LBUF_MASK;
    }

    /* Save CTRL1 Configuration. */
    base->CTRL1 = ctrl1Temp;
    /* Save MCR Configuation. */
    base->MCR = mcrTemp;

    /* Baud Rate Configuration.*/
    if (config->enableCANFD) {
        flexcan_fd_set_timing_config(handle, &config->nominalBitTiming,
                                     &config->dataBitTiming);
    } else {
        flexcan_classic_set_timing_config(handle, &config->nominalBitTiming);
    }

#if (defined(NXP_CSI_FLEXCAN_XFER_POLL) && !NXP_CSI_FLEXCAN_XFER_POLL)
    // NXP_CSI_canfd_enable_int_flag(handle);
    base->IMASK1 = 0xffffffff;
    base->IMASK2 = 0xffffffff;
    base->IMASK3 = 0xffffffff;
    base->IMASK4 = 0xffffffff;
#endif

#if (defined(NXP_CSI_FLEXCAN_ERROR_AND_WARNING_INT) && NXP_CSI_FLEXCAN_ERROR_AND_WARNING_INT)
    flexcan_enable_interrupts(handle, (FLEXCAN_ErrorInterruptEnable|FLEXCAN_RxWarningInterruptEnable|FLEXCAN_TxWarningInterruptEnable));
#endif
    
#if (defined(NXP_FLEXCAN_BUSOFF) && NXP_FLEXCAN_BUSOFF)
    /* enable busoff interrupt. */
    flexcan_enable_interrupts(handle, FLEXCAN_BusOffInterruptEnable);
#endif

#if (defined(NXP_FLEXCAN_WAKINT) && NXP_FLEXCAN_WAKINT)
    flexcan_enable_interrupts(handle, FLEXCAN_WakeUpInterruptEnable);
#endif

#if (defined(NXP_FLEXCAN_HIGH_TIMER_WRAP_INT) && NXP_FLEXCAN_HIGH_TIMER_WRAP_INT)
    flexcan_enable_interrupts(handle, FLEXCAN_HighTimerWarpInterruptEnable);
#endif
    /* mask access reserved address(0xc) error */
    ///base->ERR_IRQ_STATUS_EN &= ~ERR_IRQ_STATUS_PSLVERR_INT_EN_MASK;

    /* Re-freeze FlexCAN Module after Config finish,
     * because it may be unfreezed during config.
     * It's upper layer software's responsibility
     * to unfreeze FlexCAN when ready to take part
     * in communication on the bus.
     */
    retVal = flexcan_enter_freeze_mode(base, true);
    if (retVal != FLEXCAN_SUCCESS)
        return retVal;

    flexcan_freeze(handle, false);
    return retVal;
}

flexcan_status_e flexcan_enable(flexcan_handle_t *handle, bool enable)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    uint32_t timeoutCnt = FLEXCAN_TIMEOUT_COUNTER;
    flexcan_status_e retVal = FLEXCAN_SUCCESS;

    if (enable) {
        base->MCR &= ~CAN_MCR_MDIS_MASK;

        /* Wait FlexCAN exit from low-power mode. */
        while ((base->MCR & CAN_MCR_LPMACK_MASK) && (timeoutCnt > 0)) {
            timeoutCnt--;
        }

        if (base->MCR & CAN_MCR_LPMACK_MASK) {
            retVal = FLEXCAN_FAIL;
            ssdk_printf(SSDK_ERR, "\r\nExit low-power mode timeout\r\n");
        }
    } else {
        base->MCR |= CAN_MCR_MDIS_MASK;

        /* Wait FlexCAN enter low-power mode. */
        while (!(base->MCR & CAN_MCR_LPMACK_MASK) && (timeoutCnt > 0)) {
            timeoutCnt--;
        }

        if (!(base->MCR & CAN_MCR_LPMACK_MASK)) {
            retVal = FLEXCAN_FAIL;
            ssdk_printf(SSDK_ERR, "\r\nEnter low-power mode timeout\r\n");
        }
    }

    return retVal;
}

flexcan_status_e flexcan_freeze(flexcan_handle_t *handle, bool freeze)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    flexcan_status_e retVal;

    if (freeze) {
        retVal = flexcan_enter_freeze_mode(base, true);
    } else {
        retVal = flexcan_exit_freeze_mode(base, true);
    }

    return retVal;
}

flexcan_status_e flexcan_reset(flexcan_handle_t *handle)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    uint8_t i;
    bool moduleEnabled;
    uint32_t timeoutCnt = FLEXCAN_TIMEOUT_COUNTER;
    flexcan_status_e retVal = FLEXCAN_SUCCESS;

    /* The module must should be first exit from low power
     * mode, and then soft reset can be applied.
     */
    if (base->MCR & CAN_MCR_MDIS_MASK) {
        moduleEnabled = false;
        (void)flexcan_enable(handle, true);
    } else {
        /* Enter Disable mode and then exit,
         * to force exiting from any low power mode.
         */
        (void)flexcan_enable(handle, false);
        (void)flexcan_enable(handle, true);
        moduleEnabled = true;
    }

#if (FLEXCAN_HAS_DOZE_MODE_SUPPORT != 0)
    /* De-ASSERT DOZE Enable Bit. */
    base->MCR &= ~CAN_MCR_DOZE_MASK;
#endif

    /* Assert Soft Reset Signal. */
    base->MCR |= CAN_MCR_SOFTRST_MASK;

    /* Wait until FlexCAN reset completes. */
    while ((base->MCR & CAN_MCR_SOFTRST_MASK) && (timeoutCnt > 0)) {
        timeoutCnt--;
    }

    if (base->MCR & CAN_MCR_SOFTRST_MASK) {
        retVal = FLEXCAN_FAIL;
        ssdk_printf(SSDK_ERR, "\r\nFlexCAN reset operation timeout\r\n");
        return retVal;
    }

    /* Reset MCR rigister. */
#if FLEXCAN_HAS_GLITCH_FILTER
    base->MCR |= CAN_MCR_WRNEN_MASK | CAN_MCR_WAKSRC_MASK |
                 CAN_MCR_MAXMB(FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER - 1);
#else
    base->MCR |= CAN_MCR_WRNEN_MASK |
                 CAN_MCR_MAXMB(FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER - 1);
#endif

    /* Reset CTRL1 and CTRL2 rigister. */
    base->CTRL1 = 0U;
    base->CTRL2 =
        CAN_CTRL2_TASD(0x16) | CAN_CTRL2_RRS_MASK | CAN_CTRL2_EACEN_MASK;

    /* Clean all individual Rx Mask of Message Buffers. */
    for (i = 0; i < FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER; i++) {
        base->RXIMR[i] = 0x3FFFFFFF;
    }

    /* Enable unrestricted write access to FlexCAN memory. */
    base->CTRL2 |= CAN_CTRL2_WRMFRZ_MASK;

    /* avoid ecc error, clean reserved area 96Byte */
    for (uint8_t i = 0; i < 24; i++) {
        *(((uint32_t *)&base->RESERVED_4[0]) + i) = 0x0U;
    }

    /* Clean Global Mask of Message Buffers. */
    base->RXMGMASK = 0x3FFFFFFF;
    /* Clean Global Mask of Message Buffer 14. */
    base->RX14MASK = 0x3FFFFFFF;
    /* Clean Global Mask of Message Buffer 15. */
    base->RX15MASK = 0x3FFFFFFF;
    /* Clean Global Mask of Rx FIFO. */
    base->RXFGMASK = 0x3FFFFFFF;

    /* Reset FDCTRL register. */
    base->FDCTRL = 0x80000000;

    /* Clean Message Buffer region. */
    for (i = 0; i < FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER; i++) {
        base->MB[i].CS = 0x0;
        base->MB[i].ID = 0x0;
        base->MB[i].WORD0 = 0x0;
        base->MB[i].WORD1 = 0x0;
    }

    /* Clean CAN FD SMB region. */
    flexcan_fd_clean_smb_region(base);

    /* Enable write access restriction. */
    base->CTRL2 &= ~CAN_CTRL2_WRMFRZ_MASK;

    if (!moduleEnabled) {
        retVal = flexcan_enable(handle, true);
    }

    return retVal;
}

void flexcan_classic_set_timing_config(flexcan_handle_t *handle,
                                       const flexcan_timing_config_t *config)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(config && base);

    /* Enter Freeze Mode. */
    (void)flexcan_enter_freeze_mode(base, true);

    if (!flexcan_ext_nominal_bit_timing(config)) {
        base->CBT &= ~CAN_CBT_BTF_MASK;

        /* Cleaning previous Timing Setting. */
        base->CTRL1 &= ~(CAN_CTRL1_PRESDIV_MASK | CAN_CTRL1_RJW_MASK |
                         CAN_CTRL1_PSEG1_MASK | CAN_CTRL1_PSEG2_MASK |
                         CAN_CTRL1_PROPSEG_MASK);

        /* Updating Timing Setting according to configuration structure. */
        base->CTRL1 |= (CAN_CTRL1_PRESDIV(config->preDivider - 1U) |
                        CAN_CTRL1_RJW(config->rJumpwidth - 1U) |
                        CAN_CTRL1_PROPSEG(config->propSeg - 1U) |
                        CAN_CTRL1_PSEG1(config->phaseSeg1 - 1U) |
                        CAN_CTRL1_PSEG2(config->phaseSeg2 - 1U));
    } else {
        if ((config->preDivider - 1 > 0x3FFU) ||
            (config->rJumpwidth - 1 > 0x1FU) ||
            (config->propSeg - 1 > 0x3FU) ||
            (config->phaseSeg1 - 1 > 0x1FU ||
            (config->phaseSeg2 - 1 > 0x1FU))) {
            ssdk_printf(SSDK_ERR, "\r\nwrong arbitration phase Timing Setting\r\n");
        }

        /* Cleaning previous arbitration phase Timing Setting. */
        base->CBT &=
            ~(CAN_CBT_EPRESDIV_MASK | CAN_CBT_ERJW_MASK | CAN_CBT_EPSEG1_MASK |
              CAN_CBT_EPSEG2_MASK | CAN_CBT_EPROPSEG_MASK);

        /* Updating arbitration phase Timing Setting according to configuration
         * structure. */
        base->CBT |= (CAN_CBT_BTF(1U) | /* Use CBT instead of CTRL1. */
                      CAN_CBT_EPRESDIV(config->preDivider - 1U) |
                      CAN_CBT_ERJW(config->rJumpwidth - 1U) |
                      CAN_CBT_EPROPSEG(config->propSeg - 1U) |
                      CAN_CBT_EPSEG1(config->phaseSeg1 - 1U) |
                      CAN_CBT_EPSEG2(config->phaseSeg2 - 1U));
    }

    /* Exit Freeze Mode. */
    (void)flexcan_exit_freeze_mode(base, true);
}

void flexcan_fd_set_timing_config(
    flexcan_handle_t *handle, const flexcan_timing_config_t *arbitrPhaseConfig,
    const flexcan_timing_config_t *dataPhaseConfig)
{
    /* Assertion. */
    ASSERT(handle && arbitrPhaseConfig && dataPhaseConfig);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    /* Enter Freeze Mode. */
    (void)flexcan_enter_freeze_mode(base, true);

    if ((arbitrPhaseConfig->preDivider - 1 > 0x3FFU) ||
        (arbitrPhaseConfig->rJumpwidth - 1 > 0x1FU) ||
        (arbitrPhaseConfig->propSeg - 1 > 0x3FU) ||
        (arbitrPhaseConfig->phaseSeg1 - 1 > 0x1FU ||
        (arbitrPhaseConfig->phaseSeg2 - 1 > 0x1FU))) {
        ssdk_printf(SSDK_ERR, "\r\nwrong arbitration phase Timing Setting\r\n");
    }

    if ((dataPhaseConfig->preDivider - 1 > 0x3FFU) ||
        (dataPhaseConfig->rJumpwidth - 1 > 0x7U) ||
        (dataPhaseConfig->propSeg > 0x1FU) ||
        (dataPhaseConfig->phaseSeg1 - 1 > 0x7U ||
        (dataPhaseConfig->phaseSeg2 - 1 > 0x7U))) {
        ssdk_printf(SSDK_ERR, "\r\nwrong data phase Timing Setting\r\n");
    }

    /* Cleaning previous arbitration phase Timing Setting. */
    base->CBT &=
        ~(CAN_CBT_EPRESDIV_MASK | CAN_CBT_ERJW_MASK | CAN_CBT_EPSEG1_MASK |
          CAN_CBT_EPSEG2_MASK | CAN_CBT_EPROPSEG_MASK);

    /* Updating arbitration phase Timing Setting according to configuration
     * structure. */
    base->CBT |= (CAN_CBT_BTF(1U) | /* Use CBT instead of CTRL1. */
                  CAN_CBT_EPRESDIV(arbitrPhaseConfig->preDivider - 1U) |
                  CAN_CBT_ERJW(arbitrPhaseConfig->rJumpwidth - 1U) |
                  CAN_CBT_EPROPSEG(arbitrPhaseConfig->propSeg - 1U) |
                  CAN_CBT_EPSEG1(arbitrPhaseConfig->phaseSeg1 - 1U) |
                  CAN_CBT_EPSEG2(arbitrPhaseConfig->phaseSeg2 - 1U));

    /* Cleaning previous data phase Timing Setting. */
    base->FDCBT &= ~(CAN_FDCBT_FPRESDIV_MASK | CAN_FDCBT_FRJW_MASK |
                     CAN_FDCBT_FPSEG1_MASK | CAN_FDCBT_FPSEG2_MASK |
                     CAN_FDCBT_FPROPSEG_MASK);

    /* Updating data phase Timing Setting according to configuration structure.
     */
    base->FDCBT |= (CAN_FDCBT_FPRESDIV(dataPhaseConfig->preDivider - 1U) |
                    CAN_FDCBT_FRJW(dataPhaseConfig->rJumpwidth - 1U) |
                    CAN_FDCBT_FPROPSEG(dataPhaseConfig->propSeg) |
                    CAN_FDCBT_FPSEG1(dataPhaseConfig->phaseSeg1 - 1U) |
                    CAN_FDCBT_FPSEG2(dataPhaseConfig->phaseSeg2 - 1U));

    /* Exit Freeze Mode. */
    (void)flexcan_exit_freeze_mode(base, true);
}

void flexcan_read_wmb(flexcan_handle_t *handle, uint8_t wmbIndex,
                      flexcan_frame_t *wmb)
{
    /* Assertion. */
    ASSERT(handle && wmb);
    ASSERT(wmbIndex < FLEXCAN_HAS_WAKEUP_MESSAGE_BUFFER_NUMBER);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);
    uint32_t *payloadAddr;
    uint32_t cs_temp;

    cs_temp = base->WMB[wmbIndex].WMB_CS;
    wmb->format = (cs_temp & CAN_WMBn_CS_IDE_MASK) >> CAN_WMBn_CS_IDE_SHIFT;
    wmb->type = (cs_temp & CAN_WMBn_CS_RTR_MASK) >> CAN_WMBn_CS_RTR_SHIFT;
    wmb->length = (cs_temp & CAN_WMBn_CS_DLC_MASK) >> CAN_WMBn_CS_DLC_SHIFT;

    if (wmb->format) {
        wmb->id = base->WMB[wmbIndex].WMB_ID;
    } else {
        wmb->id = (base->WMB[wmbIndex].WMB_ID >> CAN_ID_STD_SHIFT);
    }

    /* store wakeup message payload. */
    payloadAddr = (uint32_t *)(&base->WMB[wmbIndex].WMB_D03);
    flexcan_copy_from_mb(payloadAddr, wmb->dataBuffer, wmb->length);
}

void flexcan_deinit(flexcan_handle_t *handle)
{
    ASSERT(handle);

    /* Clear MB-used bitmap. */
    handle->mb_used_mask[0] = 0x0;
    handle->mb_used_mask[1] = 0x0;

    /* Reset all Register Contents. */
    (void)flexcan_reset(handle);

    /* Disable FlexCAN module. */
    (void)flexcan_enable(handle, false);

    for (size_t i = 0; i < FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER; i++) {
        handle->mbState[i] = FLEXCAN_StateIdle;
    }

    handle->rxFifoState = FLEXCAN_StateIdle;
    uint8_t soc_info_irq_idx_start = 0, soc_info_irq_idx_end = 0;
    uint32_t i = 0;
	if(handle->controller_id == 0){
        soc_info_irq_idx_start = NXP_CAN_IRQn;
        soc_info_irq_idx_end = NXP_CAN_IRQn;
	}
#if (defined(NXP_CSI_FLEXCAN_XFER_POLL) && !NXP_CSI_FLEXCAN_XFER_POLL)
    for(i=soc_info_irq_idx_start; i<=soc_info_irq_idx_end; i++)
    {
        csi_irq_detach(i);
        csi_irq_disable(i);
    }
#endif
}

flexcan_status_e flexcan_request_sleep(flexcan_handle_t *handle, bool request)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    uint32_t timeoutCnt = FLEXCAN_TIMEOUT_COUNTER;
    flexcan_status_e retVal = FLEXCAN_SUCCESS;

    /* {lowpower_en, register mode} */
    ///base->LOWPOWER_MODE = (!!request) << 1 | 0x1;

    if (request) 
	{
        
#if 1
        flexcan_freeze(handle, true);
        base->MCR |= CAN_MCR_PNET_EN_MASK;
        /* Enable Doze Mode? */
        if (1/*config->enableDoze*/) 
        {
            base->MCR |= CAN_MCR_DOZE_MASK;
            handle->enableDoze = true;
        }
        
        timeoutCnt = 1000;
        while(timeoutCnt--){;}
        flexcan_freeze(handle, false);
        
        timeoutCnt = 1000;
        while(timeoutCnt--){;}
        
        /* sys regs ON */
        //*(volatile unsigned int *)(0x50008800) = 0x109;
        canfd_pmu_cfg_doze_en(handle);
        
        timeoutCnt = 1000;
        while(timeoutCnt--){;}
#endif

        timeoutCnt = FLEXCAN_TIMEOUT_COUNTER;
        
        /* Wait FlexCAN enter low-power mode. */
        while (!(base->MCR & CAN_MCR_LPMACK_MASK) && (timeoutCnt > 0)) {
            timeoutCnt--;
        }

        if (!(base->MCR & CAN_MCR_LPMACK_MASK)) {
            retVal = FLEXCAN_FAIL;
            ssdk_printf(SSDK_ERR, "\r\nEnter low-power mode timeout\r\n");
        }
    }
	else
	{
#if FLEXCAN_HAS_DOZE_MODE_SUPPORT
        if (handle->enableDoze) {
            base->MCR |= CAN_MCR_DOZE_MASK;
        }
#endif

#if 1
        /* sys regs ON */
        //*(volatile unsigned int *)(0x50008800) = 0x101;
        canfd_pmu_cfg_default(handle);
        
        timeoutCnt = 1000;
        while(timeoutCnt--){;}
        timeoutCnt = FLEXCAN_TIMEOUT_COUNTER;
#endif 

        /* Wait FlexCAN exit from low-power mode. */
        while ((base->MCR & CAN_MCR_LPMACK_MASK) && (timeoutCnt > 0)) {
            timeoutCnt--;
        }

        if (base->MCR & CAN_MCR_LPMACK_MASK) {
            retVal = FLEXCAN_FAIL;
            ssdk_printf(SSDK_ERR, "\r\nExit low-power mode timeout\r\n");
        }
    }

    return retVal;
}

void flexcan_set_rx_mb_global_mask(flexcan_handle_t *handle, uint32_t mask)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    /* Enter Freeze Mode. */
    (void)flexcan_enter_freeze_mode(base, true);

    /* Setting Global Mask value. */
    base->RXMGMASK = mask;
    base->RX14MASK = mask;
    base->RX15MASK = mask;

    /* Exit Freeze Mode. */
    (void)flexcan_exit_freeze_mode(base, true);
}

void flexcan_set_rx_fifo_global_mask(flexcan_handle_t *handle, uint32_t mask)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    /* Enter Freeze Mode. */
    (void)flexcan_enter_freeze_mode(base, true);

    /* Setting Rx FIFO Global Mask value. */
    base->RXFGMASK = mask;

    /* Exit Freeze Mode. */
    (void)flexcan_exit_freeze_mode(base, true);
}

void flexcan_set_rx_individual_mask(flexcan_handle_t *handle, uint8_t maskIdx,
                                    uint32_t mask)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);
    ASSERT(maskIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));

    /* Enter Freeze Mode. */
    (void)flexcan_enter_freeze_mode(base, true);

    /* Setting Rx Individual Mask value. */
    base->RXIMR[maskIdx] = mask;

    /* Exit Freeze Mode. */
    (void)flexcan_exit_freeze_mode(base, true);
}

void flexcan_set_tx_mb_config(flexcan_handle_t *handle, uint8_t mbIdx,
                              bool tx_by_interrupt)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);

    ASSERT(base);
#if 1
    ASSERT(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
#else
    assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
#endif
    ASSERT(!flexcan_is_mb_occupied(handle, mbIdx));

    uint32_t *msgBufAddr;

    /* Get message buffer address. */
    msgBufAddr = flexcan_get_msg_buf_addr(base, mbIdx);

    /* CS filed: Inactivate Message Buffer. */
    mb_cs_field(msgBufAddr) = 0U;

    /* Clean ID filed. */
    mb_id_field(msgBufAddr) = 0U;

    /* Tx by interrupt or polling? */
    if (tx_by_interrupt) {
        /* Enable Message Buffer Interrupt. */
        flexcan_enable_mb_int(base, mbIdx);
    } else {
        /* Disable Message Buffer Interrupt. */
        flexcan_disable_mb_int(base, mbIdx);
    }

    /* Set MB is used. */
    handle->mb_used_mask[BITMAP_INT(mbIdx)] |= 1U << BITMAP_BIT_IN_INT(mbIdx);
}

void flexcan_set_rx_mb_config(flexcan_handle_t *handle, uint8_t mbIdx,
                              const flexcan_rx_mb_config_t *config)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    ASSERT(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    ASSERT(!flexcan_is_mb_occupied(handle, mbIdx));
    ASSERT(config);

    uint32_t *msgBufAddr;
    uint32_t cs_temp = 0U;
    /* Get message buffer address. */
    msgBufAddr = flexcan_get_msg_buf_addr(base, mbIdx);

    /* CS field: Inactivate Message Buffer. */
    mb_cs_field(msgBufAddr) = 0U;

    /* ID field: Clean Message Buffer content. */
    mb_id_field(msgBufAddr) = 0U;

    if (config->format == FLEXCAN_EXTEND_FRAME) {
        /* Setup Message Buffer ID. */
        mb_id_field(msgBufAddr) =
            (config->id) & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK);
        /* Setup Message Buffer format. */
        cs_temp |= CAN_CS_IDE_MASK;
    } else {
        /* Setup Message Buffer ID. */
        mb_id_field(msgBufAddr) =
            ((config->id) << CAN_ID_STD_SHIFT) & CAN_ID_STD_MASK;
    }

    /* Activate Rx Message Buffer. */
    cs_temp |= CAN_CS_CODE(FLEXCAN_RxMbEmpty);
    mb_cs_field(msgBufAddr) = cs_temp;

    /* Set MB is used. */
    handle->mb_used_mask[BITMAP_INT(mbIdx)] |= 1U << BITMAP_BIT_IN_INT(mbIdx);
}

void flexcan_set_rx_fifo_config(flexcan_handle_t *handle,
                                const flexcan_rx_fifo_config_t *config)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    ASSERT(config);
    ASSERT(config->idFilterNum <= 128);

    /* Rx FIFO can only be enabled when CAN FD feature is disabled. */
    ASSERT(!(base->MCR & CAN_MCR_FDEN_MASK));

    volatile uint32_t *idFilterRegion = (volatile uint32_t *)(&base->MB[6].CS);
    uint8_t setup_mb, i, rffn = 0;

    /* Get the setup_mb value. */
    setup_mb = (base->MCR & CAN_MCR_MAXMB_MASK) >> CAN_MCR_MAXMB_SHIFT;
    setup_mb = (setup_mb < FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER)
                   ? setup_mb
                   : FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER;

    /* Enter Freeze Mode. */
    (void)flexcan_enter_freeze_mode(base, true);

    /* Determine RFFN value. */
    for (i = 0; i <= 0xF; i++) {
        if ((8 * (i + 1)) >= config->idFilterNum) {
            rffn = i;
            ASSERT(((setup_mb - 8) - (2 * rffn)) > 0);

            base->CTRL2 =
                (base->CTRL2 & ~CAN_CTRL2_RFFN_MASK) | CAN_CTRL2_RFFN(rffn);
            break;
        }
    }

    /* Clean ID filter table occuyied Message Buffer Region. */
    rffn = (rffn + 1) * 8;

    for (i = 0; i < rffn; i++) {
        idFilterRegion[i] = 0x0;
    }

    /* Disable unused Rx FIFO Filter. */
    for (i = config->idFilterNum; i < rffn; i++) {
        idFilterRegion[i] = 0xFFFFFFFFU;
    }

    /* Copy ID filter table to Message Buffer Region. */
    for (i = 0; i < config->idFilterNum; i++) {
        idFilterRegion[i] = config->filter_tab[i].filter_code;
        ssdk_printf(SSDK_DEBUG, "\r\nCAN Rx FIFO ID filter table[%d] = %x\r\n", i,
                    idFilterRegion[i]);
    }

    /* Setup ID Fitlter Type. */
    switch (config->idFilterType) {
    case FLEXCAN_RxFifoFilterTypeA:
        base->MCR = (base->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x0);
        break;

    case FLEXCAN_RxFifoFilterTypeB:
        base->MCR = (base->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x1);
        break;

    case FLEXCAN_RxFifoFilterTypeC:
        base->MCR = (base->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x2);
        break;

    case FLEXCAN_RxFifoFilterTypeD:
        /* All frames rejected. */
        base->MCR = (base->MCR & ~CAN_MCR_IDAM_MASK) | CAN_MCR_IDAM(0x3);
        break;

    default:
        break;
    }

    /* Setting Message Reception Priority. */
    if (config->priority == FLEXCAN_RxFifoPrioHigh) {
        /* Matching starts from Rx FIFO and continues on Mailboxes. */
        base->CTRL2 &= ~CAN_CTRL2_MRP_MASK;
    } else {
        /* Matching starts from Mailboxes and continues on Rx FIFO. */
        base->CTRL2 |= CAN_CTRL2_MRP_MASK;
    }

    /* Enable Rx Message FIFO. */
    base->MCR |= CAN_MCR_RFEN_MASK;

    /* Exit Freeze Mode. */
    (void)flexcan_exit_freeze_mode(base, true);

    /* Set MB 5&6&7 is used. */
    handle->mb_used_mask[0] |= (1U << 5) | (1U << 6) | (1U << 7);
}

void flexcan_create_handle(flexcan_handle_t *handle, uint8_t controller_id,
                           void *reg_base, uint8_t irq_num,
                           flexcan_transfer_callback_t callback, void *userData)
{
    ASSERT(handle);
    ASSERT(controller_id < MAX_FLEXCAN_CH);
    ASSERT(reg_base);

    /* Clean FlexCAN transfer handle. */
    memset(handle, 0, sizeof(*handle));

    handle->controller_id = controller_id;
    handle->irq_num = irq_num;
    /* Register base address. */
    handle->base_addr = reg_base;

    /* Register Callback function. */
    handle->callback = callback;
    handle->userData = userData;
}

void flexcan_enable_interrupts(flexcan_handle_t *handle, uint32_t mask)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);
    
#if 0 //Ori
    /* Solve Wake Up Interrupt. */
    if (mask & FLEXCAN_WakeUpInterruptEnable) {
        base->MCR |= CAN_MCR_WAKMSK_MASK;
    }

    /* Solve others. */
    base->CTRL1 |= (mask & (~((uint32_t)FLEXCAN_WakeUpInterruptEnable)));
#else
    if((mask & FLEXCAN_NonCTRL1AndMCRInterruptMASK) != FLEXCAN_NonCTRL1AndMCRInterruptMASK)
    {

        /* Solve Wake Up Interrupt. */
        if (mask & FLEXCAN_WakeUpInterruptEnable) {
            base->MCR |= CAN_MCR_WAKMSK_MASK;
        }

        /* Solve others. */
        base->CTRL1 |= (mask & (~((uint32_t)FLEXCAN_WakeUpInterruptEnable)));
    
    }
    else {
#if (defined(NXP_FLEXCAN_BUSOFF_DONE) && NXP_FLEXCAN_BUSOFF_DONE)
    base->CTRL2 |= (CAN_CTRL2_BOFFDONEMSK(0x1));
#endif

///NXP_FLEXCAN_HIGH_TIMER_WRAP_INT  FLEXCAN_HighTimerWarpInterruptEnable
#if (defined(NXP_FLEXCAN_HIGH_TIMER_WRAP_INT) && NXP_FLEXCAN_HIGH_TIMER_WRAP_INT)
    base->ESR2 |= (CAN_ESR2_HTWAINT_SET(0x1));
#endif
    }
#endif

    return;
}

void flexcan_disable_interrupts(flexcan_handle_t *handle, uint32_t mask)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    /* Solve Wake Up Interrupt. */
    if (mask & FLEXCAN_WakeUpInterruptEnable) {
        base->MCR &= ~CAN_MCR_WAKMSK_MASK;
    }

    /* Solve others. */
    base->CTRL1 &= ~(mask & (~((uint32_t)FLEXCAN_WakeUpInterruptEnable)));
}

uint32_t flexcan_get_mb_iflag(flexcan_handle_t *handle, uint8_t msgBufId)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    uint32_t int_flag = 0;

    if (msgBufId <= 31U) {
        int_flag = (base->IFLAG1 >> msgBufId) & 1U;
    } else if (msgBufId <= 63U) {
        int_flag = (base->IFLAG2 >> (msgBufId % 32U)) & 1U;
    } else if (msgBufId <= 95U) {
        int_flag = (base->IFLAG3 >> (msgBufId % 32U)) & 1U;
    } else if (msgBufId <= 127U) {
        int_flag = (base->IFLAG4 >> (msgBufId % 32U)) & 1U;
    } else {
        /* Invalid msgBufId value means check if any message buffer
           interrupt is active. */
        int_flag = base->IFLAG1;
        int_flag |= base->IFLAG2;
        int_flag |= base->IFLAG3;
        int_flag |= base->IFLAG4;
    }

    return int_flag;
}

flexcan_status_e flexcan_send_nonblocking(flexcan_handle_t *handle,
                                          flexcan_mb_transfer_t *xfer,
                                          uint8_t padding_val)
{
    /* Assertion. */
    ASSERT(handle);
    ASSERT(xfer);

	volatile flexcan_status_e sta;
    __attribute__((unused))  CAN_Type *base = (CAN_Type *)(handle->base_addr);

    ASSERT(xfer->mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    ASSERT(!flexcan_is_mb_occupied(handle, xfer->mbIdx));

    /* Check if Message Buffer is idle. */
    if (FLEXCAN_StateIdle == handle->mbState[xfer->mbIdx]) {
        /* Distinguish transmit type. */
        if (FLEXCAN_FrameTypeRemote == xfer->pFrame->type) {
            handle->mbState[xfer->mbIdx] = FLEXCAN_StateTxRemote;

            /* Register user Frame buffer to receive remote Frame. */
            handle->pMBFrameBuf[xfer->mbIdx] = xfer->pFrame;
        } else {
            handle->mbState[xfer->mbIdx] = FLEXCAN_StateTxData;
        }
        
        sta = flexcan_write_tx_mb(handle, xfer->mbIdx, xfer->pFrame, padding_val);

        if (FLEXCAN_SUCCESS == sta)
        {
            return FLEXCAN_SUCCESS;
        }
        else 
        {
            handle->mbState[xfer->mbIdx] = FLEXCAN_StateIdle;
            return FLEXCAN_FAIL;
        }
    }
    else 
    {
        return FLEXCAN_TX_BUSY;
    }
}

flexcan_status_e flexcan_receive_nonblocking(flexcan_handle_t *handle,
                                             flexcan_mb_transfer_t *xfer)
{
    /* Assertion. */
    ASSERT(handle);

    CAN_Type *base = (CAN_Type *)(handle->base_addr);

    /* Assertion. */
    ASSERT(xfer);
    ASSERT(xfer->mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    ASSERT(!flexcan_is_mb_occupied(handle, xfer->mbIdx));

    /* Check if Message Buffer is idle. */
    if (FLEXCAN_StateIdle == handle->mbState[xfer->mbIdx]) {
        handle->mbState[xfer->mbIdx] = FLEXCAN_StateRxData;

        /* Register Message Buffer. */
        handle->pMBFrameBuf[xfer->mbIdx] = xfer->pFrame;

#if (defined(NXP_CSI_FLEXCAN_XFER_POLL) && !NXP_CSI_FLEXCAN_XFER_POLL)
        /* Enable Message Buffer Interrupt. */
        flexcan_enable_mb_int(base, xfer->mbIdx);
#endif

        return FLEXCAN_SUCCESS;
    } else {
        return FLEXCAN_RX_BUSY;
    }
}

flexcan_status_e flexcan_receive_fifo_nonblocking(flexcan_handle_t *handle,
                                                  flexcan_fifo_transfer_t *xfer)
{
    /* Assertion. */
    ASSERT(handle);
    ASSERT(xfer);

    CAN_Type *base = (CAN_Type *)(handle->base_addr);

    /* Check if Message Buffer is idle. */
    if (FLEXCAN_StateIdle == handle->rxFifoState) {
        handle->rxFifoState = FLEXCAN_StateRxFifo;

        /* Register Message Buffer. */
        handle->pRxFifoFrameBuf = xfer->pFrame;

        /* Enable Message Buffer Interrupt. */
        flexcan_enable_mb_int(base, RX_FIFO_FRAME_AVL_MB_ID);
        flexcan_enable_mb_int(base, RX_FIFO_ALMOST_FULL_MB_ID);
        flexcan_enable_mb_int(base, RX_FIFO_OVERFLOW_MB_ID);

        return FLEXCAN_SUCCESS;
    } else {
        return FLEXCAN_RX_FIFO_BUSY;
    }
}

flexcan_status_e flexcan_write_tx_mb(flexcan_handle_t *handle, uint8_t mbIdx,
                                     const flexcan_frame_t *txFrame,
                                     uint8_t padding_val)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    ASSERT(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    ASSERT(!flexcan_is_mb_occupied(handle, mbIdx));
    ASSERT(txFrame);
    #if 0
    ASSERT(((!txFrame->isCANFDFrame) && (txFrame->length <= 8)) ||
           ((txFrame->isCANFDFrame) && (txFrame->length <= 64)));
    #else
    
    assert(((!txFrame->isCANFDFrame) && (txFrame->length <= 8)) ||
           ((txFrame->isCANFDFrame) && (txFrame->length <= 64)));
        
    if(txFrame->isCANFDFrame)
    {
        if(!(txFrame->length <= 64))
        {
            ASSERT(NULL);
        }
    }
    else 
    {
        if(!(txFrame->length <= 8))
        {
            ASSERT(NULL);
        }
    }
    #endif

    uint32_t cs_temp = 0;
    uint32_t *msgBufAddr = flexcan_get_msg_buf_addr(base, mbIdx);
    uint8_t dlc_val = flexcan_compute_dlc_val(txFrame->length);
    uint8_t payload_len = flexcan_compute_payload_len(dlc_val);
    uint32_t *mb_data = mb_data_field(msgBufAddr);
	
#if (defined(NXP_CSI_FLEXCAN_DEBUG_SW) && NXP_CSI_FLEXCAN_DEBUG_SW)
	///for test
	// static volatile unsigned int tx_data_sta = CAN_CS_CODE(FLEXCAN_TxMbDataOrRemote);
	// static volatile unsigned int tx_mb_sta = 0;
	// tx_mb_sta = (mb_cs_field(msgBufAddr) & CAN_CS_CODE_MASK);
#endif

    /* Check if Message Buffer is activated. */
    if (CAN_CS_CODE(FLEXCAN_TxMbDataOrRemote) !=
        (mb_cs_field(msgBufAddr) & CAN_CS_CODE_MASK)) {
        /* Inactive Tx Message Buffer. */
        mb_cs_field(msgBufAddr) =
            (mb_cs_field(msgBufAddr) & ~CAN_CS_CODE_MASK) |
            CAN_CS_CODE(FLEXCAN_TxMbInactive);

        if (FLEXCAN_EXTEND_FRAME == txFrame->format) {
            /* Fill Message ID field. */
            mb_id_field(msgBufAddr) = txFrame->id;
            /* Fill Message Format field. */
            cs_temp |= CAN_CS_SRR_MASK | CAN_CS_IDE_MASK;
        } else {
            /* Fill Message ID field. */
            mb_id_field(msgBufAddr) =
                ((txFrame->id) << CAN_ID_STD_SHIFT) & CAN_ID_STD_MASK;
        }

        /* Fill Message Type field. */
        if (txFrame->isCANFDFrame) {
            cs_temp |= CAN_CS_EDL_MASK | CAN_CS_SRR_MASK;

            if (txFrame->isCANFDBrsEn) {
                cs_temp |= CAN_CS_BRS_MASK;
            }
        } else {
            if (txFrame->type == FLEXCAN_FrameTypeRemote) {
                cs_temp |= CAN_CS_RTR_MASK;
            }
        }

        if (txFrame->length < payload_len) {
            /* Pad unspecified data in CAN FD frames > 8 bytes. */
            memset((void *)(txFrame->dataBuffer + txFrame->length), padding_val,
                   payload_len - txFrame->length);
        }
        cs_temp |= CAN_CS_CODE(FLEXCAN_TxMbDataOrRemote) | CAN_CS_DLC(dlc_val);

        flexcan_copy_to_mb(mb_data, (uint8_t *)txFrame->dataBuffer,
                           payload_len);

        /* Activate Tx Message Buffer. */
        mb_cs_field(msgBufAddr) = cs_temp;

        return FLEXCAN_SUCCESS;
    } else {
        /* Tx Message Buffer is activated, return immediately. */
        ssdk_printf(SSDK_DEBUG, "Write MB[%d] failed, MB cs field = 0x%x\r\n",
                    mbIdx, mb_cs_field(msgBufAddr));
        return FLEXCAN_FAIL;
    }
}

flexcan_status_e flexcan_read_rx_mb(flexcan_handle_t *handle, uint8_t mbIdx,
                                    flexcan_frame_t *rxFrame)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    ASSERT(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    ASSERT(!flexcan_is_mb_occupied(handle, mbIdx));
    ASSERT(rxFrame);

    uint32_t cs_temp;
    uint8_t rx_code;
    uint32_t *msgBufAddr = flexcan_get_msg_buf_addr(base, mbIdx);
    uint8_t rx_len;
    uint32_t *mb_data = mb_data_field(msgBufAddr);

    /* Read CS field of Rx Message Buffer to lock Message Buffer. */
    cs_temp = mb_cs_field(msgBufAddr);
    /* Get Rx Message Buffer Code field. */
    rx_code = (cs_temp & CAN_CS_CODE_MASK) >> CAN_CS_CODE_SHIFT;

    /* Check to see if Rx Message Buffer is busy. */
    if ((rx_code == FLEXCAN_RxMbFull) || (rx_code == FLEXCAN_RxMbOverrun)) {
        /* Get the message ID and format. */
        if (cs_temp & CAN_CS_IDE_MASK) {
            /* Solve Extend ID. */
            rxFrame->format = FLEXCAN_EXTEND_FRAME;
            /* Store Message ID. */
            rxFrame->id =
                mb_id_field(msgBufAddr) & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK);
        } else {
            /* Solve Standard ID. */
            rxFrame->format = FLEXCAN_STANDARD_FRAME;
            /* Store Message ID. */
            rxFrame->id =
                (mb_id_field(msgBufAddr) & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT;
        }

        if (cs_temp & CAN_CS_EDL_MASK) {
            rxFrame->isCANFDFrame = true;
            /* CAN FD doesn't support remote frame. */
            rxFrame->type = FLEXCAN_FrameTypeData;
            if (cs_temp & CAN_CS_BRS_MASK) {
                rxFrame->isCANFDBrsEn = 1;
            } else {
                rxFrame->isCANFDBrsEn = 0;
            }
        } else {
            rxFrame->isCANFDFrame = false;
            /* Get the message type. */
            if (cs_temp & CAN_CS_RTR_MASK) {
                rxFrame->type = FLEXCAN_FrameTypeRemote;
            } else {
                rxFrame->type = FLEXCAN_FrameTypeData;
            }
        }

        /* Get the message length. */
        rx_len = (cs_temp & CAN_CS_DLC_MASK) >> CAN_CS_DLC_SHIFT;
        rxFrame->length = flexcan_compute_payload_len(rx_len);

        /* Store Message Payload. */
        flexcan_copy_from_mb(mb_data, rxFrame->dataBuffer, rxFrame->length);

        /* Read free-running timer to unlock Rx Message Buffer. */
        (void)base->TIMER;

        /* Set Rx Message Buffer to Empty. */
        mb_cs_field(msgBufAddr) =
            (mb_cs_field(msgBufAddr) & ~CAN_CS_CODE_MASK) |
            CAN_CS_CODE(FLEXCAN_RxMbEmpty);

        if (rx_code == FLEXCAN_RxMbFull) {
            return FLEXCAN_SUCCESS;
        } else {
            ///tim
            if(rx_code == FLEXCAN_RxMbOverrun)
            {
                return FLEXCAN_SUCCESS;
            }
            
            return FLEXCAN_RX_OVERFLOW;
        }
    } else {
        /* Read free-running timer to unlock Rx Message Buffer. */
        (void)base->TIMER;

        ssdk_printf(SSDK_DEBUG,
                    "Read CAN MB[%d] failed, MB cs field = 0x%x\r\n", mbIdx,
                    mb_cs_field(msgBufAddr));
        return FLEXCAN_FAIL;
    }
}

flexcan_status_e flexcan_read_rx_fifo(flexcan_handle_t *handle,
                                      flexcan_frame_t *rxFrame)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base && rxFrame);

    uint32_t cs_temp;
    uint32_t *msgBufAddr = flexcan_get_msg_buf_addr(base, RX_FIFO_MB_ID);
    uint32_t *mb_data = mb_data_field(msgBufAddr);

    /* Check if Rx FIFO is Enabled. */
    if (base->MCR & CAN_MCR_RFEN_MASK) {
        /* Read CS field of Rx Message Buffer to lock Message Buffer. */
        cs_temp = base->MB[0].CS;

        /* Read data from Rx FIFO output port. */
        /* Get the message ID and format. */
        if (cs_temp & CAN_CS_IDE_MASK) {
            /* Solve Extend ID. */
            rxFrame->format = FLEXCAN_EXTEND_FRAME;
            /* Store Message ID. */
            rxFrame->id = base->MB[0].ID & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK);
        } else {
            /* Solve Standard ID. */
            rxFrame->format = FLEXCAN_STANDARD_FRAME;
            /* Store Message ID. */
            rxFrame->id =
                (base->MB[0].ID & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT;
        }

        /* Get the message type. */
        if (cs_temp & CAN_CS_RTR_MASK) {
            rxFrame->type = FLEXCAN_FrameTypeRemote;
        } else {
            rxFrame->type = FLEXCAN_FrameTypeData;
        }

        /* Get the message length. */
        rxFrame->length = (cs_temp & CAN_CS_DLC_MASK) >> CAN_CS_DLC_SHIFT;

        /* Store Message Payload. */
        flexcan_copy_from_mb(mb_data, rxFrame->dataBuffer, rxFrame->length);

        /* Read free-running timer to unlock Rx Message Buffer. */
        (void)base->TIMER;

        /* Store ID Filter Hit Index. */
        rxFrame->idHit = (uint8_t)(base->RXFIR & CAN_RXFIR_IDHIT_MASK);

        return FLEXCAN_SUCCESS;
    } else {
        return FLEXCAN_FAIL;
    }
}

uint8_t flexcan_get_mb_state(flexcan_handle_t *handle, uint8_t mbIdx)
{
    /* Assertion. */
    ASSERT(handle);

    return handle->mbState[mbIdx];
}

bool flexcan_is_mb_occupied(flexcan_handle_t *handle, uint8_t mbIdx)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);
    uint8_t lastOccupiedMb;

    /* Is Rx FIFO enabled? */
    if (base->MCR & CAN_MCR_RFEN_MASK) {
        /* Get RFFN value. */
        lastOccupiedMb =
            ((base->CTRL2 & CAN_CTRL2_RFFN_MASK) >> CAN_CTRL2_RFFN_SHIFT);
        /* Calculate the number of last Message Buffer occupied by Rx FIFO. */
        lastOccupiedMb = ((lastOccupiedMb + 1) * 2) + 5;

        if (mbIdx <= lastOccupiedMb) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

uint32_t flexcan_get_error_status(flexcan_handle_t *handle)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    return (uint32_t)(base->ESR1);
}

void flexcan_clear_err_status_flag(flexcan_handle_t *handle, uint32_t mask)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    base->ESR1 = mask;
}

uint32_t flexcan_read_mb_int_status(flexcan_handle_t *handle, uint8_t mbIdx)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    return flexcan_get_mb_int_status_flag(base, mbIdx);
}

void flexcan_abort_mb_send(flexcan_handle_t *handle, uint8_t mbIdx)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    ASSERT(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    ASSERT(!flexcan_is_mb_occupied(handle, mbIdx));

    /* Disable Message Buffer Interrupt. */
    flexcan_disable_mb_int(base, mbIdx);

    /* Un-register handle. */
    handle->pMBFrameBuf[mbIdx] = 0U;

    /* Clean Message Buffer. */
    flexcan_set_tx_mb_config(handle, mbIdx, false);

    handle->mbState[mbIdx] = FLEXCAN_StateIdle;
}

void flexcan_abort_mb_receive(flexcan_handle_t *handle, uint8_t mbIdx)
{
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    ASSERT(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
    ASSERT(!flexcan_is_mb_occupied(handle, mbIdx));

    /* Disable Message Buffer Interrupt. */
    flexcan_disable_mb_int(base, mbIdx);

    /* Un-register handle. */
    handle->pMBFrameBuf[mbIdx] = 0U;
    handle->mbState[mbIdx] = FLEXCAN_StateIdle;
}

void flexcan_abort_receive_fifo(flexcan_handle_t *handle)
{
    /* Assertion. */
    ASSERT(handle);
    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    ASSERT(base);

    /* Check if Rx FIFO is enabled. */
    if (base->MCR & CAN_MCR_RFEN_MASK) {
        /* Disable Rx Message FIFO Interrupts. */
        flexcan_disable_mb_int(base, FLEXCAN_RxFifoOverflowFlag |
                                         FLEXCAN_RxFifoWarningFlag |
                                         FLEXCAN_RxFifoFrameAvlFlag);

        /* Un-register handle. */
        handle->pRxFifoFrameBuf = 0x0;
    }

    handle->rxFifoState = FLEXCAN_StateIdle;
}

void flexcan_config_rx_mailbox(flexcan_handle_t *handle, uint8_t index,
                                      flexcan_rx_mb_config_t *rxmbcfg)
{
    flexcan_set_rx_mb_config(handle, index, rxmbcfg);

    uint32_t rx_individual_mask;
    if(rxmbcfg->format == FLEXCAN_EXTEND_FRAME){
    	//rx_individual_mask = (0x1FFFFFFF << CAN_ID_EXT_SHIFT) & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK) | (EXTENDED_DATA_ID << 30);
		rx_individual_mask = ((0x1FFFFFFF << CAN_ID_EXT_SHIFT) & (CAN_ID_EXT_MASK | CAN_ID_STD_MASK)) | (EXTENDED_DATA_ID << 30);
    }else{
    	rx_individual_mask = (0x7FF << CAN_ID_STD_SHIFT) & CAN_ID_STD_MASK;
    }
    flexcan_set_rx_individual_mask(handle, index, rx_individual_mask);
}

void flexcan_config_tx_mailbox(flexcan_handle_t *handle,
                                             uint8_t index)
{
    flexcan_set_tx_mb_config(handle, index, true);
}

int flexcan_send_mailbox(flexcan_handle_t *handle, flexcan_mb_transfer_t *xfer)
{
    if (flexcan_get_mb_state(handle, xfer->mbIdx) == FLEXCAN_StateIdle) {
        flexcan_send_nonblocking(handle, xfer, TX_PADDING_VAL);

#if (defined(NXP_CSI_FLEXCAN_XFER_POLL) && NXP_CSI_FLEXCAN_XFER_POLL)
        handle->mbState[xfer->mbIdx] = FLEXCAN_StateIdle;
#endif
    } else {
        ssdk_printf(SSDK_NOTICE,
                "controller_id=%d mb%d is not available for send\r\n",
                handle->controller_id, xfer->mbIdx);
    }
    return 0;
}

#if (defined(NXP_FLEXCAN_USE_DEV_ARR_4_IRQ) && !NXP_FLEXCAN_USE_DEV_ARR_4_IRQ)
#else
#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif
extern uint32_t soc_irq_get_irq_num(void);
//int flexcan_irq_handler(uint32_t irq, void *arg)
int flexcan_irq_handler(void *arg)
{
#if (defined(NXP_FLEXCAN_USE_DEV_ARR_4_IRQ) && !NXP_FLEXCAN_USE_DEV_ARR_4_IRQ)
#if (defined(NXP_FLEXCAN_HIGH_TIMER_WRAP_INT) && NXP_FLEXCAN_HIGH_TIMER_WRAP_INT)
    uint32_t irqn_can = soc_irq_get_irq_num();
#endif
    flexcan_handle_t *handle = (flexcan_handle_t *)arg;
#else
    csi_dev_t *p_csi_dev_t = (csi_dev_t *)arg;
    flexcan_handle_t *handle = (flexcan_handle_t *)container_of (p_csi_dev_t, flexcan_handle_t, csi_dev);
#endif
    
    ASSERT(handle);

    CAN_Type *base = (CAN_Type *)(handle->base_addr);
    #if 0
    //ASSERT(base);
    #else
    assert(base);
    #endif
    uint32_t errStatus = base->ESR1;
    uint8_t mb_num = ((base->MCR) & CAN_MCR_MAXMB_MASK) + 1U;
    flexcan_status_e status;
    uint8_t mbIdx;
    bool get_int_req = false;
    uint32_t int_req = 0U;
    /* Common frame buffer used when no buffer defined by user. */
    uint8_t rxDataBuf[64];
    flexcan_frame_t frameBuf = {.dataBuffer = rxDataBuf};

    /* FlexCAN error interrupt handling. */
    if (errStatus & (FLEXCAN_TxWarningIntFlag | FLEXCAN_RxWarningIntFlag |
                     FLEXCAN_BusOffIntFlag | FLEXCAN_ErrorIntFlag |
                     FLEXCAN_WakeUpIntFlag | FLEXCAN_ErrorFlag | FLEXCAN_BusOffDoneIntFlag)) {
        status = FLEXCAN_ERROR_STATUS;

        /* Calling Callback Function if has one. */
        if (handle->callback != NULL) {
            handle->callback(handle, status, errStatus, handle->userData);
        }

        /* Clear error interrupt flags. */
        flexcan_clear_err_status_flag(
            handle, FLEXCAN_TxWarningIntFlag | FLEXCAN_RxWarningIntFlag |
                        FLEXCAN_BusOffIntFlag | FLEXCAN_ErrorIntFlag |
                        FLEXCAN_WakeUpIntFlag | FLEXCAN_ErrorFlag | FLEXCAN_BusOffDoneIntFlag);
    }
    
#if (defined(NXP_FLEXCAN_HIGH_TIMER_WRAP_INT) && NXP_FLEXCAN_HIGH_TIMER_WRAP_INT)
    if(((irqn_can==NXP_CAN0_IRQn_TIMER) || (irqn_can==NXP_CAN1_IRQn_TIMER)) && (base->ESR2 & CAN_ESR2_HTWAINT_SET_MASK))
    {
        if (handle->callback != NULL) {
                handle->callback(handle, FLEXCAN_HIGH_TIMER_WRAP, 0x0, NULL);
            }
			/* HTWAINT is cleared by writing 1 to it. */
            //base->ESR2 |= CAN_ESR2_HTWAINT_STATE_MASK;
    }
#endif

    if (base->MCR & CAN_MCR_PNET_EN_MASK) {
        if (base->WU_MTC & CAN_WU_MTC_WTOF_MASK) {
            if (handle->callback != NULL) {
                handle->callback(handle, FLEXCAN_WAKEUP_TIMEOUT, 0x0, NULL);
            }
            base->WU_MTC |= CAN_WU_MTC_WTOF_MASK;
        }
        if (base->WU_MTC & CAN_WU_MTC_WUMF_MASK) {
            if (handle->callback != NULL) {
                handle->callback(handle, FLEXCAN_WAKEUP_MATCH, 0x0, NULL);
            }
            base->WU_MTC |= CAN_WU_MTC_WUMF_MASK;
        }
    }
//    uint32_t err_irq_status = base->ERR_IRQ_STATUS;
//
//    if (err_irq_status) {
//        ssdk_printf(SSDK_DEBUG, "ERR_IRQ_STATUS = %#x\r\n", err_irq_status);
//        base->ERR_IRQ_STATUS = err_irq_status;
//        if (err_irq_status & ERR_IRQ_STATUS_PSLVERR_MASK) {
//            ssdk_printf(SSDK_DEBUG,
//                        "access to reserved address results in error\r\n");
//        }
//    }

    /* FlexCAN message buffer interrupt handling. */
    for (mbIdx = 0U; mbIdx < mb_num; mbIdx++) {
        /* Every 32 MBs share one IMASK register and one IFLAG register. */
        if ((mbIdx & 31U) == 0U) {
            get_int_req = true;
        }

        /* A MB is checked only if it is used. */
        if (BIT_SET(handle->mb_used_mask[BITMAP_INT(mbIdx)],
                    BITMAP_BIT_IN_INT(mbIdx))) {
            if (get_int_req) {
                /* Read interrupt status: IMASK & IFLAG. */
                int_req = flexcan_get_mb_int_req(base, mbIdx >> 5);
                get_int_req = false;
            }

            /* Check if the MB's interrupt is generated. */
            if (int_req & (1U << (mbIdx & 31U))) {
                /* Frame buffer defined by user. */
                flexcan_frame_t *rxMBFrame = handle->pMBFrameBuf[mbIdx];
                flexcan_frame_t *rxFifoFrame = handle->pRxFifoFrameBuf;

                /* Rx FIFO interrupt handling. */
                if ((handle->rxFifoState != FLEXCAN_StateIdle) &&
                    (mbIdx >= RX_FIFO_FRAME_AVL_MB_ID) &&
                    (mbIdx <= RX_FIFO_OVERFLOW_MB_ID)) {
                    do {
                        switch (1U << mbIdx) {
                        case FLEXCAN_RxFifoOverflowFlag:
                            status = FLEXCAN_RX_FIFO_OVERFLOW;
                            break;

                        case FLEXCAN_RxFifoWarningFlag:
                            status = FLEXCAN_RX_FIFO_WARNING;
                            break;

                        case FLEXCAN_RxFifoFrameAvlFlag:
                            status = flexcan_read_rx_fifo(
                                handle, rxFifoFrame ? rxFifoFrame : &frameBuf);

                            if (FLEXCAN_SUCCESS == status) {
                                status = FLEXCAN_RX_FIFO_IDLE;
                            }

                            break;

                        default:
                            status = FLEXCAN_UNHANDLED;
                            break;
                        }

                        /* Clear message buffer interrupt flag. */
                        flexcan_clear_mb_int_flag(base, mbIdx);

                        /* Calling Callback Function if has one. */
                        if (handle->callback != NULL) {
                            handle->callback(handle, status, mbIdx,
                                             (void *)(rxFifoFrame ? rxFifoFrame
                                                                  : &frameBuf));
                        }
                    } while (flexcan_get_mb_int_status_flag(base, mbIdx) != 0U);
                } else {
                    switch (handle->mbState[mbIdx]) {
                    case FLEXCAN_StateRxData:
                    case FLEXCAN_StateRxRemote:
                        status = flexcan_read_rx_mb(
                            handle, mbIdx, rxMBFrame ? rxMBFrame : &frameBuf);

                        if (status == FLEXCAN_SUCCESS) {
                            status = FLEXCAN_RX_IDLE;
                        }

                        break;

                    case FLEXCAN_StateTxData:
                        handle->mbState[mbIdx] = FLEXCAN_StateIdle;
                        status = FLEXCAN_TX_IDLE;
                        break;

                    case FLEXCAN_StateTxRemote:
                        handle->mbState[mbIdx] = FLEXCAN_StateRxRemote;
                        status = FLEXCAN_TX_SWITCH_TO_RX;
                        break;

                    default:
                        status = FLEXCAN_UNHANDLED;
                        break;
                    }

                    /* Clear message buffer interrupt flag. */
                    flexcan_clear_mb_int_flag(base, mbIdx);

                    /* Calling Callback Function if has one. */
                    if (handle->callback != NULL) {
                        handle->callback(
                            handle, status, mbIdx,
                            (void *)(rxMBFrame ? rxMBFrame : &frameBuf));
                    }
                }
            }
        }
    }
    return 0;
}

/* API function implementation end */

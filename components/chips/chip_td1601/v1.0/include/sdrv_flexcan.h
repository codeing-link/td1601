/**
 * @file sdrv_flexcan.h
 *  Created on: 2023年8月25日
 *      Author: Dell
 */

#ifndef SDRV_FLEXCAN_H_
#define SDRV_FLEXCAN_H_

#include <stdbool.h>
//#include <types.h>

#include "sdrv_flexcan_priv.h"
#include "drv/sdrv_common.h"
#include "drv/common.h"

///
#define NXP_FLEXCAN_USE_DEV_ARR_4_IRQ            (0)

/* Doze mode support definition. */
#define FLEXCAN_HAS_DOZE_MODE_SUPPORT 	1

#define FLEXCAN_COPY_USE_BURST 			0

#define CORE_LITTLE_ENDIAN 				1

/* Data index converting between frame and MB. */
#if CORE_LITTLE_ENDIAN
#define IDX_CONVERT(x)  (((x) & 0xFCU) + 3U - ((x) & 3U))
#else
#define IDX_CONVERT(x)  (x)
#endif

/* Make Rx FIFO ID filter table content. */
/* ID type */
#define STANDARD_DATA_ID 0U
#define STANDARD_REMOTE_ID 2U
#define EXTENDED_DATA_ID 1U
#define EXTENDED_REMOTE_ID 3U
/* Type A */
#define MAKE_TYPE_A_FILTER(id, type)                                           \
    (((type) << 30) | ((id) << (((type)&1U) ? 1 : 19)))
/* Type B */
#define MAKE_TYPE_B_FILTER(id1, id2, type1, type2)                             \
    (((type1) << 30) | ((id1) << (((type1)&1U) ? 16 : 19)) | ((type2) << 14) | \
     ((id2) << (((type2)&1U) ? 0 : 3)))
/* Type C */
#define MAKE_TYPE_C_FILTER(id1, id2, id3, id4)                                 \
    (((id1)&0xFFU) | ((id2)&0xFFU) | ((id3)&0xFFU) | ((id4)&0xFFU))

#define FlexCanRxFifoAcceptRemoteFrame 1UL
#define FlexCanRxFifoAcceptExtFrame 1UL

// Aliases data types declarations
typedef struct flexcan_fd_config_t flexcan_fd_config_t;
typedef struct flexcan_pn_config_t flexcan_pn_config_t;
typedef struct flexcan_timing_config_t flexcan_timing_config_t;
typedef struct flexcan_config_t flexcan_config_t;
typedef struct flexcan_rx_fifo_config_t flexcan_rx_fifo_config_t;

/******************* FlexCAN data structure *********************/
typedef enum flexcan_fd_data_size {
    CAN_FD_8BYTES_PER_MB = 0,  /**< 8 bytes per message buffer. */
    CAN_FD_16BYTES_PER_MB = 1, /**< 16 bytes per message buffer. */
    CAN_FD_32BYTES_PER_MB = 2, /**< 32 bytes per message buffer. */
    CAN_FD_64BYTES_PER_MB = 3  /**< 64 bytes per message buffer. */
} flexcan_fd_data_size_e;

/** @brief FlexCAN clock source. */
typedef enum flexcan_clock_source {
    FLEXCAN_ClkSrcOsc =
        0, /**< FlexCAN Protocol Engine clock from Oscillator. */       //40M
    FLEXCAN_ClkSrcPeri =
        1 /**< FlexCAN Protocol Engine clock from Peripheral Clock. */  //48M
} flexcan_clock_source_e;

typedef enum flexcan_rx_fifo_filter_type {
    FLEXCAN_RxFifoFilterTypeA =
        0, /**< One full ID (standard and extended) per ID Filter element. */
    FLEXCAN_RxFifoFilterTypeB =
        1, /**< Two full standard IDs or two partial 14-bit ID slices per ID
              Filter Table element. */
    FLEXCAN_RxFifoFilterTypeC = 2, /**< Four partial 8-bit Standard or extended
                                      ID slices per ID Filter Table element. */
    FLEXCAN_RxFifoFilterTypeD = 3  /**< All frames rejected. */
} flexcan_rx_fifo_filter_type_e;

typedef enum flexcan_rx_fifo_priority {
    FLEXCAN_RxFifoPrioLow =
        0, /**< Matching process start from Rx Message Buffer first*/
    FLEXCAN_RxFifoPrioHigh = 1 /**< Matching process start from Rx FIFO first*/
} flexcan_rx_fifo_priority_e;

/** @brief FlexCAN frame type. */
typedef enum flexcan_frame_type {
    FLEXCAN_FrameTypeData = 0,  /**< Data frame type attribute. */
    FLEXCAN_FrameTypeRemote = 1 /**< Remote frame type attribute. */
} flexcan_frame_type_e;

/** @brief FlexCAN frame format. */
typedef enum flexcan_frame_format {
    FLEXCAN_STANDARD_FRAME = 0, /**< Standard frame format attribute. */
    FLEXCAN_EXTEND_FRAME = 1    /**< Extend frame format attribute. */
} flexcan_frame_format_e;

// Structures/unions data types declarations
struct flexcan_fd_config_t {
    uint8_t enableISOCANFD;
    uint8_t enableBRS;
    uint8_t enableTDC;
    uint8_t TDCOffset;
    flexcan_fd_data_size_e r0_mb_data_size;
    flexcan_fd_data_size_e r1_mb_data_size;
};

/** @brief Pretended Networking ID filter */
typedef struct {
    bool extendedId;  /**< Specifies if the ID is standard or extended. */
    bool remoteFrame; /**< Specifies if the frame is standard or remote. */
    uint32_t id;      /**< Specifies the ID value. */
} flexcan_pn_id_filter_t;

/** @brief Pretended Networking payload filter */
typedef struct {
    uint8_t dlcLow;       /**< Specifies the lower limit of the payload size. */
    uint8_t dlcHigh;      /**< Specifies the upper limit of the payload size. */
    uint8_t payload1[8U]; /**< Specifies the payload to be matched (for
                             MATCH_EXACT), the lower limit (for MATCH_GEQ and
                             MATCH_RANGE) or the upper limit (for MATCH_LEQ). */
    uint8_t payload2[8U]; /**< Specifies the mask (for MATCH_EXACT) or the upper
                             limit (for MATCH_RANGE). */
} flexcan_pn_payload_filter_t;

/** @brief Pretended Networking filtering combinations */
typedef enum {
    FLEXCAN_FILTER_ID,         /**< Message ID filtering only */
    FLEXCAN_FILTER_ID_PAYLOAD, /**< Message ID and payload filtering */
    FLEXCAN_FILTER_ID_NTIMES,  /**< Message ID filtering occurring a specified
                                  number of times */
    FLEXCAN_FILTER_ID_PAYLOAD_NTIMES /**< Message ID and payload filtering
                                        occurring a specified number of times */
} flexcan_pn_filter_combination_e;

/** @brief Pretended Networking matching schemes */
typedef enum {
    FLEXCAN_FILTER_MATCH_EXACT, /**< Match an exact target value. */
    FLEXCAN_FILTER_MATCH_GEQ,   /**< Match greater than or equal to a specified
                                   target value. */
    FLEXCAN_FILTER_MATCH_LEQ,   /**< Match less than or equal to a specified
                                   target value. */
    FLEXCAN_FILTER_MATCH_RANGE  /**< Match inside a range, greater than or equal
                                   to a specified lower limit and smaller than or
                                    equal to a specified upper limit. */
} flexcan_pn_filter_selection_e;

/** @brief Pretended Networking configuration structure */
struct flexcan_pn_config_t {
    bool wakeUpTimeout; /**< Specifies if an wake up event is triggered on
                           timeout. */
    bool wakeUpMatch; /**< Specifies if an wake up event is triggered on match.
                       */
    uint16_t numMatches; /**< The number of matches needed before generating an
                            wake up event. */
    uint16_t matchTimeout; /**< Defines a timeout value that generates an wake
                              up event if wakeUpTimeout is true. */
    flexcan_pn_filter_combination_e
        filterComb; /**< Defines the filtering scheme used. */
    flexcan_pn_id_filter_t
        idFilter1; /**< The configuration of the first ID filter (match exact /
                      lower limit / upper limit). */
    flexcan_pn_id_filter_t idFilter2; /**< The configuration of the second ID
                                         filter (mask / upper limit). */
    flexcan_pn_filter_selection_e
        idFilterType; /**< Defines the ID filtering scheme. */
    flexcan_pn_filter_selection_e
        payloadFilterType; /**< Defines the payload filtering scheme. */
    flexcan_pn_payload_filter_t
        payloadFilter; /**< The configuration of the payload filter. */
};

/** NOTICE: The length of the time quantum should be the same in nominal
 *          and data bit timing (i.e. preDivider should be the same in
 *          nominal and data bit timing configuration) in order to minimize
 *          the chance of error frames on the CAN bus, and to optimize the
 *          clock tolerance in networks that use CAN FD frams.
 */
struct flexcan_timing_config_t {
    uint16_t preDivider;
    uint8_t rJumpwidth;
    uint8_t propSeg;
    uint8_t phaseSeg1;
    uint8_t phaseSeg2;
};

struct flexcan_config_t {
    flexcan_clock_source_e clkSrc;
    uint8_t maxMbNum;
    bool enableLoopBack;
    bool enableLBUFTransmittedFirst;
    bool enableListenOnly;
    bool enableSelfWakeup;
    bool enableIndividMask;
    bool enableDoze;
    bool enableCANFD;
    bool enablePretendedNetworking;
    flexcan_timing_config_t nominalBitTiming;
    flexcan_timing_config_t dataBitTiming;
    flexcan_fd_config_t can_fd_cfg;
    flexcan_pn_config_t pnet_cfg;
};

typedef struct flexcan_rx_fifo_filter_table {
    uint32_t filter_code;
    uint32_t filter_mask;
} flexcan_rx_fifo_filter_table_t;

struct flexcan_rx_fifo_config_t {
    uint8_t idFilterNum;
    flexcan_rx_fifo_filter_type_e idFilterType;
    flexcan_rx_fifo_priority_e priority;
    flexcan_rx_fifo_filter_table_t *filter_tab;
};

/**
 * @brief FlexCAN Receive Message Buffer configuration structure
 *
 * This structure is used as the parameter of FLEXCAN_SetRxMbConfig() function.
 * The FLEXCAN_SetRxMbConfig() function is used to configure FlexCAN Receive
 * Message Buffer. The function abort previous receiving process, clean the
 * Message Buffer and activate the Rx Message Buffer using given Message Buffer
 * setting.
 */
typedef struct _flexcan_rx_mb_config {
    uint32_t id; /**< CAN Message Buffer Frame Identifier. */
    flexcan_frame_format_e
        format; /**< CAN Frame Identifier format(Standard of Extend). */
    flexcan_frame_type_e type; /**< CAN Frame Type(Data or Remote). */
} flexcan_rx_mb_config_t;

typedef struct _flexcan_frame {
    uint16_t
        timestamp; /**< FlexCAN internal Free-Running Counter Time Stamp. */
    uint32_t id;   /**< CAN Frame Identifier. */
    struct {
        uint32_t
            length : 7; /**< CAN frame payload length in bytes(Range: 0~64). */
        uint32_t type : 1;   /**< CAN Frame Type(DATA or REMOTE). */
        uint32_t format : 1; /**< CAN Frame Identifier(STD or EXT format). */
        uint32_t isCANFDFrame : 1; /**< CAN FD or classic frame? */
        uint32_t isCANFDBrsEn : 1; /**< CAN FD BRS enabled? */
        uint32_t reserved1 : 5;    /**< Reserved for placeholder. */
        uint32_t idHit : 9; /**< CAN Rx FIFO filter hit id(This value is only
                               used in Rx FIFO receive mode). */
        uint32_t reserved2 : 7; /**< Reserved for placeholder. */
    };
    uint8_t
        *dataBuffer; /**< Frame buffer. NOTE: For transmitting buffer, the data
                        order should be consistent with the cpu endianness. */
} flexcan_frame_t;

/** @brief FlexCAN Message Buffer transfer. */
typedef struct _flexcan_mb_transfer {
    flexcan_frame_t *pFrame; /**< The buffer of CAN Message to be transfer. */
    uint8_t mbIdx; /**< The index of Message buffer used to transfer Message. */
} flexcan_mb_transfer_t;

/** @brief FlexCAN Rx FIFO transfer. */
typedef struct _flexcan_fifo_transfer {
    flexcan_frame_t
        *pFrame; /**< The buffer of CAN Message to be received from Rx FIFO. */
} flexcan_fifo_transfer_t;

/** @brief FlexCAN handle structure definition. */
typedef struct _flexcan_handle flexcan_handle_t;

/** @brief FlexCAN transfer status. */
typedef enum _flexcan_status {
  FLEXCAN_SUCCESS = SDRV_STATUS_OK, /**< Operation succeeds. */
  FLEXCAN_FAIL =
      SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN, 1), /**< Operation fails. */
  FLEXCAN_TX_BUSY = SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                                      2), /**< Tx Message Buffer is Busy. */
  FLEXCAN_TX_IDLE = SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                                      3), /**< Tx Message Buffer is Idle. */
  FLEXCAN_TX_SWITCH_TO_RX =
      SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN, 4), /**< Remote Message is
       send out and Message buffer changed to Receive one. */
  FLEXCAN_RX_BUSY = SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                                      5), /**< Rx Message Buffer is Busy. */
  FLEXCAN_RX_IDLE = SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                                      6), /**< Rx Message Buffer is Idle. */
  FLEXCAN_RX_OVERFLOW = SDRV_ERROR_STATUS(
      SDRV_STATUS_GROUP_FLEXCAN, 7), /**< Rx Message Buffer is Overflowed. */
  FLEXCAN_RX_FIFO_BUSY = SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                                           8), /**< Rx Message FIFO is Busy. */
  FLEXCAN_RX_FIFO_IDLE = SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                                           9), /**< Rx Message FIFO is Idle. */
  FLEXCAN_RX_FIFO_OVERFLOW = SDRV_ERROR_STATUS(
      SDRV_STATUS_GROUP_FLEXCAN, 10), /**< Rx Message FIFO is overflowed. */
  FLEXCAN_RX_FIFO_WARNING =
      SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                        11), /**< Rx Message FIFO is almost overflowed. */
  FLEXCAN_ERROR_STATUS = SDRV_ERROR_STATUS(
      SDRV_STATUS_GROUP_FLEXCAN, 12), /**< FlexCAN Module Error and Status. */
  FLEXCAN_WAKEUP_TIMEOUT =
      SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                        13), /**< An wake up event occurred due to timeout. */
  FLEXCAN_WAKEUP_MATCH =
      SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                        14), /**< An wake up event occurred due to matching. */
  FLEXCAN_UNHANDLED = SDRV_ERROR_STATUS(
      SDRV_STATUS_GROUP_FLEXCAN, 15), /**< UnHadled Interrupt asserted. */
  ///
  FLEXCAN_HIGH_TIMER_WRAP =
      SDRV_ERROR_STATUS(SDRV_STATUS_GROUP_FLEXCAN,
                        16), /**< An wake up event occurred due to matching. */
} flexcan_status_e;

typedef enum flexcan_controller_id {
    FLEXCAN1 = 0,
    FLEXCAN2 = 1,
    FLEXCAN3 = 2,
    FLEXCAN4 = 3,
    FLEXCAN5 = 4,
    FLEXCAN6 = 5,
    FLEXCAN7 = 6,
    FLEXCAN8 = 7,
    FLEXCAN9 = 8,
    FLEXCAN10 = 9,
    FLEXCAN11 = 10,
    FLEXCAN12 = 11,
    FLEXCAN13 = 12,
    FLEXCAN14 = 13,
    FLEXCAN15 = 14,
    FLEXCAN16 = 15,
    FLEXCAN17 = 16,
    FLEXCAN18 = 17,
    FLEXCAN19 = 18,
    FLEXCAN20 = 19,
    FLEXCAN21 = 20,
    FLEXCAN22 = 21,
    FLEXCAN23 = 22,
    FLEXCAN24 = 23,
    MAX_FLEXCAN_CH
} flexcan_controller_id_e;

/** @brief FlexCAN transfer callback function.
 *
 *  The FlexCAN transfer callback will return value from the underlying layer.
 *  If the status equals to FLEXCAN_ERROR_STATUS, the result parameter will be
 * the Content of FlexCAN status register which can be used to get the working
 * status(or error status) of FlexCAN module. If the status equals to other
 * FlexCAN Message Buffer transfer status, the result will be the index of
 *  Message Buffer that generate transfer event.
 *  If the status equals to other FlexCAN Message Buffer transfer status, the
 * result is meaningless and should be Ignored.
 */
typedef void (*flexcan_transfer_callback_t)(flexcan_handle_t *handle,
                                            flexcan_status_e status,
                                            uint32_t result, void *userData);

/** @brief FlexCAN handle structure. */
struct _flexcan_handle {
#if (defined(NXP_FLEXCAN_USE_DEV_ARR_4_IRQ) && !NXP_FLEXCAN_USE_DEV_ARR_4_IRQ)
    csi_dev_t             csi_dev;
#else
    csi_dev_t             csi_dev[12];
#endif
    /**< Flexcan controller id. */
    uint8_t controller_id;
    /**< Flexcan irq num. */
    uint8_t irq_num;
    /**< Peripheral base address. */
    void *base_addr;
    /**< Callback function. */
    flexcan_transfer_callback_t callback;
    /**< FlexCAN callback function parameter.*/
    void *userData;
    /**< The buffer for received data from Message Buffers. */
    flexcan_frame_t
        *volatile pMBFrameBuf[FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER];
    /**< The buffer for received data from Rx FIFO. */
    flexcan_frame_t *volatile pRxFifoFrameBuf;
    /**< Message Buffer transfer state. */
    volatile uint8_t mbState[FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBER];
    /**< Rx FIFO transfer state. */
    volatile uint8_t rxFifoState;
    /* Bit map used to record if a MB is used. */
    uint32_t mb_used_mask[2];
#if FLEXCAN_HAS_DOZE_MODE_SUPPORT
    bool enableDoze;
#endif
};

/**
 * @brief FlexCAN interrupt configuration structure, default settings all
 * disabled.
 *
 * This structure contains the settings for all of the FlexCAN Module interrupt
 * configurations. Note: FlexCAN Message Buffers and Rx FIFO have their own
 * interrupts.
 */
enum _flexcan_interrupt_enable {
    FLEXCAN_BusOffInterruptEnable =
        0x8000U, /**< CAN_CTRL1_BOFFMSK_MASK, Bus Off interrupt. */
    FLEXCAN_ErrorInterruptEnable =
        0x4000U, /**< CAN_CTRL1_ERRMSK_MASK, Error interrupt. */
    FLEXCAN_RxWarningInterruptEnable =
        0x400U, /**< CAN_CTRL1_RWRNMSK_MASK, Rx Warning interrupt. */
    FLEXCAN_TxWarningInterruptEnable =
        0x800U, /**< CAN_CTRL1_TWRNMSK_MASK, Tx Warning interrupt. */
    FLEXCAN_WakeUpInterruptEnable =
        0x4000000U, /**< CAN_MCR_WAKMSK_MASK, Wake Up interrupt. */
    FLEXCAN_NonCTRL1AndMCRInterruptMASK =
        0xFF000000U, /**< CAN_MCR_WAKMSK_MASK, Wake Up interrupt. */
    FLEXCAN_HighTimerWarpInterruptEnable =
        0xFF000001U, /**< CAN_MCR_WAKMSK_MASK, Wake Up interrupt. */
};

/******************************************************************************
 * API
 *****************************************************************************/

/**
 * @brief Initialize the FlexCAN handle.
 *
 * This function initializes the FlexCAN handle which can be used for other
 * FlexCAN APIs. Usually, for a specified FlexCAN instance, user only need to
 * call this API once to get the initialized handle. NOTE: This function should
 * be called before all the other FlexCAN APIs.
 *
 * @param [in] handle FlexCAN handle pointer.
 * @param [in] controller_id FlexCAN controller_id.
 * @param [in] reg_base FlexCAN peripheral base address.
 * @param [in] irq_num irq number.
 * @param [in] callback The callback function.
 * @param [in] userData The parameter of the callback function.
 */
extern void flexcan_create_handle(flexcan_handle_t *handle, uint8_t controller_id,
                           void *reg_base, uint8_t irq_num,
                           flexcan_transfer_callback_t callback, void *userData);

/**
 * @brief Initializes a FlexCAN instance.
 *
 * This function initializes the FlexCAN module with user-defined settings.
 * This example shows how to set up the flexcan_config_t parameters and how
 * to call the flexcan_init function by passing in these parameters:
 *  @code
 *   flexcan_config_t flexcanConfig;
 *   flexcan_handle_t handle;
 *   flexcanConfig.clkSrc            = FLEXCAN_ClkSrcOsc;
 *   flexcanConfig.maxMbNum          = 14;
 *   flexcanConfig.enableLoopBack    = false;
 *   flexcanConfig.enableListenOnly  = false;
 *   flexcanConfig.enableSelfWakeup  = false;
 *   flexcanConfig.enableIndividMask = false;
 *   flexcanConfig.enableDoze        = false;
 *   flexcanConfig.enableCANFD       = false;
 *   flexcanConfig.nominalBitTiming.preDivider = 0U;
 *   flexcanConfig.nominalBitTiming.rJumpwidth = 1U;
 *   flexcanConfig.nominalBitTiming.propSeg = 3U;
 *   flexcanConfig.nominalBitTiming.phaseSeg1 = 4U;
 *   flexcanConfig.nominalBitTiming.phaseSeg2 = 2U;
 *   flexcan_init(&handle, &flexcanConfig);
 *   @endcode
 *
 * NOTE: The FlexCAN handle must be created before initializing self.
 * @param [in] handle FlexCAN handle.
 * @param [in] config Pointer to user-defined configuration structure.
 * @retval FLEXCAN_SUCCESS operate successfully
 * @retval FLEXCAN_FAIL operate timeout
 */
extern flexcan_status_e flexcan_init(flexcan_handle_t *handle,
                                     uint8_t can_id, flexcan_transfer_callback_t callback, const flexcan_config_t *config);

/**
 * @brief Enables or disable the FlexCAN module operation.
 *
 * This function enables or disables the FlexCAN module.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] enable true to enable, false to disable.
 * @retval FLEXCAN_SUCCESS operate successfully
 * @retval FLEXCAN_FAIL operate timeout
 */
extern flexcan_status_e flexcan_enable(flexcan_handle_t *handle, bool enable);

/**
 * @brief Enter or exit freeze mode.
 *
 * This function makes the FlexCAN work under Freeze Mode
 * if param freeze is true, or work under Normal Mode if
 * param freeze is false.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] freeze true to enter freeze mode, false to exit
 *        freeze mode
 * @retval FLEXCAN_SUCCESS operate successfully
 * @retval FLEXCAN_FAIL operate timeout
 */
extern flexcan_status_e flexcan_freeze(flexcan_handle_t *handle, bool freeze);

/**
 * @brief Reset the FlexCAN Instance.
 *
 * Restores the FlexCAN module to reset state, notice that this function
 * will set all the registers to reset state so the FlexCAN module can not work
 * after calling this API.
 *
 * @param [in] handle FlexCAN handle.
 * @retval FLEXCAN_SUCCESS operate successfully
 * @retval FLEXCAN_FAIL operate timeout
 */
extern flexcan_status_e flexcan_reset(flexcan_handle_t *handle);

/**
 * @brief De-initializes a FlexCAN instance.
 *
 * This function disable the FlexCAN module clock and set all register value
 * to reset value.
 *
 * @param [in] handle FlexCAN handle.
 */
extern void flexcan_deinit(flexcan_handle_t *handle);

int flexcan_send_mailbox(flexcan_handle_t *handle, flexcan_mb_transfer_t *xfer);
void flexcan_config_tx_mailbox(flexcan_handle_t *handle,
                                             uint8_t index);
void flexcan_config_rx_mailbox(flexcan_handle_t *handle, uint8_t index,
                                      flexcan_rx_mb_config_t *rxmbcfg);
/**
 * @brief Setting FlexCAN classic protocol timing characteristic.
 *
 * This function give user fine settings to CAN bus timing characteristic.
 * The function is for user who is really versed in CAN protocol, for these
 * users who just what to establish CAN communication among MCUs, just call
 * flexcan_init() and fill the baud rate field with desired one.
 * Doing this, default timing characteristic will provide to the module.
 *
 * Note: Calling flexcan_classic_set_timing_config() will override the baud rate
 * setted in flexcan_init().
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] config Pointer to the timing configuration structure.
 */
extern void
flexcan_classic_set_timing_config(flexcan_handle_t *handle,
                                  const flexcan_timing_config_t *config);

/**
 * @brief Setting FlexCAN FD protocol timing characteristic.
 *
 * This function give user fine settings to CAN bus timing characteristic.
 * The function is for user who is really versed in CAN protocol, for these
 * users who just what to establish CAN communication among MCUs, just call
 * flexcan_init() and fill the baud rate field with desired one.
 * Doing this, default timing characteristic will provide to the module.
 *
 * Note: Calling flexcan_fd_set_timing_config() will override the baud rate
 * setted in flexcan_init().
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] arbitrPhaseConfig Pointer to the arbitration phase timing
 * configuration structure.
 * @param [in] dataPhaseConfig Pointer to the data phase timing configuration
 * structure.
 */
extern void
flexcan_fd_set_timing_config(flexcan_handle_t *handle,
                             const flexcan_timing_config_t *arbitrPhaseConfig,
                             const flexcan_timing_config_t *dataPhaseConfig);

/**
 * @brief read a can message from wakeup message buffer.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] wmbIndex The FlexCAN wakeup message Buffer Index.
 * @param [out] wmb Pointer to frame buffer to store information of incoming rx
 * message.
 */
extern void flexcan_read_wmb(flexcan_handle_t *handle, uint8_t wmbIndex,
                             flexcan_frame_t *wmb);

/**
 * @brief Enter or exit lowpower mode.
 *
 * This function makes the FlexCAN work under lowpower Mode
 * if param request is true, or exit sleep Mode if
 * param request is false.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] request true to enter lowpower mode, false to exit
 *        lowpower mode.
 * @retval FLEXCAN_SUCCESS operate successfully
 * @retval FLEXCAN_FAIL operate timeout
 */
extern flexcan_status_e flexcan_request_sleep(flexcan_handle_t *handle,
                                              bool request);

/**
 * @brief Set the FlexCAN Receive Message Buffer Global Mask.
 *
 * This function Set the global mask for FlexCAN Message Buffer in matching
 * process. The configuration is only effective when Rx Individual Mask is
 * disabled in flexcan_init().
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mask Rx Message Buffer Global Mask value.
 */
extern void flexcan_set_rx_mb_global_mask(flexcan_handle_t *handle,
                                          uint32_t mask);

/**
 * @brief Set the FlexCAN Receive FIFO Global Mask.
 *
 * This function Set the global mask for FlexCAN FIFO in matching process.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mask Rx Fifo Global Mask value.
 */
extern void flexcan_set_rx_fifo_global_mask(flexcan_handle_t *handle,
                                            uint32_t mask);

/**
 * @brief Set the FlexCAN Receive Individual Mask.
 *
 * This function Set the Individual mask for FlexCAN matching process.
 * The configuration is only effective when Rx Individual Mask is enabled in
 * FLEXCAN_Init(). If Rx FIFO is disabled, the Individual Mask is applied to
 * corresponding Message Buffer. If Rx FIFO is enabled, the Individual Mask for
 * Rx FIFO occupied Message Buffer will be applied to Rx Filter with same index.
 * What calls for special attention is that only the first 32 Individual Mask
 * can be used as Rx FIFO Filter Mask.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] maskIdx The Index of individual Mask.
 * @param [in] mask Rx Individual Mask value.
 */
extern void flexcan_set_rx_individual_mask(flexcan_handle_t *handle,
                                           uint8_t maskIdx, uint32_t mask);

/**
 * @brief Configure a FlexCAN Transmit Message Buffer.
 *
 * This function abort privious transmission, clean the Message Buffer and
 * configure it as a Transmit Message Buffer.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The Message Buffer index.
 * @param [in] tx_by_interrupt Tx by interrupt (true) or polling (false).
 */
extern void flexcan_set_tx_mb_config(flexcan_handle_t *handle, uint8_t mbIdx,
                                     bool tx_by_interrupt);

/**
 * @brief Configure a FlexCAN Receive Message Buffer.
 *
 * This function clean a FlexCAN build-in Message Buffer and configure it
 * as a Receive Message Buffer.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The Message Buffer index.
 * @param [in] config Pointer to FlexCAN Message Buffer configuration structure.
 */
extern void flexcan_set_rx_mb_config(flexcan_handle_t *handle, uint8_t mbIdx,
                                     const flexcan_rx_mb_config_t *config);

/**
 * @brief Configure the FlexCAN Rx FIFO.
 *
 * This function Configure the Rx FIFO with given Rx FIFO configuration.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] config Pointer to FlexCAN Rx FIFO configuration structure.
 */
extern void flexcan_set_rx_fifo_config(flexcan_handle_t *handle,
                                       const flexcan_rx_fifo_config_t *config);

/**
 * @brief Enable FlexCAN interrupts according to provided mask.
 *
 * This function enables the FlexCAN interrupts according to provided mask. The
 * mask is a logical OR of enumeration members, see @ref
 * _flexcan_interrupt_enable.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mask The interrupts to enable. Logical OR of @ref
 * _flexcan_interrupt_enable.
 */
extern void flexcan_enable_interrupts(flexcan_handle_t *handle, uint32_t mask);

/**
 * @brief Disable FlexCAN interrupts according to provided mask.
 *
 * This function disables the FlexCAN interrupts according to provided mask. The
 * mask is a logical OR of enumeration members, see @ref
 * _flexcan_interrupt_enable.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mask The interrupts to disable. Logical OR of @ref
 * _flexcan_interrupt_enable.
 */
extern void flexcan_disable_interrupts(flexcan_handle_t *handle, uint32_t mask);

/**
 * @brief Get mailbox interrupt flag.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] msgBufId The FlexCAN Message Buffer index.
 * @retval flag value of MB.
 */
extern uint32_t flexcan_get_mb_iflag(flexcan_handle_t *handle, uint8_t msgBufId);

/**
 * @brief send message using IRQ
 *
 * This function send message using IRQ, this is non-blocking function, will
 * return right away, when message have been sent out, the send callback
 * function will be called.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] xfer FlexCAN Message Buffer transfer structure, refer to
 * #flexcan_mb_transfer_t.
 * @param [in] padding_val value used to pad unspecified data in CAN FD frames >
 * 8bytes.
 * @retval FLEXCAN_SUCCESS        Start Tx Message Buffer sending process
 * successfully.
 * @retval FLEXCAN_FAIL           Write Tx Message Buffer failed.
 * @retval FLEXCAN_TX_BUSY        Tx Message Buffer is in use.
 */
extern flexcan_status_e flexcan_send_nonblocking(flexcan_handle_t *handle,
                                                 flexcan_mb_transfer_t *xfer,
                                                 uint8_t padding_val);

/**
 * @brief Receive message using IRQ
 *
 * This function receive message using IRQ, this is non-blocking function, will
 * return right away, when message have been received, the receive callback
 * function will be called.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] xfer FlexCAN Message Buffer transfer structure, refer to
 * #flexcan_mb_transfer_t.
 * @retval FLEXCAN_SUCCESS        Start Rx Message Buffer receiving process
 * successfully.
 * @retval FLEXCAN_RX_BUSY        Rx Message Buffer is in use.
 */
extern flexcan_status_e
flexcan_receive_nonblocking(flexcan_handle_t *handle,
                            flexcan_mb_transfer_t *xfer);

/**
 * @brief Receive message from Rx FIFO using IRQ
 *
 * This function receive message using IRQ, this is non-blocking function, will
 * return right away, when all messages have been received, the receive callback
 * function will be called.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] xfer FlexCAN Rx FIFO transfer structure, refer to
 * #flexcan_fifo_transfer_t.
 * @retval FLEXCAN_SUCCESS            - Start Rx FIFO receiving process
 * successfully.
 * @retval FLEXCAN_RX_FIFO_BUSY       - Rx FIFO is currently in use.
 */
extern flexcan_status_e
flexcan_receive_fifo_nonblocking(flexcan_handle_t *handle,
                                 flexcan_fifo_transfer_t *xfer);

/**
 * @brief Write FlexCAN Message to Transmit Message Buffer.
 *
 * This function write a CAN Message to the specified Transmit Message Buffer.
 * and change the Message Buffer state to start CAN Message transmit, after
 * that the function will return immediately.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The FlexCAN Message Buffer index.
 * @param [in] txFrame Pointer to CAN message frame to be sent.
 * @param [in] padding_val value used to pad unspecified data in CAN FD frames >
 8bytes.
 * @return FLEXCAN_SUCCESS - Write Tx Message Buffer Successfully.
           FLEXCAN_FAIL    - Tx Message Buffer is currently in use.
 */
extern flexcan_status_e flexcan_write_tx_mb(flexcan_handle_t *handle,
                                            uint8_t mbIdx,
                                            const flexcan_frame_t *txFrame,
                                            uint8_t padding_val);

/**
 * @brief Read a FlexCAN Message from Receive Message Buffer.
 *
 * This function read a CAN message from a specified Receive Message Buffer.
 * The function will fill receive CAN message frame structure with
 * just received data and activate the Message Buffer again.
 * The function will return immediately.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The FlexCAN Message Buffer index.
 * @param [out] rxFrame Pointer to CAN message frame structure.
 * @retval FLEXCAN_SUCCESS            - Rx Message Buffer is full and has been
 * read successfully.
 * @retval FLEXCAN_RX_OVERFLOW        - Rx Message Buffer is already overflowed
 * and has been read successfully.
 * @retval FLEXCAN_FAIL               - Rx Message Buffer is empty.
 */
extern flexcan_status_e flexcan_read_rx_mb(flexcan_handle_t *handle,
                                           uint8_t mbIdx,
                                           flexcan_frame_t *rxFrame);

/**
 * @brief Read a FlexCAN Message from Rx FIFO.
 *
 * This function Read a CAN message from the FlexCAN build-in Rx FIFO.
 *
 * @param [in] handle FlexCAN handle.
 * @param [out] rxFrame Pointer to CAN message frame structure.
 * @retval FLEXCAN_SUCCESS - Read Message from Rx FIFO successfully.
 * @retval FLEXCAN_FAIL    - Rx FIFO is not enabled.
 */
extern flexcan_status_e flexcan_read_rx_fifo(flexcan_handle_t *handle,
                                             flexcan_frame_t *rxFrame);

/**
 * @brief Abort interrupt driven message send process.
 *
 * This function aborts interrupt driven message send process.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The FlexCAN Message Buffer index.
 */
extern void flexcan_abort_mb_send(flexcan_handle_t *handle, uint8_t mbIdx);

/**
 * @brief Abort interrupt driven message receive process.
 *
 * This function abort interrupt driven message receive process.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The FlexCAN Message Buffer index.
 */
extern void flexcan_abort_mb_receive(flexcan_handle_t *handle, uint8_t mbIdx);

/**
 * @brief Abort interrupt driven message receive from Rx FIFO process.
 *
 * This function abort interrupt driven message receive from Rx FIFO process.
 *
 * @param [in] handle FlexCAN handle.
 */
extern void flexcan_abort_receive_fifo(flexcan_handle_t *handle);

/**
 * @brief Read MB interrupt status.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The FlexCAN Message Buffer index.
 * @return uint32_t If MB interupt triggered.
 */
extern uint32_t flexcan_read_mb_int_status(flexcan_handle_t *handle,
                                           uint8_t mbIdx);

/**
 * @brief Get MB state.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The FlexCAN Message Buffer index.
 * @return uint8_t flexcan mb state.
 */
extern uint8_t flexcan_get_mb_state(flexcan_handle_t *handle, uint8_t mbIdx);

/**
 * @brief check whether mb is occupied by fifo along with filter table.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mbIdx The FlexCAN Message Buffer index.
 * @retval TRUE - MB is occupied.
 * @retval FALSE - MB isn't occupied.
 */
extern bool flexcan_is_mb_occupied(flexcan_handle_t *handle, uint8_t mbIdx);

/**
 * @brief Get reported error conditons detected in the reception and
 * transmission.
 *
 * @param [in] handle FlexCAN handle.
 * @retval value of register ESR1.
 */
extern uint32_t flexcan_get_error_status(flexcan_handle_t *handle);

/**
 * @brief Clear all error interrupt status.
 *
 * @param [in] handle FlexCAN handle.
 * @param [in] mask FlexCAN status flags.
 */
extern void flexcan_clear_err_status_flag(flexcan_handle_t *handle,
                                          uint32_t mask);

/**
 * @brief FlexCAN IRQ handle function
 *
 * This function handles the FlexCAN Error, Message Buffer and Rx FIFO IRQ
 * request.
 *
 * @param [in] irq irq number.
 * @param [in] arg FlexCAN handle.
 */
//extern int flexcan_irq_handler(uint32_t irq, void *arg);
extern int flexcan_irq_handler(void *arg);


#endif /* SDRV_FLEXCAN_H_ */



/******************************************************************************
 * @file     drv/ospi.h
 * @brief    Header File for OSPI Driver
 * @version  V1.0
 * @date     08. Apr 2020
 * @model    ospi
 ******************************************************************************/

#ifndef _DRV_OSPI_H_
#define _DRV_OSPI_H_

#include <stdint.h>
#include <drv/common.h>
#include <drv/gpio.h>
#include <drv/dma.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \enum    csi_ospi_mode_t
 *  \brief   Function mode of ospi
 */
typedef enum {
    OSPI_MASTER,             ///< OSPI Master (Output on MOSI, Input on MISO); arg = Bus Speed in bps
    OSPI_SLAVE,              ///< OSPI Slave  (Output on MISO, Input on MOSI)
} csi_ospi_mode_t;

/**
 *  \enum    csi_ospi_transfer_mode_t
 *  \brief   Function mode of ospi
 */
typedef enum {
    OSPI_TRANSFER_SEND_RECEIVE,
    OSPI_TRANSFER_SEND_ONLY,
    OSPI_TRANSFER_RECEIVE_ONLY,
    OSPI_TRANSFER_EEPROM_READ
} csi_ospi_transfer_mode_t;

/**
 *  \enum    csi_ospi_transfer_mode_t
 *  \brief   Function mode of ospi
 */
typedef enum {
    OSPI_LINE_SINGLE,
    OSPI_LINE_DUAL,
    OSPI_LINE_QUAD,
    OSPI_LINE_OCTAL
} csi_ospi_line_mode_t;

/**
 *  \enum     csi_ospi_frame_len_t
 *  \brief    OSPI data width (4bit ~ 16bit)
 */
typedef enum {
    OSPI_FRAME_LEN_4  = 4,
    OSPI_FRAME_LEN_5,
    OSPI_FRAME_LEN_6,
    OSPI_FRAME_LEN_7,
    OSPI_FRAME_LEN_8,
    OSPI_FRAME_LEN_9,
    OSPI_FRAME_LEN_10,
    OSPI_FRAME_LEN_11,
    OSPI_FRAME_LEN_12,
    OSPI_FRAME_LEN_13,
    OSPI_FRAME_LEN_14,
    OSPI_FRAME_LEN_15,
    OSPI_FRAME_LEN_16,
    OSPI_FRAME_LEN_17,
    OSPI_FRAME_LEN_18,
    OSPI_FRAME_LEN_19,
    OSPI_FRAME_LEN_20,
    OSPI_FRAME_LEN_21,
    OSPI_FRAME_LEN_22,
    OSPI_FRAME_LEN_23,
    OSPI_FRAME_LEN_24,
    OSPI_FRAME_LEN_25,
    OSPI_FRAME_LEN_26,
    OSPI_FRAME_LEN_27,
    OSPI_FRAME_LEN_28,
    OSPI_FRAME_LEN_29,
    OSPI_FRAME_LEN_30,
    OSPI_FRAME_LEN_31,
    OSPI_FRAME_LEN_32
} csi_ospi_frame_len_t;

/**
 *  \enum     csi_ospi_format_t
 *  \brief    Timing format of ospi
 */
typedef enum {
    OSPI_FORMAT_CPOL0_CPHA0 = 0,  ///< Clock Polarity 0, Clock Phase 0
    OSPI_FORMAT_CPOL0_CPHA1,      ///< Clock Polarity 0, Clock Phase 1
    OSPI_FORMAT_CPOL1_CPHA0,      ///< Clock Polarity 1, Clock Phase 0
    OSPI_FORMAT_CPOL1_CPHA1,      ///< Clock Polarity 1, Clock Phase 1
} csi_ospi_cp_format_t;

/**
 *  \enum     csi_ospi_event_t
 *  \brief    Signaled event for user by driver
 */
typedef enum {
    OSPI_EVENT_SEND_COMPLETE,           ///< Data Send completed. Occurs after call to csi_ospi_send_async to indicate that all the data has been send over
    OSPI_EVENT_RECEIVE_COMPLETE,        ///< Data Receive completed. Occurs after call to csi_ospi_receive_async to indicate that all the data has been received
    OSPI_EVENT_SEND_RECEIVE_COMPLETE,   ///< Data Send_receive completed. Occurs after call to csi_ospi_send_receive_async to indicate that all the data has been send_received
    OSPI_EVENT_ERROR_OVERFLOW,          ///< Data overflow: Receive overflow
    OSPI_EVENT_ERROR_UNDERFLOW,         ///< Data underflow: Transmit underflow
    OSPI_EVENT_ERROR                    ///< Master Mode Fault (SS deactivated when Master).Occurs in master mode when Slave Select is deactivated and indicates Master Mode Fault
} csi_ospi_event_t;

/**
 *  \struct     csi_ospi_t
 *  \brief      Ctrl block of ospi instance
 */
typedef struct csi_ospi csi_ospi_t;
struct csi_ospi {
    csi_dev_t            dev;          ///< Hw-device info
    void (*callback)(csi_ospi_t *ospi, csi_ospi_event_t event, void *arg); ///< User callback ,signaled by driver event
    void                *arg;          ///< User private param ,passed to user callback
    uint8_t             *tx_data;      ///< Output data buf
    uint32_t             tx_size;      ///< Output data size specified by user
    uint8_t             *rx_data;      ///< Input  data buf
    uint32_t             rx_size;      ///< Input  data size specified by user
    csi_error_t (*send)(csi_ospi_t *ospi, const void *data, uint32_t size); ///< The send_async func
    csi_error_t (*receive)(csi_ospi_t *ospi, void *data, uint32_t size);    ///< The receive_async func
    csi_error_t (*send_receive)(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size); ///< The send_receive_async func
    csi_state_t          state;        ///< Peripheral state
    csi_dma_ch_t        *tx_dma;
    csi_dma_ch_t        *rx_dma;
    void                *priv;
};

/**
  \brief       Initialize OSPI Interface
               Initialize the resources needed for the OSPI instance
  \param[in]   ospi    OSPI handle
  \param[in]   idx    OSPI instance index
  \return      Error code
*/
csi_error_t csi_ospi_init(csi_ospi_t *ospi, uint32_t idx);

/**
  \brief       De-initialize OSPI Interface
               stops Operation and releases the software resources used by the ospi instance
  \param[in]   ospi    Handle
  \return      None 
*/
void    csi_ospi_uninit(csi_ospi_t *ospi);

/**
  \brief       Attach the callback handler to OSPI
  \param[in]   ospi    Operate handle
  \param[in]   callback    Callback function
  \param[in]   arg         User can define it by himself as callback's param
  \return      Error code
*/
csi_error_t csi_ospi_attach_callback(csi_ospi_t *ospi, void *callback, void *arg);

/**
  \brief       Detach the callback handler
  \param[in]   ospi    Operate handle
  \return      None
*/
void        csi_ospi_detach_callback(csi_ospi_t *ospi);

/**
  \brief       Config ospi mode (master or slave)
  \param[in]   ospi     OSPI handle
  \param[in]   mode    The mode of ospi (master or slave)
  \return      Error code
*/
csi_error_t csi_ospi_mode(csi_ospi_t *ospi, csi_ospi_mode_t mode);

/**
  \brief       Config ospi transfer mode
  \param[in]   ospi     OSPI handle
  \param[in]   mode    The transfer mode of ospi
  \return      Error code
*/
csi_error_t csi_ospi_transfer_mode(csi_ospi_t *ospi, csi_ospi_transfer_mode_t mode);

/**
  \brief       Config ospi line mode
  \param[in]   ospi     OSPI handle
  \param[in]   mode    The line mode of ospi
  \return      Error code
*/
csi_error_t csi_ospi_line_mode(csi_ospi_t *ospi, csi_ospi_line_mode_t mode);

/**
  \brief       Config ospi cp format
  \param[in]   ospi       OSPI handle
  \param[in]   format    OSPI cp format
  \return      Error code
*/
csi_error_t csi_ospi_cp_format(csi_ospi_t *ospi, csi_ospi_cp_format_t format);

/**
  \brief       Config ospi frame len
  \param[in]   ospi       OSPI handle
  \param[in]   length    OSPI frame len
  \return      Error code
*/
csi_error_t csi_ospi_frame_len(csi_ospi_t *ospi, csi_ospi_frame_len_t length);

/**
  \brief       Config ospi work frequence
  \param[in]   ospi     OSPI handle
  \param[in]   baud    OSPI work baud
  \return      the actual config frequency
*/
uint32_t csi_ospi_baud(csi_ospi_t *ospi, uint32_t baud);

/**
  \brief       Sending data to OSPI transmitter,(received data is ignored)
               blocking mode ,return unti all data has been sent or err happened
  \param[in]   ospi        Handle to operate
  \param[in]   data       Pointer to buffer with data to send to OSPI transmitter
  \param[in]   size       Number of data to send(byte)
  \param[in]   timeout    Unit in mini-second
  \return      If send successful, this function shall return the num of data witch is sent successful
               otherwise, the function shall return Error code
*/
int32_t csi_ospi_send(csi_ospi_t *ospi, const void *data, uint32_t size, uint32_t timeout);

/**
  \brief       Sending data to OSPI transmitter,(received data is ignored)
               non-blocking mode,transfer done event will be signaled by driver
  \param[in]   ospi     Handle to operate
  \param[in]   data    Pointer to buffer with data to send to OSPI transmitter
  \param[in]   size    Number of data items to send(byte)
  \return      Error code
*/
csi_error_t csi_ospi_send_async(csi_ospi_t *ospi, const void *data, uint32_t size);

/**
  \brief       Receiving data from OSPI receiver
               blocking mode, return untill curtain data items are readed
  \param[in]   ospi        Handle to operate
  \param[out]  data       Pointer to buffer for data to receive from OSPI receiver
  \param[in]   size       Number of data items to receive(byte)
  \param[in]   timeout    Unit in mini-second
  \return      If receive successful, this function shall return the num of data witch is  received successful
               otherwise, the function shall return Error code
*/
int32_t csi_ospi_receive(csi_ospi_t *ospi, void *data, uint32_t size, uint32_t timeout);

/**
  \brief       Receiving data from OSPI receiver
               not-blocking mode, event will be signaled when receive done or err happend
  \param[in]   ospi     Handle to operate
  \param[out]  data    Pointer to buffer for data to receive from OSPI receiver
  \param[in]   size    Number of data items to receive(byte)
  \return      Error code
*/
csi_error_t csi_ospi_receive_async(csi_ospi_t *ospi, void *data, uint32_t size);

/**
  \brief       Dulplex,sending and receiving data at the same time
               \ref csi_ospi_event_t is signaled when operation completes or error happens
               \ref csi_ospi_get_state can get operation status
               blocking mode, this function returns after operation completes or error happens
  \param[in]   ospi         OSPI handle to operate
  \param[in]   data_out    Pointer to buffer with data to send to OSPI transmitter
  \param[out]  data_in     Pointer to buffer for data to receive from OSPI receiver
  \param[in]   size        Data size(byte)
  \return      If transfer successful, this function shall return the num of data witch is transfer successful,
               otherwise, the function shall return Error code
*/
int32_t csi_ospi_send_receive(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size, uint32_t timeout);

/**
  \brief       Transmit first then receive ,receive will begin after transmit is done
               if non-blocking mode, this function only starts the transfer,
               \ref csi_ospi_event_t is signaled when operation completes or error happens
               \ref csi_ospi_get_state can get operation status
  \param[in]   ospi         OSPI handle to operate
  \param[in]   data_out    Pointer to buffer with data to send to OSPI transmitter
  \param[out]  data_in     Pointer to buffer for data to receive from OSPI receiver
  \param[in]   size        Data size(byte)
  \return      Error code
*/
csi_error_t csi_ospi_send_receive_async(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size);

/*
  \brief       Set slave select num. Only valid for master
  \param[in]   handle       OSPI handle to operate
  \param[in]   slave_num    OSPI slave num
  \return      None
 */
void csi_ospi_select_slave(csi_ospi_t *ospi, uint32_t slave_num);

/**
  \brief       Link DMA channel to ospi device
  \param[in]   ospi       OSPI handle to operate
  \param[in]   tx_dma    The DMA channel handle for send, when it is NULL means to unlink the channel
  \param[in]   rx_dma    The DMA channel handle for receive, when it is NULL means to unlink the channel
  \return      Error code
*/
csi_error_t csi_ospi_link_dma(csi_ospi_t *ospi, csi_dma_ch_t *tx_dma, csi_dma_ch_t *rx_dma);

/**
  \brief       Get the state of ospi device
  \param[in]   ospi      OSPI handle to operate
  \param[out]  state    The state of ospi device
  \return      Error code
*/
csi_error_t csi_ospi_get_state(csi_ospi_t *ospi, csi_state_t *state);

/**
  \brief       Enable ospi power manage
  \param[in]   ospi  OSPI handle to operate
  \return      Error code
*/
csi_error_t csi_ospi_enable_pm(csi_ospi_t *ospi);

/**
  \brief       Disable ospi power manage
  \param[in]   ospi    OSPI handle to operate
  \return      Error code
*/
void csi_ospi_disable_pm(csi_ospi_t *ospi);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_OSPI_H_ */



/******************************************************************************
 * @file     drv/tdm.h
 * @brief    header file for tdm driver
 * @version  V1.0
 * @date     20. Oct 2020
 * @model    tdm
 ******************************************************************************/

#ifndef _DRV_TDM_H_
#define _DRV_TDM_H_

#include <stdint.h>
#include <stdbool.h>
#include <drv/common.h>
#include <drv/dma.h>
#include "drv/ringbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TDM_MODE_MASTER,                                     ///< TDM transmitter master mode
    TDM_MODE_SLAVE,                                      ///< TDM transmitter slave mode
} csi_tdm_mode_t;

typedef enum {
    TDM_WORKING_MODE_TDM,
    TDM_WORKING_MODE_I2S,
} csi_tdm_work_mode_t;

typedef enum {
    TDM_PROTOCOL_STANDARD,                               ///< I2S or DSP protocol
    TDM_PROTOCOL_LSB_JUSTIFIED,                          ///< LSB (right) justified protocol
    TDM_PROTOCOL_MSB_JUSTIFIED,                          ///< MSB (left) justified protocol
} csi_tdm_protocol_t;

typedef enum {
    TDM_SAMPLE_RISING_EDGE,                               ///< The rising edge of BCLK to transmit next bit
    TDM_SAMPLE_FALL_EDGE,                                 ///< The fall edge of BCLK to transmit next bit
} csi_tdm_sample_edge_t;

typedef enum {
    TDM_LEFT_DATA_FIRST,                                 ///< Left channel first 
    TDM_RIGHT_DATA_FIRST,                                ///< Right channel first
} csi_tdm_polarity_t;

typedef enum {
    TDM_SAMPLE_RATE_8000              = 8000U,           ///< TDM sample rate is 8000
    TDM_SAMPLE_RATE_11025             = 11025U,
    TDM_SAMPLE_RATE_12000             = 12000U,
    TDM_SAMPLE_RATE_16000             = 16000U,
    TDM_SAMPLE_RATE_22050             = 22050U,
    TDM_SAMPLE_RATE_24000             = 24000U,
    TDM_SAMPLE_RATE_32000             = 32000U,
    TDM_SAMPLE_RATE_44100             = 44100U,
    TDM_SAMPLE_RATE_48000             = 48000U,
    TDM_SAMPLE_RATE_96000             = 96000U,
    TDM_SAMPLE_RATE_192000            = 192000U,
    TDM_SAMPLE_RATE_256000            = 256000U,
} csi_tdm_sample_rate_t;

typedef enum {
    TDM_SAMPLE_WIDTH_16BIT = 16U,                      ///< TDM sample width is 16bit
    TDM_SAMPLE_WIDTH_24BIT = 24U,
    TDM_SAMPLE_WIDTH_32BIT = 32U,
} csi_tdm_sample_width_t;

typedef enum {
    TDM_SCLK_32FS  = 32U,                              ///< SCLK frequency is 32 times that of tdm sample rate
    TDM_SCLK_48FS  = 48U,
    TDM_SCLK_64FS  = 64U,
    TDM_SCLK_96FS  = 96U,
    TDM_SCLK_128FS = 128U,
    TDM_SCLK_192FS = 192U,
    TDM_SCLK_256FS = 256U,
} csi_tdm_sclk_freq_t;

typedef struct {
    csi_tdm_mode_t             mode;                    ///< TDM work mode
    csi_tdm_work_mode_t        work_mode;
    csi_tdm_protocol_t         protocol;
    csi_tdm_sample_edge_t      sample_edge;             ///< The edge of BCLK to transmit next bit
    csi_tdm_polarity_t         polarity;                ///< Left/Right channel transmit first
    csi_tdm_sample_rate_t      rate;                    ///< TDM sample rate
    csi_tdm_sample_width_t     width;                   ///< TDM sample width
    csi_tdm_sclk_freq_t        sclk_nfs;                ///< SCLK frequency is N times that of I2S sample rate
} csi_tdm_format_t;

typedef enum {
    TDM_EVENT_SEND_COMPLETE,
    TDM_EVENT_RECEIVE_COMPLETE,
    TDM_EVENT_TX_BUFFER_EMPTY,
    TDM_EVENT_RX_BUFFER_FULL,
    TDM_EVENT_ERROR_OVERFLOW,
    TDM_EVENT_ERROR_UNDERFLOW,
    TDM_EVENT_ERROR,
} csi_tdm_event_t;

typedef struct csi_tdm csi_tdm_t;

struct csi_tdm {
    csi_dev_t               dev;
    void (*callback)(csi_tdm_t *tdm, csi_tdm_event_t event, void *arg); ///< TDM event callback for user
    void                    *arg;                                          ///< user private param passed to user callback
    uint32_t                ch_idx;
    csi_ringbuffer_t            *tx_buf;                                       ///< TDM send buffer
    csi_ringbuffer_t            *rx_buf;                                       ///< TDM receive buffer 
    csi_dma_ch_t            *tx_dma;                                       ///< send dma channel handle
    csi_dma_ch_t            *rx_dma;                                       ///< receive dma channel handle 
    uint8_t             *tx_data;      ///< Output data buf
    uint32_t             tx_size;      ///< Output data size specified by user
    uint8_t             *rx_data;      ///< Input  data buf
    uint32_t             rx_size;      ///< Input  data size specified by user 
    csi_error_t (*send)(csi_tdm_t *spi, const void *data, uint32_t size); ///< The send_async func
    csi_error_t (*receive)(csi_tdm_t *spi, void *data, uint32_t size);    ///< The receive_async func
    uint32_t                tx_period;                                     ///< TDM send period num data will callback
    uint32_t                rx_period;                                     ///< TDM receive period num data will callback
    csi_state_t             state;                                         ///< TDM communication state
    slist_t                 next;
    void                    *priv;
};

/**
  \brief       Init tdm
  \param[in]   tdm    tdm handle to operate
  \param[in]   idx    tdm interface idx
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_init(csi_tdm_t *tdm, uint32_t idx);

/**
  \brief       Uninit tdm
  \param[in]   tdm    tdm handle to operate
  \return      none
*/
void csi_tdm_uninit(csi_tdm_t *tdm);

/**
  \brief       Enable tdm
  \param[in]   tdm      tdm handle to operate
  \param[in]   enable   True enable, False disable
  \return      None
*/
void csi_tdm_enable(csi_tdm_t *tdm, bool enable);

/**
  \brief       tdm config format
  \param[in]   tdm       tdm handle to operate
  \param[in]   format    tdm config param \ref csi_tdm_format_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_format(csi_tdm_t *tdm, csi_tdm_format_t *format);

/**
  \brief       Link DMA channel to tdm device
  \param[in]   tdm        tdm handle to operate
  \param[in]   rx_dma    The DMA channel  for receive, when it is NULL means to unused dma
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_rx_link_dma(csi_tdm_t *tdm, csi_dma_ch_t *rx_dma);

/**
  \brief       Link DMA channel to tdm device
  \param[in]   tdm        tdm handle to operate
  \param[in]   tx_dma    The DMA channel  for send, when it is NULL means to unused dma
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_tx_link_dma(csi_tdm_t *tdm, csi_dma_ch_t *tx_dma);

/**
  \brief       tdm rx buffer config
  \param[in]   tdm        tdm handle to operate
  \param[in]   buffer    tdm rx buffer
  \return      None
*/
void csi_tdm_rx_set_buffer(csi_tdm_t *tdm, csi_ringbuffer_t *buffer);

/**
  \brief       tdm tx buffer config
  \param[in]   tdm        tdm handle to operate
  \param[in]   buffer    tdm tx buffer
  \return      None
*/
void csi_tdm_tx_set_buffer(csi_tdm_t *tdm, csi_ringbuffer_t *buffer);

/**
  \brief       tdm rx set period.The value of period is to report a receive completion event
               after each period value data is received
  \param[in]   tdm        tdm handle to operate
  \param[in]   period    tdm rx period
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_rx_set_period(csi_tdm_t *tdm, uint32_t period);

/**
  \brief       tdm tx set period.The value of period is to report a send completion event
               after each period value data is sended
  \param[in]   tdm        tdm handle to operate
  \param[in]   period    tdm tx period
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_tx_set_period(csi_tdm_t *tdm, uint32_t period);

/**
  \brief       Get rx csi_ringbuffer buffer free space
  \param[in]   tdm     tdm handle to operate
  \return      Buffer free space (bytes)
*/
uint32_t csi_tdm_rx_buffer_avail(csi_tdm_t *tdm);

/**
  \brief       Get tx csi_ringbuffer buffer free space
  \param[in]   tdm     tdm handle to operate
  \return      Buffer free space (bytes)
*/
uint32_t csi_tdm_tx_buffer_avail(csi_tdm_t *tdm);

/**
  \brief       Get rx csi_ringbuffer buffer used space
  \param[in]   tdm     tdm handle to operate
  \return      Buffer used space (bytes)
*/
uint32_t csi_tdm_rx_buffer_remain(csi_tdm_t *tdm);

/**
  \brief       Get tx csi_ringbuffer buffer used space
  \param[in]   tdm     tdm handle to operate
  \return      Buffer used space (bytes)
*/
uint32_t csi_tdm_tx_buffer_remain(csi_tdm_t *tdm);

/**
  \brief       Reset the rx csi_ringbuffer, discard all data in the buffer
  \param[in]   tdm     tdm handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_rx_buffer_reset(csi_tdm_t *tdm);

/**
  \brief       Reset the tx csi_ringbuffer, discard all data in the buffer
  \param[in]   tdm     tdm handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_tx_buffer_reset(csi_tdm_t *tdm);

/**
  \brief       Read data from the buffer
               Using asynchronous receive, tdm writes the received data to the buffer
               This function reads data from the buffer, returns the number of successful reads
               Returns 0 if there is no data in the buffer
  \param[in]   tdm      Operate handle
  \param[out]  data    Pointer to receive data buffer
  \param[in]   size    Receive data size
  \return      The size of data read successfully
*/
csi_error_t csi_tdm_receive_async(csi_tdm_t *tdm, void *data, uint32_t size);

/**
  \brief       Write data to the buffer
               With asynchronous sending
               The data is first written to the buffer and then output through the tdm interface
               Return value is the number of data that was successfully written to the buffer
  \param[in]   tdm      Operate handle
  \param[out]  data    Pointer to send data buffer
  \param[in]   size    send data size
  \return      The data size that write to buffer
*/
csi_error_t csi_tdm_send_async(csi_tdm_t *tdm, const void *data, uint32_t size);

/**
  \brief       send data with sync mode
  \param[in]   tdm      Operate handle
  \param[out]  data    Pointer to send data buffer
  \param[in]   size    send data size
  \return      The data size that write to buffer
*/
int32_t csi_tdm_send(csi_tdm_t *tdm, const void *data, uint32_t size);

/**
  \brief       Start tdm single tdm asynchronous receive
  \param[in]   tdm     Operate handle
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_receive_start(csi_tdm_t *tdm);

/**
  \brief       receive data with sync mode
  \param[in]   tdm     Operate handle
  \return      error code \ref csi_error_t
*/
int32_t csi_tdm_receive_start(csi_tdm_t *tdm);

/**
  \brief       Start tdm single tdm asynchronous send
  \param[in]   tdm     Operate handle
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_send_start(csi_tdm_t *tdm);

/**
  \brief       Stop tdm single tdm asynchronous receive
  \param[in]   tdm     Operate handle
  \return      None
*/
void csi_tdm_receive_stop(csi_tdm_t *tdm);

/**
  \brief       Stop tdm single tdm asynchronous send
  \param[in]   tdm     Operate handle
  \return      None
*/
void csi_tdm_send_stop(csi_tdm_t *tdm);

/**
  \brief       Attach the callback handler to tdm
  \param[in]   tdm     Operate handle
  \param[in]   cb     Callback function
  \param[in]   arg    User private param
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_attach_callback(csi_tdm_t *tdm, void *callback, void *arg);

/**
  \brief       Detach the callback handler
  \param[in]   tdm     Operate handle
  \return      None
*/
void csi_tdm_detach_callback(csi_tdm_t *tdm);

/**
  \brief       Get tdm status
  \param[in]   tdm       tdm handle to operate
  \param[out]  state    tdm state
  \return      error code \ref csi_error_t
*/
csi_error_t csi_tdm_get_state(csi_tdm_t *tdm, csi_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_TDM_H_ */

/******************************************************************************
 * @file     drv/ospi.h
 * @brief    Header File for OSPI Driver
 * @version  V1.0
 * @date     16. 4. 2026
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
 * @enum csi_ospi_mode_t
 * @brief Operating mode of OSPI
 */
typedef enum {
    OSPI_MASTER,              /**< Master mode */
    OSPI_SLAVE,               /**< Slave mode */
} csi_ospi_mode_t;

/**
 * @enum csi_ospi_line_mode_t
 * @brief Data bus width mode for OSPI transfer
 */
typedef enum {
    OSPI_LINE_SINGLE,         /**< Single-line mode */
    OSPI_LINE_DUAL,           /**< Dual-line mode */
    OSPI_LINE_QUAD,           /**< Quad-line mode */
    OSPI_LINE_OCTAL           /**< Octal-line mode */
} csi_ospi_line_mode_t;

/**
 * @enum csi_ospi_address_size_t
 * @brief Size of address phase in bits
 */
typedef enum {
    OSPI_ADDRESS_0_BITS = 0,      /**< No address phase */
    OSPI_ADDRESS_8_BITS = 8,      /**< 8-bit address */
    OSPI_ADDRESS_16_BITS = 16,     /**< 16-bit address */
    OSPI_ADDRESS_24_BITS = 24,     /**< 24-bit address */
    OSPI_ADDRESS_32_BITS = 32      /**< 32-bit address */
} csi_ospi_address_size_t;

/**
 * @enum csi_ospi_instruction_size_t
 * @brief Size of instruction phase in bits
 */
typedef enum {
    OSPI_INSTRUCTION_0_BITS = 0,  /**< No instruction phase */
    OSPI_INSTRUCTION_4_BITS = 1,  /**< 4-bit instruction */
    OSPI_INSTRUCTION_8_BITS = 2,  /**< 8-bit instruction */
    OSPI_INSTRUCTION_16_BITS = 3 /**< 16-bit instruction */
} csi_ospi_instruction_size_t;

/**
 * @enum csi_ospi_alt_size_t
 * @brief Size of alternate byte phase in bits
 */
typedef enum {
    OSPI_ALT_0_BITS,          /**< No alternate byte phase */
    OSPI_ALT_8_BITS,          /**< 8-bit alternate byte */
    OSPI_ALT_16_BITS,         /**< 16-bit alternate byte */
    OSPI_ALT_32_BITS          /**< 32-bit alternate byte */
} csi_ospi_alt_size_t;

/**
 * @enum csi_ospi_transfer_mode_t
 * @brief OSPI transfer direction mode
 */
typedef enum {
    OSPI_TRANSFER_SEND_RECEIVE,      /**< Full-duplex send and receive */
    OSPI_TRANSFER_SEND_ONLY,         /**< Send only */
    OSPI_TRANSFER_RECEIVE_ONLY,      /**< Receive only */
    OSPI_TRANSFER_EEPROM_READ        /**< EEPROM read mode */
} csi_ospi_transfer_mode_t;

/**
 * @struct csi_ospi_command_t
 * @brief OSPI transfer command structure
 * @details Includes instruction, address, alternate, dummy cycles, and data configuration
 */
typedef struct {
    struct {
        csi_ospi_line_mode_t          bus_width;  /**< Bus width for instruction phase */
        csi_ospi_instruction_size_t   size;       /**< Bit size of instruction */
        uint32_t                      value;
        bool                          disabled;   /**< Disable instruction phase */
    } instruction;

    struct {
        csi_ospi_line_mode_t          bus_width;  /**< Bus width for address phase */
        csi_ospi_address_size_t       size;       /**< Bit size of address */
        uint32_t                      value; 
        bool                          disabled;   /**< Disable address phase */
    } address;

    struct {
        csi_ospi_line_mode_t          bus_width;  /**< Bus width for alternate phase */
        csi_ospi_alt_size_t           size;       /**< Bit size of alternate byte */
        uint32_t                      value;
        bool                          disabled;   /**< Disable alternate phase */
    } alt;

    struct {
        csi_ospi_line_mode_t          bus_width;  /**< Bus width for data phase */
        uint8_t                       frame_len;  /**< Data frame length */
        csi_ospi_transfer_mode_t      transfer_mode;
        bool                          disabled;   /**< Disable data phase */
    } data;
    
    uint8_t                           dummy_count;/**< Number of dummy cycles */
    uint8_t                           ddr_enable; /**< DDR double data rate enable */

} csi_ospi_command_t;

/**
 * @enum csi_ospi_cp_format_t
 * @brief OSPI clock polarity and phase format
 */
typedef enum {
    OSPI_FORMAT_CPOL0_CPHA0 = 0,  /**< Clock low idle, sample on leading edge */
    OSPI_FORMAT_CPOL0_CPHA1,      /**< Clock low idle, sample on trailing edge */
    OSPI_FORMAT_CPOL1_CPHA0,      /**< Clock high idle, sample on leading edge */
    OSPI_FORMAT_CPOL1_CPHA1       /**< Clock high idle, sample on trailing edge */
} csi_ospi_cp_format_t;

/**
 * @enum csi_ospi_event_t
 * @brief OSPI callback event types
 */
typedef enum {
    OSPI_EVENT_SEND_COMPLETE,           /**< Send operation completed */
    OSPI_EVENT_RECEIVE_COMPLETE,        /**< Receive operation completed */
    OSPI_EVENT_SEND_RECEIVE_COMPLETE,   /**< Send-receive operation completed */
    OSPI_EVENT_ERROR_OVERFLOW,          /**< Receive overflow error */
    OSPI_EVENT_ERROR_UNDERFLOW,         /**< Transmit underflow error */
    OSPI_EVENT_ERROR                    /**< General hardware error */
} csi_ospi_event_t;

/**
 * @struct csi_ospi_t
 * @brief OSPI device handle structure
 */
typedef struct csi_ospi csi_ospi_t;
struct csi_ospi {
    csi_dev_t    dev;          /**< Hardware device information */

    void (*callback)(csi_ospi_t *ospi, csi_ospi_event_t event, void *arg);
    void         *arg;         /**< User argument for callback */

    uint8_t      *tx_data;     /**< Transmit data buffer */
    uint32_t      tx_size;     /**< Transmit data size */
    uint8_t      *rx_data;     /**< Receive data buffer */
    uint32_t      rx_size;     /**< Receive data size */
    csi_error_t (*send)(csi_ospi_t *ospi, const void *data, uint32_t size); ///< The send_async func
    csi_error_t (*receive)(csi_ospi_t *ospi, void *data, uint32_t size);    ///< The receive_async func
    csi_error_t (*send_receive)(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size); ///< The send_receive_async func
    csi_state_t   state;       /**< Peripheral state */
    csi_dma_ch_t *tx_dma;      /**< Transmit DMA channel */
    csi_dma_ch_t *rx_dma;      /**< Receive DMA channel */
    csi_ospi_command_t *cmd;   /*commmand data*/
	uint8_t     cmd_sent;
    void         *priv;        /**< Private data for low-level driver */
};

/**
 * @brief  Initialize OSPI peripheral
 * @param  ospi: Pointer to OSPI handle
 * @param  idx: OSPI instance index
 * @return Error code
 */
csi_error_t csi_ospi_init(csi_ospi_t *ospi, uint32_t idx);

/**
 * @brief  De-initialize OSPI peripheral
 * @param  ospi: Pointer to OSPI handle
 * @return None
 */
void csi_ospi_uninit(csi_ospi_t *ospi);

/**
 * @brief  De-initialize OSPI peripheral
 * @param  ospi: Pointer to OSPI handle
 * @return None
 */
csi_error_t csi_ospi_config(csi_ospi_t *ospi, csi_ospi_command_t *cmd);

/**
 * @brief  Attach callback function for OSPI events
 * @param  ospi: Pointer to OSPI handle
 * @param  callback: Event callback function
 * @param  arg: User-defined argument passed to callback
 * @return Error code
 */
csi_error_t csi_ospi_attach_callback(csi_ospi_t *ospi, void *callback, void *arg);

/**
 * @brief  Detach OSPI event callback
 * @param  ospi: Pointer to OSPI handle
 * @return None
 */
void csi_ospi_detach_callback(csi_ospi_t *ospi);

/**
 * @brief  Set OSPI master or slave mode
 * @param  ospi: Pointer to OSPI handle
 * @param  mode: Operating mode
 * @return Error code
 */
csi_error_t csi_ospi_mode(csi_ospi_t *ospi, csi_ospi_mode_t mode);

/**
 * @brief  Set OSPI clock polarity and phase
 * @param  ospi: Pointer to OSPI handle
 * @param  format: Clock format
 * @return Error code
 */
csi_error_t csi_ospi_cp_format(csi_ospi_t *ospi, csi_ospi_cp_format_t format);

/**
 * @brief  Set OSPI baud rate
 * @param  ospi: Pointer to OSPI handle
 * @param  baud: Target baud rate
 * @return Actual configured baud rate
 */
uint32_t csi_ospi_baud(csi_ospi_t *ospi, uint32_t baud);

/**
 * @brief  Send data in blocking mode
 * @param  ospi: Pointer to OSPI handle
 * @param  data: Pointer to data buffer
 * @param  size: Number of bytes to send
 * @param  timeout: Timeout in milliseconds
 * @return Number of bytes sent or error code
 */
int32_t csi_ospi_send(csi_ospi_t *ospi, const void *data, uint32_t size, uint32_t timeout);

/**
 * @brief  Send data in non-blocking asynchronous mode
 * @param  ospi: Pointer to OSPI handle
 * @param  data: Pointer to data buffer
 * @param  size: Number of bytes to send
 * @return Error code
 */
csi_error_t csi_ospi_send_async(csi_ospi_t *ospi, const void *data, uint32_t size);

/**
 * @brief  Receive data in blocking mode
 * @param  ospi: Pointer to OSPI handle
 * @param  data: Pointer to receive buffer
 * @param  size: Number of bytes to receive
 * @param  timeout: Timeout in milliseconds
 * @return Number of bytes received or error code
 */
int32_t csi_ospi_receive(csi_ospi_t *ospi, void *data, uint32_t size, uint32_t timeout);

/**
 * @brief  Receive data in non-blocking asynchronous mode
 * @param  ospi: Pointer to OSPI handle
 * @param  data: Pointer to receive buffer
 * @param  size: Number of bytes to receive
 * @return Error code
 */
csi_error_t csi_ospi_receive_async(csi_ospi_t *ospi, void *data, uint32_t size);

/**
 * @brief  Full-duplex send and receive in blocking mode
 * @param  ospi: Pointer to OSPI handle
 * @param  data_out: Pointer to transmit buffer
 * @param  data_in: Pointer to receive buffer
 * @param  size: Number of bytes to transfer
 * @param  timeout: Timeout in milliseconds
 * @return Number of bytes transferred or error code
 */
int32_t csi_ospi_send_receive(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size, uint32_t timeout);

/**
 * @brief  Full-duplex send and receive in non-blocking asynchronous mode
 * @param  ospi: Pointer to OSPI handle
 * @param  data_out: Pointer to transmit buffer
 * @param  data_in: Pointer to receive buffer
 * @param  size: Number of bytes to transfer
 * @return Error code
 */
csi_error_t csi_ospi_send_receive_async(csi_ospi_t *ospi, const void *data_out, void *data_in, uint32_t size);

/**
 * @brief  Select slave device in master mode
 * @param  ospi: Pointer to OSPI handle
 * @param  slave_num: Slave index
 * @return None
 */
void csi_ospi_select_slave(csi_ospi_t *ospi, uint32_t slave_num);

/**
 * @brief  Link DMA channels to OSPI
 * @param  ospi: Pointer to OSPI handle
 * @param  tx_dma: TX DMA channel handle, NULL to unlink
 * @param  rx_dma: RX DMA channel handle, NULL to unlink
 * @return Error code
 */
csi_error_t csi_ospi_link_dma(csi_ospi_t *ospi, csi_dma_ch_t *tx_dma, csi_dma_ch_t *rx_dma);

/**
 * @brief  Get current OSPI state
 * @param  ospi: Pointer to OSPI handle
 * @param  state: Pointer to state structure
 * @return Error code
 */
csi_error_t csi_ospi_get_state(csi_ospi_t *ospi, csi_state_t *state);

/**
 * @brief  Enable OSPI power management
 * @param  ospi: Pointer to OSPI handle
 * @return Error code
 */
csi_error_t csi_ospi_enable_pm(csi_ospi_t *ospi);

/**
 * @brief  Disable OSPI power management
 * @param  ospi: Pointer to OSPI handle
 * @return None
 */
void csi_ospi_disable_pm(csi_ospi_t *ospi);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_OSPI_H_ */
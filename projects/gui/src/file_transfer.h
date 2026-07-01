/******************************************************************************
 * @file     file_transfer.h
 * @brief    JPG 文件传输协议状态机，底层 transport 可替换为 UART/BLE
 ******************************************************************************/

#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <stdint.h>
#include "transport_if.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FT_MAGIC_VALUE          0x5547504AUL  /* 小端字节序对应 "JPGU" */
#define FT_VERSION              1U

#define FT_CMD_START            0x01U
#define FT_CMD_DATA             0x02U
#define FT_CMD_END              0x03U
#define FT_CMD_FORMAT           0x04U
#define FT_CMD_ACK              0x80U
#define FT_CMD_NACK             0x81U

#define FT_ERR_CRC              1U
#define FT_ERR_SEQ              2U
#define FT_ERR_OFFSET           3U
#define FT_ERR_FS_WRITE         4U
#define FT_ERR_FILE_CRC         5U
#define FT_ERR_PARAM            6U
#define FT_ERR_TIMEOUT          7U
#define FT_ERR_STORAGE_FULL     8U

#define FT_DEFAULT_CHUNK_SIZE   240U
#define FT_MAX_CHUNK_SIZE       512U
#define FT_MAX_FILENAME_LEN     64U

typedef enum {
    FT_STATE_IDLE = 0,
    FT_STATE_WAIT_START,
    FT_STATE_RECV_DATA,
    FT_STATE_VERIFY_FILE,
    FT_STATE_RENAME_FILE,
    FT_STATE_DECODE_DISPLAY,
    FT_STATE_ERROR,
} file_transfer_state_t;

int file_transfer_init(const transport_t *transport);
void file_transfer_poll(void);
file_transfer_state_t file_transfer_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* FILE_TRANSFER_H */

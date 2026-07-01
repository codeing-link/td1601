/******************************************************************************
 * @file     transport_if.h
 * @brief    图片传输底层抽象接口，UART/BLE 统一使用同一套上层协议
 ******************************************************************************/

#ifndef TRANSPORT_IF_H
#define TRANSPORT_IF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*transport_rx_callback_t)(const uint8_t *data, uint32_t len);

typedef struct {
    int (*init)(void);
    int (*send)(const uint8_t *data, uint32_t len);
    int (*recv)(uint8_t *data, uint32_t max_len);
    int (*set_rx_callback)(transport_rx_callback_t cb);
} transport_t;

#ifdef __cplusplus
}
#endif

#endif /* TRANSPORT_IF_H */

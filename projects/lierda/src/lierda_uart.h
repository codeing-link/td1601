#ifndef LIERDA_UART_H
#define LIERDA_UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIERDA_UART_BAUDRATE
#define LIERDA_UART_BAUDRATE       115200U
#endif

#ifndef LIERDA_UART_RX_BUF_SIZE
#define LIERDA_UART_RX_BUF_SIZE    512U
#endif

#ifndef LIERDA_UART_SEND_TIMEOUT_MS
#define LIERDA_UART_SEND_TIMEOUT_MS 1000U
#endif

int32_t lierda_uart_init(void);
int32_t lierda_uart_send(const uint8_t *data, uint32_t len);
uint32_t lierda_uart_available(void);
uint32_t lierda_uart_read(uint8_t *data, uint32_t len);
uint8_t lierda_uart_rx_overflowed(void);
void lierda_uart_clear_rx(void);

#ifdef __cplusplus
}
#endif

#endif

/******************************************************************************
 * @file     transport_uart.c
 * @brief    UART transport 适配层，给 file_transfer 提供统一收发接口
 ******************************************************************************/

#include <stdint.h>
#include "transport_uart.h"
#include "gui_uart.h"

static transport_rx_callback_t s_uart_rx_cb;

static int transport_uart_init(void)
{
    return gui_uart_data_init();
}

static int transport_uart_send(const uint8_t *data, uint32_t len)
{
    return gui_uart_send(data, len);
}

static int transport_uart_recv(uint8_t *data, uint32_t max_len)
{
    return (int)gui_uart_read(data, max_len);
}

static int transport_uart_set_rx_callback(transport_rx_callback_t cb)
{
    /*
     * 当前裸机工程使用 poll 方式从 UART ring buffer 取数。
     * 这里保留回调注册入口，后续如果 UART/DMA 需要事件驱动，可在本层扩展，
     * 不影响 file_transfer 的协议和文件逻辑。
     */
    s_uart_rx_cb = cb;
    (void)s_uart_rx_cb;
    return 0;
}

const transport_t g_uart_transport = {
    .init = transport_uart_init,
    .send = transport_uart_send,
    .recv = transport_uart_recv,
    .set_rx_callback = transport_uart_set_rx_callback,
};

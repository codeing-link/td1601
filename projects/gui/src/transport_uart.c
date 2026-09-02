/******************************************************************************
 * @file     transport_uart.c
 * @brief    图片下载 UART transport，按编译宏选择 PC 或 BLE AT 串口模式
 *
 * 两种模式都使用板级 UART0 的 PA17/PA18：
 *   - GUI_DOWNLOAD_MODE_AT：连接 BLE/AT 模组，默认 115200 8N1；
 *   - GUI_DOWNLOAD_MODE_PC：连接 PC，默认 921600 8N1。
 *
 * 这里不做运行时协议猜测，也不剥 AT 文本头。PC 模式给 PC 工具直通，
 * AT 模式给蓝牙侧直通，稳定性优先。
 ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
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
    if ((data == NULL) || (max_len == 0U)) {
        return 0;
    }

    return (int)gui_uart_read(data, max_len);
}

static int transport_uart_set_rx_callback(transport_rx_callback_t cb)
{
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

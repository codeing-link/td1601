/******************************************************************************
 * @file     transport_ble.c
 * @brief    BLE transport 占位实现
 *
 * 说明：
 *   当前 GUI 硬件上 BLE 模组与 PC 下载共用 PA17/PA18 串口，AT 兼容处理放在
 *   transport_uart.c。本文件保留接口，后续若改成独立 BLE SDK/独立 UART 再扩展。
 ******************************************************************************/

#include <stdint.h>
#include "transport_ble.h"

static transport_rx_callback_t s_ble_rx_cb;

static int transport_ble_init(void)
{
    return -1;
}

static int transport_ble_send(const uint8_t *data, uint32_t len)
{
    (void)data;
    (void)len;
    return -1;
}

static int transport_ble_recv(uint8_t *data, uint32_t max_len)
{
    (void)data;
    (void)max_len;
    return 0;
}

static int transport_ble_set_rx_callback(transport_rx_callback_t cb)
{
    s_ble_rx_cb = cb;
    (void)s_ble_rx_cb;
    return 0;
}

const transport_t g_ble_transport = {
    .init = transport_ble_init,
    .send = transport_ble_send,
    .recv = transport_ble_recv,
    .set_rx_callback = transport_ble_set_rx_callback,
};

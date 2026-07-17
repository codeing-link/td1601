#include <stdint.h>
#include <string.h>
#include <drv/pin.h>
#include <drv/uart.h>
#include "board_config.h"
#include "lierda_uart.h"

static csi_uart_t s_data_uart;
static uint8_t s_rx_buf[LIERDA_UART_RX_BUF_SIZE];
static volatile uint32_t s_rx_head;
static volatile uint32_t s_rx_tail;
static volatile uint8_t s_rx_overflow;
static uint8_t s_irq_buf[32];

static uint32_t next_pos(uint32_t pos)
{
    return (pos + 1U) % LIERDA_UART_RX_BUF_SIZE;
}

static void rx_push(uint8_t data)
{
    uint32_t next = next_pos(s_rx_head);

    if (next == s_rx_tail) {
        s_rx_overflow = 1U;
        return;
    }

    s_rx_buf[s_rx_head] = data;
    s_rx_head = next;
}

static void uart_event_cb(csi_uart_t *uart, csi_uart_event_t event, void *arg)
{
    int32_t count;
    int32_t i;

    (void)arg;

    if (event != UART_EVENT_RECEIVE_FIFO_READABLE) {
        return;
    }

    count = csi_uart_receive(uart, s_irq_buf, sizeof(s_irq_buf), 0U);
    for (i = 0; i < count; i++) {
        rx_push(s_irq_buf[i]);
    }
}

int32_t lierda_uart_init(void)
{
    int32_t ret;

    /* UART1 is independent from the UART0 console used by printf. */
    csi_pin_set_mux(EXAMPLE_PIN_UART_TX1_PORT,
                    EXAMPLE_PIN_UART_TX1,
                    EXAMPLE_PIN_UART_TX_FUNC1);
    csi_pin_set_mux(EXAMPLE_PIN_UART_RX1_PORT,
                    EXAMPLE_PIN_UART_RX1,
                    EXAMPLE_PIN_UART_RX_FUNC1);

    ret = csi_uart_init(&s_data_uart, (uint32_t)EXAMPLE_UART_IDX1);
    if (ret < 0) {
        return ret;
    }

    ret = csi_uart_baud(&s_data_uart, LIERDA_UART_BAUDRATE);
    if (ret < 0) {
        csi_uart_uninit(&s_data_uart);
        return ret;
    }

    ret = csi_uart_format(&s_data_uart,
                          UART_DATA_BITS_8,
                          UART_PARITY_NONE,
                          UART_STOP_BITS_1);
    if (ret < 0) {
        csi_uart_uninit(&s_data_uart);
        return ret;
    }

    lierda_uart_clear_rx();
    ret = csi_uart_attach_callback(&s_data_uart, uart_event_cb, NULL);
    if (ret < 0) {
        csi_uart_uninit(&s_data_uart);
        return ret;
    }

    return 0;
}

int32_t lierda_uart_send(const uint8_t *data, uint32_t len)
{
    if ((data == NULL) || (len == 0U)) {
        return 0;
    }

    return csi_uart_send(&s_data_uart,
                         data,
                         len,
                         LIERDA_UART_SEND_TIMEOUT_MS);
}

uint32_t lierda_uart_available(void)
{
    uint32_t head = s_rx_head;
    uint32_t tail = s_rx_tail;

    if (head >= tail) {
        return head - tail;
    }

    return (LIERDA_UART_RX_BUF_SIZE - tail) + head;
}

uint32_t lierda_uart_read(uint8_t *data, uint32_t len)
{
    uint32_t count = 0U;

    if ((data == NULL) || (len == 0U)) {
        return 0U;
    }

    while ((count < len) && (s_rx_tail != s_rx_head)) {
        data[count++] = s_rx_buf[s_rx_tail];
        s_rx_tail = next_pos(s_rx_tail);
    }

    return count;
}

uint8_t lierda_uart_rx_overflowed(void)
{
    return s_rx_overflow;
}

void lierda_uart_clear_rx(void)
{
    memset(s_rx_buf, 0, sizeof(s_rx_buf));
    s_rx_head = 0U;
    s_rx_tail = 0U;
    s_rx_overflow = 0U;
}

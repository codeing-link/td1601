/******************************************************************************
 * @file     gui_uart.c
 * @brief    GUI 工程普通串口模式：UART0 中断接收 + 阻塞发送
 ******************************************************************************/

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <drv/pin.h>
#include <drv/uart.h>
#include "board_config.h"
#include "gui_uart.h"

extern csi_uart_t console_uart;

static uint8_t s_rx_buf[GUI_UART_RX_BUF_SIZE];
static volatile uint32_t s_rx_head;
static volatile uint32_t s_rx_tail;
static volatile uint8_t s_rx_overflow;
static uint8_t s_irq_tmp[32];

#if GUI_UART_MODE == GUI_UART_MODE_DATA
/*
 * DATA 模式下关闭 printf 输出，避免工程里已有调试日志混入普通串口协议。
 * 返回 0 表示没有实际输出字节。
 */
int __wrap_printf(const char *format, ...)
{
    (void)format;
    return 0;
}

int __wrap_vprintf(const char *format, va_list va)
{
    (void)format;
    (void)va;
    return 0;
}

int __wrap_puts(const char *s)
{
    (void)s;
    return 0;
}

int __wrap_putchar(int c)
{
    (void)c;
    return 0;
}

int __wrap_fputc(int ch, FILE *stream)
{
    (void)ch;
    (void)stream;
    return 0;
}
#else
extern int __real_vprintf(const char *format, va_list va);
extern int __real_puts(const char *s);
extern int __real_putchar(int c);
extern int __real_fputc(int ch, FILE *stream);

int __wrap_printf(const char *format, ...)
{
    int ret;
    va_list va;

    va_start(va, format);
    ret = __real_vprintf(format, va);
    va_end(va);

    return ret;
}

int __wrap_vprintf(const char *format, va_list va)
{
    return __real_vprintf(format, va);
}

int __wrap_puts(const char *s)
{
    return __real_puts(s);
}

int __wrap_putchar(int c)
{
    return __real_putchar(c);
}

int __wrap_fputc(int ch, FILE *stream)
{
    return __real_fputc(ch, stream);
}
#endif

static uint32_t gui_uart_next_pos(uint32_t pos)
{
    return (pos + 1U) % GUI_UART_RX_BUF_SIZE;
}

/* 中断上下文写入环形缓冲区；满了就丢弃新数据并记录溢出标志。 */
static void gui_uart_rx_push(uint8_t data)
{
    uint32_t next = gui_uart_next_pos(s_rx_head);

    if (next == s_rx_tail) {
        s_rx_overflow = 1U;
        return;
    }

    s_rx_buf[s_rx_head] = data;
    s_rx_head = next;
}

/* UART RX FIFO 可读时，尽快把硬件 FIFO 搬到软件环形缓冲区。 */
static void gui_uart_event_cb(csi_uart_t *uart, csi_uart_event_t event, void *arg)
{
    (void)arg;

    if (event == UART_EVENT_RECEIVE_FIFO_READABLE) {
        int32_t n = csi_uart_receive(uart, s_irq_tmp, sizeof(s_irq_tmp), 0);

        if (n > 0) {
            for (int32_t i = 0; i < n; i++) {
                gui_uart_rx_push(s_irq_tmp[i]);
            }
        }
    }
}

int32_t gui_uart_data_init(void)
{
    int32_t ret;

    /* 复用板级 CONSOLE 引脚定义，确保日志模式和普通串口模式使用同一组 UART0 引脚。 */
    csi_pin_set_mux(CONSOLE_TXD_PORT, CONSOLE_TXD, CONSOLE_TXD_FUNC);
    csi_pin_set_mux(CONSOLE_RXD_PORT, CONSOLE_RXD, CONSOLE_RXD_FUNC);

    ret = csi_uart_init(&console_uart, (uint32_t)CONSOLE_IDX);
    if (ret < 0) {
        return ret;
    }

    ret = csi_uart_baud(&console_uart, GUI_UART_DATA_BAUDRATE);
    if (ret < 0) {
        return ret;
    }

    ret = csi_uart_format(&console_uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    if (ret < 0) {
        return ret;
    }

    gui_uart_clear_rx();

    /*
     * attach callback 后驱动会打开接收中断。这里不预挂 receive_async 缓冲区，
     * 让驱动在 RX FIFO 可读时触发 UART_EVENT_RECEIVE_FIFO_READABLE。
     */
    ret = csi_uart_attach_callback(&console_uart, gui_uart_event_cb, NULL);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int32_t gui_uart_send(const uint8_t *data, uint32_t len)
{
    if ((data == NULL) || (len == 0U)) {
        return 0;
    }

    return csi_uart_send(&console_uart, data, len, GUI_UART_SEND_TIMEOUT_MS);
}

uint32_t gui_uart_available(void)
{
    uint32_t head = s_rx_head;
    uint32_t tail = s_rx_tail;

    if (head >= tail) {
        return head - tail;
    }

    return (GUI_UART_RX_BUF_SIZE - tail) + head;
}

uint32_t gui_uart_read(uint8_t *buf, uint32_t len)
{
    uint32_t count = 0;

    if ((buf == NULL) || (len == 0U)) {
        return 0;
    }

    /* 前台读取环形缓冲区；head 只由中断更新，tail 只由前台更新。 */
    while ((count < len) && (s_rx_tail != s_rx_head)) {
        buf[count++] = s_rx_buf[s_rx_tail];
        s_rx_tail = gui_uart_next_pos(s_rx_tail);
    }

    return count;
}

uint8_t gui_uart_rx_overflowed(void)
{
    return s_rx_overflow;
}

void gui_uart_clear_rx(void)
{
    memset(s_rx_buf, 0, sizeof(s_rx_buf));
    s_rx_head = 0U;
    s_rx_tail = 0U;
    s_rx_overflow = 0U;
}

/******************************************************************************
 * @file     gui_uart.h
 * @brief    GUI 工程 UART0 模式开关与普通串口收发接口
 ******************************************************************************/

#ifndef GUI_UART_H
#define GUI_UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 下载模式：默认 AT 蓝牙下载；需要 PC 直连下载时，改成 GUI_DOWNLOAD_MODE_PC。 */
#define GUI_DOWNLOAD_MODE_PC    0
#define GUI_DOWNLOAD_MODE_AT    1

#ifndef GUI_DOWNLOAD_MODE
#define GUI_DOWNLOAD_MODE       GUI_DOWNLOAD_MODE_AT
#endif

#if (GUI_DOWNLOAD_MODE != GUI_DOWNLOAD_MODE_AT) && \
    (GUI_DOWNLOAD_MODE != GUI_DOWNLOAD_MODE_PC)
#error "GUI_DOWNLOAD_MODE must be GUI_DOWNLOAD_MODE_AT or GUI_DOWNLOAD_MODE_PC"
#endif

/* UART0 作为日志串口使用，printf 正常输出调试日志。 */
#define GUI_UART_MODE_LOG       0

/* UART0 作为普通数据串口使用，接收走中断，业务层通过 gui_uart_read() 取数。 */
#define GUI_UART_MODE_DATA      1

/*
 * 模式开关：
 *   GUI_UART_MODE_LOG  - 日志串口，printf 正常输出调试日志。
 *   GUI_UART_MODE_DATA - 默认普通串口，printf 被重定向为空函数。
 *
 * 注意：UART0 只有一路硬件资源。DATA 模式下日志默认关闭，避免日志字节混入业务数据。
 */
#ifndef GUI_UART_MODE
#define GUI_UART_MODE           GUI_UART_MODE_DATA
#endif

/* 下载串口参数：AT 模式默认 115200，PC 模式默认 921600。 */
#ifndef GUI_UART_DATA_BAUDRATE
#if GUI_DOWNLOAD_MODE == GUI_DOWNLOAD_MODE_AT
#define GUI_UART_DATA_BAUDRATE  115200U
#else
#define GUI_UART_DATA_BAUDRATE  921600U
#endif
#endif

/* 中断接收环形缓冲区大小。921600 下建议至少 4KB，避免刷屏时短时积压。 */
#ifndef GUI_UART_RX_BUF_SIZE
#define GUI_UART_RX_BUF_SIZE    4096U
#endif

/* 阻塞发送超时时间，单位 ms。 */
#ifndef GUI_UART_SEND_TIMEOUT_MS
#define GUI_UART_SEND_TIMEOUT_MS 1000U
#endif

int32_t gui_uart_data_init(void);
int32_t gui_uart_send(const uint8_t *data, uint32_t len);
uint32_t gui_uart_available(void);
uint32_t gui_uart_read(uint8_t *buf, uint32_t len);
uint8_t gui_uart_rx_overflowed(void);
void gui_uart_clear_rx(void);

#ifdef __cplusplus
}
#endif

#endif /* GUI_UART_H */

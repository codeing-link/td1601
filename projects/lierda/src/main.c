#include <stdio.h>
#include <stdint.h>
#include <drv/tick.h>
#include "board_init.h"
#include "gpio_toggle.h"
#include "lierda_uart.h"

int main(void)
{
    static const uint8_t ready[] = "lierda UART1 ready, input will be echoed.\r\n";
    uint8_t rx_data[64];
    uint32_t rx_len;
    int32_t ret;

    ret = board_init();
    if (ret < 0) {
        while (1) {
        }
    }

    printf("\r\n[lierda] TD1601 UART demo started\r\n");
    printf("[lierda] console: UART0 PA18/PA17, 921600 8N1\r\n");

    ret = gpio_toggle_init();
    if (ret < 0) {
        printf("[lierda] PA24 LED init failed: %d\r\n", (int)ret);
        while (1) {
        }
    }
    printf("[lierda] LED: PA24, toggle every 1000 ms\r\n");

    ret = lierda_uart_init();
    if (ret < 0) {
        printf("[lierda] UART1 init failed: %d\r\n", (int)ret);
        while (1) {
        }
    }

    printf("[lierda] data: UART1 PA28/PA27, %lu 8N1\r\n",
           (unsigned long)LIERDA_UART_BAUDRATE);
    (void)lierda_uart_send(ready, sizeof(ready) - 1U);

    while (1) {
        gpio_toggle();

        rx_len = lierda_uart_read(rx_data, sizeof(rx_data));
        if (rx_len > 0U) {
            (void)lierda_uart_send(rx_data, rx_len);
        }

        if (lierda_uart_rx_overflowed() != 0U) {
            printf("[lierda] warning: UART1 RX buffer overflow\r\n");
            lierda_uart_clear_rx();
        }

        mdelay(1U);
    }
}

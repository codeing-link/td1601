


/******************************************************************************
 * @file     example_uart_intr.c
 * @brief    the main function for the UART driver intr mode
 * @version  V1.0
 * @date     18. Mar 2020
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <drv/uart.h>
#include <drv/tick.h>
#include <soc.h>
#include <board_init.h>
#include <board_config.h>

static csi_uart_t g_uart;
static volatile uint8_t rx_async_flag = 0;
static volatile uint8_t tx_async_flag = 0;
static volatile uint32_t rx_fifo_left = 0;
const uint8_t tx_test_msg[] = "I am uart";
const uint8_t str_yes[] = "yes";
static uint8_t left_buf[16], recv_buf[16];

#define EXAMPLE_UART_BAUDRATE   115200
#define UART_CHECK_RETURN(ret)                      \
    do {                                        \
        if (ret != CSI_OK) {                    \
            return -1;                          \
        }                                       \
    } while(0);

static void uart_event_cb(csi_uart_t *uart, csi_uart_event_t event, void *arg)
{
    switch (event) {
        case UART_EVENT_SEND_COMPLETE:
            tx_async_flag = 1;
            break;

        case UART_EVENT_RECEIVE_COMPLETE:
            rx_async_flag = 1;
            break;

        case UART_EVENT_RECEIVE_FIFO_READABLE:
            rx_fifo_left = csi_uart_receive(uart, left_buf, 16, 0);
            break;

        default:
            break;
    }
}

/* use console uart to show how to use intr mode */
int example_uart_intr()
{
    csi_error_t ret;

    /* init uart */
    ret = csi_uart_init(&g_uart, EXAMPLE_UART_IDX);
    UART_CHECK_RETURN(ret);

    /* set uart baudrate */
    ret = csi_uart_baud(&g_uart, EXAMPLE_UART_BAUDRATE);
    UART_CHECK_RETURN(ret);

    /* set uart format */
    ret = csi_uart_format(&g_uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    UART_CHECK_RETURN(ret);

    /* attach callback to uart device, the type should be the same with csi_uart_callback_t */
    ret = csi_uart_attach_callback(&g_uart, uart_event_cb, NULL);
    UART_CHECK_RETURN(ret);

    printf("# Start send buffer\n# when you see :  %s\n# It means send over\n\n\n", tx_test_msg);
    ret = csi_uart_send_async(&g_uart, tx_test_msg, sizeof(tx_test_msg));
    UART_CHECK_RETURN(ret);

    while (1) {
        if (tx_async_flag) {
            tx_async_flag = 0;
            break;
        }
    }

    printf("\n\n\n# Start receive buffer\n");
    printf("# If you can see : %s\n\n# Please input : yes\n\n", tx_test_msg);

    do {
        ret = csi_uart_receive_async(&g_uart, recv_buf, 3);
        UART_CHECK_RETURN(ret);

        while (1) {
            if (rx_async_flag) {
                rx_async_flag = 0;
                break;
            }
        }

        recv_buf[3] = '\0';
        printf("\n%s\n\n", recv_buf);

        if (0 == memcmp(recv_buf, str_yes, sizeof(str_yes))) {
            printf("# The uart_intr example is done.\n");
            break;
        } else {
            printf("# Please input again:\n");
        }
    } while (1);

    /* Detach the uart callback */
    csi_uart_detach_callback(&g_uart);

    /* Uninit the uart device */
    csi_uart_uninit(&g_uart);
    return 0;
}

int main(void)
{
    int ret;

    board_init();

    ret = example_uart_intr();

    if (ret != 0) {
        printf("Uart intr example fail.\n");
    }

    return ret;
}

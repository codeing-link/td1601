


/******************************************************************************
 * @file     example_uart_dma.c
 * @brief    the main function for the UART driver dma mode
 * @version  V1.0
 * @date     19. Mar 2020
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <drv/uart.h>
#include <drv/tick.h>
#include <soc.h>
#include <board_init.h>
#include <board_config.h>

static csi_uart_t g_uart;
static csi_dma_ch_t g_dma_ch_tx;
static csi_dma_ch_t g_dma_ch_rx;
static volatile uint8_t rx_async_flag = 0;
static volatile uint8_t tx_async_flag = 0;
const uint8_t tx_test_msg[] = "I am uart";
const uint8_t str_yes[] = "yes";
static uint8_t recv_buf[32] __attribute__((aligned(32)));
static char send_buf[128];

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

        default:
            break;
    }
}

/* use console uart to show how to use dma mode */
int example_uart_dma()
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

    /* Link DMA */
    ret = csi_uart_link_dma(&g_uart, &g_dma_ch_tx, &g_dma_ch_rx);
    UART_CHECK_RETURN(ret);

    strcpy(send_buf, "# Start send buffer\n\r# when you see : I am uart\n\r# It means send over\n\r\n\r\n\r");

    ret = csi_uart_send_async(&g_uart, send_buf, strlen(send_buf));
    UART_CHECK_RETURN(ret);

    while (1) {
        if (tx_async_flag) {
            tx_async_flag = 0;
            break;
        }
    }

    memcpy(send_buf, tx_test_msg, sizeof (tx_test_msg));

    ret = csi_uart_send_async(&g_uart, send_buf, sizeof(tx_test_msg));
    UART_CHECK_RETURN(ret);

    while (1) {
        if (tx_async_flag) {
            tx_async_flag = 0;
            break;
        }
    }

    strcpy(send_buf, "\n\r\n\r\n\r# Start receive buffer\n\r# If you can see : I am uart\n\r\n\r# Please input : yes\n\r\n\r");

    ret = csi_uart_send_async(&g_uart, send_buf, strlen(send_buf));
    UART_CHECK_RETURN(ret);

    while (1) {
        if (tx_async_flag) {
            tx_async_flag = 0;
            break;
        }
    }

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

        if (0 == memcmp(recv_buf, str_yes, 3)) {
            strcpy(send_buf, "# The uart_dma example is done.\n\r");
            ret = csi_uart_send_async(&g_uart, send_buf, strlen(send_buf));
            UART_CHECK_RETURN(ret);

            while (1) {
                if (tx_async_flag) {
                    tx_async_flag = 0;
                    break;
                }
            }

            break;
        } else {
            strcpy(send_buf, "# Please input again:\n\r");
            strcat(send_buf, (char *)recv_buf);
            ret = csi_uart_send_async(&g_uart, send_buf, strlen(send_buf));
            UART_CHECK_RETURN(ret);

            while (1) {
                if (tx_async_flag) {
                    tx_async_flag = 0;
                    break;
                }
            }
        }
    } while (1);

    /* Unlink DMA */
    ret = csi_uart_link_dma(&g_uart, NULL, NULL);
    UART_CHECK_RETURN(ret);

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

    ret = example_uart_dma();

    if (ret != 0) {
        printf("Uart dma example fail.\n");
    }

    return ret;
}

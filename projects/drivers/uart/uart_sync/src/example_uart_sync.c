


/******************************************************************************
 * @file     example_uart_sync.c
 * @brief    the main function for the UART driver sync mode
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
const uint8_t tx_test_msg[] = "I am uart";
const uint8_t str_yes[] = "yes";
static uint8_t recv_buf[16];

#define EXAMPLE_UART_BAUDRATE   115200
#define UART_CHECK_RETURN(ret)                  \
    do {                                        \
        if (ret != CSI_OK) {                    \
            return -1;                          \
        }                                       \
    } while(0);

/* use console uart to show how to use intr mode */
int example_uart_sync()
{
    csi_error_t ret= CSI_ERROR;
    int32_t ret_num;
	uint32_t try_cnt = 0;
    /* init uart */
    ret = csi_uart_init(&g_uart, EXAMPLE_UART_IDX);
    UART_CHECK_RETURN(ret);

    /* set uart baudrate */
    ret = csi_uart_baud(&g_uart, EXAMPLE_UART_BAUDRATE);
    UART_CHECK_RETURN(ret);

    /* set uart format */
    ret = csi_uart_format(&g_uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    UART_CHECK_RETURN(ret);

    printf("# Start send buffer\n# when you see :  %s\n# It means send over\n\n\n", tx_test_msg);
    ret_num = csi_uart_send(&g_uart, tx_test_msg, sizeof(tx_test_msg), 50);

    if (ret_num != sizeof(tx_test_msg)) {
        printf("# Send fail\n");
        return -1;
    }

    printf("\n\n\n# Start receive buffer\n");
    printf("# If you can see : %s\n\n# Please input : yes\n\n", tx_test_msg);

    do {
        ret_num = csi_uart_receive(&g_uart, recv_buf, sizeof(str_yes), 0xffffffff);

        if (ret_num != sizeof(str_yes)) {
            printf("# receive data fail\n");
            return -1;
        }

        recv_buf[3] = '\0';
        printf("\n%s\n\n", recv_buf);

        if (0 == memcmp(recv_buf, str_yes, sizeof(str_yes))) {
            printf("# The uart_sync example is done.\n");
			ret = CSI_OK;
            break;
        } else {
            ret_num = csi_uart_receive(&g_uart, recv_buf, sizeof(recv_buf), 0);
            printf("# Please input again:\n");
			ret = CSI_ERROR;
			try_cnt++;
        }
    } while (try_cnt < 3);

    /* Uninit the uart device */
    csi_uart_uninit(&g_uart);
    return 0;
}

int main(void)
{
    int ret;

    board_init();

    ret = example_uart_sync();

    if (ret != 0) {
		CSI_EXAMPLE_RESULT(-1);
    }else {
		CSI_EXAMPLE_RESULT(1);
	}

    return ret;
}

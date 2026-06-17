

/******************************************************************************
 * @file     example_uart_intr.c
 * @brief
 * @version
 * @date     2020-02-12
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include <soc.h>
#include <csi_config.h>
#include <board_config.h>
#include <board_init.h>
#include <drv/uart.h>
#include <drv/tick.h>
#include <drv/pin.h>

#define MAX_SUCCESS_NUM          2
#define DEFAULT_TRANSFER_TIMEOUT 5000
#define TEST_STR                 "1234567890abcdefghijklmnopqrstuvwxyz"
#define TEST_STR_LENGTH          strlen(TEST_STR)
csi_uart_event_t uart_master_event, uart_slave_event;
csi_uart_t uart_master, uart_slave;
csi_dma_ch_t dma_tx_ch[2];
csi_dma_ch_t dma_rx_ch[2];
uint8_t  master_rx_data[64] __attribute__((aligned(32)));
uint8_t  slave_rx_data[64]  __attribute__((aligned(32)));
static volatile uint8_t suc_num = 1;
#define EXAMPLE_UART_BAUDRATE 4000000
#define UART_CHECK_RETURN(ret)                      \
    do {                                        \
        if (ret != CSI_OK) {                    \
            return -1;                          \
        }                                       \
    } while(0);
static void master_uart_event_cb(csi_uart_t *uart, csi_uart_event_t event, void *arg)
{
    uart_master_event = event;
}

static void slave_uart_event_cb(csi_uart_t *uart, csi_uart_event_t event, void *arg)
{
    uart_slave_event = event;
}

void example_pin_uart_init(void)
{
    csi_pin_mode(EXAMPLE_PIN_UART_TX2, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_UART_TX2, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_UART_TX1, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_UART_TX1, GPIO_MODE_PULLUP);
    csi_pin_set_mux(EXAMPLE_PIN_UART_TX2, EXAMPLE_PIN_UART_TX_FUNC2);
    csi_pin_set_mux(EXAMPLE_PIN_UART_RX2, EXAMPLE_PIN_UART_RX_FUNC2);
    csi_pin_set_mux(EXAMPLE_PIN_UART_TX1, EXAMPLE_PIN_UART_TX_FUNC1);
    csi_pin_set_mux(EXAMPLE_PIN_UART_RX1, EXAMPLE_PIN_UART_RX_FUNC1);
}

int example_uart_send_receive_dma(void)
{
    uint32_t timestart;
    int ret = 0;

    /* initialize master uart */
    ret = csi_uart_init(&uart_master, EXAMPLE_UART_IDX2);
    UART_CHECK_RETURN(ret);

    /* set uart baudrate */
    ret = csi_uart_baud(&uart_master, EXAMPLE_UART_BAUDRATE);
    UART_CHECK_RETURN(ret);

    /* set uart format */
    ret = csi_uart_format(&uart_master, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    UART_CHECK_RETURN(ret);

    ret = csi_uart_attach_callback(&uart_master, master_uart_event_cb, NULL);
    UART_CHECK_RETURN(ret);

    ret = csi_uart_link_dma(&uart_master, &dma_tx_ch[0], &dma_rx_ch[0]);
    UART_CHECK_RETURN(ret);

    /* initialize slave uart */
    ret = csi_uart_init(&uart_slave, EXAMPLE_UART_IDX1);
    UART_CHECK_RETURN(ret);

    /* set uart baudrate */
    ret = csi_uart_baud(&uart_slave, EXAMPLE_UART_BAUDRATE);
    UART_CHECK_RETURN(ret);

    /* set uart format */
    ret = csi_uart_format(&uart_slave, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    UART_CHECK_RETURN(ret);

    /* attach callback to uart device, the type should be the same with csi_uart_callback_t */
    ret = csi_uart_attach_callback(&uart_slave, slave_uart_event_cb, NULL);
    UART_CHECK_RETURN(ret);

    ret = csi_uart_link_dma(&uart_slave, &dma_tx_ch[1], &dma_rx_ch[1]);
    UART_CHECK_RETURN(ret);
    /* start transfer */
    uart_master_event = -1;
    uart_slave_event = -1;

    for (ret = 0; ret < TEST_STR_LENGTH; ret++) {
        slave_rx_data[ret] = 0;
        master_rx_data[ret] = 0;
    }

    ret = csi_uart_receive_async(&uart_slave, slave_rx_data, TEST_STR_LENGTH);
    UART_CHECK_RETURN(ret);

    ret = csi_uart_send_async(&uart_master, TEST_STR, TEST_STR_LENGTH);
    UART_CHECK_RETURN(ret);

    /* wait transfer complete */
    timestart = csi_tick_get();

    while ((uart_master_event != UART_EVENT_SEND_COMPLETE) || (uart_slave_event != UART_EVENT_RECEIVE_COMPLETE)) {
        if ((csi_tick_get() - timestart) > DEFAULT_TRANSFER_TIMEOUT) {
            printf("transfer timeout!\n");
            return -1;
        }
    }

    printf("slave receive: %s\n", slave_rx_data);

    if (0 != strcmp((char *)slave_rx_data, TEST_STR)) {
        printf("transfer error!\n");
        return -1;
    }

    uart_master_event = -1;
    uart_slave_event = -1;

    for (ret = 0; ret < TEST_STR_LENGTH; ret++) {
        slave_rx_data[ret] = 0;
        master_rx_data[ret] = 0;
    }

    ret = csi_uart_receive_async(&uart_master, master_rx_data, TEST_STR_LENGTH);
    UART_CHECK_RETURN(ret);

    ret = csi_uart_send_async(&uart_slave, TEST_STR, TEST_STR_LENGTH);
    UART_CHECK_RETURN(ret);

    /* wait transfer complete */
    timestart = csi_tick_get();

    while ((uart_master_event != UART_EVENT_RECEIVE_COMPLETE) || (uart_slave_event != UART_EVENT_SEND_COMPLETE)) {
        if ((csi_tick_get() - timestart) > DEFAULT_TRANSFER_TIMEOUT) {
            printf("transfer timeout!\n");
            return -1;
        }
    }

    printf("master receive: %s\n", master_rx_data);

    if (0 != strcmp((char *)master_rx_data, TEST_STR)) {
        printf("transfer error!\n");
        return -1;
    }
    /* close uart master dma, and open uart slave dma*/
    csi_uart_link_dma(&uart_slave, NULL, NULL);
    csi_uart_link_dma(&uart_master, NULL, NULL);
    /* uninit uart */
    csi_uart_detach_callback(&uart_master);
    csi_uart_detach_callback(&uart_slave);
    csi_uart_uninit(&uart_master);
    csi_uart_uninit(&uart_slave);
    printf("transfer success!\n");
    suc_num++;
    return 0;
}

int main(void)
{
    int ret;

    board_init();
    example_pin_uart_init();

    printf("-------------------------------------\n");
    printf("example_uart_send_receive_dma()\n");
    printf("please connect EXAMPLE_PIN_UART_TX2 to EXAMPLE_PIN_UART_RX1\n");
    printf("please connect EXAMPLE_PIN_UART_RX2 to EXAMPLE_PIN_UART_TX1\n");
    printf("example_uart_send_receive_dma()\n");
    printf("-------------------------------------\n");
    ret = example_uart_send_receive_dma();

    if (ret < 0) {
        printf("run example_uart_send_dma fail\n");
    }

    if (ret < 0) {
        printf("run example_uart_receive_dma fail\n");
    }

    if (MAX_SUCCESS_NUM == suc_num) {
        CSI_EXAMPLE_RESULT(1);
    } else {
        CSI_EXAMPLE_RESULT(-1);
    }

    return 0;
}

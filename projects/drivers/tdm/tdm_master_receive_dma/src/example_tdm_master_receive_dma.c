

/******************************************************************************
 * @file     example_tdm.c
 * @brief    the main function for the TDM driver
 * @version  V1.0
 * @date     18. Jan 2021
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include "board_config.h"
#include "board_init.h"
#include <drv/uart.h>
#include <drv/timer.h>
#include <drv/tdm.h>
#include <drv/pin.h>
#include <soc.h>

csi_tdm_t tdm;

csi_dma_ch_t dma_ch_rx_handle;

#define I2S_SLAVE_RX_BUF_SIZE  2048
uint8_t tdm_slave_rx_buf[I2S_SLAVE_RX_BUF_SIZE];

volatile uint8_t cb_slave_transfer_flag = 0;

uint8_t read_data[2048] __attribute__((aligned(32)));

static void tdm_slave_event_cb_fun(csi_tdm_t *tdm, csi_tdm_event_t event, void *arg)
{
    if (event == TDM_EVENT_RECEIVE_COMPLETE) {
        cb_slave_transfer_flag = 2;
    } else if (event == TDM_EVENT_RX_BUFFER_FULL) {
        cb_slave_transfer_flag = 1;
    }
}

void example_pin_tdm_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SCLK_PORT,EXAMPLE_PIN_TDM_SCLK, EXAMPLE_PIN_TDM_SCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_WSCLK_PORT,EXAMPLE_PIN_TDM_WSCLK, EXAMPLE_PIN_TDM_WSCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA_PORT,EXAMPLE_PIN_TDM_SDA, EXAMPLE_PIN_TDM_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA1_PORT,EXAMPLE_PIN_TDM_SDA1, EXAMPLE_PIN_TDM_SDA1_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA2_PORT,EXAMPLE_PIN_TDM_SDA2, EXAMPLE_PIN_TDM_SDA2_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA3_PORT,EXAMPLE_PIN_TDM_SDA3, EXAMPLE_PIN_TDM_SDA3_FUNC);

    csi_pin_mode(EXAMPLE_PIN_TDM_SDA1_PORT,EXAMPLE_PIN_TDM_SDA1, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_TDM_SDA3_PORT,EXAMPLE_PIN_TDM_SDA3, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_TDM_SCLK_PORT,EXAMPLE_PIN_TDM_SCLK, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_TDM_WSCLK_PORT,EXAMPLE_PIN_TDM_WSCLK, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_TDM_SDA2_PORT,EXAMPLE_PIN_TDM_SDA2, GPIO_MODE_PULLUP);
}
static int test_tdm(void)
{
    csi_error_t ret;
    csi_tdm_format_t tdm_config;
    example_pin_tdm_init();
    uint8_t *data = &tdm_slave_rx_buf[0];
    for (uint32_t i = 0; i < sizeof(tdm_slave_rx_buf); i++) {
        data[i] = i & 0xff;
    }
    ret = csi_tdm_init(&tdm, EXAMPLE_TDM_IDX);
   
    if (ret != CSI_OK) {
        printf("csi_tdm_init error\n");
        return -1;
    }

    csi_tdm_attach_callback(&tdm, tdm_slave_event_cb_fun, NULL);
    tdm_config.rate = TDM_SAMPLE_RATE_48000;
    tdm_config.mode = TDM_MODE_MASTER;
    tdm_config.work_mode = TDM_WORKING_MODE_I2S;
    tdm_config.protocol = TDM_PROTOCOL_STANDARD;
    tdm_config.polarity = TDM_LEFT_DATA_FIRST;
    tdm_config.sample_edge = TDM_SAMPLE_RISING_EDGE;
    tdm_config.width = TDM_SAMPLE_WIDTH_16BIT;
    tdm_config.sclk_nfs = TDM_SCLK_256FS;

    csi_tdm_format(&tdm, &tdm_config);
    csi_tdm_rx_link_dma(&tdm, &dma_ch_rx_handle);

    printf("start tdm dma receive\n");

    csi_tdm_enable(&tdm, 1);
    while(1) {
        csi_tdm_receive_async(&tdm, read_data, sizeof(read_data));
        while(cb_slave_transfer_flag != 2);
        cb_slave_transfer_flag = 0;
        printf("receive data is:");

        for (uint32_t i = 0; i < sizeof(read_data); i++) {
            if ((i % 8) == 0) {
                printf("\n");
            }

            printf("%2x ", read_data[i]);
        }

        if (memcmp(tdm_slave_rx_buf, read_data, sizeof(read_data))) {
            printf("memcmp failed\r\n");
        } else {
            printf("tdm receive dma test success\r\n");
            break;
        }
    }

    csi_tdm_receive_stop(&tdm);
    csi_tdm_enable(&tdm, 0);
    csi_tdm_rx_link_dma(&tdm, NULL);
    csi_tdm_detach_callback(&tdm);
    csi_tdm_uninit(&tdm);

    return 0;
}

int example_tdm(void)
{
    int ret;

    ret = test_tdm();

    if (ret < 0) {
        printf("test_tdm fail\n");
        return -1;
    }

    return ret;
}

int main(void)
{
    board_init();

    printf("Example for tdm receive dma\n");

    example_tdm();

    return 0;
}

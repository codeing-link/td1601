

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

csi_dma_ch_t dma_ch_rx_handle0;

#define TDM_RX_BUF_SIZE 2048
static uint32_t tdm_tx_buf0[TDM_RX_BUF_SIZE];

csi_ringbuffer_t rx_ring_buffer0;

#define READ_BUF_SIZE 2048
uint32_t read_data0[READ_BUF_SIZE];

volatile uint8_t cb_transfer_flag0 = 0U;
volatile uint8_t cb_master_transfer_flag = 0;
static void tdm_master_event_cb_fun(csi_tdm_t *tdm, csi_tdm_event_t event, void *arg)
{
    if (event == TDM_EVENT_SEND_COMPLETE) {
        cb_master_transfer_flag = 1;
    }
}

void example_pin_tdm_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SCLK, EXAMPLE_PIN_TDM_SCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_WSCLK, EXAMPLE_PIN_TDM_WSCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA, EXAMPLE_PIN_TDM_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA1, EXAMPLE_PIN_TDM_SDA1_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA2, EXAMPLE_PIN_TDM_SDA2_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_TDM_SDA3, EXAMPLE_PIN_TDM_SDA3_FUNC);

    csi_pin_mode(EXAMPLE_PIN_TDM_SDA1, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_TDM_SDA3, GPIO_MODE_PULLUP);
}
static int test_tdm(void)
{
    csi_error_t ret;
    csi_tdm_format_t tdm_config;
    csi_pin_set_mux(EXAMPLE_I2S_MCLK,   EXAMPLE_I2S_MCLK_FUNC);
    example_pin_tdm_init();

    uint8_t *data = (uint8_t *)&tdm_tx_buf0[0];
    for (uint32_t i = 0; i < sizeof(tdm_tx_buf0); i++) {
        data[i] = i & 0xff;
    }
    ret = csi_tdm_init(&tdm, EXAMPLE_TDM_IDX);
   
    if (ret != CSI_OK) {
        printf("csi_tdm_init error\n");
        return -1;
    }

    csi_tdm_attach_callback(&tdm, tdm_master_event_cb_fun, NULL);
    tdm_config.rate = TDM_SAMPLE_RATE_48000;
    tdm_config.mode = TDM_MODE_SLAVE;
    tdm_config.work_mode = TDM_WORKING_MODE_I2S;
    tdm_config.protocol = TDM_PROTOCOL_STANDARD;
    tdm_config.polarity = TDM_LEFT_DATA_FIRST;
    tdm_config.sample_edge = TDM_SAMPLE_RISING_EDGE;
    tdm_config.width = TDM_SAMPLE_WIDTH_16BIT;
    tdm_config.sclk_nfs = TDM_SCLK_256FS;

    csi_tdm_format(&tdm, &tdm_config);

    csi_tdm_enable(&tdm, true);

    csi_pin_set_mux(EXAMPLE_I2S_WSCLK,  EXAMPLE_I2S_WSCLK_FUNC);
    csi_pin_set_mux(EXAMPLE_I2S_SCLK,   EXAMPLE_I2S_SCLK_FUNC);
    csi_tdm_send_async(&tdm, tdm_tx_buf0, sizeof(tdm_tx_buf0));

    while(!cb_master_transfer_flag);
    cb_master_transfer_flag = 0;
    mdelay(2);
    csi_tdm_send_async(&tdm, tdm_tx_buf0, sizeof(tdm_tx_buf0));
    while(!cb_master_transfer_flag);
    udelay(20);
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

    printf("Example for tdm send async\n");

    example_tdm();

    return 0;
}

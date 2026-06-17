

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
#include <drv/iic.h>
#include <drv/pin.h>
#include <soc.h>
#include <es8311.h>

typedef struct __attribute__((packed))
{
    char chunkId[4];    // "RIFF"
    uint32_t chunkSize; // Size of the rest of this chunk
    char format[4];     // "WAVE"
} RiffChunk;

typedef struct __attribute__((packed))
{
    char subChunk1Id[4];    // "fmt "
    uint32_t subChunk1Size; // Size of the rest of this subchunk (usually 16 for PCM)
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} WavFmtChunk;

typedef struct __attribute__((packed))
{
    char Data1Id[4]; // "fmt "
    uint32_t DataSize;
} WavDataChunk;

csi_tdm_t s_tdm;
csi_dma_ch_t dma_ch_rx_handle;
csi_dma_ch_t dma_ch_tx_handle;

#define READ_BUF_SIZE 2048
int16_t read_data0[READ_BUF_SIZE];

int16_t read_data1[READ_BUF_SIZE];

int16_t dest_buf[READ_BUF_SIZE];

uint8_t ping, pong;
int16_t *data_ptr;

volatile uint8_t cb_master_transfer_flag = 0;
uint8_t     end_ping, end_pong;
volatile uint8_t cb_master_transfer_flag_send = 0;
static void tdm_master_event_cb_fun(csi_tdm_t *tdm, csi_tdm_event_t event, void *arg)
{
    if (event == TDM_EVENT_RECEIVE_COMPLETE) {
        cb_master_transfer_flag = 1;
        if (ping) {
            ping = 0;
            pong = 1;
            data_ptr = read_data0;
            end_pong = 1;
            end_ping = 0;
        } else if (pong) {
            pong = 0;
            ping = 1;
            data_ptr = read_data1;
            end_ping = 1;
            end_pong = 0;
        }
        csi_tdm_receive_async(&s_tdm, data_ptr, sizeof(read_data0));
    } else if (event == TDM_EVENT_SEND_COMPLETE) {
        cb_master_transfer_flag_send = 1;
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
    ret = csi_tdm_init(&s_tdm, EXAMPLE_TDM_IDX);
   
    if (ret != CSI_OK) {
        printf("csi_tdm_init error\n");
        return -1;
    }
    csi_tdm_attach_callback(&s_tdm, tdm_master_event_cb_fun, NULL);
    tdm_config.rate = TDM_SAMPLE_RATE_48000;
    tdm_config.mode = TDM_MODE_MASTER;
    tdm_config.work_mode = TDM_WORKING_MODE_I2S;
    tdm_config.protocol = TDM_PROTOCOL_STANDARD;
    tdm_config.polarity = TDM_LEFT_DATA_FIRST;
    tdm_config.sample_edge = TDM_SAMPLE_RISING_EDGE;
    tdm_config.width = TDM_SAMPLE_WIDTH_16BIT;
    tdm_config.sclk_nfs = TDM_SCLK_256FS;

    csi_tdm_format(&s_tdm, &tdm_config);
    csi_tdm_rx_link_dma(&s_tdm, &dma_ch_rx_handle);
    csi_tdm_tx_link_dma(&s_tdm, &dma_ch_tx_handle);
    memset(read_data0, 0, sizeof(read_data0));

    ping = 1;
    pong = 0;
    data_ptr = read_data1;
    csi_tdm_enable(&s_tdm, true);
    csi_tdm_receive_async(&s_tdm, data_ptr, sizeof(read_data0));

    while (1) {
        while(!cb_master_transfer_flag);
        cb_master_transfer_flag = 0;

        if (end_ping == 1) {
			printf("1%hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx\n", read_data0[0], read_data0[1],  read_data0[2], read_data0[3], read_data0[4], read_data0[5],  read_data0[6], read_data0[7]
            ,read_data0[8], read_data0[9],  read_data0[10], read_data0[11], read_data0[12], read_data0[13],  read_data0[14], read_data0[15]);
            memcpy(dest_buf, read_data0, sizeof(read_data0));
            csi_tdm_send_async(&s_tdm, dest_buf, sizeof(dest_buf));
        } else if (end_pong == 1) {
			printf("2%hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx %hx\n", read_data1[0], read_data1[1],  read_data1[2], read_data1[3], read_data1[4], read_data1[5],  read_data1[6], read_data1[7]
            ,read_data1[8], read_data1[9],  read_data1[10], read_data1[11], read_data1[12], read_data1[13],  read_data1[14], read_data1[15]);
            memcpy(dest_buf, read_data1, sizeof(read_data1));
            csi_tdm_send_async(&s_tdm, dest_buf, sizeof(dest_buf));
        }
    }
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
    soc_set_sys_freq(CPU_196_608MHZ);
    board_init();

    printf("Example for tdm receive dma\n");

    example_tdm();

    return 0;
}

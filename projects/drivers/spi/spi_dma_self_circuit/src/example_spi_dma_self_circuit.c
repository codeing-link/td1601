

/******************************************************************************
 * @file     example_spi_intr.c
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
#include <drv/spi.h>
#include <drv/tick.h>
#include <drv/pin.h>

#define MAX_SUCCESS_NUM          1
#define DEFAULT_TRANSFER_TIMEOUT 5000
#define TEST_STR                 "1234567890abcdefghijklmnopqrstuvwxyz"
#define TEST_STR_LENGTH          strlen(TEST_STR)
csi_spi_event_t spi_master_event, spi_slave_event;
csi_spi_t spi_master, spi_slave;
csi_dma_ch_t dma_tx_ch[2];
csi_dma_ch_t dma_rx_ch[2];
uint8_t  master_rx_data[64] __attribute__((aligned(32)));
uint8_t  slave_rx_data[64]  __attribute__((aligned(32)));
static volatile uint8_t suc_num = 0;

static void master_spi_event_cb(csi_spi_t *spi, csi_spi_event_t event, void *arg)
{
    spi_master_event = event;
}

void example_pin_spi_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MISO_PORT,EXAMPLE_PIN_SPI_MISO, EXAMPLE_PIN_SPI_MISO_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MOSI_PORT,EXAMPLE_PIN_SPI_MOSI, EXAMPLE_PIN_SPI_MOSI_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_CS, EXAMPLE_PIN_SPI_CS_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_SCK_PORT,EXAMPLE_PIN_SPI_SCK, EXAMPLE_PIN_SPI_SCK_FUNC);
}

int example_spi_send_receive_dma(void)
{
    uint32_t timestart;
    int      ret = 0;

    /* initialize master spi */
    ret = csi_spi_init(&spi_master, EXAMPLE_SPI_IDX);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_attach_callback(&spi_master, master_spi_event_cb, NULL);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_mode(&spi_master, SPI_MASTER);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_cp_format(&spi_master, SPI_FORMAT_CPOL1_CPHA1);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_frame_len(&spi_master, SPI_FRAME_LEN_8);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_baud(&spi_master, 2000000);

    if (ret == 0) {
        return -1;
    }

    ret = csi_spi_link_dma(&spi_master, &dma_tx_ch[0], &dma_rx_ch[0]);

    if (ret != CSI_OK) {
        return -1;
    }

    csi_spi_select_slave(&spi_master, 0);
    /* start transfer */
    spi_master_event = -1;

    for (ret = 0; ret < TEST_STR_LENGTH; ret++) {
        slave_rx_data[ret] = 0xFF;
        master_rx_data[ret] = 0xFF;
    }

    ret = csi_spi_send_receive_async(&spi_master, TEST_STR, master_rx_data, TEST_STR_LENGTH);

    if (ret != CSI_OK) {
        return -1;
    }

    /* wait transfer complete */
    timestart = csi_tick_get();

    while ((spi_master_event != SPI_EVENT_SEND_RECEIVE_COMPLETE)) {
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
    /* close spi master dma, and open spi slave dma*/
    csi_spi_link_dma(&spi_master, NULL, NULL);

    /* uninit spi */
    csi_spi_detach_callback(&spi_master);
    csi_spi_uninit(&spi_master);
    printf("transfer success!\n");
    suc_num++;
    return 0;
}


int main(void)
{
    int ret;

    board_init();
    example_pin_spi_init();

    printf("\n\n\n");
    printf("-------------------------------------\n");
    printf("example_spi_send_receive_dma()\n");
    printf("please connect MISO and MOSI\n");
    printf("-------------------------------------\n");
    ret = example_spi_send_receive_dma();

    if (ret < 0) {
        printf("run example_spi_send_receive_dma fail\n");
    }

    if (MAX_SUCCESS_NUM == suc_num) {
        CSI_EXAMPLE_RESULT(1);
    } else {
        CSI_EXAMPLE_RESULT(-1);
    }

    return 0;
}

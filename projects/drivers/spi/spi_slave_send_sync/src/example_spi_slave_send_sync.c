

/******************************************************************************
 * @file     example_spi_intr.c
 * @brief
 * @version
 * @date     2020-02-12
 ******************************************************************************/

#include "stdio.h"
#include <soc.h>
#include <string.h>
#include <csi_config.h>
#include <board_init.h>
#include <board_config.h>
#include <drv/spi.h>
#include <drv/pin.h>
#include <drv/tick.h>

#define MAX_SUCCESS_NUM          1
#define DEFAULT_TRANSFER_TIMEOUT 5000
#define TEST_FREQ                25000000
#define TEST_STR                 "1234567890abcdefghijklmnopqrstuvwxyz"
#define TEST_STR_LENGTH          strlen(TEST_STR)
csi_spi_t spi_slave;
static volatile uint8_t suc_num = 0;

void example_pin_spi_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MISO_PORT,EXAMPLE_PIN_SPI_MISO, EXAMPLE_PIN_SPI_MISO_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MOSI_PORT,EXAMPLE_PIN_SPI_MOSI, EXAMPLE_PIN_SPI_MOSI_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_CS, EXAMPLE_PIN_SPI_CS_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_SCK_PORT,EXAMPLE_PIN_SPI_SCK, EXAMPLE_PIN_SPI_SCK_FUNC);
    csi_pin_mode(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_SLAVE_CS, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_SPI_SCK_PORT,EXAMPLE_PIN_SPI_SLAVE_SCK, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_SPI_MOSI_PORT,EXAMPLE_PIN_SPI_SLAVE_MOSI, GPIO_MODE_PULLUP);
    csi_pin_mode(EXAMPLE_PIN_SPI_MISO_PORT,EXAMPLE_PIN_SPI_SLAVE_MISO, GPIO_MODE_PULLUP);
}

int example_spi_send_sync(void)
{
    int      ret = 0;
    /* initialize master spi */
    ret = csi_spi_init(&spi_slave, EXAMPLE_SPI_SLAVE_IDX);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_mode(&spi_slave, SPI_SLAVE);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_cp_format(&spi_slave, SPI_FORMAT_CPOL0_CPHA0);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_frame_len(&spi_slave, SPI_FRAME_LEN_8);

    if (ret != CSI_OK) {
        return -1;
    }

    // ret = csi_spi_baud(&spi_slave, TEST_FREQ);

    // if (ret == 0) {
    //     return -1;
    // }

    csi_spi_select_slave(&spi_slave, 0);

    ret = csi_spi_send(&spi_slave, TEST_STR, TEST_STR_LENGTH, -1);

    if (ret != TEST_STR_LENGTH) {
        return -1;
    }

    csi_spi_uninit(&spi_slave);
    printf("transfer success!\n");
    suc_num++;
    return 0;
}

int main(void)
{
    int ret;

    board_init();
    example_pin_spi_init();

    printf("-------------------------------------\n");
    printf("example_spi_send_sync()\n");
    printf("-------------------------------------\n");
    ret = example_spi_send_sync();

    if (ret < 0) {
        printf("run example_spi_send_sync fail\n");
    }

    if (MAX_SUCCESS_NUM == suc_num) {
        CSI_EXAMPLE_RESULT(1);
    } else {
        CSI_EXAMPLE_RESULT(-1);
    }

    return 0;
}

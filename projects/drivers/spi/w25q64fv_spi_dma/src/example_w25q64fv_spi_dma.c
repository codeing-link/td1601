


/******************************************************************************
 * @file     example_spi.c
 * @brief    the main function for the SPI driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <stdio.h>
#include <string.h>

#include <soc.h>
#include <drv/spi.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <csi_config.h>
#include <board_config.h>
#include <board_init.h>

#include "w25q64fv.h"


/* example parameters */
#define MAX_SUCCESS_NUM                    3
#define EXAMPLE_FLASH_START_ADDRESS        0x50
#define EXAMPLE_FLASH_WRITE_SIZE           0x240
#define EXAMPLE_FLASH_SECTOR_ERASE_NUM     2

static csi_gpio_t gpio;
static csi_spi_t  spi;
static flash_t    flash;
static bindings_t binding;
static uint8_t tx_data[EXAMPLE_FLASH_WRITE_SIZE] __attribute__((aligned(32)));;
static uint8_t rx_data[EXAMPLE_FLASH_WRITE_SIZE] __attribute__((aligned(32)));;
static volatile uint8_t suc_num = 0;
volatile csi_spi_event_t spi_master_event;
csi_dma_ch_t dma_rx_ch[2];
static void example_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MISO_PORT,EXAMPLE_PIN_SPI_MISO, EXAMPLE_PIN_SPI_MISO_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MOSI_PORT,EXAMPLE_PIN_SPI_MOSI, EXAMPLE_PIN_SPI_MOSI_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_SCK_PORT,EXAMPLE_PIN_SPI_SCK, EXAMPLE_PIN_SPI_SCK_FUNC);

}

static void uninit()
{
    csi_spi_detach_callback(&spi);
    csi_spi_uninit(&spi);
}

static void soft_cs_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_CS,   EXAMPLE_PIN_SPI_SOFT_CS_FUNC);
    csi_gpio_init(&gpio, EXAMPLE_PIN_SPI_CS_PORT);
    csi_gpio_dir(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_DIRECTION_OUTPUT);
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_PIN_HIGH);
}

static void master_spi_event_cb(csi_spi_t *spi, csi_spi_event_t event, void *arg)
{
    spi_master_event = event;
}

static int init(void)
{
    int ret;

    example_pin_init();
    soft_cs_pin_init();

    ret = csi_spi_init(&spi, EXAMPLE_SPI_IDX);

    if (ret != CSI_OK) {
        return -1;
    }
    ret = csi_spi_attach_callback(&spi, master_spi_event_cb, NULL);

    if (ret != CSI_OK) {
        return -1;
    }
    ret = csi_spi_mode(&spi, SPI_MASTER);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_cp_format(&spi, SPI_FORMAT_CPOL0_CPHA0);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_frame_len(&spi, SPI_FRAME_LEN_8);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_baud(&spi, W25Q64FV_DEFAULT_CLK_RATE);

    if (ret == 0) {
        return -1;
    }
    ret = csi_spi_link_dma(&spi, NULL, &dma_rx_ch[0]);

    if (ret != CSI_OK) {
        return -1;
    }
    csi_spi_select_slave(&spi, 0);
    return 0;
}

static int ioctl(uint32_t value)
{
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, value);
    return 0;
}

static int send(const void *data, uint32_t size)
{
    uint32_t ret;
    ret = csi_spi_send(&spi, data, size, 1000);

    if (size != ret) {
        return -1;
    }

    return 0;
}

static int receive(void *data, uint32_t size)
{
    int ret;

    ret = csi_spi_receive_async(&spi, data, size);
    if (ret != CSI_OK) {
        return -1;
    }
    while(spi_master_event != SPI_EVENT_RECEIVE_COMPLETE);
    spi_master_event = -1;
    return 0;
}

static uint32_t w25q64fv_get_page_addr_by_num(uint32_t page_num)
{
    return page_num * W25Q64FV_PAGE_SIZE;
}

static uint32_t w25q64fv_get_sec_addr_by_num(uint32_t sec_num)
{
    return sec_num * W25Q64FV_SECTOR_SIZE;

}

int example_flash_chip_erase(void)
{
    int ret;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t addr;
    uint8_t data[W25Q64FV_PAGE_SIZE] __attribute__((aligned(32)));;

    /* erase chip */
    ret = w25q64fv_erase_chip(&flash);

    if (ret < 0) {
        printf("fail to erase chip, error code = %d!\n", ret);
        return -1;
    }

    /* check first 30 page */
    for (i = 0; i < 30; i++) {

        /* read page data */
        addr = w25q64fv_get_page_addr_by_num(i);
        ret = w25q64fv_read(&flash, addr, data, W25Q64FV_PAGE_SIZE);

        if (ret < 0) {
            printf("fail to read data, page num: %d\n", i);
            return -1;
        }

        /* compare one page data */
        for (j = 0; j < W25Q64FV_PAGE_SIZE; j++) {
            if (data[j] != 0xFF) {
                printf("check erase value fail, page num: %d, address = %08x!\n", i, i * W25Q64FV_PAGE_SIZE + j);
                return -1;
            }
        }

        printf("check erase value success, page num: %d\n", i);
    }

    return 0;
}

int example_falsh_write_read()
{
    uint32_t address, size;
    int ret;

    /* prefill tx_data buffer */
    memset(tx_data, 0xaa, sizeof(tx_data));

    /* erase chip */
    ret = w25q64fv_erase_chip(&flash);

    if (ret < 0) {
        printf("fail to erase chip, error code = %d!\n", ret);
        return -1;
    }

    /* write data to flash */
    address = EXAMPLE_FLASH_START_ADDRESS;
    size    = EXAMPLE_FLASH_WRITE_SIZE;
    ret = w25q64fv_program(&flash, address, tx_data, size);

    if (ret < 0) {
        printf("fail to program data, error code = %d!\n", ret);
        return -1;
    }

    /* read data from flash */
    address = EXAMPLE_FLASH_START_ADDRESS;
    size    = EXAMPLE_FLASH_WRITE_SIZE;
    ret = w25q64fv_read(&flash, address, rx_data, size);

    if (ret < 0) {
        printf("fail to read data, error code = %d!\n", ret);
        return -1;
    }

    /* data compare */
    ret = memcmp(tx_data, rx_data, size);

    if (ret != 0) {
        printf("data compare error!\n");
    }

    return 0;
}

int example_flash_sector_erase()
{
    int ret;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t sector_addr, page_addr;
    uint32_t page_offset;
    uint8_t  data[W25Q64FV_PAGE_SIZE];

    /* erase chip */
    sector_addr = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM);
    page_offset = sector_addr / W25Q64FV_PAGE_SIZE;
    ret = w25q64fv_erase_sector(&flash, sector_addr);

    if (ret < 0) {
        printf("fail to erase sertor, error code = %d!\n", ret);
    }

    /* check page in sector */
    for (i = 0; i < (W25Q64FV_SECTOR_SIZE / W25Q64FV_PAGE_SIZE); i++) {

        /* read page data */
        page_addr = sector_addr + i * W25Q64FV_PAGE_SIZE;
        ret = w25q64fv_read(&flash, page_addr, data, W25Q64FV_PAGE_SIZE);

        if (ret < 0) {
            printf("page read fail, page num: %d\n", i);
            return -1;
        }

        /* compare one page data */
        for (j = 0; j < W25Q64FV_PAGE_SIZE; j++) {
            if (data[j] != 0xFF) {
                printf("check erase value fail, page num: %d, address = %.8x!\n", page_offset + i, sector_addr + i * W25Q64FV_PAGE_SIZE + j);
                return -1;
            }
        }

        printf("check erase value success, page num: %d\n", page_offset + i);
    }

    return 0;
}


int main(void)
{
    int ret = 0;
    uint16_t device_id;

    board_init();

    binding.init    = init;
    binding.send    = send;
    binding.receive = receive;
    binding.ioctl   = ioctl;
    binding.uninit  = uninit;
    ret = w25q64fv_init(&flash, &binding);

    if (ret < 0) {
        printf("fail to init device, error code = %d\n", ret);
    }

    ret = w25q64fv_read_id(&flash, &device_id);

    if (ret < 0) {
        printf("fail to get device id, error code = %d\n", ret);
    }

    printf("device id: %04x\n", device_id);

    /* chip erase example */
    printf("-------------------------------------\n");
    printf("example_flash_chip_erase()\n");
    printf("-------------------------------------\n");
    ret = example_flash_chip_erase();

    if (ret < 0) {
        printf("example_flash_chip_erase() run fail\n");
        return -1;
    }

    printf("example_flash_chip_erase() run success\n\n\n");
    suc_num++;


    /* write and read date example */
    printf("\n\n\n");
    printf("-------------------------------------\n");
    printf("example_falsh_write_read()\n");
    printf("-------------------------------------\n");
    ret = example_falsh_write_read();

    if (ret < 0) {
        printf("example_falsh_write_read() run fail\n");
        return -1;
    }

    printf("example_falsh_write_read() run success\n\n\n");
    suc_num++;

    /* sector erase example */
    printf("-------------------------------------\n");
    printf("example_flash_sector_erase()\n");
    printf("-------------------------------------\n");
    ret = example_flash_sector_erase();

    if (ret < 0) {
        printf("example_flash_sector_erase() run fail\n");
        return -1;
    }

    printf("example_flash_sector_erase() run success\n\n\n");
    suc_num++;

    if (MAX_SUCCESS_NUM == suc_num) {
        CSI_EXAMPLE_RESULT(1);
    } else {
        CSI_EXAMPLE_RESULT(-1);
    }

    return 0;

}

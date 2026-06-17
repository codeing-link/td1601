


/******************************************************************************
 * @file     example_spi.c
 * @brief    the main function for the SPI driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include <soc.h>
#include <drv/spiflash.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <csi_config.h>
#include <board_config.h>
#include <board_init.h>

/* example parameters */
#define MAX_SUCCESS_NUM                    3
#define EXAMPLE_FLASH_START_ADDRESS        0x50
#define EXAMPLE_FLASH_WRITE_SIZE           0x250
#define EXAMPLE_FLASH_SECTOR_ERASE_NUM     2

#define EXAMPLE_BUF_SIZE                   0x400

static csi_gpio_t gpio;
static csi_spiflash_t spiflash;
static uint8_t tx_data[EXAMPLE_FLASH_WRITE_SIZE];
static uint8_t rx_data[EXAMPLE_FLASH_WRITE_SIZE];
static uint8_t erase_data[EXAMPLE_BUF_SIZE];
static volatile uint8_t suc_num = 0;

static void example_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MISO_PORT,EXAMPLE_PIN_SPI_MISO, EXAMPLE_PIN_SPI_MISO_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MOSI_PORT,EXAMPLE_PIN_SPI_MOSI, EXAMPLE_PIN_SPI_MOSI_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_SCK_PORT,EXAMPLE_PIN_SPI_SCK, EXAMPLE_PIN_SPI_SCK_FUNC);

}

static void soft_cs_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_CS,   EXAMPLE_PIN_SPI_SOFT_CS_FUNC);
    csi_gpio_init(&gpio, EXAMPLE_PIN_SPI_CS_PORT);
    csi_gpio_dir(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_DIRECTION_OUTPUT);
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_PIN_HIGH);
}

int example_flash_chip_erase(void)
{
    int ret;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t addr;
    csi_spiflash_info_t info;

    ret = csi_spiflash_get_flash_info(&spiflash, &info);

    if (ret < 0) {
        return -1;
    }

    /* erase chip */
    ret = csi_spiflash_erase(&spiflash, 0, info.flash_size);

    if (ret < 0) {
        printf("fail to erase chip, error code = %d!\n", ret);
        return -1;
    }

    /* check first 30 page */
    for (i = 0; i < 30; i++) {

        /* read page data */
        addr = i * info.page_size;
        ret = csi_spiflash_read(&spiflash, addr, erase_data, info.page_size);

        if (ret < 0) {
            printf("fail to read data, page num: %d\n", i);
            return -1;
        }

        /* compare one page data */
        for (j = 0; j < info.page_size; j++) {
            if (erase_data[j] != 0xFF) {
                printf("check erase value fail, page num: %d, address = %08x!\n", i, i * info.page_size + j);
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

    /* write data to flash */
    address = EXAMPLE_FLASH_START_ADDRESS;
    size    = EXAMPLE_FLASH_WRITE_SIZE;
    ret = csi_spiflash_program(&spiflash, address, tx_data, size);

    if (ret < 0) {
        printf("fail to program data, error code = %d!\n", ret);
        return -1;
    }

    /* read data from flash */
    address = EXAMPLE_FLASH_START_ADDRESS;
    size    = EXAMPLE_FLASH_WRITE_SIZE;
    ret = csi_spiflash_read(&spiflash, address, rx_data, size);

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
    csi_spiflash_info_t info;

    ret = csi_spiflash_get_flash_info(&spiflash, &info);

    if (ret < 0) {
        return -1;
    }

    /* erase chip */
    sector_addr = EXAMPLE_FLASH_SECTOR_ERASE_NUM * info.sector_size;
    page_offset = sector_addr / info.page_size;
    ret = csi_spiflash_erase(&spiflash, sector_addr, info.sector_size);

    if (ret < 0) {
        printf("fail to erase sertor, error code = %d!\n", ret);
    }

    /* check page in sector */
    for (i = 0; i < (info.sector_size / info.page_size); i++) {

        /* read page data */
        page_addr = sector_addr + i * info.page_size;
        ret = csi_spiflash_read(&spiflash, page_addr, erase_data, info.page_size);

        if (ret < 0) {
            printf("page read fail, page num: %d\n", i);
            return -1;
        }

        /* compare one page data */
        for (j = 0; j < info.page_size; j++) {
            if (erase_data[j] != 0xFF) {
                printf("check erase value fail, page num: %d, address = %.8x!\n", page_offset + i, sector_addr + i * info.page_size + j);
                return -1;
            }
        }

        printf("check erase value success, page num: %d\n", page_offset + i);
    }

    return 0;
}

static void ioctl(csi_gpio_pin_state_t value)
{
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, value);
}

int main(void)
{
    int ret = 0;
    csi_spiflash_info_t info;

    board_init();

    example_pin_init();
    soft_cs_pin_init();

    ret = csi_spiflash_spi_init(&spiflash, EXAMPLE_SPI_IDX, ioctl);

    if (ret < 0) {
        printf("fail to init device, error code = %d\n", ret);
    }

    ret = csi_spiflash_get_flash_info(&spiflash, &info);

    if (ret < 0) {
        return -1;
    }

    printf("device id : %x\n", info.flash_id);

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

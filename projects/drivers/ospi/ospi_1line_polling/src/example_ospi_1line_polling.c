/******************************************************************************
 * @file     example_Ospi.c
 * @brief    the main function for the OSPI driver
 * @version  V1.0
 * @date     07. 5 2026
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <soc.h>
#include <drv/ospi.h>
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
static csi_ospi_t  ospi;
static flash_t    flash;
static bindings_t binding;
static uint8_t tx_data[EXAMPLE_FLASH_WRITE_SIZE] __attribute__((aligned(32)));;
static uint8_t rx_data[EXAMPLE_FLASH_WRITE_SIZE] __attribute__((aligned(32)));;
static volatile uint8_t suc_num = 0;
static csi_ospi_command_t cmd;

static void example_pin_init(void)
{
    csi_pin_set_mux(PORTB, PIN4, PB4_SPI1_D0);
    csi_pin_set_mux(PORTB, PIN5, PB5_SPI1_D1);
    csi_pin_set_mux(PORTA, PIN28, PA28_SPI1_SCK);
}
static void uninit()
{
    csi_ospi_detach_callback(&ospi);
    csi_ospi_uninit(&ospi);
}

static void soft_cs_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_CS,   EXAMPLE_PIN_SPI_SOFT_CS_FUNC);
    csi_gpio_init(&gpio, EXAMPLE_PIN_SPI_CS_PORT);
    csi_gpio_dir(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_DIRECTION_OUTPUT);
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_PIN_HIGH);
}

static int init(void)
{
    int ret;

    example_pin_init();
    soft_cs_pin_init();

    ret = csi_ospi_init(&ospi, 0);
    if (ret != CSI_OK) 
    {
        return -1;
    }
    cmd.address.bus_width = OSPI_LINE_QUAD,
    cmd.address.disabled = true,
    cmd.address.size = OSPI_ADDRESS_24_BITS,
    cmd.alt.disabled = true,
    cmd.data.bus_width = OSPI_LINE_SINGLE,
    cmd.data.disabled = 0,
    cmd.data.frame_len = 0x8,
    cmd.data.transfer_mode = OSPI_TRANSFER_SEND_RECEIVE,
    cmd.ddr_enable = 0,
    cmd.dummy_count = 6,
    cmd.instruction.disabled = true,
    cmd.instruction.bus_width = OSPI_LINE_SINGLE,
    cmd.instruction.size = OSPI_INSTRUCTION_8_BITS;
    csi_ospi_config(&ospi, &cmd);
    csi_ospi_baud(&ospi, 1000000);
    csi_ospi_cp_format(&ospi, OSPI_FORMAT_CPOL0_CPHA0);
    return 0;
}

static int ioctl(uint32_t value)
{
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, value);
    return 0;
}

static int send(const void *data, uint32_t size)
{
    csi_ospi_send(&ospi, data, size, 1000);
    return 0;
}

static int receive(void *data, uint32_t size)
{
    int ret;

    ret = csi_ospi_receive(&ospi, data, size, 1000);

    if (size != ret) {
        return -1;
    }

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
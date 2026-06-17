

/******************************************************************************
 * @file     example_flash.c
 * @brief
 * @version
 * @date     2020-02-26
 ******************************************************************************/
#include "drv/qspi.h"
#include "drv/tick.h"
#include "stdio.h"
#include <string.h>
#include <csi_config.h>
#include "soc.h"
#include "board_config.h"
#include "board_init.h"
#include "w25q64fv.h"

/* example parameters */
#define EXAMPLE_FLASH_START_ADDRESS        0x50
#define EXAMPLE_FLASH_WRITE_SIZE           0x250
#define EXAMPLE_FLASH_SECTOR_ERASE_NUM     120

static uint8_t tx_data[EXAMPLE_FLASH_WRITE_SIZE];
static uint8_t rx_data[EXAMPLE_FLASH_WRITE_SIZE];
static uint8_t erased_data[W25Q64FV_PAGE_SIZE];

static uint32_t w25q64fv_get_sec_addr_by_num(uint32_t sec_num)
{
    return sec_num * W25Q64FV_SECTOR_SIZE;

}

int example_falsh_write_read()
{
    uint32_t address, size;
    int ret;

    /* prefill tx_data buffer */
    memset(tx_data, 0xaa, sizeof(tx_data));

    /* write data to flash */
    address = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM);
    size    = EXAMPLE_FLASH_WRITE_SIZE;
    ret = w25q64fv_write(address, tx_data, size);

    if (ret < 0) {
        printf("fail to program data, error code = %d!\n", ret);
        return -1;
    }

    /* read data from flash */
    address = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM);
    size    = EXAMPLE_FLASH_WRITE_SIZE;
    ret = w25q64fv_read(address, rx_data, size);

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

    /* erase chip */
    sector_addr = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM);
    page_offset = sector_addr / W25Q64FV_PAGE_SIZE;
    ret = w25q64fv_erase_sector(sector_addr);

    if (ret < 0) {
        printf("fail to erase sertor, error code = %d!\n", ret);
    }

    /* check page in sector */
    for (i = 0; i < (W25Q64FV_SECTOR_SIZE / W25Q64FV_PAGE_SIZE); i++) {

        /* read page data */
        page_addr = sector_addr + i * W25Q64FV_PAGE_SIZE;
        ret = w25q64fv_read(page_addr, erased_data, W25Q64FV_PAGE_SIZE);

        if (ret < 0) {
            printf("page read fail, page num: %d\n", i);
            return -1;
        }

        /* compare one page data */
        for (j = 0; j < W25Q64FV_PAGE_SIZE; j++) {
            if (erased_data[j] != 0xFF) {
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
    int ret;
    uint16_t device_id;
    board_init();
    w25q64fv_init();

    ret = w25q64fv_read_device_id(&device_id);

    if (ret < 0) {
        printf("fail to get device id, error code = %d\n", ret);
    }

    printf("device id: %04x\n", device_id);

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

    return 0;
}



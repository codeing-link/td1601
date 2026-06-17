

/******************************************************************************
 * @file     example_qspi_direct.c
 * @brief
 * @version
 * @date     2020-02-28
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
#define FLASH_BASE_ADDRESS                 0
#define EXAMPLE_FLASH_OPERATE_SIZE         0x250
#define EXAMPLE_FLASH_SECTOR_ERASE_NUM     120

#define MEMORY_BASE_ADDRESS                SPIFLASH_BASE

static uint8_t tx_data[EXAMPLE_FLASH_OPERATE_SIZE];
static uint8_t rx_data[EXAMPLE_FLASH_OPERATE_SIZE];

static uint32_t w25q64fv_get_sec_addr_by_num(uint32_t sec_num)
{
    return sec_num * W25Q64FV_SECTOR_SIZE;

}

int example_falsh_write_read()
{
    uint32_t start_addr, cur_addr, size, offset;
    int ret;
    uint32_t i;


    /* prefill tx_data buffer */
    memset(tx_data, 0xaa, EXAMPLE_FLASH_OPERATE_SIZE / 2);
    memset(tx_data + sizeof(tx_data) / 2, 0xdd, EXAMPLE_FLASH_OPERATE_SIZE / 2);
    memset(rx_data, 0, EXAMPLE_FLASH_OPERATE_SIZE);

    /* program data to memory use STIG interface */
    start_addr = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM);
    size       = EXAMPLE_FLASH_OPERATE_SIZE;

    csi_dcache_clean_invalid();
    ret = w25q64fv_write(start_addr, tx_data, size);

    if (ret < 0) {
        printf("fail to program memory, error code = %d!\n", ret);
        return -1;
    }

    /* read data from memory use direct access mode */
    offset     = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM) - FLASH_BASE_ADDRESS;
    start_addr = MEMORY_BASE_ADDRESS + offset;
    size       = EXAMPLE_FLASH_OPERATE_SIZE;
    cur_addr   = start_addr;
    memcpy(rx_data, (uint8_t *)start_addr, size);

    /* data compare between tx fifo and rx fifo */
    for (i = 0; i < size; i++) {
        if (tx_data[i] != rx_data[i]) {
            printf("fail to compare data, address = %08x!\n",  cur_addr);
            return -1;
        }

        cur_addr++;
    }

    /* prefill tx_data buffer */
    memset(tx_data, 0xaa, EXAMPLE_FLASH_OPERATE_SIZE / 2);
    memset(tx_data + sizeof(tx_data) / 2, 0xdd, EXAMPLE_FLASH_OPERATE_SIZE / 2);
    memset(rx_data, 0, EXAMPLE_FLASH_OPERATE_SIZE);

    /* program data to memory use direct access mode */
    offset     = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM) - FLASH_BASE_ADDRESS;
    start_addr = MEMORY_BASE_ADDRESS + offset;
    size       = EXAMPLE_FLASH_OPERATE_SIZE;
    memcpy((uint8_t *)start_addr, tx_data, size);
    ret = w25q64fv_wait_ready();

    if (ret < 0) {
        printf("fail to program memory, error code = %d!\n", ret);
    }

    /* read data from memory use direct access mode */
    start_addr = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM);
    size       = EXAMPLE_FLASH_OPERATE_SIZE;
    ret = w25q64fv_read(start_addr, rx_data, size);

    if (ret < 0) {
        printf("fail to memory read, error code = %d!\n", ret);
        return -1;
    }

    offset     = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM) - FLASH_BASE_ADDRESS;
    start_addr = MEMORY_BASE_ADDRESS + offset;
    cur_addr   = start_addr;

    /* data compare between tx fifo and rx fifo */
    for (i = 0; i < size; i++) {
        if (tx_data[i] != rx_data[i]) {
            printf("fail to compare data, address = %08x!\n",  cur_addr);
            return -1;
        }

        cur_addr++;
    }

    return 0;
}

int example_flash_sector_erase()
{
    int ret;
    uint32_t start_addr, end_addr, cur_addr;
    uint32_t offset;

    /* erase chip */
    start_addr = w25q64fv_get_sec_addr_by_num(EXAMPLE_FLASH_SECTOR_ERASE_NUM);
    ret = w25q64fv_erase_sector(start_addr);

    if (ret < 0) {
        printf("fail to erase sector, error code = %d!\n", ret);
    }

    offset     = start_addr - FLASH_BASE_ADDRESS;
    start_addr = MEMORY_BASE_ADDRESS + offset;
    end_addr   = start_addr + W25Q64FV_SECTOR_SIZE;

    /* check whether data is erased at memory  */
    for (cur_addr = start_addr; cur_addr < end_addr; cur_addr++) {
        if (*(uint8_t *)cur_addr != 0xFF) {
            printf("fail to check erase value, value is not FFh, address = %08x!\n", cur_addr);
            return -1;
        }
    }

    return 0;
}


int main(void)
{
    int ret;
    uint16_t device_id;
    board_init();

    ret = w25q64fv_init();

    if (ret < 0) {
        printf("fail to init w25q64fv, error code = %d!\n", ret);
    }

    /* get device id */
    ret = w25q64fv_read_device_id(&device_id);

    if (ret < 0) {
        printf("fail to get device id, error code = %d!\n", ret);
    }

    printf("device id: %04x\n", device_id);

    /* sector erase example */
    printf("-------------------------------------\n");
    printf("example_flash_sector_erase()\n");
    printf("-------------------------------------\n");
    ret = example_flash_sector_erase();

    if (ret < 0) {
        printf("example_flash_sector_erase() run fail, error code = %d!\n", ret);
        return -1;
    }

    printf("example_flash_sector_erase() run success\n\n\n");

    /* write and read data example */
    printf("\n\n\n");
    printf("-------------------------------------\n");
    printf("example_falsh_write_read()\n");
    printf("-------------------------------------\n");
    ret = example_falsh_write_read();

    if (ret < 0) {
        printf("example_falsh_write_read() run fail, error code = %d!\n", ret);
        return -1;
    }

    printf("example_falsh_write_read() run success\n\n\n");
    CSI_EXAMPLE_RESULT(0);

    return 0;
}



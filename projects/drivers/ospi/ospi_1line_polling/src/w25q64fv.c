


/******************************************************************************
 * @file     w25q64fv.c
 * @brief    the main function for the w25q64fv driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <soc.h>
#include <drv/spi.h>
#include <drv/tick.h>
#include <csi_config.h>
#include <board_config.h>
#include <board_init.h>

#include "w25q64fv.h"

/* W25Q64FV command */
#define W25Q64FV_READ_STATUS        0x05
#define W25Q64FV_WRITE_ENABLE       0x06
#define W25Q64FV_READ_DEVICE_ID     0x90
#define W25Q64FV_SECTOR_ERASE       0x20
#define W25Q64FV_CHIP_ERASE         0x60
#define W25Q64FV_PAGE_PROGRAM       0x02
#define W25Q64FV_READ_DATA          0x03
#define W25164FV_RELEASE_POWER_DOWN 0xAB

/* communication parameters */
#define W25Q64FV_DEFAULT_TRANSMIT_TIMEOUT  1000
#define W25Q64FV_DEFAULT_ERASE_TIMEOUT     20000

int w25q64fv_wait_ready(flash_t *flash)
{
    uint32_t time_start;
    uint8_t status;
    uint8_t cmd[4];
    int ret;

    cmd[0] = W25Q64FV_READ_STATUS;
    flash->binding->ioctl(0);
    ret = flash->binding->send(&cmd, 1);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    time_start = csi_tick_get();

    while ((csi_tick_get() - time_start) < W25Q64FV_DEFAULT_ERASE_TIMEOUT) {

        /* receive status */
        ret = flash->binding->receive(&status, 1);

        if (ret < 0) {
            flash->binding->ioctl(1);
            return -1;
        }

        if ((ret == 0) && ((status & 0x01) == 0)) {
            flash->binding->ioctl(1);
            return 0;
        }
    }

    flash->binding->ioctl(1);
    return -1;
}

static int w25q64fv_page_write(flash_t *flash, uint32_t addr, void *data, uint32_t size)
{
    uint8_t cmd[4];
    int ret;

    /* enable write operations */
    cmd[0] = W25Q64FV_WRITE_ENABLE;
    flash->binding->ioctl(0);
    ret = flash->binding->send(&cmd, 1);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    flash->binding->ioctl(1);

    ret = w25q64fv_wait_ready(flash);

    if (ret == -1) {
        return -1;
    }

    /* page program command */
    cmd[0] = W25Q64FV_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xffU;
    cmd[2] = (addr >> 8) & 0xffU;
    cmd[3] = addr & 0xffU;
    flash->binding->ioctl(0);
    ret = flash->binding->send(&cmd, 4);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    /* program data at specify page */
    ret = flash->binding->send(data, size);

    if (ret == CSI_ERROR) {
        return -1;
    }

    flash->binding->ioctl(1);

    ret = w25q64fv_wait_ready(flash);

    if (ret == CSI_ERROR) {
        return -1;
    }

    return 0;
}
int w25q64fv_read(flash_t *flash, uint32_t addr, void *data, uint32_t size)
{
    int ret;
    uint8_t cmd[4];

    /* read operations */
    cmd[0] = W25Q64FV_READ_DATA;
    cmd[1] = (addr >> 16) & 0xffU;
    cmd[2] = (addr >> 8) & 0xffU;
    cmd[3] = addr & 0xffU;
    flash->binding->ioctl(0);
    ret = flash->binding->send(cmd, 4);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    /* read data from memory */
    ret = flash->binding->receive(data, size);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    flash->binding->ioctl(1);

    ret =  w25q64fv_wait_ready(flash);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    return 0;
}

int w25q64fv_read_id(flash_t *flash, uint16_t *device_id)
{
    int ret;
    uint8_t data[4];

    /* read device id operations */
    data[0] = W25Q64FV_READ_DEVICE_ID;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    flash->binding->ioctl(0);
    ret = flash->binding->send(data, 4);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }
    /* receive status */
    ret = flash->binding->receive((uint8_t *)device_id, 2);
    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }
    flash->binding->ioctl(1);
    return 0;
}

int w25q64fv_erase_sector(flash_t *flash, uint32_t addr)
{
    uint8_t data[4];
    int ret;

    /* enable write operations */
    data[0] = W25Q64FV_WRITE_ENABLE;
    flash->binding->ioctl(0);
    ret = flash->binding->send(data, 1);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    flash->binding->ioctl(1);

    ret =  w25q64fv_wait_ready(flash);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    /* sector erase operations */
    data[0] = W25Q64FV_SECTOR_ERASE;
    data[1] = (addr >> 16) & 0xFFU;
    data[2] = (addr >> 8) & 0xFFU;
    data[3] = addr & 0xFFU;
    flash->binding->ioctl(0);
    ret = flash->binding->send(&data, 4);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    flash->binding->ioctl(1);

    ret =  w25q64fv_wait_ready(flash);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    return 0;

}

int w25q64fv_erase_chip(flash_t *flash)
{
    uint8_t cmd;
    int ret;

    /* enable write operations */
    cmd = W25Q64FV_WRITE_ENABLE;
    flash->binding->ioctl(0);
    ret = flash->binding->send(&cmd, 1);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    flash->binding->ioctl(1);

    ret = w25q64fv_wait_ready(flash);

    if (ret < -1) {
        flash->binding->ioctl(1);
        return -1;
    }

    /* chip erase operations */
    cmd = W25Q64FV_CHIP_ERASE;
    flash->binding->ioctl(0);
    ret = flash->binding->send(&cmd, 1);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    flash->binding->ioctl(1);

    ret = w25q64fv_wait_ready(flash);

    if (ret < -1) {
        flash->binding->ioctl(1);
        return -1;
    }

    return 0;
}

int w25q64fv_program(flash_t *flash, uint32_t addr, void *data, uint32_t size)
{
    int ret;
    uint32_t end_addr, current_size, current_addr;
    uint8_t *temp;

    temp = (uint8_t *)data;
    /* calculation of the size between the write address and the end of the page */
    current_size = W25Q64FV_PAGE_SIZE - (addr % W25Q64FV_PAGE_SIZE);

    /* check if the size of the data is less than the remaining place in the page */
    if (current_size > size) {
        current_size = size;
    }

    /* initialize the adress variables */
    current_addr = addr;
    end_addr = addr + size;

    /* perform the write page by page */
    do {
        ret = w25q64fv_page_write(flash, current_addr, temp, current_size);

        if (ret < 0) {
            return -1;
        }

        /* update the address and size variables for next page programming */
        current_addr += current_size;
        temp += current_size;
        current_size = ((current_addr + W25Q64FV_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : W25Q64FV_PAGE_SIZE;
    } while (current_addr < end_addr);

    return 0;
}

int w25q64fv_init(flash_t *flash, bindings_t *binding)
{
    uint8_t cmd;
    int ret;

    flash->binding = binding;
    flash->binding->init();

    /* release power down */
    cmd = W25164FV_RELEASE_POWER_DOWN;
    flash->binding->ioctl(0);
    ret = flash->binding->send(&cmd, 1);

    if (ret < 0) {
        flash->binding->ioctl(1);
        return -1;
    }

    flash->binding->ioctl(1);

    ret = w25q64fv_wait_ready(flash);

    if (ret == -1) {
        return -1;
    }

    return 0;
}

int w25q64fv_uninit(flash_t *flash)
{
    flash->binding->uninit();
    flash->binding = NULL;
    return 0;
}

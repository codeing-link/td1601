

/******************************************************************************
 * @file     w25q64fv.c
 * @brief
 * @version
 * @date     2020-02-28
 ******************************************************************************/
#include "drv/tick.h"
#include "w25q64fv.h"
#include "drv/qspi.h"
#include <drv/irq.h>

/* W25Q64FV command */
#define W25Q64FV_READ_STATUS        0x05
#define W25Q64FV_WRITE_ENABLE       0x06
#define W25Q64FV_READ_DEVICE_ID     0x90
#define W25Q64FV_SECTOR_ERASE       0x20
#define W25Q64FV_CHIP_ERASE         0x60
#define W25Q64FV_PAGE_PROGRAM       0x02
#define W25Q64FV_READ_DATA          0x03
#define W25Q64FV_FAST_READ_DUAL_IO  0xbb
#define W25Q64FV_FAST_READ_QUAL_IO  0xeb
#define W25164FV_RELEASE_POWER_DOWN 0xAB

/* communication parameters */
#define W25Q64FV_DEFAULT_TRANSMIT_TIMEOUT  0x1000000
#define MAX_TRANSFER_SIZE_ONCE             8

#if defined(CONFIG_XIP)
#define ATTRIBUTE_DATA __attribute__((section(".ram.code")))
#else
#define ATTRIBUTE_DATA
#endif

static csi_qspi_t qspi_handle;
static csi_qspi_command_t command;


ATTRIBUTE_DATA static int get_flash_status(uint8_t *status)
{
    int ret;

    /* transmit READ_STATUS command */
    command.instruction.value     = W25Q64FV_READ_STATUS;
    command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
    command.instruction.disabled  = false;
    command.alt.disabled          = true;
    command.address.disabled      = true;
    command.data.bus_width        = QSPI_CFG_BUS_SINGLE;
    ret = csi_qspi_receive(&qspi_handle, &command,  status, 1, W25Q64FV_DEFAULT_TRANSMIT_TIMEOUT);

    if (ret != 1) {
        return -1;
    }

    return 0;
}

ATTRIBUTE_DATA int w25q64fv_wait_ready()
{
    uint32_t timeout = W25Q64FV_DEFAULT_TRANSMIT_TIMEOUT;
    uint8_t status;
    int ret = -1;
    uint32_t result;

    result = csi_irq_save();

    while (timeout-- > 0) {
        ret = get_flash_status(&status);

        if ((ret == 0) && ((status & 0x01) == 0)) {
            ret = 0;
            break;
        }
    }
    csi_irq_restore(result);

    return ret;
}

ATTRIBUTE_DATA static int qspi_send_and_wait(csi_qspi_command_t *command, const void *data, uint32_t size)
{
    int      ret = 0;
    uint32_t count;

    count = csi_qspi_send(&qspi_handle, command, data, size, W25Q64FV_DEFAULT_TRANSMIT_TIMEOUT);

    if (count != size) {
        ret = -1;
    } else if (w25q64fv_wait_ready() == -1) {
        ret = -1;
    }
    return ret;
}


static csi_error_t w25q64fv_page_write(uint32_t addr, void *data, uint32_t size)
{
    uint32_t current_size, current_addr ;
    int ret;
    uint8_t *temp;

    current_addr = addr;
    temp = data;

    while (size > 0U) {
        if (current_size > MAX_TRANSFER_SIZE_ONCE) {
            current_size = MAX_TRANSFER_SIZE_ONCE;
        }

        /* enable write operations */
        command.instruction.value     = W25Q64FV_WRITE_ENABLE;
        command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
        command.instruction.disabled  = false;
        command.alt.disabled          = true;
        command.address.disabled      = true;
        command.data.bus_width        = QSPI_CFG_BUS_SINGLE;
        ret = qspi_send_and_wait(&command, NULL, 0);

        if (ret < 0) {
            return -1;
        }

        /* page program command */
        command.instruction.value     = W25Q64FV_PAGE_PROGRAM;
        command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
        command.instruction.disabled  = false;
        command.alt.disabled          = true;
        command.address.disabled      = false;
        command.address.value         = current_addr;
        command.address.size          = QSPI_ADDRESS_24_BITS;
        command.address.bus_width     = QSPI_CFG_BUS_SINGLE;
        command.data.bus_width        = QSPI_CFG_BUS_SINGLE;
        ret = qspi_send_and_wait(&command, temp, current_size);

        if (ret < 0) {
            return -1;
        }

        /* update address */
        current_addr += current_size;
        temp += current_size;
        size -= current_size;
    }

    return 0;
}

int w25q64fv_erase_chip()
{
    int ret;

    /* enable write operations */
    command.instruction.value     = W25Q64FV_WRITE_ENABLE;
    command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
    command.instruction.disabled  = false;
    command.alt.disabled          = true;
    command.address.disabled      = true;
    command.data.bus_width        = QSPI_CFG_BUS_SINGLE;
    ret = qspi_send_and_wait(&command, NULL, 0);

    if (ret < 0) {
        return -1;
    }

    /* chip erase operations */
    command.instruction.value     = W25Q64FV_CHIP_ERASE;
    command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
    command.instruction.disabled  = false;
    command.alt.disabled          = true;
    command.address.disabled      = true;
    command.data.bus_width        = QSPI_CFG_BUS_SINGLE;
    return qspi_send_and_wait(&command, NULL, 0);
}

csi_error_t w25q64fv_erase_sector(uint32_t addr)
{
    int ret;

    /* enable write operations */
    command.instruction.value     = W25Q64FV_WRITE_ENABLE;
    command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
    command.instruction.disabled  = false;
    command.alt.disabled          = true;
    command.address.disabled      = true;
    command.data.bus_width        = QSPI_CFG_BUS_SINGLE;
    ret = qspi_send_and_wait(&command, NULL, 0);

    if (ret < 0) {
        return -1;
    }

    /* sector erase operations */
    command.instruction.value     = W25Q64FV_SECTOR_ERASE;
    command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
    command.instruction.disabled  = false;
    command.alt.disabled          = true;
    command.address.disabled      = false;
    command.address.value         = addr;
    command.address.size          = QSPI_ADDRESS_24_BITS;
    command.address.bus_width     = QSPI_CFG_BUS_SINGLE;
    command.data.bus_width        = QSPI_CFG_BUS_SINGLE;
    return qspi_send_and_wait(&command, NULL, 0);
}

csi_error_t w25q64fv_read(uint32_t addr, void *data, uint32_t size)
{
    int ret;

    /* read operations */
    command.instruction.value     = W25Q64FV_READ_DATA;
    command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
    command.instruction.disabled  = false;
    command.alt.disabled          = true;
    command.address.disabled      = false;
    command.address.value         = addr;
    command.address.size          = QSPI_ADDRESS_24_BITS;
    command.address.bus_width     = QSPI_CFG_BUS_SINGLE;
    command.data.bus_width        = QSPI_CFG_BUS_SINGLE;
    ret = csi_qspi_receive(&qspi_handle, &command, data, size, W25Q64FV_DEFAULT_TRANSMIT_TIMEOUT);

    if (ret != size) {
        return -1;
    }

    return w25q64fv_wait_ready();
}



csi_error_t w25q64fv_write(uint32_t addr, void *data, uint32_t size)
{
    uint32_t end_addr, current_size, current_addr;
    uint8_t  *temp;

    temp = data;
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
        w25q64fv_page_write(current_addr, temp, current_size);

        /* update the address and size variables for next page programming */
        current_addr += current_size;
        temp += current_size;
        current_size = ((current_addr + W25Q64FV_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : W25Q64FV_PAGE_SIZE;
    } while (current_addr < end_addr);

    return w25q64fv_wait_ready();
}

int w25q64fv_read_device_id(uint16_t *device_id)
{
    int ret;

    /* read device id operations */
    command.instruction.value     = W25Q64FV_READ_DEVICE_ID;
    command.instruction.bus_width = QSPI_CFG_BUS_SINGLE;
    command.instruction.disabled  = false;
    command.alt.disabled          = true;
    command.address.disabled      = false;
    command.address.value         = 0;
    command.address.size          = QSPI_ADDRESS_24_BITS;
    command.address.bus_width     = QSPI_CFG_BUS_SINGLE;
    command.data.bus_width        = QSPI_CFG_BUS_SINGLE;

    ret = csi_qspi_receive(&qspi_handle, &command, device_id, 2, W25Q64FV_DEFAULT_TRANSMIT_TIMEOUT);

    if (ret != 2) {
        return -1;
    }

    return 0;
}

int w25q64fv_init()
{
    csi_error_t ret;
    ret = csi_qspi_init(&qspi_handle, 0);

    if (ret != CSI_OK) {
        return -1;
    }
    return 0;
}



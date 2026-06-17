

/******************************************************************************
 * @file     w25q64fv.h
 * @brief
 * @version
 * @date     2020-02-28
 ******************************************************************************/

#ifndef _W25Q64FV_H_
#define _W25Q64FV_H_

#include <stdint.h>
#include <stdbool.h>
#include <drv/common.h>

/* W25Q64FV command */
#define W25Q64FV_READ_STATUS        0x05
#define W25Q64FV_WRITE_EN           0x6
#define W25Q64FV_ERASE_SECTOR       0x20
#define W25Q64FV_ERASE_CHIP         0x60
#define W25Q64FV_WRITE_COMMAND      0x2
#define W25Q64FV_READ_COMMAND       0x3
#define W25Q64FV_POWER_DOWN_COMMAND 0xB9
#define W25Q64FV_POWER_UP_COMMAND   0xAB

/* W25Q64FV information */
#define W25Q64FV_BLOCK_SIZE         0x10000
#define W25Q64FV_SECTOR_SIZE        0x1000
#define W25Q64FV_PAGE_SIZE          0x100
#define W25Q64FV_BASEADDR           0x0
#define W25Q64FV_END                0x800000
#define W25Q64FV_SECTOR_NUM         0x800

#ifdef __cplusplus
extern "C" {
#endif

/**
  \brief       polling busy status with timeout
  \param[in]   void
  \return      0: success -1: fail
*/
int w25q64fv_wait_ready(void);

/**
  \brief       sets all memory within the device to the erased state(FFH)
  \param[in]   void
  \return      0: success -1: fail
*/

int w25q64fv_erase_chip(void);

/**
  \brief       sets specified sector within the device to the erased state(FFH)
  \param[in]   addr: start address of sector
  \return      0: success -1: fail
*/

int w25q64fv_erase_sector(uint32_t addr);

/**
  \brief       read amout of data from memory
  \param[in]   addr: start address
  \param[in]   data: point to read data buffer
  \param[in]   size: read size
  \return      0: success -1: fail
*/

int w25q64fv_read(uint32_t addr, void *data, uint32_t size);

/**
  \brief       program data at erased memory locations
  \param[in]   addr: start address
  \param[in]   data: point to write data buffer
  \param[in]   size: program size
  \return      0: success 1: fail
*/

int w25q64fv_write(uint32_t addr, void *data, uint32_t size);

/**
  \brief       read device id
  \param[in]   device_id: point to data buffer
  \return      0: success 1: fail
*/

int w25q64fv_read_device_id(uint16_t *device_id);

/**
  \brief       initialize w25q64fv
  \param[in]   void
  \return      0: success 1: fail
*/
int w25q64fv_init();

#ifdef __cplusplus
}
#endif

#endif  /* _W25Q64FV_H_*/

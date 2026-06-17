

/******************************************************************************
 * @file     w25q64fv.h
 * @brief    header file for w25q64fv driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#ifndef __W25Q64FV_H_
#define __W25Q64FV_H_
#ifdef __cplusplus
extern "C" {
#endif

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
#define W25Q64FV_DEFAULT_CLK_RATE           1000000

/**
 *  \struct     bindings_t
 *  \brief      low level communication interface
 */
typedef struct {
    int (*init)(void);
    void (*uninit)(void);
    int (*ioctl)(uint32_t value);
    int (*send)(const void *data, uint32_t size);
    int (*receive)(void *data, uint32_t size);
    int (*send_receive)(const void *data_out, void *data_in, uint32_t num);
} bindings_t;

/**
 *  \struct    flash_t
 *  \brief     ctrl block of flash instance
 */
typedef struct {
    bindings_t *binding;
} flash_t;

/**
  \brief       sets all memory within the device to the erased state(FFH)
  \param[in]   flash: point to flash handle
  \return      0: success -1: fail
*/

int w25q64fv_erase_chip(flash_t *flash);

/**
  \brief       sets specified sector within the device to the erased state(FFH)
  \param[in]   flash: point to flash handle
  \param[in]   addr : start address of sector
  \return      0: success -1: fail
*/

int w25q64fv_erase_sector(flash_t *flash, uint32_t addr);

/**
  \brief       read amout of data from memory
  \param[in]   flash: point to flash handle
  \param[in]   addr: start address
  \param[in]   data: point to read data buffer
  \param[in]   size: read size
  \return      0: success -1: fail
*/

int w25q64fv_read(flash_t *flash, uint32_t addr, void *data, uint32_t size);

/**
  \brief       program data at erased memory locations
  \param[in]   flash: point to flash handle
  \param[in]   addr: start address
  \param[in]   data: point to write data buffer
  \param[in]   size: program size
  \return      0: success 1: fail
*/

int w25q64fv_program(flash_t *flash, uint32_t addr, void *data, uint32_t size);

/**
  \brief       read device id
  \param[in]   flash: point to flash handle
  \param[in]   device_id: point to data buffer
  \return      0: success 1: fail
*/

int w25q64fv_read_id(flash_t *flash, uint16_t *device_id);

/**
  \brief       initialize w25q64fv
  \param[in]   flash  : point to flash handle
  \param[in]   binding: low level communicaton interface
  \return      0: success 1: fail
*/
int w25q64fv_init(flash_t *flash, bindings_t *binding);

#ifdef __cplusplus
}
#endif

#endif /* __W25Q64FV_H_ */


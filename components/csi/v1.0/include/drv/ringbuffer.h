

/******************************************************************************
* @file     drv/ringbuffer.h
* @brief    Header File for RINGBUFFER Driver
* @version  V1.0
* @date     August 15. 2020
******************************************************************************/
#ifndef _DRV_RINGBUFFER_H_
#define _DRV_RINGBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include <stdbool.h>

typedef struct csi_ringbuffer {
    uint8_t *buffer;
    uint32_t size;
    uint32_t write;
    uint32_t read;
    uint32_t data_len;
} csi_ringbuffer_t;

void csi_ringbuffer_reset(csi_ringbuffer_t *fifo);
uint32_t csi_ringbuffer_len(csi_ringbuffer_t *fifo);
uint32_t csi_ringbuffer_avail(csi_ringbuffer_t *fifo);
bool csi_ringbuffer_is_empty(csi_ringbuffer_t *fifo);
bool csi_ringbuffer_is_full(csi_ringbuffer_t *fifo);

/*write to csi_ringbuffer*/
uint32_t csi_ringbuffer_in(csi_ringbuffer_t *fifo, const void *in, uint32_t len);

/*read to csi_ringbuffer*/
uint32_t csi_ringbuffer_out(csi_ringbuffer_t *fifo, void *out, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_RINGBUFFER_H_ */

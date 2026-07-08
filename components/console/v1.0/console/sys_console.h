

/******************************************************************************
 * @file     sys_console.h
 * @brief    header file for acmp driver
 * @version  V1.0
 * @date     2019-12-19
 ******************************************************************************/

#ifndef _SYS_CONSOLE_H_
#define _SYS_CONSOLE_H_

#include <stdint.h>
#include <soc.h>
#include "drv/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    port_name_t port;
    pin_name_t pin;
    pin_func_t func;
} console_port_t;

typedef struct {
    console_port_t tx;
    console_port_t rx;
    uint32_t uart_id;
    uint32_t baudrate;
    csi_uart_t *uart;
} sys_console_t;

int32_t console_init(sys_console_t *handle);

#ifdef __cplusplus
}
#endif

#endif /*_SYS_CONSOLE_H_*/


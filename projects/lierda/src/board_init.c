#include <stdint.h>
#include "board_config.h"
#include "console/sys_console.h"
#include "board_init.h"

#define LIERDA_CONSOLE_BAUDRATE 921600U

static sys_console_t s_console;

extern void __ChipInitHandler(void);

int board_init(void)
{
    __ChipInitHandler();

    s_console.uart_id = (uint32_t)CONSOLE_IDX;
    s_console.baudrate = LIERDA_CONSOLE_BAUDRATE;
    s_console.tx.port = CONSOLE_TXD_PORT;
    s_console.tx.pin = CONSOLE_TXD;
    s_console.tx.func = CONSOLE_TXD_FUNC;
    s_console.rx.port = CONSOLE_RXD_PORT;
    s_console.rx.pin = CONSOLE_RXD;
    s_console.rx.func = CONSOLE_RXD_FUNC;
    s_console.uart = NULL;

    return console_init(&s_console);
}

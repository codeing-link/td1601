


/******************************************************************************
 * @file     minilibc_port.c
 * @brief    minilibc port
 * @version  V1.0
 * @date     26. Dec 2017
 ******************************************************************************/

#include <stdio.h>
#include <csi_config.h>
#ifndef CONFIG_KERNEL_NONE
#include <csi_kernel.h>
#endif

#include <drv/uart.h>

csi_uart_t console_uart;

__attribute__((weak)) int write(int __fd, __const void *__buf, int __n)
{
    return 0;
}

int fputc(int ch, FILE *stream)
{
    (void)stream;

    if (ch == '\n') {
        csi_uart_putc(&console_uart, (uint8_t)'\r');
    }

    csi_uart_putc(&console_uart, (uint8_t)ch);

    return 0;
}

int fgetc(FILE *stream)
{
    uint8_t ch;
    (void)stream;

    ch = csi_uart_getc(&console_uart);

    return (int)ch;
}

int os_critical_enter(unsigned int *lock)
{
    (void)lock;
#ifndef CONFIG_KERNEL_NONE
    csi_kernel_sched_suspend();
#endif

    return 0;
}

int os_critical_exit(unsigned int *lock)
{
    (void)lock;
#ifndef CONFIG_KERNEL_NONE
    csi_kernel_sched_resume(0);
#endif

    return 0;
}

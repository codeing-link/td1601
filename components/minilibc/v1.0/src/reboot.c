
#include <stdio.h>
#include <csi_config.h>
#include <drv/wdt.h>

static csi_wdt_t g_wdt_handle;
void sys_reboot(void)
{
    csi_wdt_init(&g_wdt_handle, 0U);
    csi_wdt_set_timeout(&g_wdt_handle, 1);
    csi_wdt_start(&g_wdt_handle);
}

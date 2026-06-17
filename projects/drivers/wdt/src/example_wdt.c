


/******************************************************************************
 * @file     example_wdt.c
 * @brief    the main function for the WDT driver
 * @version  V1.0
 * @date     1. April 2020
 ******************************************************************************/

#include <stdio.h>
#include <soc.h>
#include <drv/wdt.h>
#include <board_init.h>

/**
 * The exact time-out can be calculated according to the formula
 * freq = 67500000 Hz
 * user changeable para is 10 (range is 0 ~ 15)
 * 994ms = ((0x10000 << 10)/ (67500000 / 1000))
 */
#define WDT_TIMEOUT_MS                  (994U)               ///< timeout: 0.94s

static csi_wdt_t g_wdt_handle;

static void wdt_event_cb_fun(csi_wdt_t *wdt, void *arg)
{
    (void)arg;
    csi_wdt_feed(wdt);
    printf("enter cb fun\n");
}

static int32_t test_wdt_feed(void)
{
    int32_t ret = 0;
    int32_t state;
    uint32_t i;

    ///<!!! if wdt attach callback, system only enter irq, can not system reset
    state = csi_wdt_attach_callback(&g_wdt_handle, wdt_event_cb_fun, NULL);

    if (state < 0) {
        printf("csi_wdt_attach_callback error\n");
        ret = -1;
    }

    state = csi_wdt_set_timeout(&g_wdt_handle, WDT_TIMEOUT_MS);

    if (state < 0) {
        printf("csi_wdt_set_timeout error\n");
        ret = -1;
    }

    state = csi_wdt_start(&g_wdt_handle);

    if (state < 0) {
        printf("csi_wdt_start error\n");
        ret = -1;
    }

    for (i = 0U; i < 5U; i++) {
        mdelay(WDT_TIMEOUT_MS - 1U);
        state = csi_wdt_feed(&g_wdt_handle);

        if (state < 0) {
            ret = -1;
        }
    }

    if (0 == ret) {
        printf("test_wdt_feed OK \n");
    }

    return ret;
}

static int32_t test_wdt_reset(void)
{
    printf("ready to callback func\n");
    mdelay(WDT_TIMEOUT_MS + 200U);           ///< enter irq and clear irq state

    ///<!!! if wdt detach callback, system only system reset
    csi_wdt_detach_callback(&g_wdt_handle);

    printf("system should be reset\n");

    mdelay(WDT_TIMEOUT_MS + 1U);

    printf("system should be restarted, and not be here!\n");

    csi_wdt_uninit(&g_wdt_handle);

    return -1;
}

int example_wdt(void)
{
    int ret = 0;
    int32_t state;

    printf("delay 5s to start test wdt example\n");

    mdelay(5000);

    state = csi_wdt_init(&g_wdt_handle, 0U);

    if (state < 0) {
        printf("csi_wdt_init error\n");
        ret = -1;
    }

    state = test_wdt_feed();

    if (state < 0) {
        printf("test_wdt_feed error\n");
        ret = -1;
    }

    state = test_wdt_reset();

    if (state < 0) {
        printf("test_wdt_reset error\n");
        ret = -1;
    }

    return ret;
}

int main(void)
{
    board_init();
	#ifdef CONFIG_CHIP_LIGHT_FM
	extern void soc_enable_wdt_reset(void);
	soc_enable_wdt_reset();
	#endif

    return example_wdt();
}

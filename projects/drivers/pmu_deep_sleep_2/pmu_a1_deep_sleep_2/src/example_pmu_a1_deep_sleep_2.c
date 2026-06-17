


/******************************************************************************
 * @file     pmu_a1_deep_sleep_2.c
 * @brief    the main function for the PMU Backup mode example
 * @version  V1.0
 * @date     02. June 2018
 ******************************************************************************/
#ifndef CONFIG_TEST_WAKEUP_RTC
#define CONFIG_TEST_WAKEUP_RTC     0

#define RTC_ALARM_DELAY            5

#define RTC_TIME_CURRENT_YEAR              (120)
#define RTC_TIME_CURRENT_MON               (0)
#define RTC_TIME_CURRENT_DAY               (5)
#define RTC_TIME_CURRENT_HOUR              (1)
#define RTC_TIME_CURRENT_MIN               (1)
#define RTC_TIME_CURRENT_SEC               (1)
#endif

#include <stdio.h>
#include <csi_config.h>
#include <soc.h>
#include <board_init.h>
#include <drv/uart.h>
#include <drv/gpio.h>
#include <drv/pm.h>
#include <drv/pin.h>
#if CONFIG_TEST_WAKEUP_RTC
#include <drv/rtc.h>
#endif
#include <board_config.h>
extern int32_t drv_get_boot_type(void);

#if CONFIG_TEST_WAKEUP_RTC

static csi_rtc_t g_rtc_handle;

void rtc_event_cb_fun(csi_rtc_t *rtc_handle, void *arg)
{
    printf("enter rtc cb\n");
}

void config_rtc_wakeup_source(uint32_t timeout)
{
    csi_rtc_time_t rtctime;
    int32_t state;

    state = csi_rtc_init(&g_rtc_handle, EXAMPLE_WAKEUP_RTC_IDX);

    if (state < 0) {
        printf("csi_rtc_init error\n");
    }

    csi_pm_config_wakeup_source(RTC_WAKEUP_NUM, true);
    rtctime.tm_year    = RTC_TIME_CURRENT_YEAR;
    rtctime.tm_mon     = RTC_TIME_CURRENT_MON ;
    rtctime.tm_mday    = RTC_TIME_CURRENT_DAY ;
    rtctime.tm_hour    = RTC_TIME_CURRENT_HOUR;
    rtctime.tm_min     = RTC_TIME_CURRENT_MIN ;
    rtctime.tm_sec     = RTC_TIME_CURRENT_SEC ;
    state = csi_rtc_set_time(&g_rtc_handle, &rtctime);

    if (state < 0) {
        printf("csi_rtc_set_time error\n");
    }

    rtctime.tm_sec +=  timeout;
    csi_rtc_set_alarm(&g_rtc_handle, &rtctime, rtc_event_cb_fun, NULL);
    mdelay(timeout * (1000));

}
#endif

int32_t test_pmu_deep_sleep_2_mode(void)
{   
    int32_t ret;

    if (drv_get_boot_type()) {
#if CONFIG_TEST_WAKEUP_RTC
        printf("sleep_2 mode backup timer time is %d S\n", RTC_ALARM_DELAY);
#endif
        printf("test deep sleep 2 mode successfully\n");
        return 0;
    }

    printf("must run in flash\n");

    csi_pm_init();

#if CONFIG_TEST_WAKEUP_RTC
    (void)ret;
    config_rtc_wakeup_source(RTC_ALARM_DELAY);
#else
    printf("please change the wakeup pin %s to low\n", EXAMPLE_BOARD_WAKEUP_PIN_NAME);
    ret = csi_pm_config_wakeup_source(EXAMPLE_WAKEUP_NUM, true);

    if (ret < 0) {
        printf("csi_pm_config_wakeup_source failed\n");
        return -1;
    }

#endif

#if defined(CONFIG_CHIP_A1_LC_FPGA) || defined(CONFIG_CHIP_A1_LC)
    ///< Set all pins to gpio function except jtag point
    uint32_t backup_pd = wj_get_gpio_pulldown();
    uint32_t backup_pu = wj_get_gpio_pullup();

    wj_pmu_reg_t *pmu_base = (wj_pmu_reg_t *)WJ_PMU_BASE;
    // uint32_t io_num = wj_get_io_wakeup(pmu_base);
    // wj_set_gpio_pullup(~io_num);
    // wj_set_gpio_pulldown(io_num);
    wj_set_gpio_pullup(0xffffffff);
    wj_set_gpio_pulldown(0);
    wj_io_latch_polar_en0(pmu_base);

    csi_pm_enter_sleep(PM_MODE_DEEP_SLEEP_2);

    wj_io_latch_polar_disable0(pmu_base);
    wj_set_gpio_pulldown(backup_pd);
    wj_set_gpio_pullup(backup_pu);
#else
    csi_pm_enter_sleep(PM_MODE_DEEP_SLEEP_2);
#endif

    csi_pm_uninit();

    printf("test deep sleep 2 mode successfully\n");

    return 0;
}

static void gpio_config(void)
{
    csi_pin_set_mux(WAKEUP_PIN, WAKEUP_PIN_FUNC);
    csi_pin_wakeup(WAKEUP_PIN, 1);
}

int example_pmu(void)
{
    gpio_config();
    test_pmu_deep_sleep_2_mode();
    return 0;
}

int main(void)
{
    board_init();
    printf("delay 3s...\n");
    mdelay(3000);
    return example_pmu();
}

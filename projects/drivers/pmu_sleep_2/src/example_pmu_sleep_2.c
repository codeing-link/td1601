


/******************************************************************************
 * @file     example_pmu_sleep_2.c
 * @brief    the main function for the PMU DOZE example
 * @version  V1.0
 * @date     02. June 2018
 ******************************************************************************/
#ifndef CONFIG_TEST_WAKEUP_RTC
//#define CONFIG_TEST_WAKEUP_RTC     0

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
#if CONFIG_TEST_WAKEUP_RTC
#include <drv/rtc.h>
#endif
#include <drv/pin.h>
#include <board_config.h>


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
    mdelay(timeout*(1000));
}
#endif

int32_t test_pmu_sleep_2_mode(void)
{
    int32_t ret;

    printf("test sleep_2 mode\n");
    csi_pm_init();

#if CONFIG_TEST_WAKEUP_RTC
    config_rtc_wakeup_source(RTC_ALARM_DELAY);
#else
    printf("please change the wakeup pin %s to gnd\n", EXAMPLE_BOARD_WAKEUP_PIN_NAME);
    ret = csi_pm_config_wakeup_source(EXAMPLE_WAKEUP_NUM, true);

    if (ret < 0) {
        printf("csi_pm_config_wakeup_source failed\n");
        return -1;
    }

#endif

    ret = csi_pm_enter_sleep(PM_MODE_SLEEP_2);
#if CONFIG_TEST_WAKEUP_RTC
    printf("sleep_1 mode wakeup timer time is %d S\n", RTC_ALARM_DELAY);
#endif

    if (ret < 0) {
        printf("soc_pm_enter_sleep failed\n");
        return -1;
    }

#ifdef CONFIG_TEST_WAKEUP_RTC
    csi_pm_config_wakeup_source(RTC_WAKEUP_NUM, false);
#else
    ret = csi_pm_config_wakeup_source(EXAMPLE_WAKEUP_NUM, false);
#endif
    csi_pm_uninit();

    printf("test sleep_2 mode successfully\n");

    return 0;
}

static void gpio_config(void)
{
    csi_pin_set_mux(WAKEUP_PIN_PORT, WAKEUP_PIN, WAKEUP_PIN_FUNC);
    csi_pin_wakeup(WAKEUP_PIN_PORT, WAKEUP_PIN, 1);
#ifdef CONFIG_CHIP_PANGU
    gpio_pin_handle_t pin = NULL;
    pin = csi_gpio_pin_initialize(WAKEUP_PIN, NULL);

    csi_gpio_pin_config_mode(pin, GPIO_MODE_PULLNONE);
    csi_gpio_pin_config_direction(pin, GPIO_DIRECTION_INPUT);
    csi_gpio_pin_set_irq(pin, GPIO_IRQ_MODE_HIGH_LEVEL, 1);
#endif
}


int example_pmu(void)
{
    gpio_config();
    test_pmu_sleep_2_mode();
    return 0;
}

int main(void)
{
    board_init();
    printf("delay 3s...\n");
    mdelay(3000);
    return example_pmu();
}

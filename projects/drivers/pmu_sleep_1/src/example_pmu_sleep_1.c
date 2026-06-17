


/******************************************************************************
 * @file     example_pmu_sleep_1.c
 * @brief    the main function for the PMU SLEEP example
 * @version  V1.0
 * @date     02. June 2018
 ******************************************************************************/

#ifndef CONFIG_TEST_WAKEUP_TIME
#define CONFIG_TEST_WAKEUP_TIME   0
#define TIME_DELAY  3000000
#endif

#include <stdio.h>
#include <csi_config.h>
#include <soc.h>
#include <board_init.h>
#include <drv/uart.h>
#include <drv/gpio.h>
#if CONFIG_TEST_WAKEUP_TIME
#include <drv/rtc.h>
#endif
#include <drv/pm.h>
#include <drv/timer.h>
#include <drv/pin.h>
#include <board_config.h>

#if CONFIG_TEST_WAKEUP_TIME
static csi_timer_t      g_timer_handle;
#define REG32(addr)     (*(volatile uint32_t *)(addr))
static void timer_event_cb_reload_fun(csi_timer_t *timer_handle, void *arg)
{
    printf("Timer event entered.\n");
}

void config_timer_wakeup_source(uint32_t timeout)
{
    int32_t state;

    state = csi_timer_init(&g_timer_handle, (uint32_t)EXAMPLE_WAKEUP_TIMER_IDX);

    if (state < 0) {
        printf("csi_timer_init error\n");
    }

    state = csi_timer_attach_callback(&g_timer_handle, timer_event_cb_reload_fun, NULL);

    if (state < 0) {
        printf("csi_timer_attach_callback error\n");
    }

    csi_pm_config_wakeup_source(TIMER_WAKEUP_NUM, true);

    state = csi_timer_start(&g_timer_handle, timeout);

    if (state < 0) {
        printf("start timer failt!\n");
    }
}
#endif

int32_t test_pmu_sleep_1_mode(void)
{
    int32_t ret;

    printf("test sleep_1 mode\n");

    csi_pm_init();

#if CONFIG_TEST_WAKEUP_TIME
    config_timer_wakeup_source(TIME_DELAY);
#else
    printf("please change the wakeup pin %s to gnd\n", EXAMPLE_BOARD_WAKEUP_PIN_NAME);
    ret = csi_pm_config_wakeup_source(EXAMPLE_WAKEUP_NUM, true);

    if (ret < 0) {
        printf("csi_pm_config_wakeup_source failed\n");
        return -1;
    }

#endif

    ret = csi_pm_enter_sleep(PM_MODE_SLEEP_1);

    if (ret < 0) {
        printf("soc_pm_enter_sleep_1 failed\n");
        return -1;
    }

#if CONFIG_TEST_WAKEUP_TIME
    uint32_t timer_cur;
    uint32_t timer_load;
    timer_load = csi_timer_get_load_value(&g_timer_handle);
    timer_cur = csi_timer_get_remaining_value(&g_timer_handle);
    printf("sleep_1 mode wakeup timer time is %d\n", timer_load - timer_cur);
#endif



#if CONFIG_TEST_WAKEUP_TIME
    csi_pm_config_wakeup_source(TIMER_WAKEUP_NUM, false);
#else
    csi_pm_config_wakeup_source(EXAMPLE_WAKEUP_NUM, false);
#endif
    csi_pm_uninit();

    printf("test sleep_1 mode successfully\n");

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
    test_pmu_sleep_1_mode();
    return 0;
}

int main(void)
{
    board_init();
    printf("delay 3s...\n");
    mdelay(3000);
    return example_pmu();
}

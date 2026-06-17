

/******************************************************************************
 * @file     example_etb.c
 * @brief    the main function for the etb driver
 * @version  V2.0
 * @date     26. March 2020
 ******************************************************************************/
#include <stdio.h>
#include <soc.h>
#include <drv/timer.h>
#include <drv/etb.h>
#include <board_init.h>
#include <board_config.h>

#define TEST_TIM0   2
#define TEST_TIM1   3

extern const uint16_t timer_hs_num[]; 
extern const uint16_t timer_start_ht_num[];
extern const uint16_t timer_stop_ht_num[];

static csi_timer_t      g_timer_handle;
static csi_timer_t      g_timer1_handle;
static volatile uint8_t time_user_defined_flag = 0;
static uint32_t old_value;

static int32_t etb_config(void)
{
    int32_t ret = CSI_OK;
    csi_etb_config_t etb_config;
    int32_t etb_ch0 = csi_etb_ch_alloc(ETB_CH_ONE_TRIGGER_ONE);

    if (etb_ch0 == CSI_ERROR) {
        ret = CSI_ERROR;
    }

    int32_t etb_ch1 = csi_etb_ch_alloc(ETB_CH_ONE_TRIGGER_ONE);

    if (etb_ch1 == CSI_ERROR) {
        ret = CSI_ERROR;
    }

    etb_config.src_ip = (uint8_t)(timer_hs_num[TEST_TIM0]);
    etb_config.dst_ip = (uint8_t)(timer_start_ht_num[TEST_TIM1]);
    etb_config.ch_type = ETB_CH_ONE_TRIGGER_ONE;
    etb_config.trig_mode = ETB_HARDWARE_TRIG;
    csi_etb_ch_config((int32_t)etb_ch0, &etb_config);
    etb_config.src_ip = (uint8_t)(timer_hs_num[TEST_TIM1]);
    etb_config.dst_ip = (uint8_t)(timer_stop_ht_num[TEST_TIM0]);
    csi_etb_ch_config((int32_t)etb_ch1, &etb_config);

    csi_etb_ch_start(etb_ch0);
    csi_etb_ch_start(etb_ch1);

    return ret;
}

static void timer_event_cb_reload_fun(csi_timer_t *timer_handle, void *arg)
{
    (void)arg;
    time_user_defined_flag = 1;
    old_value = csi_timer_get_remaining_value(&g_timer1_handle);
    printf("ETB trigger starting timer1\n");
}

static int32_t test_user_defined_fun(csi_timer_t *timer_handle, csi_timer_t *timer1_handle)
{
    int32_t ret = 0;
    int32_t state;

    time_user_defined_flag = 0;

    state = csi_timer_start(timer_handle, 5000000);

    if (state < 0) {
        ret = -1;
    }

    state = csi_timer_start(timer1_handle, 3000000);

    if (state < 0) {
        ret = -1;
    }

    csi_timer_stop(timer1_handle);

    while(!time_user_defined_flag);
    uint32_t new_value = old_value;
    do {
        new_value = csi_timer_get_remaining_value(timer1_handle);
    } while (new_value <= old_value);

    printf("ETB trigger stopping timer0\n");

    csi_timer_stop(timer1_handle);

    return ret;
}


static int32_t csi_timer_test_reload_fun(uint8_t timer1_num, uint8_t timer2_num)
{
    int32_t ret = 0;
    int32_t state;

    state = csi_timer_init(&g_timer_handle, (uint32_t)timer1_num);

    if (state < 0) {
        printf("csi_timer_init error\n");
        ret = -1;
    }

    state = csi_timer_init(&g_timer1_handle, (uint32_t)timer2_num);

    if (state < 0) {
        printf("csi_timer_init error\n");
        ret = -1;
    }

    state = csi_timer_attach_callback(&g_timer_handle, timer_event_cb_reload_fun, NULL);

    if (state < 0) {
        printf("csi_timer_attach_callback error\n");
        ret = -1;
    }

    state = csi_timer_attach_callback(&g_timer1_handle, NULL, NULL);

    if (state < 0) {
        printf("csi_timer_attach_callback error\n");
        ret = -1;
    }

    state = test_user_defined_fun(&g_timer_handle, &g_timer1_handle);

    if (state < 0) {
        printf("test_user_defined_fun error\n");
        ret = -1;
    }

    csi_timer_detach_callback(&g_timer_handle);
    csi_timer_detach_callback(&g_timer1_handle);

    csi_timer_uninit(&g_timer_handle);
    csi_timer_uninit(&g_timer1_handle);

    return ret;
}

static int32_t test_etb(void)
{
    int32_t ret = 0;
    int32_t state;

    etb_config();
    state = csi_timer_test_reload_fun(TEST_TIM0, TEST_TIM1);

    if (state < 0) {
        printf("test_reload_fun failed\n");
        ret = -1;
    }

    return ret;
}

int example_etb(void)
{
    int ret = 0;
    int32_t state;

    state = test_etb();

    if (state < 0) {
        printf("test etb failed\n");
        ret = -1;
        CSI_EXAMPLE_RESULT(-1);
    } else {
        printf("test etb successfully\n");
        CSI_EXAMPLE_RESULT(1); 
    }

    return ret;
}

int main(void)
{
    board_init();

    return example_etb();
}

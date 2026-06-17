

/******************************************************************************
 * @file     example_timer.c
 * @brief    the main function for the timer driver
 * @version  V1.0
 * @date     26. March 2020
 ******************************************************************************/
#include <stdio.h>
#include <soc.h>
#include <drv/timer.h>
#include <board_init.h>
#include <board_config.h>

static csi_timer_t      g_timer_handle;
static volatile uint8_t time_user_defined_flag = 0;

static void timer_event_cb_reload_fun(csi_timer_t *timer_handle, void *arg)
{
    (void)arg;
    time_user_defined_flag = 1;
    printf("enter cb for mode user_defined\n");
}

static int32_t test_user_defined_fun(csi_timer_t *timer_handle)
{
    uint32_t timeout = 20000;
    int32_t ret = 0;
    int32_t state;

    time_user_defined_flag = 0;

    state = csi_timer_start(timer_handle, 10000000);

    if (state < 0) {
        ret = -1;
    }

    do {
        timeout--;
        mdelay(1);

        if (0 == timeout) {
            ret = -1;
        }
    } while (0 == time_user_defined_flag);

    csi_timer_stop(timer_handle);

    return ret;
}


static int32_t csi_timer_test_reload_fun(uint8_t timer_num)
{
    int32_t ret = 0;
    int32_t state;

    printf("start timer example\n");
    state = csi_timer_init(&g_timer_handle, (uint32_t)timer_num);

    if (state < 0) {
        printf("csi_timer_init error\n");
        ret = -1;
    }

    state = csi_timer_attach_callback(&g_timer_handle, timer_event_cb_reload_fun, NULL);

    if (state < 0) {
        printf("csi_timer_attach_callback error\n");
        ret = -1;
    }

    state = test_user_defined_fun(&g_timer_handle);

    if (state < 0) {
        printf("test_user_defined_fun error\n");
        ret = -1;
    }

    csi_timer_detach_callback(&g_timer_handle);

    printf("time out after 10s passed\n");

    csi_timer_uninit(&g_timer_handle);

    return ret;
}

static int32_t test_timer(uint8_t timer_num)
{
    int32_t ret = 0;
    int32_t state;

    state = csi_timer_test_reload_fun(timer_num);

    if (state < 0) {
        printf("test_reload_fun failed\n");
        ret = -1;
    }

    return ret;
}

int example_timer(uint8_t timer_num)
{
    int ret = 0;
    int32_t state;

    state = test_timer(timer_num);

    if (state < 0) {
        printf("test timer%d failed\n", timer_num);
        CSI_EXAMPLE_RESULT(-1);
        ret = -1;
    } else {
        printf("test timer%d successfully\n", timer_num);
        CSI_EXAMPLE_RESULT(1);
    }

    return ret;
}

int main(void)
{
    board_init();

    return example_timer(0);
}

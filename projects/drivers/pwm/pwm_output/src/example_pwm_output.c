


/******************************************************************************
 * @file     example_pwm.c
 * @brief    the main function for the PWM driver
 * @version  V1.0
 * @date     26. March 2020
 ******************************************************************************/
#include <stdio.h>
#include <soc.h>
#include <drv/pwm.h>
#include <drv/pin.h>
#include <board_init.h>
#include <board_config.h>

static csi_pwm_t g_pwm_handle;


void  pwm_output_pinmux_init(void)
{
    csi_pin_set_mux(PORTA, EXAMPLE_PWM_CH, EXAMPLE_PWM_CH_FUNC);
}

int32_t  pwm_signal_output(void)
{
    int32_t state;
    int32_t ret = 0;

    state = csi_pwm_init(&g_pwm_handle, EXAMPLE_PWM_IDX);

    if (state < 0) {
        printf("csi_pwm_init error\n");
        ret = -1;
    }

    /**
     * PWM output example, period is 30us, low level is 10us
    */
    state = csi_pwm_out_config(&g_pwm_handle, EXAMPLE_PWM_CH_IDX, 30, 10, PWM_POLARITY_LOW);

    if (state < 0) {
        printf("first csi_pwm_config error\n");
        ret = -1;
    }

    state = csi_pwm_out_start(&g_pwm_handle, EXAMPLE_PWM_CH_IDX);

    if (state < 0) {
        printf("first csi_pwm_out_start error\n");
        ret = -1;
    }

    printf("PWM%d is work in output mode, period=30us, low level=10us \n", EXAMPLE_PWM_CH_IDX);
    mdelay(2500);

    csi_pwm_out_stop(&g_pwm_handle, EXAMPLE_PWM_CH_IDX);

    /**
     * PWM output example, period is 20us, high level is 5us
    */
    state = csi_pwm_out_config(&g_pwm_handle, EXAMPLE_PWM_CH_IDX, 20, 5, PWM_POLARITY_HIGH);

    if (state < 0) {
        printf("second csi_pwm_config error\n");
        ret = -1;
    }

    state = csi_pwm_out_start(&g_pwm_handle, EXAMPLE_PWM_CH_IDX);

    if (state < 0) {
        printf("second csi_pwm_out_start error\n");
        ret = -1;
    }

    printf("PWM%d is work in output mode, period=20us, high level=5us \n", EXAMPLE_PWM_CH_IDX);
    mdelay(2500);

    csi_pwm_out_stop(&g_pwm_handle, EXAMPLE_PWM_CH_IDX);

    csi_pwm_uninit(&g_pwm_handle);

    return ret;
}

int example_pwm()
{
    int ret = 0;
    int32_t state;

    pwm_output_pinmux_init();
    state = pwm_signal_output();

    if (state < 0) {
        printf("pwm_signal_output error\n");
        ret = -1;
    } else {
        printf("pwm_signal_output successful\n");
    }
    CSI_EXAMPLE_RESULT(state);

    return ret;
}

int main(void)
{
    board_init();

    return example_pwm();
}

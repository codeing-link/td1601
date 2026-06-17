


/******************************************************************************
 * @file     example_pwm.c
 * @brief    the main function for the PWM driver
 * @version  V1.0
 * @date     26. March 2020
 ******************************************************************************/
#include <stdio.h>
#include <soc.h>
#include <drv/pwm.h>
#include <drv/gpio.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <board_init.h>
#include <board_config.h>

static csi_pwm_t g_pwm_handle;
static uint32_t  g_pwm_time_us;
volatile static uint8_t   cb_pwm_flag = 0;

/**
 * Analog PWM signal sent to input capture pin for acquisition
*/
#define PWM_SIGNAL_OUTPUT_PRIIOD                             (10U)

void  pwm_event_cb_fun(csi_pwm_t *pwm_handle, csi_pwm_event_t event, uint8_t ch, uint32_t time_us, void *arg)
{
    switch (event) {
        case PWM_EVENT_CAPTURE_POSEDGE:
            printf(" POSEDGE   %6d   %6d\n", time_us, time_us - g_pwm_time_us);
            cb_pwm_flag = 1;
            break;

        case PWM_EVENT_CAPTURE_NEGEDGE:
            printf(" NEGEDGE   %6d   %6d\n", time_us, time_us - g_pwm_time_us);
            cb_pwm_flag = 1;
            break;

        case PWM_EVENT_CAPTURE_BOTHEDGE:
            printf(" BOTHEDGE   %6d   %6d\n", time_us, time_us - g_pwm_time_us);
            cb_pwm_flag = 1;
            break;

        default:
            printf(" Error\n");
            break;
    }

}

void pwm_capture_pinmux_init(void)
{
    csi_pin_set_mux(EXAMPLE_PWM_CAPTURE_GPIO_PIN_PORT, EXAMPLE_PWM_CAPTURE_GPIO_PIN, EXAMPLE_PWM_CAPTURE_GPIO_PIN_FUNC);
    csi_pin_set_mux(EXAMPLE_PWM_CAPTURE_CH_PORT, EXAMPLE_PWM_CAPTURE_CH, EXAMPLE_PWM_CAPTURE_CH_FUNC);
}

int32_t pwm_signal_capture(void)
{
    int32_t state;
    int32_t ret = 0;
    uint32_t loop = 1;

    state = csi_pwm_init(&g_pwm_handle, EXAMPLE_PWM_IDX);

    if (state < 0) {
        printf("csi_pwm_init error\n");
        ret = -1;
    }

    csi_gpio_t test_gpio;

    csi_gpio_init(&test_gpio, 0);
    csi_gpio_dir(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_DIRECTION_OUTPUT);
    csi_gpio_mode(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK,  GPIO_MODE_PULLUP);
    state = csi_pwm_attach_callback(&g_pwm_handle, pwm_event_cb_fun, NULL);

    if (state < 0) {
        printf("csi_pwm_attach_callback error\n");
        ret = -1;
    }

    printf("Num Expect(us) TripMode Actual(us) DiffVal(us)\n");

    do {
        g_pwm_time_us = loop * PWM_SIGNAL_OUTPUT_PRIIOD * 1000;
        printf("%2d   %6d    ", loop, g_pwm_time_us);

#define PWM_CAPTURE_POLARITY            PWM_CAPTURE_POLARITY_NEGEDGE

#if (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_POSEDGE)
        state = csi_pwm_capture_config(&g_pwm_handle, EXAMPLE_PWM_CAPTURE_CH_IDX, PWM_CAPTURE_POLARITY_POSEDGE, loop);
#elif (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_NEGEDGE)
        state = csi_pwm_capture_config(&g_pwm_handle, EXAMPLE_PWM_CAPTURE_CH_IDX, PWM_CAPTURE_POLARITY_NEGEDGE, loop);
#elif (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_BOTHEDGE)
        state = csi_pwm_capture_config(&g_pwm_handle, EXAMPLE_PWM_CAPTURE_CH_IDX, PWM_CAPTURE_POLARITY_BOTHEDGE, loop);
#endif

        if (state < 0) {
            printf("csi_pwm_capture_config error\n");
            ret = -1;
        }


        state = csi_pwm_capture_start(&g_pwm_handle, EXAMPLE_PWM_CAPTURE_CH_IDX);

        if (state < 0) {
            printf("csi_pwm_capture_start error\n");
            ret = -1;
        }

        cb_pwm_flag = 0;
#if (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_POSEDGE)
        csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_LOW);
#elif (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_NEGEDGE)
        csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_HIGH);
#elif (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_BOTHEDGE)
        csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_LOW);
#endif
        mdelay(PWM_SIGNAL_OUTPUT_PRIIOD / 2);

        while (1) {
#if (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_POSEDGE)
            csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_LOW);
#elif (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_NEGEDGE)
            csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_HIGH);
#elif (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_BOTHEDGE)
            csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_LOW);
#endif
            mdelay(PWM_SIGNAL_OUTPUT_PRIIOD / 2);

#if (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_POSEDGE)
            csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_HIGH);
#elif (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_NEGEDGE)
            csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_LOW);
#elif (PWM_CAPTURE_POLARITY == PWM_CAPTURE_POLARITY_BOTHEDGE)
            csi_gpio_write(&test_gpio, EXAMPLE_PWM_CAPTURE_GPIO_MASK, GPIO_PIN_HIGH);
#endif

            mdelay(PWM_SIGNAL_OUTPUT_PRIIOD / 2);

            if (cb_pwm_flag) {
                break;
            }

        }
    } while (loop++ < 50);

    csi_pwm_capture_stop(&g_pwm_handle, EXAMPLE_PWM_CAPTURE_CH_IDX);

    csi_pwm_detach_callback(&g_pwm_handle);

    csi_pwm_uninit(&g_pwm_handle);

    return ret;
}

int example_pwm()
{
    int ret = 0;
    int32_t state;

    pwm_capture_pinmux_init();
    state = pwm_signal_capture();

    if (state < 0) {
        printf("pwm_signal_capture error\n");
        ret = -1;
    } else {
        printf("pwm_signal_capture successful\n");
    }
    CSI_EXAMPLE_RESULT(state);

    return ret;
}

int main(void)
{
    board_init();

    return example_pwm();
}

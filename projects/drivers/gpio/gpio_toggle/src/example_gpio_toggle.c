

/******************************************************************************
 * @file     example_gpio_toggle.c
 * @brief
 * @version
 * @date     2020-01-08
 ******************************************************************************/

#include <stdio.h>

#include <soc.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include <drv/tick.h>
#include <board_config.h>
#include <board_init.h>

static csi_gpio_t gpio;

void gpio_toggle(void)
{
    uint32_t tmp = 0;

    csi_pin_set_mux(EXAMPLE_TOGGLE_GPIO_PIN ,  EXAMPLE_TOGGLE_GPIO_PIN_FUNC);
    csi_pin_set_mux(EXAMPLE_TOGGLE_GPIO_PIN1,  EXAMPLE_TOGGLE_GPIO_PIN1_FUNC);

    csi_gpio_init(&gpio, EXAMPLE_TOGGLE_GPIO_IDX);
    csi_gpio_dir(&gpio, EXAMPLE_TOGGLE_GPIO_PIN_MASK, GPIO_DIRECTION_OUTPUT);
    csi_gpio_write(&gpio, EXAMPLE_TOGGLE_GPIO_PIN_MASK,  GPIO_PIN_LOW);

    while (1) {

        /* toggle pin interval */
        csi_gpio_toggle(&gpio, EXAMPLE_TOGGLE_GPIO_PIN_MASK);
        tmp ^= 1;
        printf("gpio set to: %s \r\n", (tmp == 1) ? "high level" : "low level");
        mdelay(1000);
    }
}

/*****************************************************************************
test_gpio: main function of the gpio test

INPUT: NULL

RETURN: NULL

*****************************************************************************/
int example_gpio()
{
    gpio_toggle();
    return 0;
}

int main(void)
{
    board_init();

    return example_gpio();
}

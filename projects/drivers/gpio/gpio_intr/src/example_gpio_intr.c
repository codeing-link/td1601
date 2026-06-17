


/******************************************************************************
 * @file     example_gpio.c
 * @brief    the main function for the GPIO driver
 * @version  V1.0
 * @date     02. June 2017
 ******************************************************************************/
#include <stdio.h>
#include <soc.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include <drv/tick.h>
#include <board_config.h>
#include <board_init.h>

volatile static bool intr_flag = false;
static csi_gpio_t gpio;

static void gpio_interrupt_handler(csi_gpio_t *gpio, uint32_t pin_mask, void *arg)
{
    intr_flag = true;
}

void gpio_falling_edge_interrupt(void)
{
    csi_pin_set_mux(EXAMPLE_INTR_GPIO_PIN,  EXAMPLE_INTR_GPIO_PIN_FUNC);
    csi_pin_set_mux(EXAMPLE_INTR_GPIO_PIN1, EXAMPLE_INTR_GPIO_PIN1_FUNC);
    csi_gpio_init(&gpio, EXAMPLE_INTR_GPIO_IDX);
    csi_gpio_mode(&gpio, EXAMPLE_INTR_GPIO_PIN_MASK, GPIO_MODE_PULLUP);
    csi_gpio_attach_callback(&gpio, gpio_interrupt_handler, NULL);
    csi_gpio_dir(&gpio, EXAMPLE_INTR_GPIO_PIN_MASK, GPIO_DIRECTION_INPUT);
    csi_gpio_irq_mode(&gpio, EXAMPLE_INTR_GPIO_PIN_MASK, GPIO_IRQ_MODE_FALLING_EDGE);
    csi_gpio_irq_enable(&gpio, EXAMPLE_INTR_GPIO_PIN_MASK, true);

    printf("please change the gpio PA%d pin from high to low\r\n",EXAMPLE_INTR_GPIO_PIN);

    while (1) {
        if (intr_flag) {
            printf("gpio pin passed!!!\n");
            intr_flag = false;
            CSI_EXAMPLE_RESULT(1);
        }
    }
}

/*****************************************************************************
test_gpio: main function of the gpio test

INPUT: NULL

RETURN: NULL

*****************************************************************************/
int example_gpio()
{
    gpio_falling_edge_interrupt();
    return 0;
}

int main(void)
{
    board_init();

    return example_gpio();
}

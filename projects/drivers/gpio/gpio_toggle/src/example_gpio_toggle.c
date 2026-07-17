

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

/* GPIO 句柄（端口级对象，整个端口共用一个） */
static csi_gpio_t gpio;

/*
 * PA24 = 端口 A（PORTA，对应端口索引 0）的第 24 号引脚。
 * soc.h 中 PIN24 已被定义为 (1 << 24)，即引脚掩码，
 * 因此设置方向 / 翻转时直接传 PIN24 即可，无需再写 (1 << 24)。
 */
#define LED_GPIO_PORT   PORTA           /* 引脚所在端口：端口 A */
#define LED_GPIO_PIN    PIN24           /* 引脚掩码：第 24 号引脚 = (1 << 24) */
#define LED_GPIO_FUNC   PIN_FUNC_GPIO   /* 引脚复用功能：普通 GPIO */

void gpio_toggle(void)
{
    uint32_t tmp = 0;

    /* 1. 引脚复用配置：将 PA24 复用为 GPIO 功能 */
    csi_pin_set_mux(LED_GPIO_PORT, LED_GPIO_PIN, LED_GPIO_FUNC);

    /* 2. 初始化 GPIO 端口：绑定 gpio 句柄到端口 0（PA24 属于端口 A / 0 号端口） */
    csi_gpio_init(&gpio, LED_GPIO_PORT);

    /* 3. 设置引脚方向：将 PA24 配置为输出模式（用于控制 LED 亮灭） */
    csi_gpio_dir(&gpio, LED_GPIO_PIN, GPIO_DIRECTION_OUTPUT);

    /* 4. 给定初始电平：先输出低电平 */
    csi_gpio_write(&gpio, LED_GPIO_PIN, GPIO_PIN_LOW);

    /* 死循环：持续翻转引脚电平，实现 LED 闪烁 */
    while (1) {
        /* 翻转 PA24 电平：高 <-> 低 自动切换（LED 亮灭的核心） */
        csi_gpio_toggle(&gpio, LED_GPIO_PIN);

        /* 软件标记当前电平状态（仅用于打印，无硬件控制作用） */
        tmp ^= 1;
        //printf("gpio set to: %s \r\n", (tmp == 1) ? "high level" : "low level");

        /* 延时 1000 毫秒：控制闪烁速度，数值越大闪烁越慢 */
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

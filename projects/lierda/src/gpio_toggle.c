#include <stdint.h>
#include <soc.h>
#include <drv/gpio.h>
#include <drv/pin.h>
#include <drv/tick.h>
#include "gpio_toggle.h"

#define LED_GPIO_PORT       PORTA
#define LED_GPIO_PIN        PIN24
#define LED_GPIO_FUNC       PIN_FUNC_GPIO
#define LED_TOGGLE_PERIOD_MS 1000ULL

static csi_gpio_t s_gpio;
static uint64_t s_last_toggle_ms;
static uint8_t s_initialized;

int gpio_toggle_init(void)
{
    csi_error_t ret;

    /* 将 PA24 复用为普通 GPIO。 */
    ret = csi_pin_set_mux(LED_GPIO_PORT, LED_GPIO_PIN, LED_GPIO_FUNC);
    if (ret != CSI_OK) {
        return (int)ret;
    }

    /* PA24 属于 GPIOA，初始化端口并配置为输出。 */
    ret = csi_gpio_init(&s_gpio, LED_GPIO_PORT);
    if (ret != CSI_OK) {
        return (int)ret;
    }

    ret = csi_gpio_dir(&s_gpio, LED_GPIO_PIN, GPIO_DIRECTION_OUTPUT);
    if (ret != CSI_OK) {
        csi_gpio_uninit(&s_gpio);
        return (int)ret;
    }

    /* 初始输出低电平，之后每 1000 ms 翻转一次。 */
    csi_gpio_write(&s_gpio, LED_GPIO_PIN, GPIO_PIN_LOW);
    s_last_toggle_ms = csi_tick_get_ms();
    s_initialized = 1U;

    return 0;
}

void gpio_toggle(void)
{
    uint64_t now_ms;

    if (s_initialized == 0U) {
        return;
    }

    now_ms = csi_tick_get_ms();
    if ((now_ms - s_last_toggle_ms) >= LED_TOGGLE_PERIOD_MS) {
        csi_gpio_toggle(&s_gpio, LED_GPIO_PIN);
        s_last_toggle_ms = now_ms;
    }
}

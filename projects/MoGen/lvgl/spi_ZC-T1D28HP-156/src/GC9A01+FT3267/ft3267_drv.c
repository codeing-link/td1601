#include "ft3267_drv.h"

#include <soc.h>
#include <drv/iic.h>
#include <drv/gpio.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <board_config.h>

#define TP_INT_PIN      PB20
#define TP_RST_PIN      PB17
#define TP_GPIO_IDX     1
#define TP_RST_MSK      (1 << (TP_RST_PIN - 32))
#define TP_INT_MSK      (1 << (TP_INT_PIN - 32))

#define IIC_TIMEOUT     100

static csi_iic_t  ft_iic;
static csi_gpio_t tp_gpio;
static volatile uint8_t tp_int_flag = 0;

static void tp_int_handler(csi_gpio_t *gpio, uint32_t pin_mask, void *arg)
{
    tp_int_flag = 1;
}

uint8_t ft3267_is_touched(void)
{
    if (tp_int_flag) {
        tp_int_flag = 0;
        return 1;
    }
    return 0;
}

static void ft_read_regs(uint8_t reg, uint8_t *data, uint8_t len)
{
    csi_iic_master_send(&ft_iic, FT3267_ADDR, &reg, 1, IIC_TIMEOUT);
    csi_iic_master_receive(&ft_iic, FT3267_ADDR, data, len, IIC_TIMEOUT);
}

static void tp_reset(void)
{
    uint32_t s;
    csi_gpio_write(&tp_gpio, TP_RST_MSK, 0);
    s = csi_tick_get_ms(); while ((csi_tick_get_ms() - s) < 10);
    csi_gpio_write(&tp_gpio, TP_RST_MSK, 1);
    s = csi_tick_get_ms(); while ((csi_tick_get_ms() - s) < 300);
}

void ft3267_init(void)
{
    csi_pin_set_mux(TP_RST_PIN, PIN_FUNC_GPIO);
    csi_pin_set_mux(TP_INT_PIN, PIN_FUNC_GPIO);
    csi_gpio_init(&tp_gpio, TP_GPIO_IDX);
    csi_gpio_dir(&tp_gpio, TP_RST_MSK, GPIO_DIRECTION_OUTPUT);
    csi_gpio_mode(&tp_gpio, TP_INT_MSK, GPIO_MODE_PULLUP);
    csi_gpio_attach_callback(&tp_gpio, tp_int_handler, NULL);
    csi_gpio_dir(&tp_gpio, TP_INT_MSK, GPIO_DIRECTION_INPUT);
    csi_gpio_irq_mode(&tp_gpio, TP_INT_MSK, GPIO_IRQ_MODE_FALLING_EDGE);
    csi_gpio_irq_enable(&tp_gpio, TP_INT_MSK, true);
    tp_reset();

    csi_pin_set_mux(EXAMPLE_PIN_IIC_SDA, EXAMPLE_PIN_IIC_SDA_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_IIC_SCL, EXAMPLE_PIN_IIC_SCL_FUNC);
    csi_iic_init(&ft_iic, EXAMPLE_IIC_IDX);
    csi_iic_mode(&ft_iic, IIC_MODE_MASTER);
    csi_iic_addr_mode(&ft_iic, IIC_ADDRESS_7BIT);
    csi_iic_speed(&ft_iic, IIC_BUS_SPEED_FAST);
}

uint8_t ft3267_read_touch(uint16_t *x, uint16_t *y)
{
    uint8_t points = 0;
    uint8_t raw[5];

    ft_read_regs(FT3267_TOUCH_POINTS, &points, 1);
    points &= 0x0F;
    if (points == 0) return 0;

    ft_read_regs(FT3267_TOUCH1_XH, raw, 4);
    *x = ((raw[0] & 0x0F) << 8) | raw[1];
    *y = ((raw[2] & 0x0F) << 8) | raw[3];
    return points;
}

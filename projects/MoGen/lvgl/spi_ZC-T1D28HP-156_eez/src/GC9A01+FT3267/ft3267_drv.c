#include "ft3267_drv.h"

#include <soc.h>
#include <drv/iic.h>
#include <drv/gpio.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <board_config.h>

#define TP_INT_PIN      PB20
#define TP_RST_PIN      PB17

/* Touch panel logical resolution (matches the GC9A01 240x240 LCD). */
#define TP_RES_X        240
#define TP_RES_Y        240
#define TP_GPIO_IDX     1
#define TP_RST_MSK      (1 << (TP_RST_PIN - 32))
#define TP_INT_MSK      (1 << (TP_INT_PIN - 32))

/*
 * Keep the per-transfer I2C timeout SHORT. The touch read runs inside LVGL's
 * indev poll (~30 ms). If a single transfer ever stalls (NAK / bus busy) the
 * old 100 ms timeout would block the whole UI loop long enough to miss the
 * sample window, so taps were dropped ("have to press several times"). 20 ms
 * is far longer than a real FT3267 read needs yet bounds any stall.
 */
#define IIC_TIMEOUT     20

static csi_iic_t  ft_iic;
static csi_gpio_t tp_gpio;
static volatile uint8_t tp_int_flag = 0;

static void tp_int_handler(csi_gpio_t *gpio, uint32_t pin_mask, void *arg)
{
    tp_int_flag = 1;
}

/*
 * Consume the touch INT flag: returns 1 if the controller asserted INT (i.e.
 * touch activity happened) since the last call, then clears it.
 *
 * The FT3267 pulls INT low on a falling edge whenever the touch state changes.
 * We use this to detect the START of a press immediately, even if the LCD is
 * busy blocking the main loop on a full-screen SPI flush: the ISR sets the flag
 * the instant the finger lands, so the press can never be missed just because
 * lv_timer_handler() was stuck inside lcd_fill_rect(). Dragging/holding is then
 * handled by polling (see ft3267_read_touch()), because INT does not keep
 * pulsing while the finger stays down.
 */
uint8_t ft3267_int_consume(void)
{
    if (tp_int_flag) {
        tp_int_flag = 0;
        return 1;
    }
    return 0;
}

/*
 * Returns 0 on success, -1 on any I2C error/timeout.
 *
 * The CSI I2C calls return the number of bytes actually transferred, or a
 * NEGATIVE error code (CSI_TIMEOUT / CSI_ERROR) on failure. The old code
 * IGNORED these return values. On a timeout csi_iic_master_receive() leaves
 * the caller's buffer UNTOUCHED (it never runs its copy loop), so 'data' kept
 * stale/garbage bytes from the previous read or the stack. Those garbage bytes
 * were then decoded as a real touch -> spurious coordinates like 4096 4096,
 * which is exactly the "after running a while" symptom. By propagating the
 * error we let the caller reject the sample instead of trusting dirty data.
 */
static int ft_read_regs(uint8_t reg, uint8_t *data, uint8_t len)
{
    if (csi_iic_master_send(&ft_iic, FT3267_ADDR, &reg, 1, IIC_TIMEOUT) != 1) {
        return -1;
    }
    if (csi_iic_master_receive(&ft_iic, FT3267_ADDR, data, len, IIC_TIMEOUT) != (int32_t)len) {
        return -1;
    }
    return 0;
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
    /*
     * Read the touch-point count AND the first point's coordinates in ONE
     * contiguous burst starting at reg 0x02 (TD_STATUS, then XH,XL,YH,YL).
     * The old code issued TWO separate reg+read transactions; halving the I2C
     * traffic roughly halves the chance of a stalled/dropped read and keeps
     * the whole sample well inside the LVGL poll window -> fewer missed taps.
     *
     *   raw[0] = 0x02 TD_STATUS  (low nibble = number of touch points)
     *   raw[1] = 0x03 P1_XH      (bits[3:0] = X[11:8], bits[7:6] = event flag)
     *   raw[2] = 0x04 P1_XL      (X[7:0])
     *   raw[3] = 0x05 P1_YH      (bits[3:0] = Y[11:8])
     *   raw[4] = 0x06 P1_YL      (Y[7:0])
     */
    uint8_t raw[5];

    /* Reject the whole sample on any I2C error/timeout so we never decode
     * stale garbage into bogus coordinates (the 4096 4096 symptom). */
    if (ft_read_regs(FT3267_TOUCH_POINTS, raw, 5) != 0) {
        return 0;
    }

    uint8_t points = raw[0] & 0x0F;
    if (points == 0 || points > 5) {
        return 0;                       /* no/invalid touch */
    }

    uint16_t rx = (uint16_t)(((raw[1] & 0x0F) << 8) | raw[2]);
    uint16_t ry = (uint16_t)(((raw[3] & 0x0F) << 8) | raw[4]);

    /* Clamp to the panel. A glitchy controller can momentarily report a
     * coordinate outside 0..(res-1); feeding that to LVGL corrupts hit-testing
     * and produces the odd out-of-range values seen in the logs. */
    if (rx >= TP_RES_X) rx = TP_RES_X - 1;
    if (ry >= TP_RES_Y) ry = TP_RES_Y - 1;

    *x = rx;
    *y = ry;
    return points;
}

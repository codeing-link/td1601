#include "lv_port.h"
#include "lcd_drv.h"
#include "ft3267_drv.h"
#include <drv/tick.h>
#include "lvgl/lvgl.h"

#define TFT_HOR_RES  LCD_WIDTH
#define TFT_VER_RES  LCD_HEIGHT

/*
 * SINGLE-buffered partial rendering.
 *
 * Our my_flush_cb() is fully SYNCHRONOUS: lcd_fill_rect() shifts the whole area
 * out over SPI and only then do we call lv_disp_flush_ready(). That is the
 * correct pairing with a SINGLE buffer — LVGL renders a chunk, we send it to
 * completion, and only after it is fully on the wire does LVGL reuse the buffer
 * for the next chunk. The buffer can never be overwritten while it is being
 * sent.
 *
 * A DOUBLE buffer here was actively harmful: LVGL's double-buffer path assumes
 * an ASYNC flush (return immediately, signal lv_disp_flush_ready() later from a
 * DMA/IRQ completion). Because we instead signal ready synchronously, LVGL's
 * wait_for_flushing() never blocks and it swaps/reuses the other buffer
 * immediately — so on busy frames (page switches with many dirty rects) it
 * could start rendering the next chunk into a buffer whose pixels we were still
 * reading out, producing the colored garbage blocks. Going back to a single
 * buffer removes that race entirely.
 *
 * One buffer of 1/4 screen (240 x 60 x 2 B/px = 28800 bytes).
 */
#define LV_BUF_LINES  60
#define LV_BUF_SIZE   (TFT_HOR_RES * LV_BUF_LINES * 2)   /* bytes, 2 B/px */

static uint32_t tick_get_wrapper(void) {
    return (uint32_t)csi_tick_get_ms();
}

static void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lcd_fill_rect(area->x1, area->y1, area->x2 + 1, area->y2 + 1, (uint16_t *)px_map);
    lv_disp_flush_ready(disp);
}

/*
 * Interrupt-assisted touch read with a short press-latch (debounce-on-release).
 *
 * LVGL polls this callback every LV_DEF_REFR_PERIOD (16 ms). Two problems made
 * touch feel unresponsive:
 *
 *  1) Blocking SPI flushes. lcd_fill_rect() shifts a whole partial area out over
 *     SPI synchronously, so while LVGL is flushing, lv_timer_handler() (and thus
 *     this read callback) cannot run. A press that lands during a flush could be
 *     missed entirely if we only sampled I2C at poll time.
 *  2) Single-poll I2C dropouts. The FT3267 read can occasionally return "no
 *     point" for one poll even though the finger is still down (TD_STATUS not
 *     refreshed yet, or one I2C burst glitched), splitting one tap into two
 *     short presses so the widget's CLICKED never fires.
 *
 * Fixes here:
 *  - INT does NOT keep pulsing while the finger stays down, so dragging/holding
 *    is handled by polling ft3267_read_touch() every tick once pressed.
 *  - Once pressed, we keep reporting PRESSED at the LAST KNOWN coordinate until
 *    we've read "no point" for TOUCH_RELEASE_DEBOUNCE consecutive polls, riding
 *    out single-poll dropouts. A real lift still releases within ~2 polls.
 *
 * CRITICAL: we must NEVER report PRESSED without a coordinate that belongs to
 * the CURRENT touch. An earlier version forced PRESSED on a bare INT event and
 * reused the static last_x/last_y from the PREVIOUS touch. That made LVGL latch
 * the new press onto the OLD widget's location: you tapped icon 1 but icon 2
 * (the previously-touched spot) fired. So:
 *  - A press may only BEGIN on a real, freshly read coordinate.
 *  - The INT flag is consumed only to refine release-debounce while ALREADY
 *    pressed; it can never start a press on its own.
 */
#define TOUCH_RELEASE_DEBOUNCE  2

static void my_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    static int32_t last_x = 0;
    static int32_t last_y = 0;
    static uint8_t pressed = 0;
    static uint8_t miss = 0;

    /* Consume the INT flag every poll so it never goes stale. It only matters
     * while we are already pressed (helps ride out a single dropped sample);
     * it must NOT be allowed to start a press, because at that moment we have
     * no valid coordinate for the new touch. */
    uint8_t int_event = ft3267_int_consume();

    uint16_t tx = 0, ty = 0;
    if (ft3267_read_touch(&tx, &ty)) {
        printf("touch point x=%u y=%u\n", (unsigned int)tx, (unsigned int)ty);
        /* Valid point read: the ONLY way a press is allowed to begin, and the
         * authoritative source of coordinates for press + drag. */
        last_x = (int32_t)tx;
        last_y = (int32_t)ty;
        pressed = 1;
        miss = 0;
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else if (pressed && ((miss + 1) < TOUCH_RELEASE_DEBOUNCE || int_event)) {
        /*
         * Already pressed but this poll read no point. Hold the press at the
         * last (valid, current-touch) coordinate to absorb a single dropout.
         * If INT also fired the finger is almost certainly still down, so allow
         * one extra hold. We do NOT enter here unless 'pressed' is already set,
         * so the coordinate is guaranteed to belong to the current touch.
         */
        miss++;
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        pressed = 0;
        miss = 0;
        data->point.x = last_x;     /* keep coord stable on release for LVGL */
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lv_port_init(void)
{
    lv_tick_set_cb(tick_get_wrapper);

    lv_display_t *display = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565_SWAPPED);
    static uint8_t buf1[LV_BUF_SIZE];
    /* Single buffer: second pointer is NULL. See the buffer comment above for
     * why double buffering races with our synchronous flush. */
    lv_display_set_buffers(display, buf1, NULL, LV_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, my_flush_cb);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_read_cb);
}

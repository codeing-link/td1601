#include <board_init.h>
#include <drv/tick.h>
#include <drv/timer.h>
#include <csi_core.h>
#include "GC9A01+FT3267/lcd_drv.h"
#include "GC9A01+FT3267/ft3267_drv.h"

/* 功能选择: 1 = LVGL RGB 全屏刷新 FPS 测试, 0 = LVGL Hello World */
#define APP_MODE_RGB_FPS_TEST   1

#include "lvgl/lvgl.h"

#define TFT_HOR_RES  LCD_WIDTH
#define TFT_VER_RES  LCD_HEIGHT

volatile uint32_t csi_timer_ms = 0;

#if APP_MODE_RGB_FPS_TEST
/*==========================================================================
 * LVGL RGB 全屏刷新 FPS 测试
 *==========================================================================*/

static uint8_t disp_buf[TFT_HOR_RES * TFT_VER_RES * 2];

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    lcd_set_windows(area->x1, area->y1, area->x2, area->y2);
    lcd_send_pixels(px_map, w * h * 2);
    lv_disp_flush_ready(disp);
}

static lv_color_t rgb_colors[] = {
    {.red = 0xFF, .green = 0x00, .blue = 0x00},  // red
    {.red = 0x00, .green = 0xFF, .blue = 0x00},  // green
    {.red = 0x00, .green = 0x00, .blue = 0xFF},  // blue
    {.red = 0xFF, .green = 0xFF, .blue = 0xFF},  // white
    {.red = 0x00, .green = 0x00, .blue = 0x00},  // black
    {.red = 0xFF, .green = 0xFF, .blue = 0x00},  // yellow
    {.red = 0xFF, .green = 0x00, .blue = 0xFF},  // magenta
    {.red = 0x00, .green = 0xFF, .blue = 0xFF},  // cyan
};

int main(void)
{
    board_init();
    lcd_init();
    lcd_clear(0x0000);

    lv_init();
    lv_tick_set_cb(csi_tick_get_ms);

    lv_display_t *disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, disp_buf, NULL, sizeof(disp_buf), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(disp, flush_cb);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *panel = lv_obj_create(scr);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, TFT_HOR_RES, TFT_VER_RES);
    lv_obj_set_pos(panel, 0, 0);

    uint8_t color_count = sizeof(rgb_colors) / sizeof(rgb_colors[0]);
    uint8_t color_idx = 0;
    uint32_t frame_count = 0;
    uint32_t t_last_print = csi_tick_get_ms();

    printf("=== LVGL RGB FPS Test Start ===\n");
    printf("Resolution: %dx%d, SPI CLK: 68MHz\n", LCD_WIDTH, LCD_HEIGHT);
    printf("LVGL buf: %u bytes (1/%d screen)\n",
           (unsigned)sizeof(disp_buf1), (TFT_HOR_RES * TFT_VER_RES * 2) / (int)sizeof(disp_buf1));

    while (1) {
        lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(panel, rgb_colors[color_idx], 0);
        lv_obj_invalidate(panel);

        lv_timer_handler();

        color_idx = (color_idx + 1) % color_count;
        frame_count++;

        uint32_t t_now = csi_tick_get_ms();
        uint32_t elapsed = t_now - t_last_print;

        if (elapsed >= 1000) {
            uint32_t fps_x10 = frame_count * 10000 / elapsed;
            printf("FPS: %u.%u  (%u frames / %u ms)\n",
                   (unsigned)(fps_x10 / 10), (unsigned)(fps_x10 % 10),
                   (unsigned)frame_count, (unsigned)elapsed);
            frame_count = 0;
            t_last_print = t_now;
        }
    }
}

#else
/*==========================================================================
 * LVGL Hello World (原有功能)
 *==========================================================================*/

static uint8_t disp_buf1[TFT_HOR_RES * TFT_VER_RES];
static uint8_t disp_buf2[TFT_HOR_RES * TFT_VER_RES];

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    lcd_set_windows(area->x1, area->y1, area->x2, area->y2);
    lcd_send_pixels(px_map, w * h * 2);
    lv_disp_flush_ready(disp);
}

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t x, y;
    if (ft3267_read_touch(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lcd_test_color_bars(void)
{
    uint16_t colors[] = {
        0xF800,  // red
        0x07E0,  // green
        0x001F,  // blue
        0xFFE0,  // yellow
        0xF81F,  // magenta
        0x07FF,  // cyan
        0xFFFF,  // white
        0x0000,  // black
    };
    uint16_t bar_height = LCD_HEIGHT / 8;
    uint16_t line_buf[LCD_WIDTH];
    uint16_t i, y;

    for (i = 0; i < 8; i++) {
        uint16_t j;
        for (j = 0; j < LCD_WIDTH; j++) {
            line_buf[j] = colors[i];
        }
        for (y = 0; y < bar_height; y++) {
            lcd_fill_rect(0, i * bar_height + y, LCD_WIDTH, i * bar_height + y + 1, line_buf);
        }
    }
}

static int32_t csi_timer_test_reload_fun(uint8_t timer_num);
static uint32_t timer_ms(void);

int main(void)
{
    board_init();

    //csi_icache_disable();
    //csi_dcache_disable();
    //csi_timer_test_reload_fun(0);

    lcd_init();
    ft3267_init();

    lcd_clear(0x0000);
    lcd_test_color_bars();
    lcd_delay_ms(1000);

    lv_init();
#if 1
    lv_tick_set_cb(csi_tick_get_ms);
#else
    lv_tick_set_cb(timer_ms);
#endif

    lv_display_t *disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAP);
    lv_display_set_buffers(disp, disp_buf1, disp_buf2, sizeof(disp_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);

    // lv_indev_t *indev = lv_indev_create();
    // lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    // lv_indev_set_read_cb(indev, touch_read_cb);

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello world");
    lv_obj_center(label);

    printf("begin\n");
    while (1) {
        lv_timer_handler();
        lcd_delay_ms(20);
    }
}

#endif /* APP_MODE_RGB_FPS_TEST */


#if !APP_MODE_RGB_FPS_TEST
/*************************************************/
static csi_timer_t      g_timer_handle;

static void timer_event_cb_reload_fun(csi_timer_t *timer_handle, void *arg)
{
    (void)arg;
    csi_timer_ms++;
}

static uint32_t timer_ms(void)
{
    return csi_timer_ms;
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

    state = csi_timer_start(&g_timer_handle, 1000);
    if (state < 0) {
        printf("test_user_defined_fun error\n");
        ret = -1;
    }

    return ret;
}
#endif /* !APP_MODE_RGB_FPS_TEST */

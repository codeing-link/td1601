

/******************************************************************************
 * @file     example_lcd_gc9a01.c
 * @brief    GC9A01 LCD driver example for TD1601 platform
 * @version  V1.1
 * @date     2024
 ******************************************************************************/
#include <stdio.h>
#include <string.h>

#include <soc.h>
#include <drv/tick.h>
#include <board_config.h>
#include <board_init.h>

#include "lcd_drv.h"
#include "ft3267_drv.h"

#define TEST_FPS_RGB        1
#define TEST_TOUCH_DRAW     0

#if TEST_FPS_RGB

#define FPS_TEST_FRAMES     30

static void lcd_fps_test_rgb(void)
{
    uint16_t colors[] = {
        0xF800,  // Red
        0x07E0,  // Green
        0x001F,  // Blue
    };
    const char *color_names[] = {"Red", "Green", "Blue"};
    uint32_t t_start, t_end, elapsed_ms;
    uint16_t i, c;

    printf("\n===== RGB Full-screen Fill FPS Test =====\n");
    printf("Resolution: %dx%d, SPI CLK: 68MHz, DMA: %s\n", LCD_WIDTH, LCD_HEIGHT,
           LCD_SPI_DMA_EN ? "ON" : "OFF");
    printf("Data per frame: %d bytes (RGB565)\n", LCD_WIDTH * LCD_HEIGHT * 2);
    printf("Frames per color: %d\n\n", FPS_TEST_FRAMES);

    for (c = 0; c < 3; c++) {
        t_start = csi_tick_get_ms();

        for (i = 0; i < FPS_TEST_FRAMES; i++) {
            lcd_clear(colors[c]);
        }

        t_end = csi_tick_get_ms();
        elapsed_ms = t_end - t_start;

        uint32_t fps_x10 = (uint32_t)FPS_TEST_FRAMES * 10000 / elapsed_ms;
        printf("[%s] %d frames in %lu ms => %lu.%lu FPS\n",
               color_names[c], FPS_TEST_FRAMES, (unsigned long)elapsed_ms,
               (unsigned long)(fps_x10 / 10), (unsigned long)(fps_x10 % 10));
    }

    printf("\n--- RGB Cycling (continuous, report every 30 frames) ---\n");
    uint32_t frame_count = 0;
    t_start = csi_tick_get_ms();

    while (1) {
        lcd_clear(colors[frame_count % 3]);
        frame_count++;

        if ((frame_count % 30) == 0) {
            t_end = csi_tick_get_ms();
            elapsed_ms = t_end - t_start;
            uint32_t fps_x10 = (uint32_t)30 * 10000 / elapsed_ms;
            printf("frames=%lu, FPS=%lu.%lu\n",
                   (unsigned long)frame_count,
                   (unsigned long)(fps_x10 / 10),
                   (unsigned long)(fps_x10 % 10));
            t_start = t_end;
        }
    }
}

#endif /* TEST_FPS_RGB */

#if TEST_TOUCH_DRAW

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

#endif /* TEST_TOUCH_DRAW */

int main(void)
{
    board_init();

    printf("GC9A01 LCD init...\n");
    lcd_init();
    printf("LCD init done.\n");

#if TEST_FPS_RGB
    lcd_fps_test_rgb();
#endif

#if TEST_TOUCH_DRAW
    printf("LCD clear to black...\n");
    lcd_clear(0x0000);

    printf("LCD color bars test...\n");
    lcd_test_color_bars();
    printf("LCD test done.\n");

    printf("Touch init...\n");
    ft3267_init();
    printf("Touch ready. Touch the screen to draw points.\n");

    lcd_clear(0x0000);

    while (1) {
        uint16_t x, y;
        if (ft3267_is_touched() && ft3267_read_touch(&x, &y)) {
            printf("touch: x=%d y=%d\n", x, y);
            lcd_draw_point(x, y, 0xF800);
        }
    }
#endif

    return 0;
}


#ifndef LCD_DRV_H
#define LCD_DRV_H

#include <stdint.h>

#define LCD_SPI_DMA_EN      1

#define LCD_HEIGHT          240
#define LCD_WIDTH           240

void lcd_init(void);
void lcd_clear(uint16_t color);
void lcd_set_windows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
void lcd_fill_rect(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color);
void lcd_display(uint16_t *image);
void lcd_set_backlight(uint8_t value);
void lcd_delay_ms(uint32_t ms);

#endif //LCD_DRV_H

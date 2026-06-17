
#include "lcd_drv.h"

#include <stdio.h>
#include <string.h>
#include <soc.h>
#include <drv/spi.h>
#include <drv/dma.h>
#include <drv/gpio.h>
#include <drv/tick.h>
#include <drv/pin.h>
#include <board_config.h>

#define LCD_SPI_IDX         0
// TD1601 -> APB0 -> SPI0 CLK MAX 75M
#define LCD_SPI_CLK_RATE    68000000	// (69 就不行了, 待分析)

#define LCD_PIN_MOSI        PA25
#define LCD_PIN_SCK         PA24
#define LCD_PIN_CS          PA22
#define LCD_PIN_DC          PA23
#define LCD_PIN_RST         PA21

#define LCD_PIN_MOSI_FUNC   PA25_SPI0_MOSI
#define LCD_PIN_SCK_FUNC    PA24_SPI0_SCK

#define LCD_PIN_CS_FUNC     PIN_FUNC_GPIO
#define LCD_PIN_DC_FUNC     PIN_FUNC_GPIO
#define LCD_PIN_RST_FUNC    PIN_FUNC_GPIO

#define LCD_GPIO_IDX        0
#define LCD_PIN_CS_MSK      (1 << LCD_PIN_CS)
#define LCD_PIN_DC_MSK      (1 << LCD_PIN_DC)
#define LCD_PIN_RST_MSK     (1 << LCD_PIN_RST)

#define LCD_CS_HIGH()       csi_gpio_write(&lcd_gpio, LCD_PIN_CS_MSK, 1)
#define LCD_CS_LOW()        csi_gpio_write(&lcd_gpio, LCD_PIN_CS_MSK, 0)
#define LCD_DC_HIGH()       csi_gpio_write(&lcd_gpio, LCD_PIN_DC_MSK, 1)
#define LCD_DC_LOW()        csi_gpio_write(&lcd_gpio, LCD_PIN_DC_MSK, 0)
#define LCD_RST_HIGH()      csi_gpio_write(&lcd_gpio, LCD_PIN_RST_MSK, 1)
#define LCD_RST_LOW()       csi_gpio_write(&lcd_gpio, LCD_PIN_RST_MSK, 0)

#define LCD_DMA_TIMEOUT     5000

static csi_spi_t    lcd_spi;
static csi_gpio_t   lcd_gpio;

#if LCD_SPI_DMA_EN
static csi_dma_ch_t lcd_tx_dma;
static csi_dma_ch_t lcd_rx_dma;
static volatile int lcd_spi_event;

static void lcd_spi_event_cb(csi_spi_t *spi, csi_spi_event_t event, void *arg)
{
    lcd_spi_event = (int)event;
}
#endif

void lcd_delay_ms(uint32_t ms)
{
    uint32_t start = csi_tick_get_ms();
    while ((csi_tick_get_ms() - start) < ms);
}

static void lcd_gpio_init(void)
{
    csi_pin_set_mux(LCD_PIN_CS, LCD_PIN_CS_FUNC);
    csi_pin_set_mux(LCD_PIN_DC, LCD_PIN_DC_FUNC);
    csi_pin_set_mux(LCD_PIN_RST, LCD_PIN_RST_FUNC);

    csi_gpio_init(&lcd_gpio, LCD_GPIO_IDX);
    csi_gpio_dir(&lcd_gpio, LCD_PIN_CS_MSK | LCD_PIN_DC_MSK | LCD_PIN_RST_MSK, GPIO_DIRECTION_OUTPUT);

    LCD_CS_HIGH();
    LCD_DC_HIGH();
    LCD_RST_HIGH();
}

static void lcd_spi_init(void)
{
    csi_pin_set_mux(LCD_PIN_MOSI, LCD_PIN_MOSI_FUNC);
    csi_pin_set_mux(LCD_PIN_SCK, LCD_PIN_SCK_FUNC);

    csi_spi_init(&lcd_spi, LCD_SPI_IDX);
    csi_spi_mode(&lcd_spi, SPI_MASTER);
    csi_spi_cp_format(&lcd_spi, SPI_FORMAT_CPOL0_CPHA0);
    csi_spi_frame_len(&lcd_spi, SPI_FRAME_LEN_8);
    csi_spi_baud(&lcd_spi, LCD_SPI_CLK_RATE);
    csi_spi_select_slave(&lcd_spi, 0);

#if LCD_SPI_DMA_EN
    csi_spi_link_dma(&lcd_spi, &lcd_tx_dma, &lcd_rx_dma);
    csi_spi_attach_callback(&lcd_spi, lcd_spi_event_cb, NULL);
#endif
}

static void lcd_spi_send(const uint8_t *data, uint32_t size)
{
#if LCD_SPI_DMA_EN
    uint32_t timestart;
    lcd_spi_event = -1;
    csi_spi_send_async(&lcd_spi, data, size);
    timestart = csi_tick_get_ms();
    while (lcd_spi_event != SPI_EVENT_SEND_COMPLETE) {
        if ((csi_tick_get_ms() - timestart) > LCD_DMA_TIMEOUT) break;
    }
#else
    csi_spi_send(&lcd_spi, data, size, 1000);
#endif
}

static void lcd_send_command(uint8_t cmd)
{
    LCD_DC_LOW();
    LCD_CS_LOW();
    lcd_spi_send(&cmd, 1);
    LCD_CS_HIGH();
}

static void lcd_send_data_8(uint8_t data)
{
    LCD_DC_HIGH();
    LCD_CS_LOW();
    lcd_spi_send(&data, 1);
    LCD_CS_HIGH();
}

static void lcd_send_data_16(uint16_t data)
{
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    LCD_DC_HIGH();
    LCD_CS_LOW();
    lcd_spi_send(buf, 2);
    LCD_CS_HIGH();
}

static void lcd_reset(void)
{
    LCD_RST_HIGH();
    lcd_delay_ms(50);
    LCD_RST_LOW();
    lcd_delay_ms(50);
    LCD_RST_HIGH();
    lcd_delay_ms(120);
}

static void lcd_init_reg(void)
{
    lcd_send_command(0xFE);
    lcd_send_command(0xEF);

    lcd_send_command(0xEB);
    lcd_send_data_8(0x14);

    lcd_send_command(0x84);
    lcd_send_data_8(0x60);

    lcd_send_command(0x85);
    lcd_send_data_8(0xF1);

    lcd_send_command(0x86);
    lcd_send_data_8(0xFF);

    lcd_send_command(0x87);
    lcd_send_data_8(0x28);

    lcd_send_command(0x88);
    lcd_send_data_8(0x0A);

    lcd_send_command(0x89);
    lcd_send_data_8(0x21);

    lcd_send_command(0x8A);
    lcd_send_data_8(0x00);

    lcd_send_command(0x8B);
    lcd_send_data_8(0x80);

    lcd_send_command(0x8C);
    lcd_send_data_8(0x01);

    lcd_send_command(0x8D);
    lcd_send_data_8(0x03);

    lcd_send_command(0x8E);
    lcd_send_data_8(0xDF);

    lcd_send_command(0x8F);
    lcd_send_data_8(0x52);

    lcd_send_command(0xB6);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x00);

    lcd_send_command(0x36);
    lcd_send_data_8(0x48);

    lcd_send_command(0x3A);
    lcd_send_data_8(0x05);

    lcd_send_command(0x90);
    lcd_send_data_8(0x08);
    lcd_send_data_8(0x08);
    lcd_send_data_8(0x08);
    lcd_send_data_8(0x08);

    lcd_send_command(0xBD);
    lcd_send_data_8(0x06);

    lcd_send_command(0xA6);
    lcd_send_data_8(0x74);

    lcd_send_command(0xBF);
    lcd_send_data_8(0x1C);

    lcd_send_command(0xA7);
    lcd_send_data_8(0x45);

    lcd_send_command(0xA9);
    lcd_send_data_8(0xBB);

    lcd_send_command(0xB8);
    lcd_send_data_8(0x63);

    lcd_send_command(0xBC);
    lcd_send_data_8(0x00);

    lcd_send_command(0xFF);
    lcd_send_data_8(0x60);
    lcd_send_data_8(0x01);
    lcd_send_data_8(0x04);

    lcd_send_command(0xC0);
    lcd_send_data_8(0x0E);

    lcd_send_command(0xC3);
    lcd_send_data_8(0x18);

    lcd_send_command(0xC4);
    lcd_send_data_8(0x18);

    lcd_send_command(0xC9);
    lcd_send_data_8(0x3F);

    lcd_send_command(0xBE);
    lcd_send_data_8(0x11);

    lcd_send_command(0xE1);
    lcd_send_data_8(0x10);
    lcd_send_data_8(0x0E);

    lcd_send_command(0xDF);
    lcd_send_data_8(0x21);
    lcd_send_data_8(0x10);
    lcd_send_data_8(0x02);

    lcd_send_command(0xF0);
    lcd_send_data_8(0x4C);
    lcd_send_data_8(0x10);
    lcd_send_data_8(0x09);
    lcd_send_data_8(0x09);
    lcd_send_data_8(0x86);
    lcd_send_data_8(0x32);

    lcd_send_command(0xF1);
    lcd_send_data_8(0x48);
    lcd_send_data_8(0x75);
    lcd_send_data_8(0x95);
    lcd_send_data_8(0x2E);
    lcd_send_data_8(0x34);
    lcd_send_data_8(0x8F);

    lcd_send_command(0xF2);
    lcd_send_data_8(0x4C);
    lcd_send_data_8(0x10);
    lcd_send_data_8(0x09);
    lcd_send_data_8(0x09);
    lcd_send_data_8(0x86);
    lcd_send_data_8(0x32);

    lcd_send_command(0xF3);
    lcd_send_data_8(0x48);
    lcd_send_data_8(0x75);
    lcd_send_data_8(0x95);
    lcd_send_data_8(0x2E);
    lcd_send_data_8(0x34);
    lcd_send_data_8(0x8F);

    lcd_send_command(0xED);
    lcd_send_data_8(0x1B);
    lcd_send_data_8(0x0B);

    lcd_send_command(0xAC);
    lcd_send_data_8(0x47);

    lcd_send_command(0xAE);
    lcd_send_data_8(0x77);

    lcd_send_command(0xCB);
    lcd_send_data_8(0x02);

    lcd_send_command(0xCD);
    lcd_send_data_8(0x63);

    lcd_send_command(0x70);
    lcd_send_data_8(0x07);
    lcd_send_data_8(0x07);
    lcd_send_data_8(0x04);
    lcd_send_data_8(0x0E);
    lcd_send_data_8(0x0F);
    lcd_send_data_8(0x09);
    lcd_send_data_8(0x07);
    lcd_send_data_8(0x08);
    lcd_send_data_8(0x03);

    lcd_send_command(0xE8);
    lcd_send_data_8(0x34);

    lcd_send_command(0x62);
    lcd_send_data_8(0x18);
    lcd_send_data_8(0x0D);
    lcd_send_data_8(0x71);
    lcd_send_data_8(0xED);
    lcd_send_data_8(0x70);
    lcd_send_data_8(0x70);
    lcd_send_data_8(0x18);
    lcd_send_data_8(0x0F);
    lcd_send_data_8(0x71);
    lcd_send_data_8(0xEF);
    lcd_send_data_8(0x70);
    lcd_send_data_8(0x70);

    lcd_send_command(0x63);
    lcd_send_data_8(0x18);
    lcd_send_data_8(0x11);
    lcd_send_data_8(0x71);
    lcd_send_data_8(0xF1);
    lcd_send_data_8(0x70);
    lcd_send_data_8(0x70);
    lcd_send_data_8(0x18);
    lcd_send_data_8(0x13);
    lcd_send_data_8(0x71);
    lcd_send_data_8(0xF3);
    lcd_send_data_8(0x70);
    lcd_send_data_8(0x70);

    lcd_send_command(0x64);
    lcd_send_data_8(0x3B);
    lcd_send_data_8(0x29);
    lcd_send_data_8(0xF1);
    lcd_send_data_8(0x01);
    lcd_send_data_8(0xF1);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x0A);

    lcd_send_command(0x66);
    lcd_send_data_8(0x3C);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0xCD);
    lcd_send_data_8(0x67);
    lcd_send_data_8(0x45);
    lcd_send_data_8(0x45);
    lcd_send_data_8(0x10);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x00);

    lcd_send_command(0x67);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x3C);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x01);
    lcd_send_data_8(0x54);
    lcd_send_data_8(0x10);
    lcd_send_data_8(0x32);
    lcd_send_data_8(0x98);

    lcd_send_command(0x74);
    lcd_send_data_8(0x10);
    lcd_send_data_8(0x69);
    lcd_send_data_8(0x80);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x00);
    lcd_send_data_8(0x4E);
    lcd_send_data_8(0x00);

    lcd_send_command(0x35);
    lcd_send_command(0x21);
    lcd_delay_ms(120);

    lcd_send_command(0x11);
    lcd_delay_ms(120);

    lcd_send_command(0x29);
    lcd_delay_ms(120);

    lcd_send_command(0x2C);
}

void lcd_set_windows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    lcd_send_command(0x2A);
    lcd_send_data_8(Xstart >> 8);
    lcd_send_data_8(Xstart & 0xFF);
    lcd_send_data_8(Xend >> 8);
    lcd_send_data_8(Xend & 0xFF);

    lcd_send_command(0x2B);
    lcd_send_data_8(Ystart >> 8);
    lcd_send_data_8(Ystart & 0xFF);
    lcd_send_data_8(Yend >> 8);
    lcd_send_data_8(Yend & 0xFF);

    lcd_send_command(0x2C);
}

void lcd_init(void)
{
    lcd_gpio_init();
    lcd_spi_init();
    lcd_reset();
    lcd_init_reg();
    lcd_clear(0x0000);
}

static uint8_t lcd_line_buf[LCD_WIDTH * 2];

void lcd_clear(uint16_t color)
{
    uint16_t i, j;
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    for (j = 0; j < LCD_WIDTH; j++) {
        lcd_line_buf[j * 2]     = hi;
        lcd_line_buf[j * 2 + 1] = lo;
    }

    lcd_set_windows(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    LCD_DC_HIGH();
    LCD_CS_LOW();

    for (i = 0; i < LCD_HEIGHT; i++) {
        lcd_spi_send(lcd_line_buf, LCD_WIDTH * 2);
    }

    LCD_CS_HIGH();
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_windows(x, y, x, y);
    lcd_send_data_16(color);
}

void lcd_fill_rect(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color)
{
    uint32_t len = (uint32_t)(Xend - Xstart) * (Yend - Ystart);

    lcd_set_windows(Xstart, Ystart, Xend - 1, Yend - 1);
    LCD_DC_HIGH();
    LCD_CS_LOW();
    lcd_spi_send((const uint8_t *)color, len * 2);
    LCD_CS_HIGH();
}

void lcd_display(uint16_t *image)
{
    uint32_t total = (uint32_t)LCD_WIDTH * LCD_HEIGHT;

    lcd_set_windows(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    LCD_DC_HIGH();
    LCD_CS_LOW();
    lcd_spi_send((const uint8_t *)image, total * 2);
    LCD_CS_HIGH();
}

void lcd_set_backlight(uint8_t value)
{
    (void)value;
}

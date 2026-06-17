
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
#include <dw_spi_ll.h>      /* dw_spi_get_status(), DW_SPI_SR_* — for the CS-safe idle barrier */

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
#else
    /*
     * Put the DW SPI core into TRANSMIT-ONLY mode, once, here.
     *
     * The LCD link is strictly write-only (MISO is not used). csi_spi_init()
     * leaves the core in the default TX+RX mode, where every byte clocked out
     * also clocks a byte INTO the RX FIFO. Since our flush path never drains
     * RX, on any transfer longer than the 32-deep RX FIFO it OVERFLOWS, which
     * makes the SR status bits (BUSY/TFE) — the very bits lcd_spi_wait_idle()
     * relies on — behave unreliably, and leaves the driver's readable flag in a
     * state that can make a later csi_spi_send() bail out as CSI_BUSY. TX-only
     * mode never fills RX, so the status bits stay clean for the whole frame.
     *
     * TMOD lives in CTRLR0 and the DW core only accepts CTRLR0 writes while it
     * is DISABLED, so we bracket the change with disable/enable. This runs once
     * at init; the hot path (lcd_spi_send) never touches the mode again.
     */
    {
        dw_spi_regs_t *spi_base = (dw_spi_regs_t *)HANDLE_REG_BASE((&lcd_spi));
        dw_spi_disable(spi_base);
        dw_spi_set_tx_mode(spi_base);
        dw_spi_enable(spi_base);
    }
#endif
}

static inline void lcd_spi_wait_idle(void);

static void lcd_spi_send(const uint8_t *data, uint32_t size)
{
#if LCD_SPI_DMA_EN
    uint32_t timestart;
    lcd_spi_event = -1;
    csi_spi_send_async(&lcd_spi, data, size);
    timestart = csi_tick_get_ms();
    while (lcd_spi_event != SPI_EVENT_SEND_COMPLETE) {
        if ((csi_tick_get_ms() - timestart) > LCD_DMA_TIMEOUT) {
            printf("[LCD] DMA timeout! event=%d size=%u\n", lcd_spi_event, (unsigned int)size);
            break;
        }
    }
#else
    /*
     * Dedicated, transmit-only PIO sender for the LCD. This deliberately does
     * NOT use csi_spi_send(), which is unfit for streaming a long, single-
     * direction, externally-CS-gated pixel buffer because it:
     *   (1) runs in TX+RX mode and never drains RX  -> RX FIFO overflow corrupts
     *       the SR status bits we depend on;
     *   (2) does disable -> reconfigure -> enable on EVERY call, so a buffer too
     *       big for one pass forces a 2nd call that re-inits the core MID-STREAM
     *       while CS is still low -> the pixel stream is cut, every following
     *       byte shifts, and (because the RGB565 high byte carries red) the
     *       displaced region tints red -> the reddish blocks, size varying with
     *       where the break landed;
     *   (3) gates entry on its readable/writeable flags, so a previous transfer
     *       that left RX dirty can make it return CSI_BUSY and stall the flush.
     *
     * Here the core was already configured ONCE (master, 8-bit, TX-only, enabled
     * in lcd_spi_init) and stays untouched.
     *
     * We do NOT compute "free slots" from the TX FIFO occupancy counter. That
     * counter (TXFLR) is FIFO-depth-width dependent: when the FIFO is full some
     * DW SSI instances report the depth value, and "depth - level" can then
     * UNDERFLOW to a huge number, causing a burst far larger than the FIFO that
     * silently drops every write past slot 32 -> massive pixel loss and worse
     * color blocks. Instead we push one byte at a time gated ONLY on the
     * SR.TFNF (TX FIFO Not Full) status bit, the canonical DW SPI flow-control
     * flag. While TFNF is set there is guaranteed room for at least one byte, so
     * a write can never be dropped, regardless of FIFO depth or instance quirks.
     */
    dw_spi_regs_t *spi_base = (dw_spi_regs_t *)HANDLE_REG_BASE((&lcd_spi));
    uint32_t remaining = size;

    while (remaining > 0U) {
        /* Block until the TX FIFO has room for at least one byte. No timeout:
         * in TX-only mode the core always drains at SCLK, so TFNF always comes
         * back, and an early bail-out is exactly the bug we removed. */
        while (!(dw_spi_get_status(spi_base) & DW_SPI_SR_TFNF)) {
            ;
        }
        dw_spi_transmit_data(spi_base, (uint32_t)(*data));
        data++;
        remaining--;
    }

    /*
     * Guarantee the last bit is physically off the wire before we return, so
     * the caller can safely toggle DC (mid-sequence in lcd_set_windows) or
     * raise CS without truncating the transfer. See lcd_spi_wait_idle().
     */
    lcd_spi_wait_idle();
#endif
}

/*
 * Wait until the SPI controller has truly finished shifting the last bit out
 * before the caller is allowed to raise CS.
 *
 * Why this is needed: csi_spi_send() ends with a "while (SR.BUSY)" loop, but
 * its own comment admits "SR.BUSY has some delay before be valid". On the DW
 * SPI core, BUSY is deasserted (and only asserts a few cycles after the FIFO
 * starts draining), so that loop can fall through BEFORE the final byte has
 * left the shift register. If we raise CS at that instant we cut the transfer
 * short. When the truncated transfer is a window command/coordinate in
 * lcd_set_windows(), the GC9A01 ends up with a WRONG address window and the
 * next pixel block lands in the wrong place -> a large color block at a random
 * position. This is rare normally but gets much more likely under load (fast
 * taps), which matches the observed symptom exactly.
 *
 * The barrier: poll SR directly and require TFE (TX FIFO empty) AND !BUSY to
 * hold for several consecutive reads. Demanding a run of stable idle reads
 * filters out the "BUSY not asserted yet" window, so we never raise CS mid-bit.
 */
static inline void lcd_spi_wait_idle(void)
{
    dw_spi_regs_t *spi_base = (dw_spi_regs_t *)HANDLE_REG_BASE((&lcd_spi));
    const uint32_t idle_mask = DW_SPI_SR_TFE | DW_SPI_SR_BUSY;
    int stable = 0;

    /* Need TFE=1 and BUSY=0 -> (SR & idle_mask) == DW_SPI_SR_TFE. Require it to
     * stay that way for 8 back-to-back reads to ride out the BUSY assert delay. */
    while (stable < 8) {
        if ((dw_spi_get_status(spi_base) & idle_mask) == DW_SPI_SR_TFE) {
            stable++;
        } else {
            stable = 0;
        }
    }
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
    uint8_t buf[4];

    /* CS stays low for the whole window setup. DC toggles only between the
     * command byte and its 4 data bytes. This collapses 11 single-byte SPI
     * transactions + ~22 CS toggles into 5 transfers + 1 CS pair. */
    LCD_CS_LOW();

    /* Column address set (0x2A) */
    LCD_DC_LOW();
    buf[0] = 0x2A;
    lcd_spi_send(buf, 1);
    LCD_DC_HIGH();
    buf[0] = Xstart >> 8;
    buf[1] = Xstart & 0xFF;
    buf[2] = Xend >> 8;
    buf[3] = Xend & 0xFF;
    lcd_spi_send(buf, 4);

    /* Row address set (0x2B) */
    LCD_DC_LOW();
    buf[0] = 0x2B;
    lcd_spi_send(buf, 1);
    LCD_DC_HIGH();
    buf[0] = Ystart >> 8;
    buf[1] = Ystart & 0xFF;
    buf[2] = Yend >> 8;
    buf[3] = Yend & 0xFF;
    lcd_spi_send(buf, 4);

    /* Memory write (0x2C) */
    LCD_DC_LOW();
    buf[0] = 0x2C;
    lcd_spi_send(buf, 1);

    LCD_CS_HIGH();
}

void lcd_init(void)
{
    lcd_gpio_init();
    lcd_spi_init();
    lcd_reset();
    lcd_init_reg();
    lcd_clear(0x0000);
}

void lcd_clear(uint16_t color)
{
    uint16_t i, j;
    uint8_t data[2];
    data[0] = color >> 8;
    data[1] = color & 0xFF;

    lcd_set_windows(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    LCD_DC_HIGH();
    LCD_CS_LOW();

#if LCD_SPI_DMA_EN
    uint8_t line_buf[LCD_WIDTH * 2];
    for (j = 0; j < LCD_WIDTH; j++) {
        line_buf[j * 2] = data[0];
        line_buf[j * 2 + 1] = data[1];
    }
    for (i = 0; i < LCD_HEIGHT; i++) {
        lcd_spi_send(line_buf, LCD_WIDTH * 2);
    }
#else
    for (i = 0; i < LCD_HEIGHT; i++) {
        for (j = 0; j < LCD_WIDTH; j++) {
            lcd_spi_send(data, 2);
        }
    }
#endif

    LCD_CS_HIGH();
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_set_windows(x, y, x, y);
    lcd_send_data_16(color);
}

void lcd_fill_rect(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color)
{
    /*
     * Xend/Yend are EXCLUSIVE here: my_flush_cb() passes (area->x2 + 1,
     * area->y2 + 1). During fast taps / rapid screen switches LVGL can hand us
     * degenerate or slightly out-of-range areas. If we don't guard them:
     *   - a zero-width/height area makes "Xend - 1" underflow (uint16 wraps to
     *     0xFFFF), so lcd_set_windows() programs a window spanning the whole
     *     panel -> any following write lands as a huge color block;
     *   - an out-of-range Xend/Yend programs an illegal GC9A01 window, so the
     *     pixel stream is written to the wrong GRAM location -> random blocks.
     * Clamp to the panel, reject empty areas, and derive BOTH the window and
     * the byte count from the same w/h so they can never disagree.
     */
    if (Xend > LCD_WIDTH)  Xend = LCD_WIDTH;
    if (Yend > LCD_HEIGHT) Yend = LCD_HEIGHT;
    if (Xstart >= Xend || Ystart >= Yend) {
        return;                 /* nothing to draw */
    }

    uint32_t w   = (uint32_t)(Xend - Xstart);
    uint32_t h   = (uint32_t)(Yend - Ystart);
    uint32_t len = w * h;

    lcd_set_windows(Xstart, Ystart, (uint16_t)(Xend - 1), (uint16_t)(Yend - 1));
    LCD_DC_HIGH();
    LCD_CS_LOW();

    /*
     * The LVGL display is configured with LV_COLOR_FORMAT_RGB565_SWAPPED, so
     * px_map already holds the bytes in the order the GC9A01 expects. Send the
     * whole rectangle in ONE SPI transfer instead of one call per pixel: this
     * removes ~57600 csi_spi_send() calls / CS toggles / byte-splits per full
     * refresh, which is the dominant cost in the PIO path.
     */
    lcd_spi_send((const uint8_t *)color, len * 2);

    LCD_CS_HIGH();
}

void lcd_display(uint16_t *image)
{
    uint16_t i, j;
    lcd_set_windows(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    LCD_DC_HIGH();
    LCD_CS_LOW();

#if LCD_SPI_DMA_EN
    for (i = 0; i < LCD_HEIGHT; i++) {
        lcd_spi_send((const uint8_t *)&image[i * LCD_WIDTH], LCD_WIDTH * 2);
    }
#else
    uint8_t buf[2];
    for (i = 0; i < LCD_HEIGHT; i++) {
        for (j = 0; j < LCD_WIDTH; j++) {
            buf[0] = image[i * LCD_WIDTH + j] >> 8;
            buf[1] = image[i * LCD_WIDTH + j] & 0xFF;
            lcd_spi_send(buf, 2);
        }
    }
#endif

    LCD_CS_HIGH();
}

void lcd_set_backlight(uint8_t value)
{
    (void)value;
}



/******************************************************************************
 * @file     main.c
 * @brief    hello world
 * @version  V1.0
 * @date     03. April 2020
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <drv/uart.h>
#include "board_config.h"
#include "board_init.h"

#include <soc.h>
#include <drv/spi.h>
#include <drv/pin.h>
#include <dw_spi_ll.h>
#include "lcd.h"

volatile csi_spi_event_t spi_master_event = 7;
csi_spi_t spi;
static csi_gpio_t gpio_cs;
static csi_gpio_t gpio_dc;
static csi_gpio_t gpio_led;
static csi_gpio_t gpio_rst;
static csi_dma_ch_t dma_tx_ch;

static uint8_t img_buf_a[2048] __attribute__((aligned(32)));;
static uint8_t img_buf_b[2048] __attribute__((aligned(32)));;
static void example_pin_init(void)
{
    csi_pin_set_mux(PA23, PA23_SPI0_CS);
    csi_pin_set_mux(PA24, PA24_SPI0_SCK);
    csi_pin_set_mux(PA25, PA25_SPI0_MOSI);
    csi_pin_set_mux(PA26, PA26_SPI0_MISO);
    csi_pin_mode(PA23, GPIO_MODE_PULLUP);
    csi_pin_mode(PA24, GPIO_MODE_PULLUP);
    csi_pin_mode(PA25, GPIO_MODE_PULLUP);
    csi_pin_mode(PA26, GPIO_MODE_PULLUP);
}

static void soft_cs_pin_init(void)
{
    csi_pin_set_mux(PA15, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio_cs, 0);
    csi_gpio_write(&gpio_cs, (1 << 15), 1);
    csi_gpio_dir(&gpio_cs, (1 << 15), GPIO_DIRECTION_OUTPUT);

    csi_pin_set_mux(PA6, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio_dc, 0);
    csi_gpio_dir(&gpio_dc, (1 << 6), GPIO_DIRECTION_OUTPUT);

    csi_pin_set_mux(PA8, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio_led, 0);
    csi_gpio_dir(&gpio_led, (1 << 8), GPIO_DIRECTION_OUTPUT);

    csi_pin_set_mux(PA7, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio_rst, 0);
    csi_gpio_dir(&gpio_rst, (1 << 7), GPIO_DIRECTION_OUTPUT);
}

static void master_spi_event_cb(csi_spi_t *spi, csi_spi_event_t event, void *arg)
{
    spi_master_event = event;
}

static int spi_init(void)
{
    int ret;

    example_pin_init();
    soft_cs_pin_init();

    ret = csi_spi_init(&spi, 0);
    if (ret != CSI_OK)
    {
        return -1;
    }
    ret = csi_spi_mode(&spi, SPI_MASTER);

    if (ret != CSI_OK)
    {
        return -2;
    }

    ret = csi_spi_cp_format(&spi, SPI_FORMAT_CPOL1_CPHA1);
    if (ret != CSI_OK)
    {
        return -4;
    }

    ret = csi_spi_attach_callback(&spi, master_spi_event_cb, NULL);
	if (ret != CSI_OK) {
		return -4;
	}
    ret = csi_spi_frame_len(&spi, SPI_FRAME_LEN_8);
    if (ret != CSI_OK) {
        return -5;
    }

    ret = csi_spi_baud(&spi, 50 * 1000 * 1000);
    if (ret == 0)
    {
        return -6;
    }
	
	ret = csi_spi_link_dma(&spi, &dma_tx_ch, NULL);
	if (ret != CSI_OK) {
		return -7;
	}

    dw_spi_disable(DW_SPI0_BASE);
    dw_spi_set_tx_mode(DW_SPI0_BASE);
    dw_spi_enable(DW_SPI0_BASE);
    csi_spi_select_slave(&spi, 0);
    return 0;
}

int ioctl(uint32_t value)
{
    csi_gpio_write(&gpio_cs, (1 << 15), value);
    return 0;
}

int dc_ioctl(uint32_t value)
{
    csi_gpio_write(&gpio_dc, (1 << 6), value);
    return 0;
}

int led_ioctl(uint32_t value)
{
    csi_gpio_write(&gpio_led, (1 << 8), value);
    return 0;
}

int rst_ioctl(uint32_t value)
{
    csi_gpio_write(&gpio_rst, (1 << 7), value);
    return 0;
}

//__ASM(".global bmp_start, bmp_end; "
//      "bmp_start: "
//      ".incbin \"src/NStar.bin\";"
//      "bmp_end: ");
// extern uint32_t bmp_start, bmp_end;
#include <drv/timer.h>
int test_count = 0;
int test_count_old = 0;
csi_timer_t test_timer;
static void tick_event_cb(csi_timer_t *timer_handle, void *arg)
{
    printf("test count is %d\r\n", test_count - test_count_old);
    test_count_old = test_count;
}

void timer_test(void)
{
    csi_timer_init(&test_timer, 2);
    csi_timer_attach_callback(&test_timer, tick_event_cb, NULL);
    csi_timer_start(&test_timer, (1000000U / 1));
}

int main(void)
{
    int ret;
    board_init();

    printf("Hello World!\n");

    ret = spi_init();
    printf("spi_init ret: %d\n", ret);
    LCD_Init();

    mdelay(2000);

    LCD_SetWindows(0, 0, 480 - 1, 320 - 1); // 设置显示窗口
    uint8_t *img_ptr = (uint16_t *)0x18008000;
	uint8_t *img_buf = NULL;
	uint32_t imgsize = 2048;
	uint32_t imagesize_tt = 460800;
	int i = 0;
	timer_test();
	while (1) {
        LCD_CS_CLR;
        LCD_RS_SET;
        uint32_t count = imagesize_tt/imgsize;
        uint32_t pad = imagesize_tt % imgsize;

        img_ptr = (uint16_t *)0x18008000;
        
        memcpy(img_buf_a, img_ptr, imgsize);
        img_ptr += imgsize;
        img_buf = img_buf_a;
        for (i = 0; i < count; i++) {
        ret = csi_spi_send_async(&spi, img_buf, imgsize);
        if (ret != CSI_OK) {
            return -1;
        }

        if (i % 2) {
            memcpy(img_buf_b, img_ptr, imgsize);
            img_buf = img_buf_b;
        } else {
            memcpy(img_buf_a, img_ptr, imgsize);
            img_buf = img_buf_a;
        }
        img_ptr += imgsize;
        while(spi_master_event == 7);
        spi_master_event = 7;
        }
        if (i % 2) {
            memcpy(img_buf_b, img_ptr, pad);
            img_buf = img_buf_b;
        } else {
            memcpy(img_buf_a, img_ptr, pad);
            img_buf = img_buf_a;
        }
        ret = csi_spi_send_async(&spi, img_buf, pad);

        if (ret != CSI_OK) {
            return -1;
        }
        while(spi_master_event == 7);
        spi_master_event = 7;
        test_count++;
        LCD_CS_SET;
	}
    return 0;
}
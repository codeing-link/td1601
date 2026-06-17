

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
#include <drv/ospi.h>
#include <drv/pin.h>

#include "LCD.h"

csi_ospi_t ospi;
static csi_gpio_t gpio_cs;
static csi_gpio_t gpio_dc;
static csi_gpio_t gpio_led;
static csi_gpio_t gpio_rst;

static void example_pin_init(void)
{
    csi_pin_set_mux(PB4, PB4_SPI1_D0);
    csi_pin_set_mux(PB5, PB5_SPI1_D1);
    csi_pin_set_mux(PB6, PB6_SPI1_D2);
    csi_pin_set_mux(PB7, PB7_SPI1_D3);
    csi_pin_set_mux(PB8, PB8_SPI1_D4);
    csi_pin_set_mux(PB9, PB9_SPI1_D5);
    csi_pin_set_mux(PB10, PB10_SPI1_D6);
    csi_pin_set_mux(PB11, PB11_SPI1_D7);
    csi_pin_set_mux(PA28, PA28_SPI1_SCK);
    // csi_pin_set_mux(PA2, PA2_SPI1_CS);
    // csi_pin_set_mux(PA27, PA27_SPI1_CS);
    csi_pin_mode(PB4, GPIO_MODE_PULLUP);
    csi_pin_mode(PB5, GPIO_MODE_PULLUP);
    csi_pin_mode(PB6, GPIO_MODE_PULLUP);
    csi_pin_mode(PB7, GPIO_MODE_PULLUP);
    csi_pin_mode(PB8, GPIO_MODE_PULLUP);
    csi_pin_mode(PB9, GPIO_MODE_PULLUP);
    csi_pin_mode(PB10, GPIO_MODE_PULLUP);
    csi_pin_mode(PB11, GPIO_MODE_PULLUP);
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

static int spi_init(void)
{
    int ret;

    example_pin_init();
    soft_cs_pin_init();

    ret = csi_ospi_init(&ospi, 0);
    if (ret != CSI_OK)
    {
        return -1;
    }

    ret = csi_ospi_transfer_mode(&ospi, OSPI_TRANSFER_SEND_ONLY);
    if (ret != CSI_OK) {
        return -2;
    }

    ret = csi_ospi_line_mode(&ospi, OSPI_LINE_OCTAL);
    if (ret != CSI_OK)
    {
        return -3;
    }
    ret = csi_ospi_cp_format(&ospi, OSPI_FORMAT_CPOL1_CPHA1);
    if (ret != CSI_OK)
    {
        return -4;
    }

    ret = csi_ospi_frame_len(&ospi, OSPI_FRAME_LEN_8);
    if (ret != CSI_OK) {
        return -5;
    }

    ret = csi_ospi_baud(&ospi, 30 * 1000 * 1000);
    if (ret == 0)
    {
        return -6;
    }

    csi_ospi_select_slave(&ospi, 0);
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

__ASM(".global bmp_start1, bmp_end1; "
      "bmp_start1: "
      ".incbin \"Nstar-8080.bin\";"
      "bmp_end1: ");

__ASM(".global bmp_start, bmp_end; "
      "bmp_start: "
      ".incbin \"Nstar-80802.bin\";"
      "bmp_end: ");
extern uint32_t bmp_start, bmp_end;
extern uint32_t bmp_start1, bmp_end1;
uint8_t picture_buff[30 * 3];
uint16_t pixel_buff[30] = {0};

// 彩条显色
void DispBand(void)
{
    unsigned int i, j, k;
    unsigned int color[8] = {BLUE, GREEN, RED, GBLUE, BRED, YELLOW, BLACK, WHITE};
    LCD_Set_Window(0, 0, lcddev.width, lcddev.height);
    LCD_WriteRAM_Prepare(); // 开始写入GRAM

    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < lcddev.height / 8; j++)
        {
            for (k = 0; k < lcddev.width; k++)
            {

                LCD_WriteRAM(color[i]);
            }
        }
    }
    for (j = 0; j < lcddev.height % 8; j++)
    {
        for (k = 0; k < lcddev.width; k++)
        {

            LCD_WriteRAM(color[7]);
        }
    }

    mdelay(1000);
}
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
    LCD_Display_Dir(USE_LCM_DIR); // 屏幕方向
    LCD_Clear(WHITE);             // 清屏
    DispBand();
    timer_test();
    uint16_t *img_ptr = (uint16_t *)&bmp_start;
    uint16_t *img_ptr1 = (uint16_t *)&bmp_start1;
     for (test_count = 0 ; test_count < 0xFFFF; test_count++)
    {
        // LCD_Fill2(0, 0, 128 - 1, 160 - 1, test_count);

        LCD_Fill3(0, 0, 160 - 1, 128 - 1, img_ptr);
        mdelay(100);
        LCD_Fill3(0, 0, 160 - 1, 128 - 1, img_ptr1);
        mdelay(100);
        // LCD_WR_DATA(img_ptr[test_count]>>8);		//���͸�8Ϊ
		// LCD_WR_DATA(img_ptr[test_count]);

        // LCD_Color_Fill(0, 0, 128 - 1, 160 - 1, 0xFFFF);
        // mdelay(1);
        // LCD_Color_Fill(0, 0, 128 - 1, 160 - 1, 0xF800);
        // mdelay(1);
        // LCD_Color_Fill(0, 0, 128 - 1, 160 - 1, 0x07E0);
        // mdelay(1);
        // LCD_Color_Fill(0, 0, 128 - 1, 160 - 1, 0xFFE0);
        // mdelay(1);
    }
    return 0;
}

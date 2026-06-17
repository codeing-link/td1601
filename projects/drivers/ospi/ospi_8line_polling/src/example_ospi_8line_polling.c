/******************************************************************************
 * @file     example_Ospi.c
 * @brief    the main function for the OSPI driver
 * @version  V1.0
 * @date     07. 5 2026
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
#include <soc.h>
#include <drv/spi.h>
#include <drv/tick.h>
#include <csi_config.h>
#include "w25q64fv.h"

uint32_t flash_read_addr = 0x0;
uint32_t img_size_1pic = 40960;
#include <drv/ringbuffer.h>
static uint8_t total_buf[81920];
static uint8_t pic_buf[40960]; __attribute__((aligned(32)));;
static 	uint16_t recv_buf[128*160];
csi_ringbuffer_t rx_ring_buffer;
static csi_gpio_t gpio;
static csi_spi_t  spi;
static flash_t    flash;
static bindings_t binding;
static volatile uint8_t suc_num = 0;
volatile csi_spi_event_t spi_master_event;
csi_dma_ch_t dma_rx_ch[2];
static int spi_ioctl(uint32_t value);
static void example_spi_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MISO_PORT,EXAMPLE_PIN_SPI_MISO, EXAMPLE_PIN_SPI_MISO_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_MOSI_PORT,EXAMPLE_PIN_SPI_MOSI, EXAMPLE_PIN_SPI_MOSI_FUNC);
    csi_pin_set_mux(EXAMPLE_PIN_SPI_SCK_PORT,EXAMPLE_PIN_SPI_SCK,  EXAMPLE_PIN_SPI_SCK_FUNC);

}

static void uninit()
{
    csi_spi_detach_callback(&spi);
    csi_spi_uninit(&spi);
}

static void soft_spi_cs_pin_init(void)
{
    csi_pin_set_mux(EXAMPLE_PIN_SPI_CS_PORT,EXAMPLE_PIN_SPI_CS,   EXAMPLE_PIN_SPI_SOFT_CS_FUNC);
    csi_gpio_init(&gpio, EXAMPLE_PIN_SPI_CS_PORT);
    csi_gpio_dir(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_DIRECTION_OUTPUT);
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, GPIO_PIN_HIGH);
}

uint32_t pic_addr = 0;
static void master_spi_event_cb(csi_spi_t *spi, csi_spi_event_t event, void *arg)
{
    spi_ioctl(1);
    spi_master_event = event;
	flash_read_addr += img_size_1pic;
    pic_addr += img_size_1pic;
	if (flash_read_addr >= 0x1d6000) {
		flash_read_addr = 0;
		return;
	}
}

static int init(void)
{
    int ret;

    example_spi_pin_init();
    soft_spi_cs_pin_init();

    ret = csi_spi_init(&spi, EXAMPLE_SPI_IDX);

    if (ret != CSI_OK) {
        return -1;
    }
    ret = csi_spi_attach_callback(&spi, master_spi_event_cb, NULL);

    if (ret != CSI_OK) {
        return -1;
    }
    ret = csi_spi_mode(&spi, SPI_MASTER);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_cp_format(&spi, SPI_FORMAT_CPOL0_CPHA0);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_frame_len(&spi, SPI_FRAME_LEN_8);

    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_spi_baud(&spi, W25Q64FV_DEFAULT_CLK_RATE);

    if (ret == 0) {
        return -1;
    }
    csi_spi_select_slave(&spi, 0);
    return 0;
}

static int spi_ioctl(uint32_t value)
{
    csi_gpio_write(&gpio, EXAMPLE_PIN_SPI_CS, value);
    return 0;
}

static int send(const void *data, uint32_t size)
{
    uint32_t ret;
    ret = csi_spi_send(&spi, data, size, 1000);

    if (size != ret) {
        return -1;
    }

    return 0;
}

static int receive(void *data, uint32_t size)
{
    int ret;

    ret = csi_spi_receive_async(&spi, data, size);
    return 0;
}

static uint32_t w25q64fv_get_page_addr_by_num(uint32_t page_num)
{
    return page_num * W25Q64FV_PAGE_SIZE;
}

static uint32_t w25q64fv_get_sec_addr_by_num(uint32_t sec_num)
{
    return sec_num * W25Q64FV_SECTOR_SIZE;

}

int example_flash_chip_erase(void)
{
    int ret;
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t addr;
    uint8_t data[W25Q64FV_PAGE_SIZE] __attribute__((aligned(32)));;

    /* erase chip */
    ret = w25q64fv_erase_chip(&flash);

    if (ret < 0) {
        printf("fail to erase chip, error code = %d!\n", ret);
        return -1;
    }

    /* check first 30 page */
    for (i = 0; i < 30; i++) {

        /* read page data */
        addr = w25q64fv_get_page_addr_by_num(i);
        ret = w25q64fv_read(&flash, addr, data, W25Q64FV_PAGE_SIZE);

        if (ret < 0) {
            printf("fail to read data, page num: %d\n", i);
            return -1;
        }

        /* compare one page data */
        for (j = 0; j < W25Q64FV_PAGE_SIZE; j++) {
            if (data[j] != 0xFF) {
                printf("check erase value fail, page num: %d, address = %08x!\n", i, i * W25Q64FV_PAGE_SIZE + j);
                return -1;
            }
        }

        printf("check erase value success, page num: %d\n", i);
    }

    return 0;
}


int example_falsh_write_read()
{
    uint32_t address, size;
    int ret;
    address = flash_read_addr;
    size    = img_size_1pic;
    ret = w25q64fv_read(&flash, address, pic_buf, size);

    return 0;
}

int exmaple_spi_test(void)
{
    int ret = 0;
    uint16_t device_id;

    board_init();

    binding.init    = init;
    binding.send    = send;
    binding.receive = receive;
    binding.ioctl   = spi_ioctl;
    binding.uninit  = uninit;
    ret = w25q64fv_init(&flash, &binding);

    if (ret < 0) {
        printf("fail to init device, error code = %d\n", ret);
    }

    /* write and read date example */
    printf("\n\n\n");
    printf("-------------------------------------\n");
    printf("example_falsh_write_read()\n");
    printf("-------------------------------------\n");
    ret = example_falsh_write_read();

    if (ret < 0) {
        printf("example_falsh_write_read() run fail\n");
        return -1;
    }

    printf("example_falsh_write_read() run success\n\n\n");

    return 0;

}
csi_ospi_t ospi;
csi_ospi_command_t cmd;
static csi_gpio_t gpio_cs;
static csi_gpio_t gpio_dc;
static csi_gpio_t gpio_led;
static csi_gpio_t gpio_rst;

static void example_pin_init(void)
{
    csi_pin_set_mux(PORTB, PIN4, PB4_SPI1_D0);
    csi_pin_set_mux(PORTB, PIN5, PB5_SPI1_D1);
    csi_pin_set_mux(PORTB, PIN6, PB6_SPI1_D2);
    csi_pin_set_mux(PORTB, PIN7, PB7_SPI1_D3);
    csi_pin_set_mux(PORTB, PIN8, PB8_SPI1_D4);
    csi_pin_set_mux(PORTB, PIN9, PB9_SPI1_D5);
    csi_pin_set_mux(PORTB, PIN10, PB10_SPI1_D6);
    csi_pin_set_mux(PORTB, PIN11, PB11_SPI1_D7);
    csi_pin_set_mux(PORTA, PIN28, PA28_SPI1_SCK);
    csi_pin_mode(PORTB, PIN4, GPIO_MODE_PULLUP);
    csi_pin_mode(PORTB, PIN5, GPIO_MODE_PULLUP);
    csi_pin_mode(PORTB, PIN6, GPIO_MODE_PULLUP);
    csi_pin_mode(PORTB, PIN7, GPIO_MODE_PULLUP);
    csi_pin_mode(PORTB, PIN8, GPIO_MODE_PULLUP);
    csi_pin_mode(PORTB, PIN9, GPIO_MODE_PULLUP);
    csi_pin_mode(PORTB, PIN10, GPIO_MODE_PULLUP);
    csi_pin_mode(PORTB, PIN11, GPIO_MODE_PULLUP);
}

static void soft_cs_pin_init(void)
{
    csi_pin_set_mux(PORTA, PIN15, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio_cs, 0);
    csi_gpio_write(&gpio_cs, (1 << 15), 1);
    csi_gpio_dir(&gpio_cs, (1 << 15), GPIO_DIRECTION_OUTPUT);

    csi_pin_set_mux(PORTA, PIN6, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio_dc, 0);
    csi_gpio_dir(&gpio_dc, (1 << 6), GPIO_DIRECTION_OUTPUT);

    csi_pin_set_mux(PORTA, PIN8, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio_led, 0);
    csi_gpio_dir(&gpio_led, (1 << 8), GPIO_DIRECTION_OUTPUT);

    csi_pin_set_mux(PORTA, PIN7, PIN_FUNC_GPIO);
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
    cmd.data.transfer_mode = OSPI_TRANSFER_SEND_ONLY;
    cmd.data.frame_len = 8;
    cmd.data.bus_width = OSPI_LINE_OCTAL;
    cmd.data.disabled = false;
    cmd.address.disabled = true;
	cmd.address.bus_width = OSPI_LINE_OCTAL;
    cmd.address.size = OSPI_ADDRESS_0_BITS;
    cmd.alt.disabled = true;
    cmd.ddr_enable = false;
    cmd.instruction.disabled = false;
	cmd.instruction.bus_width = OSPI_LINE_OCTAL;
	cmd.instruction.size = OSPI_INSTRUCTION_8_BITS;
    cmd.dummy_count = 0;
    csi_ospi_config(&ospi, &cmd);

    ret = csi_ospi_cp_format(&ospi, OSPI_FORMAT_CPOL1_CPHA1);
    if (ret != CSI_OK)
    {
        return -4;
    }

    ret = csi_ospi_baud(&ospi, 25 * 1000 * 1000);
    if (ret == 0)
    {
        return -6;
    }

    csi_ospi_select_slave(&ospi, 0);
    return ret;
}
void LCD_WR_REG_OSPI(uint16_t data)
{
	LCD_RS_CLR;
 	LCD_CS_CLR; 
#if 0

#else
    cmd.instruction.value = data;
    csi_ospi_send(&ospi, &data, 0, 1000);
#endif
 	LCD_CS_SET; 			
}

void LCD_WR_DATA_OSPI(uint16_t data)
{
	LCD_RS_SET;
	LCD_CS_CLR;
#if 0
#else
    cmd.instruction.value = data;
    csi_ospi_send(&ospi, &data, 0, 1000);
#endif
	LCD_CS_SET;		
}

void LCD_WR_DATA2_OSPI(uint16_t data)
{
#if 0
#else
    cmd.instruction.value = data;
    csi_ospi_send(&ospi, &data, 0, 1000);
#endif
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

extern uint32_t bmp_start, bmp_end;
extern uint32_t bmp_start1, bmp_end1;

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
int rrert;
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

extern void LCD_Fill3(uint16_t sx,uint16_t sy,uint16_t ex,uint16_t ey,uint16_t *color);
int main(void)
{
    int ret;
    board_init();
    rx_ring_buffer.buffer = total_buf;
    rx_ring_buffer.size = sizeof(total_buf);
    csi_ringbuffer_reset(&rx_ring_buffer);
    printf("Hello World!\n");
	exmaple_spi_test();
    ret = spi_init();
    LCD_WR_REG = LCD_WR_REG_OSPI;
    LCD_WR_DATA = LCD_WR_DATA_OSPI;
    LCD_WR_DATA2 = LCD_WR_DATA2_OSPI;

    printf("spi_init ret: %d\n", ret);

    LCD_Init();
    LCD_Display_Dir(USE_LCM_DIR); // 屏幕方向
    LCD_Clear(WHITE);             // 清屏
    DispBand();
	uint32_t read_size = 0;
    int ret_size = 0;
	while(1) {
        while(spi_master_event != SPI_EVENT_RECEIVE_COMPLETE);
        spi_master_event = -1;
		LCD_Fill3(0, 0, 128 - 1, 160 - 1, (uint16_t *)pic_buf);
        rrert = w25q64fv_read(&flash, flash_read_addr, (uint8_t *)&pic_buf, img_size_1pic);
        printf("ret:%d",rrert);
	}

    return 0;
}

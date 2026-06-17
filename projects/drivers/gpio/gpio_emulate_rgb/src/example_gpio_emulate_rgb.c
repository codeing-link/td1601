

/******************************************************************************
 * @file     example_etb.c
 * @brief    the main function for the etb driver
 * @version  V2.0
 * @date     26. March 2020
 ******************************************************************************/
#include <stdio.h>
#include <soc.h>
#include <drv/edma.h>
#include <drv/etb.h>
#include <drv/gpio.h>
#include <drv/timer.h>
#include <drv/pin.h>
#include <board_init.h>
#include <board_config.h>


#define TEST_TIM0   2
extern const uint16_t timer_hs_num[]; 

static csi_timer_t g_timer_handle;
static csi_gpio_t gpio;
static csi_edma_ch_t edma_ch;

static uint32_t *tx_data = (uint32_t *)CAN_BUF0_BASE_ADDR;

static volatile uint8_t dma_cplt_flag = 0;

void wj_edma_event_cb(csi_edma_ch_t *dma, csi_edma_event_t event, void *arg)
{
    (void)dma, (void)arg;
    if (event == EDMA_EVENT_TRANSFER_DONE) {
        dma_cplt_flag = 1;
        printf("cplt\n");
        // csi_edma_ch_stop(dma);
    } else if (event == EDMA_EVENT_TRANSFER_HALF_DONE) {
        // printf("half cplt\n");
    }
}

static int32_t tim_config(void)
{
    int32_t ret = 0;
    int32_t state;

    state = csi_timer_init(&g_timer_handle, (uint32_t)TEST_TIM0);
    if (state < 0) {
        printf("csi_timer_init error\n");
        ret = -1;
    }

    return ret;
}

static int32_t edma_config(void)
{
    int32_t ret = 0;
    csi_edma_ch_config_t config = {0};

    ret = csi_edma_ch_alloc(&edma_ch, -1, -1);
    if (ret == CSI_OK) {
        csi_edma_ch_attach_callback(&edma_ch, wj_edma_event_cb, NULL);
    }
    /* configure dma channel */
    config.src_inc = EDMA_ADDR_INC;
    config.dst_inc = EDMA_ADDR_CONSTANT;
    config.src_tw  = EDMA_DATA_WIDTH_32_BITS;
    config.dst_tw  = EDMA_DATA_WIDTH_16_BITS;

    config.group_len = 2U;
    config.trans_dir = EDMA_MEM2PERH;
    config.handshake = 82U;

    csi_edma_ch_config(&edma_ch, &config);
	
	return ret;
}

static void gpio_config(void)
{
    csi_pin_set_mux(PORTB, PIN11, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio, 1);
    csi_gpio_write(&gpio, PIN11,  GPIO_PIN_LOW);
    csi_gpio_dir(&gpio, PIN11, GPIO_DIRECTION_OUTPUT);
}

/**
 * @brief 将 RGB 颜色值转换为兼容底层 16 位物理内存的字操作时序数据
 *
 * @param data_buf 目标缓冲区基地址 (uint32_t 类型)
 * @param rgb 24-bit RGB 颜色值
 * @param index 像素索引
 */
void set_rgb_pixel(uint32_t *data_buf, uint32_t rgb, uint16_t index)
{
    uint32_t *p = data_buf;
    
    // 1 个像素 = 24 bit = 72 个 uint16_t = 36 个 uint32_t
    p += index * 36; 

    // 每次循环处理 2 个 bit，打包成 3 个 Word
    for (size_t i = 0; i < 24; i += 2)
    {
        // 提取当前 bit (偶数位) 和 下一个 bit (奇数位)
        uint8_t bit_curr = (rgb >> (23 - i)) & 0x01;
        uint8_t bit_next = (rgb >> (22 - i)) & 0x01;

        // Word 0: 覆盖 [当前 bit 的 H1] 和 [当前 bit 的 H2]
        // H1 固定为高电平(0x0800)，H2 取决于当前 bit
        uint32_t word0 = 0x00000800; 
        if (bit_curr) {
            word0 |= 0x08000000; 
        }
        *p++ = word0;

        // Word 1: 覆盖 [当前 bit 的 H3] 和 [下一个 bit 的 H1]
        // 当前 bit 的 H3 固定为低电平(0x0000)
        // 下一个 bit 的 H1 固定为高电平(0x0800)
        uint32_t word1 = 0x08000000; // 高16位固定为 0x0800
        *p++ = word1;

        // Word 2: 覆盖 [下一个 bit 的 H2] 和 [下一个 bit 的 H3]
        // H2 取决于下一个 bit，H3 固定为低电平(0x0000)
        uint32_t word2 = 0x00000000;
        if (bit_next) {
            word2 |= 0x00000800; 
        }
        *p++ = word2;
    }
}

void set_rgb_strip(uint32_t *led_data, uint16_t led_len)
{
    *(volatile uint32_t *)0x4000051c = 0x102;
    *(volatile uint32_t *)0x60048000 = 0x5890080f;

    uint32_t *base_p = (uint32_t *)CAN_BUF0_BASE_ADDR;
    uint32_t *p;

    for (size_t i = 0; i < led_len; i++)
    {
        p = base_p + (i * 36);

        // 每次循环处理 2 个 bit，打包成 3 个 Word
        for (size_t j = 0; j < 24; j += 2)
        {
            // 提取当前 bit (偶数位) 和 下一个 bit (奇数位)
            uint8_t bit_curr = (led_data[i] >> (23 - j)) & 0x01;
            uint8_t bit_next = (led_data[i] >> (22 - j)) & 0x01;

            // Word 0: 覆盖 [当前 bit 的 H1] 和 [当前 bit 的 H2]
            // H1 固定为高电平(0x0800)，H2 取决于当前 bit
            uint32_t word0 = 0x00000800; 
            if (bit_curr) {
                word0 |= 0x08000000; 
            }
            *p++ = word0;

            // Word 1: 覆盖 [当前 bit 的 H3] 和 [下一个 bit 的 H1]
            // 当前 bit 的 H3 固定为低电平(0x0000)
            // 下一个 bit 的 H1 固定为高电平(0x0800)
            uint32_t word1 = 0x08000000; // 高16位固定为 0x0800
            *p++ = word1;

            // Word 2: 覆盖 [下一个 bit 的 H2] 和 [下一个 bit 的 H3]
            // H2 取决于下一个 bit，H3 固定为低电平(0x0000)
            uint32_t word2 = 0x00000000;
            if (bit_next) {
                word2 |= 0x00000800; 
            }
            *p++ = word2;
        }
    }
}

#define COLOR_RED     0x00004B00
#define COLOR_ORANGE  0x00144B00
#define COLOR_YELLOW  0x004B4B00
#define COLOR_GREEN   0x004B0000
#define COLOR_CYAN    0x004B004B
#define COLOR_BLUE    0x0000004B
#define COLOR_PURPLE  0x00002626
#define COLOR_WHITE   0x004B4B4B

// 定义 10 帧的动画数据，每帧 8 颗灯
const uint32_t animation_frames[10][8] = {
    {COLOR_RED,     0,          0,          0,          0,          0,          0,          0},          // 1: 红
    {0,             COLOR_ORANGE, 0,        0,          0,          0,          0,          0},          // 2: 橙
    {0,             0,          COLOR_YELLOW, 0,        0,          0,          0,          0},          // 3: 黄
    {0,             0,          0,          COLOR_GREEN, 0,         0,          0,          0},          // 4: 绿
    {0,             0,          0,          0,          COLOR_CYAN, 0,          0,          0},          // 5: 青
    {0,             0,          0,          0,          0,          COLOR_BLUE, 0,          0},          // 6: 蓝
    {0,             0,          0,          0,          0,          0,          COLOR_PURPLE, 0},        // 7: 紫
    {0,             0,          0,          0,          0,          0,          0,          COLOR_WHITE},// 8: 白
    // {COLOR_RED,     COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_PURPLE, COLOR_WHITE}, // 9: 全亮
    // {0,             0,          0,          0,          0,          0,          0,          0}           // 10: 全灭
};

int main(void)
{
    board_init();

    tim_config();
    edma_config();
    gpio_config();

    *(volatile uint32_t *)0x4000051c = 0x102;
    *(volatile uint32_t *)0x60048000 = 0x5890080f;

    int32_t delay_tick = 201;

    set_rgb_pixel(tx_data, COLOR_RED, 0);
    set_rgb_pixel(tx_data, COLOR_ORANGE, 1);
    set_rgb_pixel(tx_data, COLOR_YELLOW, 2);
    set_rgb_pixel(tx_data, COLOR_GREEN, 3);
    set_rgb_pixel(tx_data, COLOR_CYAN, 4);
    set_rgb_pixel(tx_data, COLOR_BLUE, 5);
    set_rgb_pixel(tx_data, COLOR_PURPLE, 6);
    set_rgb_pixel(tx_data, 0x00555555, 7);

    csi_edma_ch_start(&edma_ch, (void *)tx_data, (void *)(0x60018000), 144 * 8);

    extern csi_error_t csi_timer_start_cycle(csi_timer_t *timer, uint32_t cycles);
    csi_timer_start_cycle(&g_timer_handle, 34);

    while (1)
    {
        // 遍历 10 帧
        for (uint8_t frame = 0; frame < 8; frame++) 
        {
            // 将当前帧的 8 颗灯数据写入缓冲区
            set_rgb_strip((uint32_t *)animation_frames[frame], 8);

            while (dma_cplt_flag != 1)
                ;

            // csi_timer_stop(&g_timer_handle);
            dma_cplt_flag = 0;
            csi_edma_ch_start(&edma_ch, (void *)(tx_data), (void *)(0x60018000), 144 * 8);
            // csi_timer_start_1(&g_timer_handle, 1);

            // 延时 100ms 控制动画速度
            mdelay(delay_tick);
        }
        delay_tick -= 20;
        if (delay_tick <= 0)
        {
            delay_tick = 101;
        }
    }

    while (1)
    {
        mdelay(1000);
    }
}
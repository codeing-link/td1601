

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

#define EMULATE_GPIO_PORT   PORTB
#define EMULATE_GPIO    PIN19
#define TEST_TIM0   2
extern const uint16_t timer_hs_num[]; 
static uint32_t s_count = 0;
static csi_timer_t g_timer_handle;
static csi_gpio_t gpio;
static csi_edma_ch_t edma_ch;
static uint32_t *s_emulate_data = (uint32_t *)CAN_BUF0_BASE_ADDR;
static volatile uint8_t dma_cplt_flag = 0;

void wj_edma_event_cb(csi_edma_ch_t *dma, csi_edma_event_t event, void *arg)
{
    (void)dma, (void)arg;
    if (event == EDMA_EVENT_TRANSFER_DONE) {
        dma_cplt_flag = 1;
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
    config.dst_tw  = EDMA_DATA_WIDTH_32_BITS;

    config.group_len = 4U;
    config.trans_dir = EDMA_MEM2PERH;
    config.handshake = timer_hs_num[TEST_TIM0];

    csi_edma_ch_config(&edma_ch, &config);
	
	return ret;
}

static void gpio_config(void)
{
    csi_pin_set_mux(EMULATE_GPIO_PORT, EMULATE_GPIO, PIN_FUNC_GPIO);
    csi_gpio_init(&gpio, 1);
    csi_gpio_write(&gpio, EMULATE_GPIO,  GPIO_PIN_HIGH);
    csi_gpio_dir(&gpio, EMULATE_GPIO, GPIO_DIRECTION_OUTPUT);
}

int gpio_emulate_uart_send(char data)
{
    s_emulate_data[0] = 0;
    s_emulate_data[9] = EMULATE_GPIO;
    for(int i = 1; i < 9; i++) {
        if (data & 1) {
            s_emulate_data[i] = EMULATE_GPIO;
        } else {
            s_emulate_data[i] = 0;
        }
        data >>= 1;
    }
    csi_edma_ch_start(&edma_ch, (void *)s_emulate_data, (void *)(0x60018000), 10 * 4);
    while(!dma_cplt_flag);
    dma_cplt_flag = 0;
    return 0;
}

int gpio_emulate_uart_init(uint32_t baud)
{
    tim_config();
    edma_config();
    gpio_config();
    *(volatile uint32_t *)0x4000051c = 0x102;
    *(volatile uint32_t *)0x60048000 = 0x5890080f;
    s_count = soc_get_timer_freq(TEST_TIM0)/baud;
    extern csi_error_t csi_timer_start_cycle(csi_timer_t *timer, uint32_t cycles);
    csi_timer_start_cycle(&g_timer_handle, s_count);

    return 0;
}

const char *str = "GPIO EMULATE UART SUCCESS\r\n";
int main(void)
{
    board_init();
    gpio_emulate_uart_init(4800);
    while(*str !='\0')
    {
        gpio_emulate_uart_send(*str);
        str++;
    }
    gpio_emulate_uart_send('\n');
}
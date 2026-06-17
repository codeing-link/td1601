#include <stdio.h>
#include <string.h>
#include <drv/pin.h>
#include <drv/dma.h>
#include <drv/uart.h>
#include <drv/porting.h>
#include <soc.h>
#include <board_init.h>
#include <board_config.h>

static volatile uint8_t dma_cb_flag = 0;
static csi_dma_ch_t g_dma_ch;
#define EXAMPLE_UART_IDX1       1               // 串口1
#define EXAMPLE_UART_BAUDRATE   115200          // 波特率

#ifndef DMA_M2M_SIZE
#define DMA_M2M_SIZE    256
#endif
#ifndef DMA_LL_SIZE
#define DMA_LL_SIZE 20
#endif
#define TEST_LLI_COUNT      3

/* -------------------------- 全局变量 -------------------------- */
static csi_uart_t g_uart;                       // 串口1句柄
static uint8_t uart_inited = 0;                 // 初始化标志
uint8_t lli_buf[TEST_LLI_COUNT * DMA_LL_SIZE] __attribute__((aligned(4)));

static uint8_t p_src[TEST_LLI_COUNT][DMA_M2M_SIZE] = {0};
static uint8_t p_dst[TEST_LLI_COUNT][DMA_M2M_SIZE] __attribute__((aligned(32)));
uint32_t uart_thr_addr = 0x60010000;
static void dma_event_cb_fun(csi_dma_ch_t *dma, csi_dma_event_t event, void *arg)
{
    if (event == DMA_EVENT_TRANSFER_ERROR) {
    } else if (event == DMA_EVENT_TRANSFER_DONE) {
        dma_cb_flag = 1;
    }
}
/**
 * @brief 初始化UART1
 */
int uart1_init(void)
{
    if (uart_inited) {
        printf("UART1 already inited, skip\n");
        return 0;
    }

    csi_error_t ret;
    // 1. 初始化UART1
    ret = csi_uart_init(&g_uart, EXAMPLE_UART_IDX1);

    // 2. 配置波特率
    ret = csi_uart_baud(&g_uart, EXAMPLE_UART_BAUDRATE);

    // 3. 配置数据格式（8N1）
    ret = csi_uart_format(&g_uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
    ret = csi_uart_flowctrl(&g_uart, UART_FLOWCTRL_NONE);
    uart_inited = 1;
    printf("UART1 init success (115200 8N1)\n");
    return ret;
}
/**
 * @brief 串口引脚初始化
 */
void example_pin_uart_init(void)
{
    csi_pin_set_mux(EXAMPLE_UART_PIN_PORT, EXAMPLE_PIN_UART_TX1, EXAMPLE_PIN_UART_TX_FUNC1);
    csi_pin_set_mux(EXAMPLE_UART_PIN_PORT, EXAMPLE_PIN_UART_RX1, EXAMPLE_PIN_UART_RX_FUNC1);
    printf("UART1 pin mux init success\n");
}
/* 链表测试函数 */
static int32_t dma_test_link_list(void)
{
    csi_error_t ret;
    csi_dma_ch_config_t config;
    csi_dma_link_list_item_t item;

    memset(p_dst, 0, sizeof(p_src));
    memset(&g_dma_ch, 0, sizeof(csi_dma_ch_t));

    ret = csi_dma_ch_alloc(&g_dma_ch, -1, -1);
    if (ret != CSI_OK) {
        return -1;
    }

    ret = csi_dma_ch_attach_callback(&g_dma_ch, dma_event_cb_fun, NULL);
    if (ret != CSI_OK) {
        csi_dma_ch_free(&g_dma_ch);
        return -1;
    }

    memset(&config, 0, sizeof(csi_dma_ch_config_t));
    config.src_inc = DMA_ADDR_CONSTANT;
    config.dst_inc = DMA_ADDR_INC;
    config.src_tw = DMA_DATA_WIDTH_8_BITS;
    config.dst_tw = DMA_DATA_WIDTH_8_BITS;
    config.group_len = 1;
    config.trans_dir = DMA_PERH2MEM;
    config.link_list_en = 1;

    config.link_list_config.lli_buf = lli_buf;
    config.link_list_config.link_list_num = 0;

    soc_dcache_clean_invalid_range((unsigned long)p_dst, sizeof(p_dst));

    /* 添加链表项 */
    item.srcaddr = (uint32_t *)uart_thr_addr;
    item.dstaddr = p_dst[0];
    item.length  = DMA_M2M_SIZE;
    csi_dma_add_link_list_item(&g_dma_ch, &config, &item, 0);

    item.srcaddr = (uint32_t *)uart_thr_addr;
    item.dstaddr = p_dst[1];
    item.length  = DMA_M2M_SIZE;
    csi_dma_add_link_list_item(&g_dma_ch, &config, &item, 0);

    item.srcaddr = (uint32_t *)uart_thr_addr;
    item.dstaddr = p_dst[2];
    item.length  = DMA_M2M_SIZE;
    csi_dma_add_link_list_item(&g_dma_ch, &config, &item, 1);
    config.handshake = 2;
    /* 启动链表DMA */
    csi_dma_ch_start_linklist(&g_dma_ch, &config);

    printf("DMA link list running...\n");
    while (!dma_cb_flag);

    csi_dma_ch_stop(&g_dma_ch);
    csi_dma_ch_free(&g_dma_ch);
    printf("dma link list test OK\n");
    return 0;
}

static int32_t test_dma(void)
{
    example_pin_uart_init();
    uart1_init();
    return dma_test_link_list();
}

int example_dma_ll_p2m(void)
{
    int ret;
    ret = test_dma();

    if (ret < 0) {
        printf("test dma ll p2m failed\n");
        return -1;
    }

    printf("test dma ll p2m successfully\n");
    return 0;
}

int main(void)
{
    int ret;
    board_init();
    ret = example_dma_ll_p2m();
    return ret;
}
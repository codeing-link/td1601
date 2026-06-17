#include <stdio.h>
#include <string.h>
#include <drv/dma.h>
#include <drv/porting.h>
#include <soc.h>
#include <board_init.h>
#include <board_config.h>

static volatile uint8_t dma_cb_flag = 0;
static csi_dma_ch_t g_dma_ch;

#ifndef DMA_M2M_SIZE
#define DMA_M2M_SIZE    256
#endif
#ifndef DMA_LL_SIZE
#define DMA_LL_SIZE 20
#endif
#define TEST_LLI_COUNT      3

uint8_t lli_buf[TEST_LLI_COUNT * DMA_LL_SIZE] __attribute__((aligned(4)));

static uint8_t p_src[TEST_LLI_COUNT][DMA_M2M_SIZE] = {0};
static uint8_t p_dst[TEST_LLI_COUNT][DMA_M2M_SIZE] __attribute__((aligned(32)));

static void sleep(uint32_t k)
{

    int i, j;
    for (i = 0; i < 1000; i++) {
        for (j = 0; j < k; j++);
    }
}

static void dma_event_cb_fun(csi_dma_ch_t *dma, csi_dma_event_t event, void *arg)
{
    if (event == DMA_EVENT_TRANSFER_ERROR) {
    } else if (event == DMA_EVENT_TRANSFER_DONE) {
        dma_cb_flag = 1;
    }
}

/* 链表测试函数 */
static int32_t dma_test_link_list(void)
{
    uint32_t i, k;
    csi_error_t ret;
    csi_dma_ch_config_t config;
    csi_dma_link_list_item_t item;

    for (k = 0; k < TEST_LLI_COUNT; k++) {
        for (i = 0; i < DMA_M2M_SIZE; i++) {
            p_src[k][i] = i + k * 100;
        }
    }

    memset(p_dst, 0, sizeof(p_dst));
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
    config.src_inc = DMA_ADDR_INC;
    config.dst_inc = DMA_ADDR_INC;
    config.src_tw = DMA_DATA_WIDTH_32_BITS;
    config.dst_tw = DMA_DATA_WIDTH_32_BITS;
    config.group_len = 8;
    config.trans_dir = DMA_MEM2MEM;
    config.link_list_en = 1;

    config.link_list_config.lli_buf = lli_buf;
    config.link_list_config.link_list_num = 0;

    soc_dcache_clean_invalid_range((unsigned long)p_src, sizeof(p_src));
    soc_dcache_clean_invalid_range((unsigned long)p_dst, sizeof(p_dst));

    /* 添加链表项 */
    item.srcaddr = p_src[0];
    item.dstaddr = p_dst[0];
    item.length  = DMA_M2M_SIZE;
    csi_dma_add_link_list_item(&g_dma_ch, &config, &item, 0);

    item.srcaddr = p_src[1];
    item.dstaddr = p_dst[1];
    item.length  = DMA_M2M_SIZE;
    csi_dma_add_link_list_item(&g_dma_ch, &config, &item, 0);

    item.srcaddr = p_src[2];
    item.dstaddr = p_dst[2];
    item.length  = DMA_M2M_SIZE;
    csi_dma_add_link_list_item(&g_dma_ch, &config, &item, 1);

    /* 启动链表DMA */
    csi_dma_ch_start_linklist(&g_dma_ch, &config);

    printf("DMA link list running...\n");
    while (!dma_cb_flag);

    /* 数据校验 */
    for (k = 0; k < TEST_LLI_COUNT; k++) {
        for (i = 0; i < DMA_M2M_SIZE; i++) {
            if (p_dst[k][i] != p_src[k][i]) {
                printf("DMA compare failed at %d,%d\n", k, i);
                return -1;
            }
        }
    }

    csi_dma_ch_stop(&g_dma_ch);
    csi_dma_ch_free(&g_dma_ch);
    printf("dma link list test OK\n");
    return 0;
}

static int32_t test_dma(void)
{
    return dma_test_link_list();
}

int example_dma_ll_m2m(void)
{
    int ret;
    ret = test_dma();

    if (ret < 0) {
        printf("test dma ll m2m failed\n");
        return -1;
    }

    printf("test dma ll m2m successfully\n");
    return 0;
}

int main(void)
{
    int ret;
    board_init();
    ret = example_dma_ll_m2m();
    return ret;
}
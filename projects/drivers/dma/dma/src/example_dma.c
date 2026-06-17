

/******************************************************************************
 * @file     example_dmac.c
 * @brief    the main function for the DMAC driver
 * @version  V1.0
 * @date     19. Mar 2020
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <drv/dma.h>
#include <drv/porting.h>
#include <soc.h>
#include <board_init.h>
#include <board_config.h>

static  volatile uint8_t dma_cb_flag = 0;
static csi_dma_ch_t g_dma_ch;
#ifndef DMA_M2M_SIZE
#define DMA_M2M_SIZE    512
#endif

#define DMA_CHECK_RETURN(ret)                      \
    do {                                        \
        if (ret != CSI_OK) {                    \
            return -1;                          \
        }                                       \
    } while(0);

static uint8_t p_src[DMA_M2M_SIZE] = {0};
static uint8_t p_dst[DMA_M2M_SIZE] __attribute__((aligned(32)));

static void sleep(uint32_t k)
{
    int i, j;

    for (i = 0; i < 1000; i++) {
        for (j = 0; j < k; j++);
    }
}
static void dma_event_cb_fun(csi_dma_ch_t *dma, csi_dma_event_t event, void *arg)
{
    if (event == DMA_EVENT_TRANSFER_ERROR) {/* DMA transfer ERROR */
    } else if (event == DMA_EVENT_TRANSFER_DONE) {/* DMA transfer complete */
        dma_cb_flag = 1;
    } else if (event == DMA_EVENT_TRANSFER_HALF_DONE) {
    }
}

static int32_t dma_test_mem2mem(void)
{
    uint32_t i;
    csi_error_t ret;
    csi_dma_ch_config_t config;

    for (i = 0; i < DMA_M2M_SIZE; i++) {
        p_src[i] = i;
    }

    memset(p_dst, 0, DMA_M2M_SIZE);
    memset(&g_dma_ch, 0, sizeof(csi_dma_ch_t));
    ret = csi_dma_ch_alloc(&g_dma_ch, -1, -1);
    DMA_CHECK_RETURN(ret);

    ret = csi_dma_ch_attach_callback(&g_dma_ch, dma_event_cb_fun, NULL);
    DMA_CHECK_RETURN(ret);

    memset(&config, 0, sizeof(csi_dma_ch_config_t));
    config.src_inc = DMA_ADDR_INC;
    config.dst_inc = DMA_ADDR_INC;
    config.src_tw = DMA_DATA_WIDTH_8_BITS;
    config.dst_tw = DMA_DATA_WIDTH_8_BITS;
    /* config for wj_dma */
    config.group_len = 8;
    config.trans_dir = DMA_MEM2MEM;
    config.src_reload_en = 0;
    config.dst_reload_en = 0;
    config.half_int_en = 1;

    ret = csi_dma_ch_config(&g_dma_ch, &config);

    if (ret != CSI_OK) {
        csi_dma_ch_free(&g_dma_ch);
        printf("csi_dma_config_channel error\n");
        return -1;
    }

    soc_dcache_clean_invalid_range((unsigned long)p_src, DMA_M2M_SIZE);
    soc_dcache_clean_invalid_range((unsigned long)p_dst, DMA_M2M_SIZE);
    csi_dma_ch_start(&g_dma_ch, p_src, p_dst, DMA_M2M_SIZE);

    printf("sleep or do other things while data in transfermation using DMA\n");
    sleep(1);

    //while (csi_dma_get_status(dma_handle, ch) != DMA_STATE_DONE) ;
    while (!dma_cb_flag);

    for (i = 0; i < DMA_M2M_SIZE; i++) {
        if (p_dst[i] != p_src[i]) {
            printf("faild:%d\n", i);
            return -1;
        }
    }

    csi_dma_ch_stop(&g_dma_ch);
    csi_dma_ch_free(&g_dma_ch);
    printf("dma_mem2mem_test_func OK\n");
    return 0;
}

static int32_t test_dma(void)
{
    int32_t ret;

    ret = dma_test_mem2mem();

    if (ret < 0) {
        printf("test dma mem to mem error\n");
        return -1;
    }

    return 0;
}

int example_dma(void)
{
    int ret;
    ret = test_dma();

    if (ret < 0) {
        printf("test dmac failed\n");
        CSI_EXAMPLE_RESULT(-1);
        return -1;
    }

    printf("test dmac successfully\n");
    CSI_EXAMPLE_RESULT(1);
    return 0;
}

int main(void)
{
    int ret;
    board_init();
    ret = example_dma();
    return ret;
}


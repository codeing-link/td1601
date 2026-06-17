


/******************************************************************************
 * @file     example_adc.c
 * @brief    the main function for the adc driver
 * @version  V1.0
 * @date     1. October 2017
 ******************************************************************************/
#include <stdio.h>
#include <string.h>

#include <soc.h>
#include <csi_config.h>
#include <board_config.h>
#include <board_init.h>
#include <drv/adc.h>
#include <drv/pin.h>
#include <drv/tick.h>
#include <drv/edma.h>

#define ADC_READ_SIZE 32
#define DEFAULT_ADC_READ_TIMEOUT  2000
#define MAX_SUCCESS_NUM    2
#define ADC_GND_VALUE      0
#define ADC_VDD_VALUE      4095
#define CHECK_GND          0
#define CHECK_VDD          1
#define ACCURACY_VALUE     100

#ifndef EXAMPLE_ADC_DMA_GPIO_IDX
#define EXAMPLE_ADC_DMA_GPIO_IDX 0
#endif

static volatile uint8_t suc_num = 0;
static int adc_event = -1;
#ifdef CONFIG_CHIP_TD1601
static uint32_t *rx_data = (uint32_t *)CAN_BUF0_BASE_ADDR;
static csi_edma_ch_t edma_ch;
static csi_adc_dma_t adc_dma_ch = {
    .dma = NULL,
    .edma = &edma_ch,
};
#else
static uint32_t rx_data[ADC_READ_SIZE]__attribute__((aligned(32)));
static csi_edma_ch_t dma_ch;
static csi_adc_dma_t adc_dma_ch = {
    .dma = &dma_ch,
    .edma = NULL,
};
#endif
static csi_adc_t adc;

static csi_gpio_t gpio;

void example_input_cfg_a(void)
{
    csi_pin_set_mux(EXAMPLE_ADC_INPUT,  EXAMPLE_ADC_INPUT_FUNC);
    csi_pin_mode(EXAMPLE_ADC_INPUT, GPIO_MODE_PULLDOWN);
    csi_gpio_init(&gpio, EXAMPLE_ADC_DMA_GPIO_IDX);
    csi_gpio_dir(&gpio, EXAMPLE_ADC_INPUT_MASK, GPIO_DIRECTION_OUTPUT);
    csi_gpio_write(&gpio, EXAMPLE_ADC_INPUT_MASK,  GPIO_PIN_LOW);
}

void example_input_cfg_b(void)
{
    csi_pin_set_mux(EXAMPLE_ADC_INPUT,  EXAMPLE_ADC_INPUT_FUNC);
    csi_pin_mode(EXAMPLE_ADC_INPUT, GPIO_MODE_PULLUP);
    csi_gpio_write(&gpio, EXAMPLE_ADC_INPUT_MASK,  GPIO_PIN_HIGH);
}

void example_pin_adc_init(void)
{
    csi_pin_set_mux(EXAMPLE_ADC_CHANNEL0_PIN,  EXAMPLE_ADC_CHANNEL0_PIN_FUNC);
    csi_pin_set_mux(EXAMPLE_ADC_CHANNEL1_PIN,  EXAMPLE_ADC_CHANNEL1_PIN_FUNC);
    csi_pin_set_mux(EXAMPLE_ADC_CHANNEL2_PIN,  EXAMPLE_ADC_CHANNEL2_PIN_FUNC);
    csi_pin_set_mux(EXAMPLE_ADC_CHANNEL3_PIN,  EXAMPLE_ADC_CHANNEL3_PIN_FUNC);
}

static void adc_event_cb(csi_adc_t *adc, csi_adc_event_t event, void *arg)
{
    adc_event = event;
}

static void dump_adc_data(char *prefix, uint32_t *data, uint16_t size)
{
    printf("================== %s ==================\n", prefix);

    while (size--) {
        printf("%d, ", *data++);
    }

    printf("\n=============================================\n");
}

int _adc_dma_continue(uint32_t val)
{
    int      ret, i;
    uint32_t timestart;

    ret = csi_adc_freq_div(&adc, 32);

    if (ret == 0) {
        return -1;
    }

    ret = csi_adc_sampling_time(&adc, 2);

    if (ret < 0) {
        return -1;
    }

    ret = csi_adc_continue_mode(&adc, true);

    if (ret < 0) {
        return -1;
    }

    ret = csi_adc_channel_enable(&adc, csi_pin_get_adc_channel(EXAMPLE_ADC_CHANNEL0_PIN), true);

    if (ret < 0) {
        return -1;
    }

    ret = csi_adc_attach_callback(&adc, adc_event_cb, NULL);

    if (ret < 0) {
        return -1;
    }

    ret = csi_adc_set_buffer(&adc, rx_data, ADC_READ_SIZE);

    if (ret < 0) {
        return -1;
    }

    ret = csi_adc_link_dma(&adc, &adc_dma_ch);

    if (ret < 0) {
        return -1;
    }

    /* first conversion */
    adc_event = -1;
    ret = csi_adc_start_async(&adc);

    if (ret < 0) {
        return -1;
    }

    timestart = csi_tick_get_ms();

    while (adc_event != ADC_EVENT_CONVERT_COMPLETE) {
        if ((csi_tick_get_ms() - timestart) > DEFAULT_ADC_READ_TIMEOUT) {
            return -1;
        }
    }

    dump_adc_data("rx_data", rx_data, ADC_READ_SIZE);

    if (val == CHECK_GND) {
        for (i = 0; i < ADC_READ_SIZE; i++) {
            if (rx_data[i] > ADC_GND_VALUE + ACCURACY_VALUE) {
                return -1;
            }
        }
    } else {
        for (i = 0; i < ADC_READ_SIZE; i++) {
            if (rx_data[i] < ADC_VDD_VALUE - ACCURACY_VALUE) {
                return -1;
            }
        }
    }

    ret = csi_adc_stop_async(&adc);

    if (ret < 0) {
        return -1;
    }

    /* try conversion again */
#ifdef CONFIG_CHIP_TD1601
    memset(rx_data, 0, sizeof(uint32_t) * ADC_READ_SIZE);
#else
    memset(rx_data, 0, sizeof(rx_data));
#endif
    adc_event = -1;

    ret = csi_adc_set_buffer(&adc, rx_data, ADC_READ_SIZE);

    if (ret < 0) {
        return -1;
    }

    ret = csi_adc_start_async(&adc);

    if (ret < 0) {
        return -1;
    }

    timestart = csi_tick_get_ms();

    while (adc_event != ADC_EVENT_CONVERT_COMPLETE) {
        if ((csi_tick_get_ms() - timestart) > DEFAULT_ADC_READ_TIMEOUT) {
            return -1;
        }
    }

    dump_adc_data("rx_data", rx_data, ADC_READ_SIZE);

    if (val == CHECK_GND) {
        for (i = 0; i < ADC_READ_SIZE; i++) {
            if (rx_data[i] > ADC_GND_VALUE + ACCURACY_VALUE) {
                return -1;
            }
        }
    } else {
        for (i = 0; i < ADC_READ_SIZE; i++) {
            if (rx_data[i] < ADC_VDD_VALUE - ACCURACY_VALUE) {
                return -1;
            }
        }
    }

    ret = csi_adc_stop_async(&adc);

    if (ret < 0) {
        return -1;
    }

    return 0;
}

int example_adc_dma(uint32_t val)
{
    int ret = 0;

    example_pin_adc_init();

    ret = csi_adc_init(&adc, csi_pin_get_adc_index(EXAMPLE_ADC_CHANNEL0_PIN));

    if (ret < 0) {
        return -1;
    }

    ret = _adc_dma_continue(val);

    if (ret < 0) {
        return -1;
    }

    ret = csi_adc_link_dma(&adc, NULL);

    if (ret < 0) {
        return -1;
    }

    csi_adc_uninit(&adc);
    return 0;
}


int main()
{
    int ret;

    board_init();

    example_pin_adc_init();

    printf("-------------------------------------\n");
    printf("example_adc_dma()\n");
    printf("ADC_VREFP connect high level\n");
    printf("ADC_VREFN connect low level\n");
    printf("-------------------------------------\n");
    example_input_cfg_a();
    ret = example_adc_dma(CHECK_GND);

    if (ret >= 0) {
        suc_num++;
    }

    example_input_cfg_b();
    ret = example_adc_dma(CHECK_VDD);

    if (ret >= 0) {
        suc_num++;
    }

    if (suc_num == MAX_SUCCESS_NUM) {
        printf("run example_adc_dma success\n");
        CSI_EXAMPLE_RESULT(1);
    } else {
        printf("run example_adc_dma fail\n");
        CSI_EXAMPLE_RESULT(-1);
        return -1;
    }

    return 0;
}

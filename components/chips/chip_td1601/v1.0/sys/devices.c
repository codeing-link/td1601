/*
 * Copyright (C) 2017-2020 NStar-Chip Holding Limited
 */

/******************************************************************************
 * @file     devices.c
 * @brief    source file for the devices
 * @version  V1.0
 * @date     2019-12-18
******************************************************************************/

#include <stdio.h>
#include <csi_config.h>
#include <soc.h>
#include <drv/uart.h>
#include <drv/timer.h>
#include <drv/dma.h>
#include <drv/iic.h>
#include <drv/rtc.h>
#include <drv/spi.h>
#include <drv/adc.h>
#include <drv/gpio.h>
#include <drv/irq.h>
#include <drv/pin.h>

const csi_perip_info_t g_soc_info[] = {
    {DW_UART0_BASE,            DW_UART0_IRQn,            0,    DEV_DW_UART_TAG},
    {DW_UART1_BASE,            DW_UART1_IRQn,            1,    DEV_DW_UART_TAG},
    {DW_UART2_BASE,            DW_UART2_IRQn,            2,    DEV_DW_UART_TAG},
    {DW_UART3_BASE,            DW_UART3_IRQn,            3,    DEV_DW_UART_TAG},
    {DW_DMA_BASE,              DW_DMA_IRQn,              0,    DEV_DW_DMA_TAG},
    {WJ_DMA_BASE,              WJ_DMA_IRQn,              0,    DEV_WJ_DMA_TAG},
    {DW_IIC_BASE,              DW_IIC_IRQn,              0,    DEV_DW_IIC_TAG},
    {DW_IIC1_BASE,             DW_IIC1_IRQn,             1,    DEV_DW_IIC_TAG},
    {DW_TIMER0_BASE,           DW_TIMER0_IRQn,           0,    DEV_DW_TIMER_TAG},
    {DW_TIMER1_BASE,           DW_TIMER1_IRQn,           1,    DEV_DW_TIMER_TAG},
    {DW_TIMER2_BASE,           DW_TIMER2_IRQn,           2,    DEV_DW_TIMER_TAG},
    {DW_TIMER3_BASE,           DW_TIMER3_IRQn,           3,    DEV_DW_TIMER_TAG},
    {DW_TIMER4_BASE,           DW_TIMER4_IRQn,           4,    DEV_DW_TIMER_TAG},
    {DW_TIMER5_BASE,           DW_TIMER5_IRQn,           5,    DEV_DW_TIMER_TAG},
    {DW_TIMER6_BASE,           DW_TIMER6_IRQn,           6,    DEV_DW_TIMER_TAG},
    {DW_TIMER7_BASE,           DW_TIMER7_IRQn,           7,    DEV_DW_TIMER_TAG},
    {DW_WDT_BASE,              DW_WDT_IRQn,              0,    DEV_DW_WDT_TAG},
    {DW_GPIO_BASE,             DW_GPIO_IRQn,             0,    DEV_DW_GPIO_TAG},
    {DW_GPIO1_BASE,            DW_GPIO1_IRQn,            1,    DEV_DW_GPIO_TAG},
    {DW_SPI0_BASE,             DW_SPI0_IRQn,             0,    DEV_DW_SPI_TAG},
    {DW_OSPI0_BASE,            DW_OSPI0_IRQn,            0,    DEV_DW_OSPI_TAG},
    {CD_QSPI_BASE,             CD_QSPI_IRQn,             0,    DEV_CD_QSPI_TAG},
    {WJ_PWM_BASE,              WJ_PWM_IRQn,              0,    DEV_WJ_PWM_TAG},
    {WJ_RTC_BASE,              WJ_RTC_IRQn,              0,    DEV_WJ_RTC_TAG},
    {WJ_I2S0_BASE,             WJ_I2S0_IRQn,             0,    DEV_WJ_I2S_TAG},
    {WJ_I2S1_BASE,             WJ_I2S1_IRQn,             1,    DEV_WJ_I2S_TAG},
    {WJ_ADC_BASE,              WJ_ADC_IRQn,              0,    DEV_WJ_ADC_TAG},
    {WJ_ADC1_BASE,             WJ_ADC1_IRQn,             1,    DEV_WJ_ADC_TAG},
    {WJ_TDM_BASE,              WJ_TDM_IRQn,             0,    DEV_WJ_TDM_TAG},
    {NXP_CAN0_BASE,            NXP_CAN_IRQn,            0,  DEV_NXP_FLEXCAN_TAG},
    {WJ_EFUSE_BASE,            -1,                       0,    DEV_WJ_EFUSE_TAG},
    {0, 0, 0, 0}
};

const uint8_t g_dma_chnum[] = {8, 0};

const uint16_t timer_hs_num[4] = {80, 81, 82, 83};
const uint16_t gpio_hs_num[16] = {88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103};
const uint16_t gpio1_hs_num[16] = {58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73};
const uint16_t timer_start_ht_num[8] = {130, 131, 134, 135, 194, 195, 198, 199};
const uint16_t timer_stop_ht_num[8] = {128, 129, 132, 133, 192, 193, 196, 197};
const uint16_t adc_start_ht_num = 64;
const uint16_t adc_stop_ht_num = 65;
const uint16_t dma_ch_tfrdone_hs_num[] = {8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53};
const uint16_t dma_ch_evtdone_hs_num[] = {6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51};

/* DMA handshake number */
/* The member of uart_tx_hs_num is the handshake number for ETB */
const uint16_t tdm_tx_hs_num[1] = {5};
const uint16_t tdm_rx_hs_num[1] = {4};
const uint16_t uart_tx_hs_num[4] = {1, 3, 13, 15};
const uint16_t uart_rx_hs_num[4] = {0, 2, 12, 14};
const uint16_t iic_tx_hs_num[2]  = {11, 5};
const uint16_t iic_rx_hs_num[2]  = {10, 4};
const uint16_t eiic_tx_hs_num[2]  = {86, 84};
const uint16_t eiic_rx_hs_num[2]  = {87, 85};
const uint16_t spi_tx_hs_num[1]  = {9};
const uint16_t spi_rx_hs_num[1]  = {8};
const uint16_t ospi_tx_hs_num[1]  = {7};
const uint16_t ospi_rx_hs_num[1]  = {6};
const uint16_t espi_tx_hs_num[1]  = {104};
const uint16_t espi_rx_hs_num[1]  = {105};
const uint16_t i2s_tx_hs_num[2]  = {13, 15};
const uint16_t i2s_rx_hs_num[2]  = {12, 14};
const uint16_t ei2s_tx_hs_num[2]  = {55, 57};
const uint16_t ei2s_rx_hs_num[2]  = {54, 56};
const uint16_t pwmr_tx_hs_num[1]  = {106};

const csi_dma_ch_desc_t tdm0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t uart0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t uart1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t uart2_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t uart3_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t iic0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t iic1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t spi0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t ospi_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};
const csi_dma_ch_desc_t i2s0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s2_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s3_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s4_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s5_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s6_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t adc_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t pwmr_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0xff, 0xff}
};


const csi_dma_ch_spt_list_t dma_spt_list[] = {
    {DEV_DW_UART_TAG, 0, uart0_dma_ch_list},
    {DEV_DW_UART_TAG, 1, uart1_dma_ch_list},
    {DEV_DW_UART_TAG, 2, uart2_dma_ch_list},
    {DEV_DW_UART_TAG, 3, uart3_dma_ch_list},
    {DEV_DW_IIC_TAG,  0, iic0_dma_ch_list},
    {DEV_DW_IIC_TAG,  1, iic1_dma_ch_list},
    {DEV_DW_SPI_TAG,  0, spi0_dma_ch_list},
    {DEV_DW_OSPI_TAG, 0, ospi_dma_ch_list},
    {DEV_WJ_ADC_TAG,  0, adc_dma_ch_list},
    {DEV_WJ_I2S_TAG,  0, i2s0_dma_ch_list},
    {DEV_WJ_I2S_TAG,  1, i2s1_dma_ch_list},
    {DEV_WJ_PWMR_TAG, 0, pwmr_dma_ch_list},
    {DEV_WJ_TDM_TAG,  0, tdm0_dma_ch_list},
    {0xFFFFU,         0xFFU,         NULL},
};

const csi_pinmap_t adc_pinmap[] = {
    {PORTA, PIN3,           0,     0,           PA3_ADC_A1 },
    {PORTA, PIN4,           0,     1,           PA4_ADC_A2 },
    {PORTA, PIN5,           0,     2,           PA5_ADC_A3 },
    {PORTA, PIN6,           0,     3,           PA6_ADC_A4 },
    {PORTA, PIN7,           0,     4,           PA7_ADC_A5 },
    {PORTA, PIN8,           0,     5,           PA8_ADC_A6 },
    {PORTA, PIN9,           0,     6,           PA9_ADC_A7 },
    {PORTA, PIN15,          0,     8,           PA15_ADC_A9 },
    {PORTA, PIN16,          0,     9,           PA16_ADC_A10 },
    {PORTA, PIN17,          0,     10,          PA17_ADC_A11 },
    {PORTA, PIN18,          0,     12,          PA18_ADC_A13 },
    {PORTA, PIN19,          0,     7,           PA19_ADC_A8 },
    {PORTB, PIN2,           1,     14,          PB2_ADC1_A15 },
    {PORTB, PIN4,           1,     6,           PB4_ADC1_A7 },
    {PORTB, PIN5,           1,     8,           PB5_ADC1_A9 },
    {PORTB, PIN6,           1,     9,           PB6_ADC1_A10 },
    {PORTB, PIN7,           1,     10,          PB7_ADC1_A11 },
    {PORTB, PIN8,           1,     11,          PB8_ADC1_A12 },
    {PORTB, PIN13,          1,     7,           PB13_ADC1_A8 },
    {PORTB, PIN14,          0,     13,          PB14_ADC_A14 },
    {PORTB, PIN15,          0,     11,          PB15_ADC_A12 },
    {PORTB, PIN18,          1,     12,          PB18_ADC1_A13 },
    {PORTB, PIN19,          1,     13,          PB19_ADC1_A14 },
    {PORTB, PIN21,          0,     14,          PB21_ADC_A15 },
    {PORTB, PIN22,          1,     0,           PB22_ADC1_A1 },
    {PORTB, PIN23,          1,     1,           PB23_ADC1_A2 },
    {PORTB, PIN28,          1,     2,           PB28_ADC1_A3 },
    {PORTB, PIN29,          1,     3,           PB29_ADC1_A4 },
    {PORTB, PIN30,          1,     4,           PB30_ADC1_A5 },
    {PORTB, PIN31,          1,     5,           PB31_ADC1_A6 },
    {0xFFFFFFFFU,  0xFFFFFFFFU,   0xFFU, 0xFFU,       0xFFFFFFFFU },
};

const csi_pinmap_t pwm_pinmap[] = {
    {PORTA,  PIN0,    0,    0,    PA0_PWM_CH0  },
    {PORTA,  PIN1,    0,    1,    PA1_PWM_CH1  },
    {PORTA,  PIN2,    0,    2,    PA2_PWM_CH2  },
    {PORTA,  PIN3,    0,    3,    PA3_PWM_CH3  },
    {PORTA,  PIN4,    0,    4,    PA4_PWM_CH4  },
    {PORTA,  PIN5,    0,    5,    PA5_PWM_CH5  },
    {PORTA,  PIN6,    0,    6,    PA6_PWM_CH6  },
    {PORTA,  PIN7,    0,    7,    PA7_PWM_CH7  },
    {PORTA,  PIN8,    0,    8,    PA8_PWM_CH8  },
    {PORTA,  PIN9,    0,    9,    PA9_PWM_CH9  },
    {PORTA,  PIN15,   0,    4,    PA15_PWM_CH4 },
    {PORTA,  PIN16,   0,    5,    PA16_PWM_CH5 },
    {PORTA,  PIN17,   0,   10,    PA17_PWM_CH10},
    {PORTA,  PIN18,   0,   11,    PA18_PWM_CH11},
    {PORTA,  PIN23,   0,    0,    PA23_PWM_CH0 },
    {PORTA,  PIN24,   0,    1,    PA24_PWM_CH1 },
    {PORTA,  PIN25,   0,    2,    PA25_PWM_CH2 },
    {PORTA,  PIN26,   0,    3,    PA26_PWM_CH3 },
    {PORTA,  PIN27,   0,    4,    PA27_PWM_CH4 },
    {PORTA,  PIN28,   0,    5,    PA28_PWM_CH5 },
    {PORTA,  PIN29,   0,    6,    PA29_PWM_CH6 },
    {PORTA,  PIN30,   0,    7,    PA30_PWM_CH7 },
    {PORTA,  PIN31,   0,    8,    PA31_PWM_CH8 },
    {0xFFFFFFFFU, 0xFFFFFFFFU,   0xFFU, 0xFFU,       0xFFFFFFFFU },
};

const csi_pinmap_t gpio_pinmap[] = {
    {PORTA,   PIN0,           0,     0,           PIN_FUNC_GPIO },
    {PORTA,   PIN1,           0,     1,           PIN_FUNC_GPIO },
    {PORTA,   PIN2,           0,     2,           PIN_FUNC_GPIO },
    {PORTA,   PIN3,           0,     3,           PIN_FUNC_GPIO },
    {PORTA,   PIN4,           0,     4,           PIN_FUNC_GPIO },
    {PORTA,   PIN5,           0,     5,           PIN_FUNC_GPIO },
    {PORTA,   PIN6,           0,     6,           PIN_FUNC_GPIO },
    {PORTA,   PIN7,           0,     7,           PIN_FUNC_GPIO },
    {PORTA,   PIN8,           0,     8,           PIN_FUNC_GPIO },
    {PORTA,   PIN9,           0,     9,           PIN_FUNC_GPIO },
    {PORTA,   PIN10,          0,    10,           PIN_FUNC_GPIO },
    {PORTA,   PIN11,          0,    11,           PIN_FUNC_GPIO },
    {PORTA,   PIN12,          0,    12,           PIN_FUNC_GPIO },
    {PORTA,   PIN13,          0,    13,           PIN_FUNC_GPIO },
    {PORTA,   PIN14,          0,    14,           PIN_FUNC_GPIO },
    {PORTA,   PIN15,          0,    15,           PIN_FUNC_GPIO },
    {PORTA,   PIN16,          0,    16,           PIN_FUNC_GPIO },
    {PORTA,   PIN17,          0,    17,           PIN_FUNC_GPIO },
    {PORTA,   PIN18,          0,    18,           PIN_FUNC_GPIO },
    {PORTA,   PIN19,          0,    19,           PIN_FUNC_GPIO },
    {PORTA,   PIN20,          0,    20,           PIN_FUNC_GPIO },
    {PORTA,   PIN21,          0,    21,           PIN_FUNC_GPIO },
    {PORTA,   PIN22,          0,    22,           PIN_FUNC_GPIO },
    {PORTA,   PIN23,          0,    23,           PIN_FUNC_GPIO },
    {PORTA,   PIN24,          0,    24,           PIN_FUNC_GPIO },
    {PORTA,   PIN25,          0,    25,           PIN_FUNC_GPIO },
    {PORTA,   PIN26,          0,    26,           PIN_FUNC_GPIO },
    {PORTA,   PIN27,          0,    27,           PIN_FUNC_GPIO },
    {PORTA,   PIN28,          0,    28,           PIN_FUNC_GPIO },
    {PORTA,   PIN29,          0,    29,           PIN_FUNC_GPIO },
    {PORTA,   PIN30,          0,    30,           PIN_FUNC_GPIO },
    {PORTA,   PIN31,          0,    31,           PIN_FUNC_GPIO },
    {PORTB,   PIN0,           1,     0,           PIN_FUNC_GPIO },
    {PORTB,   PIN1,           1,     1,           PIN_FUNC_GPIO },
    {PORTB,   PIN2,           1,     2,           PIN_FUNC_GPIO },
    {PORTB,   PIN3,           1,     3,           PIN_FUNC_GPIO },
    {PORTB,   PIN4,           1,     4,           PIN_FUNC_GPIO },
    {PORTB,   PIN5,           1,     5,           PIN_FUNC_GPIO },
    {PORTB,   PIN6,           1,     6,           PIN_FUNC_GPIO },
    {PORTB,   PIN7,           1,     7,           PIN_FUNC_GPIO },
    {PORTB,   PIN8,           1,     8,           PIN_FUNC_GPIO },
    {PORTB,   PIN9,           1,     9,           PIN_FUNC_GPIO },
    {PORTB,   PIN10,          1,    10,           PIN_FUNC_GPIO },
    {PORTB,   PIN11,          1,    11,           PIN_FUNC_GPIO },
    {PORTB,   PIN12,          1,    12,           PIN_FUNC_GPIO },
    {PORTB,   PIN13,          1,    13,           PIN_FUNC_GPIO },
    {PORTB,   PIN14,          1,    14,           PIN_FUNC_GPIO },
    {PORTB,   PIN15,          1,    15,           PIN_FUNC_GPIO },
    {PORTB,   PIN16,          1,    16,           PIN_FUNC_GPIO },
    {PORTB,   PIN17,          1,    17,           PIN_FUNC_GPIO },
    {PORTB,   PIN18,          1,    18,           PIN_FUNC_GPIO },
    {PORTB,   PIN19,          1,    19,           PIN_FUNC_GPIO },
    {PORTB,   PIN20,          1,    20,           PIN_FUNC_GPIO },
    {PORTB,   PIN21,          1,    21,           PIN_FUNC_GPIO },
    {PORTB,   PIN22,          1,    22,           PIN_FUNC_GPIO },
    {PORTB,   PIN23,          1,    23,           PIN_FUNC_GPIO },
    {PORTB,   PIN24,          1,    24,           PIN_FUNC_GPIO },
    {PORTB,   PIN25,          1,    25,           PIN_FUNC_GPIO },
    {PORTB,   PIN26,          1,    26,           PIN_FUNC_GPIO },
    {PORTB,   PIN27,          1,    27,           PIN_FUNC_GPIO },
    {PORTB,   PIN28,          1,    28,           PIN_FUNC_GPIO },
    {PORTB,   PIN29,          1,    29,           PIN_FUNC_GPIO },
    {PORTB,   PIN30,          1,    30,           PIN_FUNC_GPIO },
    {PORTB,   PIN31,          1,    31,           PIN_FUNC_GPIO },
    {0xFFFFFFFFU,  0xFFFFFFFFU,   0xFFU, 0xFFU,       0xFFFFFFFFU   },
};

const csi_pinmap_t uart_pinmap[] = {
    {PORTA,    PIN0,             0,     PIN_UART_RX,    PA0_UART0_RX   },
    {PORTA,    PIN1,             0,     PIN_UART_TX,    PA1_UART0_TX   },
    {PORTA,    PIN4,             1,    PIN_UART_CTS,    PA4_UART1_CTS  },
    {PORTA,    PIN5,             1,    PIN_UART_RTS,    PA5_UART1_RTS  },
    {PORTA,    PIN6,             0,     PIN_UART_TX,    PA6_UART0_TX   },
    {PORTA,    PIN8,             0,     PIN_UART_RX,    PA8_UART0_RX   },
    {PORTA,    PIN10,            0,     PIN_UART_TX,    PA10_UART0_TX  },
    {PORTA,    PIN16,            1,     PIN_UART_TX,    PA16_UART1_TX  },
    {PORTA,    PIN17,            0,     PIN_UART_RX,    PA17_UART0_RX  },
    {PORTA,    PIN18,            0,     PIN_UART_TX,    PA18_UART0_TX  },
    {PORTA,    PIN19,            1,     PIN_UART_RX,    PA19_UART1_RX  },
    {PORTA,    PIN20,            1,     PIN_UART_TX,    PA20_UART1_TX  },
    {PORTA,    PIN21,            0,    PIN_UART_CTS,    PA21_UART0_CTS },
    {PORTA,    PIN21,            1,    PIN_UART_CTS,    PA21_UART1_CTS },
    {PORTA,    PIN22,            0,    PIN_UART_RTS,    PA22_UART0_RTS },
    {PORTA,    PIN22,            1,    PIN_UART_RTS,    PA22_UART1_RTS },
    {PORTA,    PIN23,            0,     PIN_UART_TX,    PA23_UART0_TX  },
    {PORTA,    PIN24,            0,     PIN_UART_RX,    PA24_UART0_RX  },
    {PORTA,    PIN25,            0,    PIN_UART_CTS,    PA25_UART0_CTS },
    {PORTA,    PIN26,            0,    PIN_UART_RTS,    PA26_UART0_RTS },
    {PORTA,    PIN27,            1,     PIN_UART_RX,    PA27_UART1_RX  },
    {PORTA,    PIN28,            1,     PIN_UART_TX,    PA28_UART1_TX  },
    {PORTA,    PIN29,            1,    PIN_UART_CTS,    PA29_UART1_CTS },
    {PORTA,    PIN30,            1,    PIN_UART_RTS,    PA30_UART1_RTS },
    {PORTA,    PIN31,            0,     PIN_UART_TX,    PA31_UART0_TX  },
    {PORTB,    PIN0,             0,     PIN_UART_DE,    PB0_UART0_DE  },
    {PORTB,    PIN1,             0,     PIN_UART_RE,    PB1_UART0_RE  },
    {PORTB,    PIN2,             1,     PIN_UART_DE,    PB2_UART1_DE  },
    {PORTB,    PIN3,             1,     PIN_UART_RE,    PB3_UART1_RE  },
    {PORTB,    PIN12,            2,     PIN_UART_RX,    PB12_UART2_RXD  },
    {PORTB,    PIN13,            2,     PIN_UART_TX,    PB13_UART2_TXD  },
    {PORTB,    PIN14,            2,     PIN_UART_CTS,    PB14_UART2_CTS_N  },
    {PORTB,    PIN15,            2,     PIN_UART_RTS,    PB15_UART2_RTS_N  },
    {PORTB,    PIN16,            2,     PIN_UART_DE,    PB16_UART2_DE  },
    {PORTB,    PIN17,            2,     PIN_UART_RE,    PB17_UART2_RE  },
    {PORTB,    PIN18,            3,     PIN_UART_RX,    PB18_UART3_RXD  },
    {PORTB,    PIN19,            3,     PIN_UART_TX,    PB19_UART3_TXD  },
    {PORTB,    PIN20,            3,     PIN_UART_CTS,    PB20_UART3_CTS_N  },
    {PORTB,    PIN21,            3,     PIN_UART_RTS,    PB21_UART3_RTS_N  },
    {0xFFFFFFFFU,  0xFFFFFFFFU, 0xFFU, 0xFFU,      0xFFFFFFFFU  },
};

const csi_pinmap_t iic_pinmap[] = {
    {PORTA,    PIN15,            0, PIN_IIC_SCL, PA15_IIC0_SCL},
    {PORTA,    PIN16,            0, PIN_IIC_SDA, PA16_IIC0_SDA},
    {0xFFFFFFFFU,   0xFFFFFFFFU, 0xFFU,       0xFFU,   0xFFFFFFFFU},
};


const csi_pinmap_t spi_pinmap[] = {
    {PORTA,    PIN0,             0,   PIN_SPI_CS,    PA0_SPI0_CS},
    {PORTA,    PIN1,             0,  PIN_SPI_SCK,   PA1_SPI0_SCK},
    {PORTA,    PIN23,            0,   PIN_SPI_CS,   PA23_SPI0_CS},
    {PORTA,    PIN24,            0,  PIN_SPI_SCK,  PA24_SPI0_SCK},
    {PORTA,    PIN25,            0, PIN_SPI_MOSI, PA25_SPI0_MOSI},
    {PORTA,    PIN26,            0, PIN_SPI_MISO, PA26_SPI0_MISO},
//    {PORTA,  PIN27,            1,   PIN_SPI_CS,   PA27_SPI1_CS},
//    {PORTA,  PIN28,            1,  PIN_SPI_SCK,  PA28_SPI1_SCK},
//    {PORTA,  PIN29,            1, PIN_SPI_MOSI, PA29_SPI1_MOSI},
//    {PORTA,  PIN30,            1, PIN_SPI_MISO, PA30_SPI1_MISO},
    {0xFFFFFFFFU,  0xFFFFFFFFU, 0xFFU,        0xFFU,    0xFFFFFFFFU},
};

const csi_pinmap_t i2s_pinmap[] = {
    {PORTA,    PIN2,             1,      PIN_I2S_SCLK, PA2_I2S1_SCLK },
    {PORTA,    PIN3,             1,     PIN_I2S_WSCLK, PA3_I2S1_WSCLK},
    {PORTA,    PIN5,             1,       PIN_I2S_SDA, PA5_I2S1_SDA  },
    {PORTA,    PIN6,             0,      PIN_I2S_SCLK, PA6_I2S0_SCLK },
    {PORTA,    PIN7,             0,     PIN_I2S_WSCLK, PA7_I2S0_WSCLK},
    {PORTA,    PIN8,             0,       PIN_I2S_SDA, PA8_I2S0_SDA  },
    {PORTA,    PIN10,            0,      PIN_I2S_MCLK, PA10_I2S0_MCLK},
    {PORTA,    PIN26,            1,      PIN_I2S_MCLK, PA26_I2S1_MCLK},
    {0xFFFFFFFFU,  0xFFFFFFFFU, 0xFFU, 0xFFU, 0xFFFFFFFFU   },
};

// const csi_clkmap_t clk_map[] = {
//     {TIM0_CLK,   DEV_DW_TIMER_TAG,      0},
//     {TIM1_CLK,   DEV_DW_TIMER_TAG,      1},
//     {RTC0_CLK,   DEV_WJ_RTC_TAG,        0},
//     {WDT_CLK,    DEV_DW_WDT_TAG,        0},
//     {SPI0_CLK,   DEV_DW_SPI_TAG,        0},
//     {UART0_CLK,  DEV_DW_UART_TAG,       0},
//     {IIC0_CLK,   DEV_DW_IIC_TAG,        0},
//     {PWM_CLK,    DEV_WJ_PWM_TAG,        0},
//     {QSPI0_CLK,  DEV_CD_QSPI_TAG,       0},
//     {PWMR_CLK,   DEV_BLANK_TAG,         0},
//     {EFUSE_CLK,  DEV_WJ_EFUSE_TAG,      0},
//     {I2S0_CLK,   DEV_WJ_I2S_TAG,        0},
//     {I2S1_CLK,   DEV_WJ_I2S_TAG,        1},
//     {GPIO0_CLK,  DEV_DW_GPIO_TAG,       0},
//     {TIM2_CLK,   DEV_DW_TIMER_TAG,      2},
//     {TIM3_CLK,   DEV_DW_TIMER_TAG,      3},
//     {SPI1_CLK,   DEV_DW_SPI_TAG,        1},
//     {UART1_CLK,  DEV_DW_UART_TAG,       1},
//     {I2S567_CLK, DEV_WJ_I2S_TAG,        5},
//     {I2S567_CLK, DEV_WJ_I2S_TAG,        6},
//     {I2S567_CLK, DEV_WJ_I2S_TAG,        7},
//     {ADC_CLK,    DEV_WJ_ADC_TAG,        0},
//     {ETB_CLK,    DEV_WJ_ETB_TAG,        0},
//     {I2S2_CLK,   DEV_WJ_I2S_TAG,        2},
//     {IOC_CLK,    DEV_BLANK_TAG,         0},
//     {CODEC_CLK,  DEV_RCHBAND_CODEC_TAG, 0},
//     {0xFFFFFFFFU, 0xFFFFU,          0xFFU}
// };


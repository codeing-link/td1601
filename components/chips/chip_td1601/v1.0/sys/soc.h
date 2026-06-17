/*
 * Copyright (C) 2017-2020 NStar-Chip Holding Limited
 */

/******************************************************************************
 * @file     soc.h
 * @brief    CSI Core Peripheral Access Layer Header File for
 *           CSKYSOC Device Series
 * @version  V1.0
 * @date     7. April 2020
 ******************************************************************************/

#ifndef _SOC_H_
#define _SOC_H_

#include <stdint.h>
#include <csi_core.h>
#include <sys_clk.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EHS_VALUE
#define EHS_VALUE               48000000U
#endif

#ifndef ELS_VALUE
#define ELS_VALUE               32768U
#endif

#ifndef IHS_VALUE
#define IHS_VALUE               48000000U
#endif

#ifndef ILS_VALUE
#define ILS_VALUE               32768U
#endif

typedef enum {
    User_Software_IRQn             =   0U,      /* User software interrupt */
    Supervisor_Software_IRQn       =   1U,      /* Supervisor software interrupt */
    Machine_Software_IRQn          =   3U,      /* Machine software interrupt */
    User_Timer_IRQn                =   4U,      /* User timer interrupt */
    Supervisor_Timer_IRQn          =   5U,      /* Supervisor timer interrupt */
    CORET_IRQn                     =   7U,      /* core Timer Interrupt */
    Machine_External_IRQn          =   11U,     /* Machine external interrupt */
    DW_GPIO_IRQn                   =  16U + 0U,
    DW_TIMER0_IRQn                 =  16U + 2U,
    DW_TIMER1_IRQn                 =  16U + 3U,
    WJ_ADC1_IRQn                   =  16U + 4U,
    WJ_TDM_IRQn                    =  16U + 4U,
    DW_WDT_IRQn                    =  16U + 5U,
    DW_UART0_IRQn                  =  16U + 6U,
    DW_UART1_IRQn                  =  16U + 7U,
    DW_IIC_IRQn                    =  16U + 9U,
    NXP_CAN_IRQn                   =  16U + 10U,
    DW_OSPI0_IRQn                   = 16U + 11U,
    DW_SPI0_IRQn                   = 16U + 12U,
    WJ_RTC_IRQn                    = 16U + 13U,
    WJ_I2S0_IRQn                   = 16U + 14U,
    WJ_ADC_IRQn                    = 16U + 15U,
    WJ_I2S1_IRQn                   = 16U + 16U,
    DW_DMA_IRQn                    = 16U + 17U,
    WJ_PMU_IRQn                    = 16U + 18U,
    WJ_DMA_IRQn                    = 16U + 18U,
    WJ_PWM_IRQn                    = 16U + 19U,
    DW_UART2_IRQn                   = 16U + 20U,
    DW_GPIO1_IRQn                   = 16U + 21U,
    DW_UART3_IRQn                   = 16U + 22U,
    DW_TIMER2_IRQn                 = 16U + 23U,
    DW_TIMER3_IRQn                 = 16U + 24U,
    CD_QSPI_IRQn                   = 16U + 25U,
    DW_IIC1_IRQn                   = 16U + 25U,
    DW_TIMER4_IRQn                 = 16U + 26U,
    DW_TIMER5_IRQn                 = 16U + 27U,
    DW_TIMER6_IRQn                 = 16U + 28U,
    DW_TIMER7_IRQn                 = 16U + 29U,
    WJ_PMU_WAKEUP_IRQn             = 16U + 31U,
} irqn_type_t;

typedef enum {
    DW_GPIO_Wakeupn                =    0U,     /* GPIO0 wakeup */
    DW_TIM0_Wakeupn                =    1U,     /* TIM0 wakeup */
    DW_TIM1_Wakeupn                =    2U,     /* TIM1 wakeup */
    WJ_WDT_Wakeupn                 =    3U,     /* WDT wakeup */
    DW_UART0_Wakeupn               =    4U,     /* UART0 wakeup */
    DW_UART1_Wakeupn               =    5U,     /* UART1 wakeup */
    DW_IIC_Wakeupn                 =    7U,     /* I2C0 wakeup */
    DW_SPI1_Wakeupn                =    9U,     /* SPI1 wakeup */
    DW_SPI0_Wakeupn                =   10U,     /* SPI0 wakeup */
    WJ_RTC_Wakeupn                 =   11U,     /* RTC0 wakeup */
    WJ_I2S0_Wakeupn                =   12U,     /* I2S0 wakeup*/
    WJ_ADC_Wakeupn                 =   13U,     /* ADC wakeup */
    WJ_I2S1_Wakeupn                =   14U,     /* I2S1 wakeup*/
    DW_DMA_Wakeupn                 =   15U,     /* DMA0 wakeup */
    WJ_PWM_Wakeupn                 =   16U,     /* PWM wakeup */
    WJ_GPIO1_Wakeupn                =   18U,     /* I2S2 wakeup*/
    WJ_UART2_Wakeupn                =   19U,     /* I2S3 wakeup*/
    DW_TIM2_Wakeupn                =   20U,     /* TIM2 wakeup */
    DW_TIM3_Wakeupn                =   21U,     /* TIM3 wakeup */
    DW_TIM4_Wakeupn                =   23U,     /* TIM4 wakeup */
    DW_TIM5_Wakeupn                =   24U,     /* TIM5 wakeup */
    DW_TIM6_Wakeupn                =   25U,     /* TIM6 wakeup */
    DW_TIM7_Wakeupn                =   26U,     /* TIM7 wakeup */
    WJ_HAD_Wakeupn                 =   28U,     /* HAD wakeup */
    WJ_IOCTL_Wakeupn               =   29U,     /* IOCTOL wakeup */
    WJ_UART3_Wakeupn               =   30U,
} wakeupn_type_t;

typedef enum {
    DW_TIM0_CLK_MANAGERN           =  0U,
    DW_TIM1_CLK_MANAGERN           =  1U,
    DW_RTC_CLK_MANAGERN            =  2U,
    DW_RTC1_CLK_MANAGERN           =  2U,
    DW_WDT_CLK_MANAGERN            =  3U,
    DW_SPI0_CLK_MANAGERN           =  4U,
    DW_UART0_CLK_MANAGERN          =  5U,
    DW_UART2_CLK_MANAGERN          =  5U,
    DW_IIC0_CLK_MANAGERN           =  6U,
    WJ_PWM_CLK_MANAGERN            =  7U,
    WJ_QSPIC0_CLK_MANAGERN         =  8U,
    WJ_PWMR_CLK_MANAGERN           =  9U,
    WJ_EFUSE_CLK_MANAGERN          = 10U,
    WJ_I2S0_CLK_MANAGERN           = 11U,
    WJ_I2S1_CLK_MANAGERN           = 12U,
    DW_GPIO0_CLK_MANAGERN          = 13U,
    DW_GPIO1_CLK_MANAGERN          = 13U,
    DW_TIM2_CLK_MANAGERN           = 14U,
    DW_TIM3_CLK_MANAGERN           = 15U,
    DW_SPI1_CLK_MANAGERN           = 16U,
    DW_UART1_CLK_MANAGERN          = 17U,
    DW_I2C1_CLK_MANAGERN           = 18U,
    WJ_ADC_CLK_MANAGERN            = 19U,
    WJ_ETB_CLK_MANAGERN            = 20U,
    WJ_I2S2_CLK_MANAGERN           = 21U,
    WJ_I2S3_CLK_MANAGERN           = 22U,
    WJ_IOCTRL_CLK_MANAGERN         = 23U,
    WJ_CODEC_CLK_MANAGERN          = 24U,
    DW_DMA0_CLK_MANAGERN           = 27U,
    DW_DMA1_CLK_MANAGERN           = 27U,
    WJ_USB_CLK_MANAGERN            = 28U,
} clk_manager_type_t;

typedef enum {
    DW_UART0_RX_DMAN               = 0U,
    DW_UART0_TX_DMAN               = 1U,
    DW_UART1_RX_DMAN               = 2U,
    DW_UART1_TX_DMAN               = 3U,
    MEMORY_DMAN                    = 4U,
} dman_type_t;


typedef enum {
    PIN0    = (1 << 0),
    PIN1    = (1 << 1),
    PIN2    = (1 << 2),
    PIN3    = (1 << 3),
    PIN4    = (1 << 4),
    PIN5    = (1 << 5),
    PIN6    = (1 << 6),
    PIN7    = (1 << 7),
    PIN8    = (1 << 8),
    PIN9    = (1 << 9),
    PIN10   = (1 << 10),
    PIN11   = (1 << 11),
    PIN12   = (1 << 12),
    PIN13   = (1 << 13),
    PIN14   = (1 << 14),
    PIN15   = (1 << 15),
    PIN16   = (1 << 16),
    PIN17   = (1 << 17),
    PIN18   = (1 << 18),
    PIN19   = (1 << 19),
    PIN20   = (1 << 20),
    PIN21   = (1 << 21),
    PIN22   = (1 << 22),
    PIN23   = (1 << 23),
    PIN24   = (1 << 24),
    PIN25   = (1 << 25),
    PIN26   = (1 << 26),
    PIN27   = (1 << 27),
    PIN28   = (1 << 28),
    PIN29   = (1 << 29),
    PIN30   = (1 << 30),
    PIN31   = (1 << 31),
} pin_name_t;

typedef enum {
    PORTA,
    PORTB,
} port_name_t;


typedef enum {
    PA0_I2S0_SCLK                  =  0U,
    PA0_SPI0_CS                    =  1U,
    PA0_UART0_RX                   =  2U,
    PA0_PWM_CH0                    =  3U,
    PA0_ADC_VREFP                  =  5U,
    PA1_I2S0_WSCLK                 =  0U,
    PA1_SPI0_SCK                   =  1U,
    PA1_UART0_TX                   =  2U,
    PA1_PWM_CH1                    =  3U,
    PA1_ADC_VREFN                  =  6U,
    PA2_I2S1_SCLK                  =  0U,
    PA2_IIC0_SCL                   =  1U,
    PA2_SPI1_CS                    =  2U,
    PA2_PWM_CH2                    =  3U,
    PA3_I2S1_WSCLK                 =  0U,
    PA3_IIC0_SDA                   =  1U,
    PA3_SPI1_SCK                   =  2U,
    PA3_PWM_CH3                    =  3U,
    PA3_ADC_A1                     =  8U,
    PA4_I2S0_SDA                   =  0U,
    PA4_SPI0_MOSI                  =  1U,
    PA4_UART1_CTS                  =  2U,
    PA4_PWM_CH4                    =  3U,
    PA4_ADC_A2                     =  9U,
    PA5_I2S1_SDA                   =  0U,
    PA5_SPI0_MISO                  =  1U,
    PA5_UART1_RTS                  =  2U,
    PA5_PWM_CH5                    =  3U,
    PA5_ADC_A3                     = 10U,
    PA6_I2S0_SCLK                  =  0U,
    PA6_UART0_TX                   =  1U,
    PA6_PWM_CH6                    =  3U,
    PA6_ADC_A4                     = 11U,
    PA7_I2S0_WSCLK                 =  0U,
    PA7_PWM_CH7                    =  3U,
    PA7_ADC_A5                     = 12U,
    PA8_I2S0_SDA                   =  0U,
    PA8_IIC0_SCL                   =  1U,
    PA8_UART0_RX                   =  2U,
    PA8_PWM_CH8                    =  3U,
    PA8_ADC_A6                     = 13U,
    PA9_I2S1_SDA                   =  0U,
    PA9_IIC0_SDA                   =  1U,
    PA9_PWM_CH9                    =  3U,
    PA9_ADC_A7                     = 14U,
    PA10_I2S0_MCLK                 =  0U,
    PA10_UART0_TX                  =  1U,
    PA15_IIC0_SCL                  =  0U,
    PA15_SPI0_CS                   =  1U,
    PA15_PWM_CH4                   =  3U,
    PA15_ADC_A9                    = 20U,
    PA16_IIC0_SDA                  =  0U,
    PA16_SPI0_SCK                  =  1U,
    PA16_UART1_TX                  =  2U,
    PA16_PWM_CH5                   =  3U,
    PA16_ADC_A10                   = 21U,
    PA17_UART0_RX                  =  0U,
    PA17_SPI0_MOSI                 =  1U,
    PA17_I2S0_SCLK                 =  2U,
    PA17_PWM_CH10                  =  3U,
    PA17_ADC_A11                   = 22U,
    PA18_UART0_TX                  =  0U,
    PA18_SPI0_MISO                 =  1U,
    PA18_I2S0_WSCLK                =  2U,
    PA18_PWM_CH11                  =  3U,
    PA18_ADC_A13                   = 23U,
    PA19_JTAG_TMS                  =  0U,
    PA19_UART1_RX                  =  1U,
    PA19_I2S1_SCLK                 =  2U,
    PA19_IIC0_SCL                  =  3U,
    PA19_ADC_A8                    = 24U,
    PA20_JTAG_TCK                  =  0U,
    PA20_UART1_TX                  =  1U,
    PA20_I2S1_WSCLK                =  2U,
    PA20_IIC0_SDA                  =  3U,
    PA21_UART0_CTS                 =  0U,
    PA21_UART1_CTS                 =  1U,
    PA21_I2S0_SDA                  =  2U,
    PA21_IIC0_SCL                  =  3U,
    PA22_UART0_RTS                 =  0U,
    PA22_UART1_RTS                 =  1U,
    PA22_I2S1_SDA                  =  2U,
    PA22_IIC0_SDA                  =  3U,
    PA23_IIC0_SCL                  =  0U,
    PA23_UART0_TX                  =  1U,
    PA23_PWM_CH0                   =  2U,
    PA23_SPI0_CS                   =  3U,
    PA24_IIC0_SDA                  =  0U,
    PA24_UART0_RX                  =  1U,
    PA24_PWM_CH1                   =  2U,
    PA24_SPI0_SCK                  =  3U,
    PA25_UART0_CTS                 =  1U,
    PA25_PWM_CH2                   =  2U,
    PA25_SPI0_MOSI                 =  3U,
    PA26_I2S1_MCLK                 =  0U,
    PA26_UART0_RTS                 =  1U,
    PA26_PWM_CH3                   =  2U,
    PA26_SPI0_MISO                 =  3U,
    PA27_I2S0_SCLK                 =  0U,
    PA27_UART1_RX                  =  1U,
    PA27_PWM_CH4                   =  2U,
    PA27_SPI1_CS                   =  3U,
    PA28_I2S0_WSCLK                =  0U,
    PA28_UART1_TX                  =  1U,
    PA28_PWM_CH5                   =  2U,
    PA28_SPI1_SCK                  =  3U,
    PA29_I2S1_SCLK                 =  0U,
    PA29_UART1_CTS                 =  1U,
    PA29_PWM_CH6                   =  2U,
    PA30_I2S1_WSCLK                =  0U,
    PA30_UART1_RTS                 =  1U,
    PA30_PWM_CH7                   =  2U,
    PA31_I2S0_SDA                  =  0U,
    PA31_PWM_CH8                   =  2U,
    PA31_UART0_TX                  =  3U,

    PB0_UART0_DE                   =  0U,
    PB1_UART0_RE                   =  0U,
    PB2_UART1_DE                   =  0U,
    PB2_ADC1_A15                   =  7U,
    PB3_UART1_RE                   =  0U,
    PB4_SPI1_D0                    =  0U,
    PB4_ADC1_A7                    =  9U,
    PB5_SPI1_D1                    =  0U,
    PB5_ADC1_A9                    =  10U,
    PB6_SPI1_D2                    =  0U,
    PB6_ADC1_A10                    =  11U,
    PB7_SPI1_D3                    =  0U,
    PB7_ADC1_A11                   =  12U,
    PB8_SPI1_D4                    =  0U,
    PB8_ADC1_A12                   =  13U,
    PB9_SPI1_D5                    =  0U,
    PB10_SPI1_D6                   =  0U,
    PB11_SPI1_D7                   =  0U,
    PB12_UART2_RXD                 =  0U,
    PB13_UART2_TXD                 =  0U,
    PB13_ADC1_A8                  =  18U,
    PB14_UART2_CTS_N               =  0U,
    PB14_ADC_A14                   =  19U,
    PB15_UART2_RTS_N               =  0U,
    PB15_ADC_A12                   =  20U,
    PB16_UART2_DE                  =  0U,
    PB17_UART2_RE                  =  0U,
    PB18_UART3_RXD                 =  0U,
    PB18_ADC1_A13                  =  23U,
    PB19_UART3_TXD                 =  0U,
    PB19_ADC1_A14                  =  24U,
    PB20_UART3_CTS_N               =  0U,
    PB21_UART3_RTS_N               =  0U,
    PB21_ADC_A15                   =  26U,
    PB22_TDM_SCLKOUT               =  0U,
    PB22_ADC1_A1                   =  27U,
    PB23_TDM_WSOUT                 =  0U,
    PB23_ADC1_A2                   =  28U,
    PB24_IIC1_SCL                  =  0U,
    PB25_IIC1_SDA                  =  0U,
    PB26_CAN_RX                    =  0U,
    PB27_CAN_TX                    =  0U,
    PB28_TDM_D0                    =  0U,
    PB28_ADC1_A3                   =  33U,
    PB29_TDM_D1                    =  0U,
    PB29_ADC1_A4                   =  34U,
    PB30_TDM_D2                    =  0U,
    PB30_ADC1_A5                   =  35U,
    PB31_TDM_D3                    =  0U,
    PB31_ADC1_A6                   =  36U,
    PIN_FUNC_GPIO                  =  4U,
} pin_func_t;

#define CONFIG_GPIO_NUM             2U
#define CONFIG_IRQ_NUM              32U + 16U

///< AHB
#define SPIFLASH_BASE               0x18000000UL
#define SPIFLASH_SIZE               0x800000U

#define SRAM_BASE                   0x20000000UL
#define SRAM_SIZE                   0x10000U

#define WJ_PMU_BASE                 0x40000000UL
#define WJ_PMU_SIZE                 0x1000U

#define DW_DMA_BASE                 0x40001000UL
#define DW_DMA_SIZE                 0x1000U

#define WJ_DMA_BASE                 0x80000000UL
#define WJ_DMA_SIZE                 0x1000U

///< APB0
#define DW_TIMER0_BASE              0x50000000UL
#define DW_TIMER0_SIZE              0x14U
#define DW_TIMER1_BASE              (DW_TIMER0_BASE+DW_TIMER0_SIZE)
#define DW_TIMER1_SIZE              DW_TIMER0_SIZE

#define DW_TIMER2_BASE              0x50000400UL
#define DW_TIMER2_SIZE              0x14U
#define DW_TIMER3_BASE              (DW_TIMER2_BASE+DW_TIMER2_SIZE)
#define DW_TIMER3_SIZE              DW_TIMER2_SIZE

#define WJ_RTC_BASE                 0x50004000UL
#define WJ_RTC_SIZE                 0x400U

#define DW_WDT_BASE                 0x50008000UL
#define DW_WDT_SIZE                 0x400U

#define DW_SPI0_BASE                0x5000C000UL
#define DW_SPI0_SIZE                0x400U

#define DW_UART0_BASE               0x50010000UL
#define DW_UART0_SIZE               0x1000U

#define DW_IIC_BASE                 0x50014000UL
#define DW_IIC_SIZE                 0x1000U

#define DW_IIC1_BASE                0x60014000UL
#define DW_IIC1_SIZE                0x1000U

#define DW_GPIO_BASE                0x50018000UL
#define DW_GPIO_SIZE                0x400U

#define DW_GPIO1_BASE               0x60018000UL
#define DW_GPIO1_SIZE               0x400U

#define WJ_PWM_BASE                 0x5001C000UL
#define WJ_PWM_SIZE                 0x400U

#define CD_QSPI_BASE                0x5002C000UL
#define CD_QSPI_SIZE                0x400U

#define WJ_I2S0_BASE                0x50040000UL
#define WJ_I2S0_SIZE                0x400U

#define WJ_I2S1_BASE                0x50040400UL
#define WJ_I2S1_SIZE                0x400U

#define WJ_EFUSE_BASE               0x5003C000UL
#define WJ_EFUSE_SIZE               0x400U

///< APB1
#define DW_TIMER4_BASE              0x60000000UL
#define DW_TIMER4_SIZE              0x14U
#define DW_TIMER5_BASE              (DW_TIMER4_BASE + DW_TIMER4_SIZE)
#define DW_TIMER5_SIZE              DW_TIMER4_SIZE

#define DW_TIMER6_BASE              0x60000400UL
#define DW_TIMER6_SIZE              0x14U
#define DW_TIMER7_BASE              (DW_TIMER6_BASE + DW_TIMER6_SIZE)
#define DW_TIMER7_SIZE              0x14U

#define DW_OSPI0_BASE                0x6000C000UL
#define DW_OSPI0_SIZE                0x400U

#define DW_UART1_BASE               0x60010000UL
#define DW_UART1_SIZE               0x1000U

#define DW_UART2_BASE               0x50038000UL
#define DW_UART2_SIZE               0x1000U

#define DW_UART3_BASE               0x50043C00UL
#define DW_UART3_SIZE               0x1000U

#define WJ_ADC_BASE                 0x60020000UL
#define WJ_ADC_SIZE                 0x400U

#define WJ_ADC1_BASE                 0x60040000UL
#define WJ_ADC1_SIZE                 0x400U

#define WJ_ETB_BASE                 0x60024000UL
#define WJ_ETB_SIZE                 0x1000U

#define WJ_IOC_BASE                 0x60030000UL
#define WJ_IOC_SIZE                 0x1000U

#define WJ_TDM_BASE                 0x60044000UL
#define WJ_TDM_SIZE                 0x400U

#define NXP_CAN0_BASE               0x60048000U
#define NXP_FLEXCAN_0_SYSREG_BASE   0x4000051CU

#define CAN_BUF0_BASE_ADDR          0x60048080
#define CAN_BUF0_SIZE               0x800
#define CAN_BUF1_BASE_ADDR          0x60049000
#define CAN_BUF1_SIZE               0x800
#define CAN_BUF2_BASE_ADDR          0x6004a050
#define CAN_BUF2_SIZE               0x9B0

#define CONFIG_RTC_FAMILY_D

#ifdef __cplusplus
}
#endif

#endif  /* _SOC_H_ */

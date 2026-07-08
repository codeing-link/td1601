

/******************************************************************************
 * @file     board_config.h
 * @brief    header File for pin definition
 * @version  V1.0
 * @date     02. June 2018
 ******************************************************************************/

#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <stdint.h>
#include "soc.h"

#ifdef __cplusplus
extern "C" {
#endif
#define CONSOLE_TXD_PORT               PORTA
#define CONSOLE_RXD_PORT               PORTA
#define CONSOLE_IDX                    0
#define CONSOLE_TXD                    PIN18
#define CONSOLE_RXD                    PIN17
#define CONSOLE_TXD_FUNC               PA18_UART0_TX
#define CONSOLE_RXD_FUNC               PA17_UART0_RX
#define CLOCK_GETTIME_USE_TIMER_ID 0

#define WAKEUP_PIN_PORT                 PORTA
#define WAKEUP_PIN                      PIN4
#define WAKEUP_PIN_FUNC                 PIN_FUNC_GPIO
#define EXAMPLE_WAKEUP_NUM              WJ_IOCTL_Wakeupn
#define EXAMPLE_BOARD_WAKEUP_PIN_NAME   "A4"
/* example pin manager */
#define EXAMPLE_UART_IDX                0
#define EXAMPLE_PIN_UART_TX             PIN18           /* board X7A: TXD0 */
#define EXAMPLE_PIN_UART_RX             PIN17            /* board X7A: RXD0 */
#define EXAMPLE_PIN_UART_TX_FUNC        PA18_UART0_TX
#define EXAMPLE_PIN_UART_RX_FUNC        PA17_UART0_RX

#define EXAMPLE_WAKEUP_RTC_IDX          0
#define RTC_WAKEUP_NUM                  RTC_Wakeupn

#define EXAMPLE_WAKEUP_TIMER_IDX        0
#define TIMER_WAKEUP_NUM                TIM0_0_Wakeupn

#define EXAMPLE_UART_PIN_PORT                PORTA
#define EXAMPLE_UART_IDX1                1
#define EXAMPLE_PIN_UART_TX1_PORT        PORTA
#define EXAMPLE_PIN_UART_TX1             PIN28           /* board X7A: TXD1 */
#define EXAMPLE_PIN_UART_RX1_PORT        PORTA
#define EXAMPLE_PIN_UART_RX1             PIN27            /* board X7A: RXD1 */
#define EXAMPLE_PIN_UART_RTS1_PORT       PORTA
#define EXAMPLE_PIN_UART_RTS1            PIN5
#define EXAMPLE_PIN_UART_CTS1_PORT        PORTA
#define EXAMPLE_PIN_UART_CTS1            PIN4
#define EXAMPLE_PIN_UART_RTS_FUNC1       PA5_UART1_RTS
#define EXAMPLE_PIN_UART_CTS_FUNC1       PA4_UART1_CTS
#define EXAMPLE_PIN_UART_TX_FUNC1        PA28_UART1_TX
#define EXAMPLE_PIN_UART_RX_FUNC1        PA27_UART1_RX

#define EXAMPLE_UART_IDX2                2
#define EXAMPLE_PIN_UART_TX2_PORT        PORTB
#define EXAMPLE_PIN_UART_TX2             PIN13           /* board X7A: TXD2 */
#define EXAMPLE_PIN_UART_RX2_PORT        PORTB
#define EXAMPLE_PIN_UART_RX2             PIN12            /* board X7A: RXD2 */
#define EXAMPLE_PIN_UART_TX_FUNC2        PB13_UART2_TXD
#define EXAMPLE_PIN_UART_RX_FUNC2        PB12_UART2_RXD

#define EXAMPLE_UART_BLE_IDX             1
#define EXAMPLE_PIN_UART_BLE_TX             PIN28           /* board X7A: TXD2 */
#define EXAMPLE_PIN_UART_BLE_RX             PIN27            /* board X7A: RXD2 */
#define EXAMPLE_PIN_UART_BLE_TX_FUNC        PA28_UART1_TX
#define EXAMPLE_PIN_UART_BLE_RX_FUNC        PA27_UART1_RX

/* example dma */
#define EXAMPLE_DMAC_CTRL_IDX           0

// #define EXAMPLE_SPI_IDX                 0
// #define EXAMPLE_PIN_SPI_MOSI            PA1     /* board X7A: MOSI */
// #define EXAMPLE_PIN_SPI_MISO            PA2   /* board X7A: MISO */
// #define EXAMPLE_PIN_SPI_CS              PA3     /* board X7A: CS */
// #define EXAMPLE_PIN_SPI_SCK             PA0     /* board X7A: SCK */
// #define EXAMPLE_PIN_SPI_MOSI_FUNC       PA1_SPI0_MOSI
// #define EXAMPLE_PIN_SPI_MISO_FUNC       PA2_SPI0_MISO
// #define EXAMPLE_PIN_SPI_CS_FUNC         PA3_SPI0_CS0
// #define EXAMPLE_PIN_SPI_SOFT_CS_FUNC    PIN_FUNC_GPIO
// #define EXAMPLE_PIN_SPI_SCK_FUNC        PA0_SPI0_CLK

#define EXAMPLE_SPI_IDX                 0
#define EXAMPLE_PIN_SPI_MOSI_PORT       PORTA
#define EXAMPLE_PIN_SPI_MOSI            PIN25     /* board X7A: MOSI */
#define EXAMPLE_PIN_SPI_MISO_PORT       PORTA
#define EXAMPLE_PIN_SPI_MISO            PIN26   /* board X7A: MISO */
#define EXAMPLE_PIN_SPI_CS_PORT         PORTA
#define EXAMPLE_PIN_SPI_CS              PIN23     /* board X7A: CS */
#define EXAMPLE_PIN_SPI_SCK_PORT        PORTA
#define EXAMPLE_PIN_SPI_SCK             PIN24     /* board X7A: SCK */
#define EXAMPLE_PIN_SPI_MOSI_FUNC       PA25_SPI0_MOSI
#define EXAMPLE_PIN_SPI_MISO_FUNC       PA26_SPI0_MISO
#define EXAMPLE_PIN_SPI_CS_FUNC         PA23_SPI0_CS

#define EXAMPLE_PIN_SPI_SOFT_CS_FUNC    PIN_FUNC_GPIO
#define EXAMPLE_PIN_SPI_SCK_FUNC        PA24_SPI0_SCK

#define EXAMPLE_SPI_SLAVE_IDX           0
#define EXAMPLE_PIN_SPI_SLAVE_MOSI      PIN25    /* board X7A: A17 */
#define EXAMPLE_PIN_SPI_SLAVE_MISO      PIN26     /* board X7A: CH0 */
#define EXAMPLE_PIN_SPI_SLAVE_CS        PIN23     /* board X7A: A15 */
#define EXAMPLE_PIN_SPI_SLAVE_SCK       PIN24    /* board X7A: A16 */
#define EXAMPLE_PIN_SPI_SLAVE_MOSI_FUNC PA25_SPI0_MOSI
#define EXAMPLE_PIN_SPI_SLAVE_MISO_FUNC PA26_SPI0_MISO
#define EXAMPLE_PIN_SPI_SLAVE_CS_FUNC   PA23_SPI0_CS
#define EXAMPLE_PIN_SPI_SLAVE_SCK_FUNC  PA24_SPI0_SCK

#define EXAMPLE_PIN_IIC_SLAVE_SDA_PORT        PORTB
#define EXAMPLE_PIN_IIC_SLAVE_SCL_PORT        PORTB
#define EXAMPLE_IIC_SLAVE_IDX           1
#define EXAMPLE_PIN_IIC_SLAVE_SDA             PIN25
#define EXAMPLE_PIN_IIC_SLAVE_SCL             PIN24
#define EXAMPLE_PIN_IIC_SLAVE_SDA_FUNC        PB25_IIC1_SDA
#define EXAMPLE_PIN_IIC_SLAVE_SCL_FUNC        PB24_IIC1_SCL

#define EXAMPLE_PIN_IIC_SDA_PORT        PORTA
#define EXAMPLE_PIN_IIC_SCL_PORT        PORTA
#define EXAMPLE_IIC_IDX                 0
#define EXAMPLE_PIN_IIC_SDA             PIN16
#define EXAMPLE_PIN_IIC_SCL             PIN15
#define EXAMPLE_PIN_IIC_SDA_FUNC        PA16_IIC0_SDA
#define EXAMPLE_PIN_IIC_SCL_FUNC        PA15_IIC0_SCL

#define EXAMPLE_GPIO_PIN                PIN0
#define EXAMPLE_BOARD_GPIO_PIN_NAME     "MOSI"
#define EXAMPLE_GPIO_PIN_FUNC           PIN_FUNC_GPIO

#define EXAMPLE_TOGGLE_GPIO_PIN_PORT    PORTB
#define EXAMPLE_TOGGLE_GPIO_PIN         PIN1
#define EXAMPLE_TOGGLE_GPIO_PIN1        PIN2
#define EXAMPLE_TOGGLE_GPIO_PIN_FUNC    PIN_FUNC_GPIO
#define EXAMPLE_TOGGLE_GPIO_PIN1_FUNC   PIN_FUNC_GPIO
#define EXAMPLE_TOGGLE_GPIO_IDX         PORTB
#define EXAMPLE_TOGGLE_GPIO_PIN_MASK   ( (EXAMPLE_TOGGLE_GPIO_PIN) | (EXAMPLE_TOGGLE_GPIO_PIN1) )

#define EXAMPLE_INTR_GPIO_PIN_PORT           PORTB
#define EXAMPLE_INTR_GPIO_PIN1_PORT          PORTB
#define EXAMPLE_INTR_GPIO_PIN           PIN4
#define EXAMPLE_INTR_GPIO_PIN1          PIN5
#define EXAMPLE_INTR_GPIO_PIN_FUNC      PIN_FUNC_GPIO
#define EXAMPLE_INTR_GPIO_PIN1_FUNC     PIN_FUNC_GPIO
#define EXAMPLE_INTR_GPIO_IDX           PORTB
#define EXAMPLE_INTR_GPIO_PIN_NAME      "PB4"
#define EXAMPLE_INTR_GPIO_PIN1_NAME     "PB5"
#define EXAMPLE_INTR_GPIO_PIN_MASK    ( (EXAMPLE_INTR_GPIO_PIN) | (EXAMPLE_INTR_GPIO_PIN1) )

#define EXAMPLE_PWM_IDX                 0
#define EXAMPLE_PWM_CH_IDX              4
#define EXAMPLE_PWM_CH_PORT             PORTA
#define EXAMPLE_PWM_CH                  PIN31
#define EXAMPLE_PWM_CH_FUNC             PA31_PWM_CH8

#define EXAMPLE_PWM_TIMER_CH_IDX          0

#define EXAMPLE_PWM_CAPTURE_CH_IDX        4
#define EXAMPLE_PWM_CAPTURE_CH_PORT       PORTA
#define EXAMPLE_PWM_CAPTURE_CH            PIN31
#define EXAMPLE_PWM_CAPTURE_CH_FUNC       PA31_PWM_CH8
#define EXAMPLE_PWM_CAPTURE_GPIO_PIN_PORT PORTA
#define EXAMPLE_PWM_CAPTURE_GPIO_PIN      PIN6
#define EXAMPLE_PWM_CAPTURE_GPIO_PIN_FUNC 4
#define EXAMPLE_PWM_CAPTURE_GPIO_MASK     ( 1 << 6 )

#define QSPIFLASH_IDX                   0
#define EXAMPLE_QSPI_IDX                0

#define EXAMPLE_I2S_IDX                 0
#define EXAMPLE_I2S_MCLK                PIN10            /* board X7A: CH5 */
#define EXAMPLE_I2S_SCLK                PIN6             /* board X7A: A6 */
#define EXAMPLE_I2S_WSCLK               PIN7             /* board X7A: A7 */
#define EXAMPLE_I2S_SDA                 PIN8             /* board X7A: A8  */
#define EXAMPLE_I2S_MCLK_FUNC           PA10_I2S0_MCLK
#define EXAMPLE_I2S_SCLK_FUNC           PA6_I2S0_SCLK
#define EXAMPLE_I2S_WSCLK_FUNC          PA7_I2S0_WSCLK
#define EXAMPLE_I2S_SDA_FUNC            PA8_I2S0_SDA

#define EXAMPLE_I2S_SLAVE_IDX           1
#define EXAMPLE_I2S_SLAVE_MCLK          PIN26
#define EXAMPLE_I2S_SLAVE_SCLK          PIN2
#define EXAMPLE_I2S_SLAVE_WSCLK         PIN3
#define EXAMPLE_I2S_SLAVE_SDA           PIN5
#define EXAMPLE_I2S_SLAVE_MCLK_FUNC     PA26_I2S1_MCLK
#define EXAMPLE_I2S_SLAVE_SCLK_FUNC     PA2_I2S1_SCLK
#define EXAMPLE_I2S_SLAVE_WSCLK_FUNC    PA3_I2S1_WSCLK
#define EXAMPLE_I2S_SLAVE_SDA_FUNC      PA5_I2S1_SDA

#define EXAMPLE_TDM_IDX                 0
#define EXAMPLE_PIN_TDM_SCLK_PORT           PORTB
#define EXAMPLE_PIN_TDM_SCLK                PIN22             /* board X7A: A6 */
#define EXAMPLE_PIN_TDM_WSCLK_PORT          PORTB
#define EXAMPLE_PIN_TDM_WSCLK               PIN23             /* board X7A: A7 */
#define EXAMPLE_PIN_TDM_SDA_PORT            PORTB
#define EXAMPLE_PIN_TDM_SDA                 PIN28             /* board X7A: A8  */
#define EXAMPLE_PIN_TDM_SDA1_PORT           PORTB
#define EXAMPLE_PIN_TDM_SDA1                PIN29
#define EXAMPLE_PIN_TDM_SDA2_PORT           PORTB
#define EXAMPLE_PIN_TDM_SDA2                PIN30
#define EXAMPLE_PIN_TDM_SDA3_PORT           PORTB
#define EXAMPLE_PIN_TDM_SDA3                PIN31
#define EXAMPLE_PIN_TDM_SCLK_FUNC           PB22_TDM_SCLKOUT
#define EXAMPLE_PIN_TDM_WSCLK_FUNC          PB23_TDM_WSOUT
#define EXAMPLE_PIN_TDM_SDA_FUNC            PB28_TDM_D0
#define EXAMPLE_PIN_TDM_SDA1_FUNC            PB29_TDM_D1
#define EXAMPLE_PIN_TDM_SDA2_FUNC            PB30_TDM_D2
#define EXAMPLE_PIN_TDM_SDA3_FUNC            PB31_TDM_D3

#define EXAMPLE_ADC_CHANNEL_USR_DEF0         0
#define EXAMPLE_ADC_CH_PIN_USR_DEF0          EXAMPLE_ADC_CH0
#define EXAMPLE_ADC_CH_PIN_USR_DEF0_FUNC     EXAMPLE_ADC_CH0_FUNC
#define EXAMPLE_ADC_CHANNEL_DEF1             7
#define EXAMPLE_ADC_CH_PIN_USR_DEF1          EXAMPLE_ADC_CH7
#define EXAMPLE_ADC_CH_PIN_USR_DEF1_FUNC     EXAMPLE_ADC_CH7_FUNC
#define EXAMPLE_ADC_CHANNEL_DEF2             8
#define EXAMPLE_ADC_CH_PIN_USR_DEF2          EXAMPLE_ADC_CH8
#define EXAMPLE_ADC_CH_PIN_USR_DEF2_FUNC     EXAMPLE_ADC_CH8_FUNC

#define EXAMPLE_QSPI_IDX                0
#define EXAMPLE_PIN_QSPI_MOSI           -1
#define EXAMPLE_PIN_QSPI_MISO           -1
#define EXAMPLE_PIN_QSPI_CS             -1
#define EXAMPLE_PIN_QSPI_SCK            -1
#define EXAMPLE_PIN_QSPI_WP             -1
#define EXAMPLE_PIN_QSPI_HOLD           -1
#define EXAMPLE_PIN_QSPI_MOSI_FUNC      PIN_FUNC_GPIO
#define EXAMPLE_PIN_QSPI_MISO_FUNC      PIN_FUNC_GPIO
#define EXAMPLE_PIN_QSPI_CS_FUNC        PIN_FUNC_GPIO
#define EXAMPLE_PIN_QSPI_WP_FUNC        PIN_FUNC_GPIO
#define EXAMPLE_PIN_QSPI_HOLD_FUNC      PIN_FUNC_GPIO
#define EXAMPLE_PIN_QSPI_SOFT_CS_FUNC   PIN_FUNC_GPIO
#define EXAMPLE_PIN_QSPI_SCK_FUNC       PIN_FUNC_GPIO

#define EXAMPLE_PIN_QSPI_CS_GPIO_IDX    0
#define EXAMPLE_PIN_QSPI_CS_MSK         -1

/* tests pin manager */
#define TEST_USART_IDX                  1
#define TEST_PIN_USART_TX               PIN28         /* board X7A: A15*/
#define TEST_PIN_USART_RX               PIN27         /* board X7A: A16*/
#define TEST_PIN_USART_TX_FUNC          PA28_UART1_TX
#define TEST_PIN_USART_RX_FUNC          PA27_UART1_RX

#define TEST_IIC_IDX                    0
#define TEST_PIN_IIC_SDA                PIN16
#define TEST_PIN_IIC_SCL                PIN15
#define TEST_PIN_IIC_SDA_FUNC           PA16_I2C0_SDA
#define TEST_PIN_IIC_SCL_FUNC           PA15_I2C0_SCL

#define TEST_USI_IIC_IDX                1
#define TEST_PIN_USI_IIC_SDA            PIN17
#define TEST_PIN_USI_IIC_SCL            PIN16
#define TEST_PIN_USI_IIC_SDA_FUNC       PA17_USI1_SD0
#define TEST_PIN_USI_IIC_SCL_FUNC       PA16_USI1_SCLK

#define TEST_SPI_IDX                    1
#define TEST_PIN_SPI_MOSI               PIN17            /* board X7A: MOSI */
#define TEST_PIN_SPI_MISO               PIN16            /* board X7A: MISO */
#define TEST_PIN_SPI_CS                 PIN15             /* board X7A: CS */
#define TEST_PIN_SPI_SCK                PIN18             /* board X7A: SCK */
#define TEST_PIN_SPI_MOSI_FUNC          PA17_SPI2_MOSI
#define TEST_PIN_SPI_MISO_FUNC          PA16_SPI2_MISO
#define TEST_PIN_SPI_CS_FUNC            PA15_SPI2_CS0
#define TEST_PIN_SPI_SCK_FUNC           PA18_SPI2_CLK

#define TEST_SPI_SLAVE_IDX              2
#define TEST_PIN_SPI_SLAVE_MOSI         PIN23            /* board X7A: A17 */
#define TEST_PIN_SPI_SLAVE_MISO         PIN24            /* board X7A: CH0 */
#define TEST_PIN_SPI_SLAVE_CS           PIN25            /* board X7A: A15 */
#define TEST_PIN_SPI_SLAVE_SCK          PIN22            /* board X7A: A16 */
#define TEST_PIN_SPI_SLAVE_MOSI_FUNC    PA23_SPI4_MOSI
#define TEST_PIN_SPI_SLAVE_MISO_FUNC    PA24_SPI4_MISO
#define TEST_PIN_SPI_SLAVE_CS_FUNC      PA25_SPI4_CS0
#define TEST_PIN_SPI_SLAVE_SCK_FUNC     PA22_SPI4_CLK
#define TEST_PIN_SPI_SLAVE_SOFT_CS_FUNC PIN_FUNC_GPIO
#define TEST_PIN_SPI_SLAVE_CS_MSK       (TEST_PIN_SPI_SLAVE_CS )

#define TEST_GPIO_PIN                   PIN25
#define TEST_BOARD_GPIO_PIN_NAME        "MOSI"
#define TEST_GPIO_PIN_FUNC              PIN_FUNC_GPIO

#define TEST_PWM_IDX                    0
#define TEST_PWM_CH                     0
#define TEST_PWM_CH0                    PIN23
#define TEST_PWM_CH0_FUNC               PA23_PWM_CH0

#define TEST_BAUD_IDX                   0
#define TEST_BAUD_TXD                   PIN0
#define TEST_BAUD_RXD                   PIN1
#define TEST_BAUD_TXD_FUNC              PA0_UART0_TX
#define TEST_BAUD_RXD_FUNC              PA1_UART0_RX

#define TEST_BAUD_PWM_IDX               0
#define TEST_BAUD_PWM_CH                1
#define TEST_BAUD_PWM_PORT              PIN1
#define TEST_BAUD_PWM_FUNC              PA1_PWM_CH2

#define TEST_I2S_IDX                    0
#define TEST_I2S_MCLK                   PIN9
#define TEST_I2S_SCLK                   PIN8
#define TEST_I2S_WSCLK                  PIN7
#define TEST_I2S_SDA                    PIN6
#define TEST_I2S_MCLK_FUNC              PB9_I2S_MCLK
#define TEST_I2S_SCLK_FUNC              PB8_I2S_SCLK
#define TEST_I2S_WSCLK_FUNC             PB7_I2S_WSCLK
#define TEST_I2S_SDA_FUNC               PB6_I2S_SDA

#define EXAMPLE_MBOX_IDX                0
#define EXAMPLE_MBOX_TARGET_CPU         0

#define EXAMPLE_ADC_CHANNEL0_PORT       PORTA
#define EXAMPLE_ADC_CHANNEL0            0
#define EXAMPLE_ADC_CHANNEL0_PIN        EXAMPLE_ADC_CH0
#define EXAMPLE_ADC_CHANNEL0_PIN_FUNC   EXAMPLE_ADC_CH0_FUNC
#define EXAMPLE_ADC_CHANNEL1_PORT       PORTB
#define EXAMPLE_ADC_CHANNEL1            1
#define EXAMPLE_ADC_CHANNEL1_PIN        EXAMPLE_ADC_CH1
#define EXAMPLE_ADC_CHANNEL1_PIN_FUNC   EXAMPLE_ADC_CH1_FUNC
#define EXAMPLE_ADC_CHANNEL2_PORT       PORTA
#define EXAMPLE_ADC_CHANNEL2            2
#define EXAMPLE_ADC_CHANNEL2_PIN        EXAMPLE_ADC_CH2
#define EXAMPLE_ADC_CHANNEL2_PIN_FUNC   EXAMPLE_ADC_CH2_FUNC
#define EXAMPLE_ADC_CHANNEL3_PORT       PORTA
#define EXAMPLE_ADC_CHANNEL3            3
#define EXAMPLE_ADC_CHANNEL3_PIN        EXAMPLE_ADC_CH3
#define EXAMPLE_ADC_CHANNEL3_PIN_FUNC   EXAMPLE_ADC_CH3_FUNC

#define EXAMPLE_ADC_INPUT_PORT PORTA
#define EXAMPLE_ADC_INPUT               PIN25
#define EXAMPLE_ADC_INPUT_FUNC          PIN_FUNC_GPIO
#define EXAMPLE_ADC_INPUT_MASK          ( EXAMPLE_ADC_INPUT )

#define EXAMPLE_ADC_IDX     0
#define EXAMPLE_ADC_CH0     PIN9
#define EXAMPLE_ADC_CH0_FUNC     PA9_ADC_A7
#define EXAMPLE_ADC_CH1     PIN4
#define EXAMPLE_ADC_CH1_FUNC     PB4_ADC1_A7
#define EXAMPLE_ADC_CH2     PIN5
#define EXAMPLE_ADC_CH2_FUNC     PA5_ADC_A3
#define EXAMPLE_ADC_CH3     PIN6
#define EXAMPLE_ADC_CH3_FUNC     PA6_ADC_A4
#define EXAMPLE_ADC_CH4     PIN7
#define EXAMPLE_ADC_CH4_FUNC     PA7_ADC_A5
#define EXAMPLE_ADC_CH5     PIN8
#define EXAMPLE_ADC_CH5_FUNC     PA8_ADC_A6
#define EXAMPLE_ADC_CH6     PIN9
#define EXAMPLE_ADC_CH6_FUNC     PA9_ADC_A7
#define EXAMPLE_ADC_CH7     PIN10
#define EXAMPLE_ADC_CH7_FUNC     PA10_ADC_A8
#define EXAMPLE_ADC_CH8     PIN15
#define EXAMPLE_ADC_CH8_FUNC     PA15_ADC_A9
#define EXAMPLE_ADC_CH9     PIN16
#define EXAMPLE_ADC_CH9_FUNC     PA16_ADC_A10
#define EXAMPLE_ADC_CH10    PIN17
#define EXAMPLE_ADC_CH10_FUNC    PA17_ADC_A11
#define EXAMPLE_ADC_CH11    PIN18
#define EXAMPLE_ADC_CH11_FUNC    PA18_ADC_A12
#define EXAMPLE_ADC_CH12    PIN19
#define EXAMPLE_ADC_CH12_FUNC    PA19_ADC_A13
#define EXAMPLE_ADC_CH13    PIN20
#define EXAMPLE_ADC_CH13_FUNC    PA20_ADC_A14
#define EXAMPLE_ADC_CH14    PIN21
#define EXAMPLE_ADC_CH14_FUNC   PA21_ADC_A15

#ifdef __cplusplus
}
#endif

#endif /* _BOARD_CONFIG_H_ */


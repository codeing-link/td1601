#ifndef _LCD_CONFIG_H_
#define _LCD_CONFIG_H_

#include "soc.h"

// SPI0 (GC9A01 LCD)
#define LCD_SPI_IDX             0
#define LCD_PIN_MOSI            PA25
#define LCD_PIN_MOSI_FUNC       PA25_SPI0_MOSI
#define LCD_PIN_SCK             PA24
#define LCD_PIN_SCK_FUNC        PA24_SPI0_SCK
#define LCD_PIN_DC              PA23
#define LCD_PIN_CS              PA22
#define LCD_PIN_RESET           PA21

// GPIO PA bank (idx 0), bit masks — PA_n = n
#define LCD_GPIO_IDX            0
#define LCD_DC_MSK              (1U << 23)
#define LCD_CS_MSK              (1U << 22)
#define LCD_RST_MSK             (1U << 21)

// IIC0 (FT3267 touch)
#define TP_IIC_IDX              0
#define TP_PIN_SDA              PA16
#define TP_PIN_SDA_FUNC         PA16_IIC0_SDA
#define TP_PIN_SCL              PA15
#define TP_PIN_SCL_FUNC         PA15_IIC0_SCL

// TP control GPIOs (PB bank, idx 1), bit masks — PB_n = n (within port)
#define TP_GPIO_IDX             1
#define TP_PIN_RESET            PB17
#define TP_PIN_INT              PB20
#define TP_RST_MSK              (1U << 17)
#define TP_INT_MSK              (1U << 20)

#endif /* _LCD_CONFIG_H_ */

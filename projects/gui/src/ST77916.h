/******************************************************************************
 * @file     ST77916.h
 * @brief    ST77916 OSPI(四线) LCD 驱动（TD1601 / T-Head E906FD 裸机 CSI 版）
 *           由 ESP-IDF (esp_lcd) 版本移植而来：
 *             - 显示总线改用 csi_ospi（DW_OSPI0，四线 QUAD），参考 ospi 例子
 *             - 片选 CS 用软件 GPIO 控制（PA15），每次传输前后拉低/拉高
 *             - 复位/背光用 GPIO 直连（RST=PA7，背光=PA4）
 *             - 删除 FreeRTOS / LVGL / esp_lcd / TCA9554 全部依赖
 * @version  V2.0
 * @date     2026-06-17
 ******************************************************************************/

#ifndef _ST77916_H_
#define _ST77916_H_

#include <stdint.h>
#include <stdbool.h>
#include <soc.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include <drv/ospi.h>
#include <drv/tick.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================================================
 * 屏幕参数
 * =========================================================================*/
#define EXAMPLE_LCD_WIDTH            (360)   /* 屏宽 */
#define EXAMPLE_LCD_HEIGHT           (360)   /* 屏高 */
#define EXAMPLE_LCD_COLOR_BITS       (16)    /* RGB565，每像素 16bit */

/* ===========================================================================
 * OSPI 总线参数（DW_OSPI0 控制器，idx 0）
 * =========================================================================*/
#define LCD_OSPI_IDX                 (0)             /* OSPI 控制器索引 */
/* 命令波特率：10MHz，单线模式，用于 CASET/RASET 等初始化及窗口命令
 *   参考工程 TFT_SPI_Write_Byte 使用 120*100000 ≈ 12MHz，此处取整 10MHz */
#define LCD_OSPI_CMD_BAUD_HZ         (10 * 1000000)  /* 命令/参数波特率，10MHz  */
/* 数据波特率：50MHz，QUAD 四线模式，用于像素数据传输
 *   参考工程 LCD_startWriteMutileData 使用 500*100000 = 50MHz */
#define LCD_OSPI_DATA_BAUD_HZ        (50 * 1000000)  /* 像素数据波特率，50MHz  */
/* 兼容旧接口：OSPI_Init 初始化时使用命令波特率作为默认值 */
#define LCD_OSPI_BAUD_HZ             LCD_OSPI_CMD_BAUD_HZ

/* ST77916 QSPI 操作码（与 ESP esp_lcd_st77916 一致）
 *   0x02：单线写命令/参数；0x32：四线写显存色数据 */
#define LCD_OPCODE_WRITE_CMD         (0x02)
#define LCD_OPCODE_WRITE_COLOR       (0x32)

/* ===========================================================================
 * OSPI 数据/时钟引脚（复用为 SPI1 的 D0-D3 + SCK，需 csi_pin_set_mux）
 *   D0=PB4  D1=PB5  D2=PB6  D3=PB7  SCK=PA28
 * =========================================================================*/
#define LCD_OSPI_D0_PORT             PORTB
#define LCD_OSPI_D0_PIN              PIN4
#define LCD_OSPI_D0_FUNC             PB4_SPI1_D0
#define LCD_OSPI_D1_PORT             PORTB
#define LCD_OSPI_D1_PIN              PIN5
#define LCD_OSPI_D1_FUNC             PB5_SPI1_D1
#define LCD_OSPI_D2_PORT             PORTB
#define LCD_OSPI_D2_PIN              PIN6
#define LCD_OSPI_D2_FUNC             PB6_SPI1_D2
#define LCD_OSPI_D3_PORT             PORTB
#define LCD_OSPI_D3_PIN              PIN7
#define LCD_OSPI_D3_FUNC             PB7_SPI1_D3
#define LCD_OSPI_SCK_PORT            PORTA
#define LCD_OSPI_SCK_PIN             PIN28
#define LCD_OSPI_SCK_FUNC            PA28_SPI1_SCK

/* ===========================================================================
 * 控制 IO 引脚（软件 CS / 复位 / 背光，均为 PORTA GPIO）
 *   CS=PA15  RST=PA7  背光=PA4
 * =========================================================================*/
#define LCD_CS_PORT                  PORTA
#define LCD_CS_PIN                   PIN15           /* 软件片选 */
#define LCD_RST_PORT                 PORTA
#define LCD_RST_PIN                  PIN7            /* 复位（原 EXIO2） */
#define LCD_BL_PORT                  PORTA
#define LCD_BL_PIN                   PIN4            /* 背光（原 LEDC GPIO5） */

#define Backlight_MAX                (100)           /* 背光亮度上限（开关式仅区分 0/非0） */

/* 背光有效电平：1 = 高电平点亮，0 = 低电平点亮。
 * 实测本板 PA4 低电平点亮、高电平熄灭，故为低有效。 */
#define LCD_BL_ACTIVE_HIGH           (0)

/* ===========================================================================
 * ST77916 厂商初始化命令表结构
 * =========================================================================*/
typedef struct {
    uint8_t cmd;            /* 命令字节 */
    uint8_t data[16];       /* 参数（最多 14 字节，留 16 余量） */
    uint8_t data_len;       /* 参数长度 */
    uint8_t delay_ms;       /* 该命令后延时（ms），0 表示不延时 */
} st77916_init_cmd_t;

/* ===========================================================================
 * 对外接口
 * =========================================================================*/
extern uint8_t LCD_Backlight;

/* LCD 初始化总入口（复位 + OSPI init + 命令表 + 背光 + 自测彩条），主程序调用 */
void LCD_Init(void);

/* 设置显示窗口并写入一块色数据；color 指向 RGB565 像素数组 */
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color);

/* 仅设置显示窗口（CASET + RASET + RAMWR 命令），不发像素数据 */
void LCD_setWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);

/* 向当前已打开的窗口写入一行像素数据（配合 LCD_setWindow 使用） */
void LCD_writeRowData(const uint16_t *color, uint16_t count);

/* 背光初始化（已在 LCD_Init 内调用） */
void Backlight_Init(void);
/* 背光亮度设置：开关式，Light==0 灭，>0 亮（保留 0~100 接口形态） */
void Set_Backlight(uint8_t Light);

/* 仅做 ST77916 控制器初始化（复位 + OSPI + 命令表），供 LCD_Init 内部使用 */
void ST77916_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* _ST77916_H_ */

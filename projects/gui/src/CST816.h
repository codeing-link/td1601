/******************************************************************************
 * @file     CST816.h
 * @brief    CST816(S) 电容触摸驱动（TD1601 / T-Head E906FD 裸机 CSI 版）
 *           由 ESP-IDF (esp_lcd_touch) 版本移植而来：
 *             - I2C 总线从 driver/i2c + esp_lcd_panel_io_i2c 换成 csi_iic
 *             - 寄存器读写用 csi_iic_mem_send / csi_iic_mem_receive
 *             - 复位脚从 TCA9554 EXIO1 改为 TD1601 空闲 PA 口 GPIO 直连
 *             - INT 用 csi_gpio 下降沿中断触发读坐标
 *             - 删除 FreeRTOS / esp_lcd_touch 全部依赖
 * @version  V1.0
 * @date     2026-06-17
 ******************************************************************************/

#ifndef _CST816_H_
#define _CST816_H_

#include <stdint.h>
#include <stdbool.h>
#include <soc.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include <drv/iic.h>
#include <drv/tick.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===========================================================================
 * I2C 总线参数（IIC0，参考 projects/drivers/iic 例子默认引脚）
 * =========================================================================*/
#define CST816_IIC_IDX               (0)            /* IIC0 控制器索引 */
#define CST816_I2C_ADDR              (0x15)         /* CST816S 7-bit 地址 */
#define CST816_I2C_FREQ              IIC_BUS_SPEED_FAST  /* 400kHz */

/* I2C 引脚（IIC0：SDA=PA9, SCL=PA8，按实际接线） */
#define CST816_SDA_PORT              PORTA
#define CST816_SDA_PIN               PIN20//PIN9
#define CST816_SDA_FUNC              PA20_IIC0_SDA//PA9_IIC0_SDA
#define CST816_SCL_PORT              PORTA
#define CST816_SCL_PIN               PIN21//PIN8
#define CST816_SCL_FUNC              PA21_IIC0_SCL//PA8_IIC0_SCL

/* ===========================================================================
 * 控制 IO 引脚（替代原 TCA9554 EXIO1 / ESP GPIO4，按实际接线）
 *   TP_INT=PA0   TP_RST=PA6
 * =========================================================================*/
#define CST816_INT_PORT              PORTA
#define CST816_INT_PIN               PIN0           /* 触摸中断脚（原 GPIO4），下降沿有效 */
#define CST816_RST_PORT              PORTA
#define CST816_RST_PIN               PIN6           /* 触摸复位脚（原 EXIO1） */

/* ===========================================================================
 * CST816 寄存器
 * =========================================================================*/
#define CST816_REG_DATA_START        (0x02)         /* 触点数据起始寄存器 */
#define CST816_REG_CHIP_ID           (0xA7)         /* 芯片 ID 寄存器 */
#define CST816_REG_AUTOSLEEP         (0xFE)         /* 自动休眠控制寄存器 */

#define CST816_POINT_NUM_MAX         (1)            /* 最大触点数 */

/* ===========================================================================
 * 触摸数据（精简本地结构，替代 esp_lcd_touch_handle_t）
 * =========================================================================*/
typedef struct {
    uint16_t x;            /* X 坐标 */
    uint16_t y;            /* Y 坐标 */
    uint8_t  points;       /* 当前触点数（0 表示无触摸） */
} cst816_data_t;

/* ===========================================================================
 * 对外接口
 * =========================================================================*/

/* 触摸初始化：I2C + 复位 + 读 ID + INT 中断 */
void Touch_Init(void);

/* 主循环调用：若 INT 触发则读取一次坐标到 out，返回 true 表示有新坐标 */
bool Touch_Poll(cst816_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* _CST816_H_ */

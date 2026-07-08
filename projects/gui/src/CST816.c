/******************************************************************************
 * @file     CST816.c
 * @brief    CST816(S) 电容触摸驱动（TD1601 / T-Head E906FD 裸机 CSI 版）
 *
 *  移植说明（ESP-IDF -> TD1601 CSI）：
 *    - I2C 总线：driver/i2c + esp_lcd_panel_io_i2c  ->  csi_iic
 *    - 寄存器读：esp_lcd_panel_io_rx_param  ->  csi_iic_mem_receive
 *    - 寄存器写：esp_lcd_panel_io_tx_param  ->  csi_iic_mem_send
 *    - 复位脚：  TCA9554 EXIO1  ->  PA12 GPIO 直连
 *    - INT：     esp_lcd_touch 中断  ->  csi_gpio 下降沿中断 + 回调置标志
 *    - 临界区：  portENTER/EXIT_CRITICAL  ->  csi_irq_save/csi_irq_restore
 *    - 延时：    vTaskDelay  ->  mdelay
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <drv/irq.h>
#include "CST816.h"

/* I2C 主机句柄 */
static csi_iic_t s_iic;
/* INT/RST 控制脚的 GPIO 端口句柄（PA11 INT / PA12 RST，同属端口 A） */
static csi_gpio_t s_tp_gpio;

/* INT 中断标志：回调里置 1，主循环 Touch_Poll 消费后清 0 */
static volatile uint8_t s_tp_irq_flag = 0;

/* I2C 读写超时（ms） */
#define CST816_I2C_TIMEOUT           (100)

/* ===========================================================================
 * I2C 寄存器读写封装
 * =========================================================================*/

/* 从 reg 读 len 字节到 data；返回 0 成功 */
static int cst816_read_reg(uint8_t reg, uint8_t *data, uint32_t len)
{
    int32_t num = csi_iic_mem_receive(&s_iic, CST816_I2C_ADDR, reg,
                                      IIC_MEM_ADDR_SIZE_8BIT, data, len,
                                      CST816_I2C_TIMEOUT);
    return ((uint32_t)num == len) ? 0 : -1;
}

/* 向 reg 写 len 字节；返回 0 成功 */
static int cst816_write_reg(uint8_t reg, const uint8_t *data, uint32_t len)
{
    int32_t num = csi_iic_mem_send(&s_iic, CST816_I2C_ADDR, reg,
                                   IIC_MEM_ADDR_SIZE_8BIT, data, len,
                                   CST816_I2C_TIMEOUT);
    return ((uint32_t)num == len) ? 0 : -1;
}

/* ===========================================================================
 * 复位 / ID / 休眠
 * =========================================================================*/

/* 触摸硬复位：拉低 10ms，再拉高 50ms（原走 TCA9554 EXIO1，现走 PA12） */
static void cst816_reset(void)
{
    csi_gpio_write(&s_tp_gpio, CST816_RST_PIN, GPIO_PIN_LOW);
    mdelay(10);
    csi_gpio_write(&s_tp_gpio, CST816_RST_PIN, GPIO_PIN_HIGH);
    mdelay(50);
}

/* 读芯片 ID（寄存器 0xA7），打印用于调试 */
static void cst816_read_id(void)
{
    uint8_t id = 0;
    if (cst816_read_reg(CST816_REG_CHIP_ID, &id, 1) == 0) {
        printf("CST816: chip id = 0x%02x\r\n", id);
    } else {
        printf("CST816: read id failed\r\n");
    }
}

/* 关闭自动休眠（写 0xFE = 1：!sleep）。Sleep=false 表示不休眠，保持唤醒 */
static void cst816_autosleep(bool sleep)
{
    uint8_t v = (uint8_t)(!sleep);
    cst816_write_reg(CST816_REG_AUTOSLEEP, &v, 1);
}

/* ===========================================================================
 * INT 中断回调
 * =========================================================================*/
static void cst816_int_callback(csi_gpio_t *gpio, uint32_t pins, void *arg)
{
    (void)gpio;
    (void)arg;
    if (pins & CST816_INT_PIN) {
        s_tp_irq_flag = 1;     /* 触摸事件，置标志，主循环去读坐标 */
    }
}

/* ===========================================================================
 * 引脚 / 初始化
 * =========================================================================*/

/* I2C 引脚复用 */
static void cst816_pin_iic_init(void)
{
    csi_pin_set_mux(CST816_SDA_PORT, CST816_SDA_PIN, CST816_SDA_FUNC);
    csi_pin_set_mux(CST816_SCL_PORT, CST816_SCL_PIN, CST816_SCL_FUNC);
}

/* I2C 主机初始化：master / 7-bit / 400kHz */
static int cst816_iic_init(void)
{
    csi_error_t ret;

    cst816_pin_iic_init();

    ret = csi_iic_init(&s_iic, CST816_IIC_IDX);
    if (ret != CSI_OK) {
        printf("CST816: csi_iic_init error\r\n");
        return -1;
    }

    ret = csi_iic_mode(&s_iic, IIC_MODE_MASTER);
    if (ret != CSI_OK) {
        printf("CST816: csi_iic_mode error\r\n");
        return -1;
    }

    ret = csi_iic_addr_mode(&s_iic, IIC_ADDRESS_7BIT);
    if (ret != CSI_OK) {
        printf("CST816: csi_iic_addr_mode error\r\n");
        return -1;
    }

    ret = csi_iic_speed(&s_iic, CST816_I2C_FREQ);
    if (ret != CSI_OK) {
        printf("CST816: csi_iic_speed error\r\n");
        return -1;
    }

    return 0;
}

/* INT/RST GPIO 初始化：RST 输出，INT 输入 + 下降沿中断 */
static void cst816_gpio_init(void)
{
    /* 引脚复用为普通 GPIO */
    csi_pin_set_mux(CST816_RST_PORT, CST816_RST_PIN, PIN_FUNC_GPIO);
    csi_pin_set_mux(CST816_INT_PORT, CST816_INT_PIN, PIN_FUNC_GPIO);

    /* 端口 A 初始化（INT 与 RST 同端口，共用句柄） */
    csi_gpio_init(&s_tp_gpio, CST816_RST_PORT);

    /* RST 输出，INT 输入 */
    csi_gpio_dir(&s_tp_gpio, CST816_RST_PIN, GPIO_DIRECTION_OUTPUT);
    csi_gpio_dir(&s_tp_gpio, CST816_INT_PIN, GPIO_DIRECTION_INPUT);
    csi_gpio_mode(&s_tp_gpio, CST816_INT_PIN, GPIO_MODE_PULLUP);

    /* INT 下降沿中断 + 回调 */
    csi_gpio_attach_callback(&s_tp_gpio, cst816_int_callback, NULL);
    csi_gpio_irq_mode(&s_tp_gpio, CST816_INT_PIN, GPIO_IRQ_MODE_FALLING_EDGE);
    csi_gpio_irq_enable(&s_tp_gpio, CST816_INT_PIN, true);
}

void Touch_Init(void)
{
    /* 1. INT/RST GPIO */
    cst816_gpio_init();

    /* 2. I2C 主机 */
    if (cst816_iic_init() != 0) {
        printf("CST816: I2C init failed\r\n");
        return;
    }
    printf("CST816: I2C initialized\r\n");

    /* 3. 复位控制器 */
    cst816_reset();

    /* 4. 读 ID + 关闭自动休眠 */
    cst816_read_id();
    cst816_autosleep(false);

    printf("CST816: touch controller ready\r\n");
}

/* ===========================================================================
 * 坐标读取
 * =========================================================================*/

/*
 * 触摸数据布局（从寄存器 0x02 起读 5 字节）：
 *   byte0: 触点数
 *   byte1: bit[3:0] = X 高 4 位
 *   byte2: X 低 8 位
 *   byte3: bit[3:0] = Y 高 4 位
 *   byte4: Y 低 8 位
 *   => x = (byte1 & 0x0F) << 8 | byte2;  y 同理
 */
static int cst816_read_point(cst816_data_t *out)
{
    uint8_t buf[5];

    if (cst816_read_reg(CST816_REG_DATA_START, buf, sizeof(buf)) != 0) {
        return -1;
    }

    uint8_t num = buf[0];
    if (num > CST816_POINT_NUM_MAX) {
        num = CST816_POINT_NUM_MAX;
    }

    out->points = num;
    if (num > 0) {
        out->x = (uint16_t)(((buf[1] & 0x0F) << 8) | buf[2]);
        out->y = (uint16_t)(((buf[3] & 0x0F) << 8) | buf[4]);
    } else {
        out->x = 0;
        out->y = 0;
    }
    return 0;
}

bool Touch_Poll(cst816_data_t *out)
{
    uint32_t flags;
    uint8_t got;

    /* 临界区里读并清中断标志（替代 portENTER/EXIT_CRITICAL） */
    flags = csi_irq_save();
    got = s_tp_irq_flag;
    s_tp_irq_flag = 0;
    csi_irq_restore(flags);

    if (!got) {
        return false;
    }

    if (cst816_read_point(out) != 0) {
        return false;
    }

    /* 仅当真有触点时上报 */
    return (out->points > 0);
}

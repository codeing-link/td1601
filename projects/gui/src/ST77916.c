/******************************************************************************
 * @file     ST77916.c
 * @brief    ST77916 OSPI(四线) LCD 驱动（TD1601 / T-Head E906FD 裸机 CSI 版）
 *
 *  移植说明（ESP-IDF -> TD1601 CSI）：
 *    - 显示总线：esp_lcd_panel_io_spi  ->  csi_ospi（DW_OSPI0，四线 QUAD）
 *    - 片选 CS： 控制器内部 CS  ->  软件 GPIO（PA15），传输前后拉低/拉高
 *    - 命令下发：esp_lcd 自动遍历 vendor_config  ->  本文件手动遍历命令表下发
 *    - 复位脚：  TCA9554 EXIO2  ->  PA7 GPIO 直连
 *    - 背光：    LEDC PWM(GPIO5)  ->  PA4 GPIO 开关
 *    - 延时：    vTaskDelay(pdMS_TO_TICKS(n))  ->  mdelay(n)
 *    - 日志：    ESP_LOGx / ESP_ERROR_CHECK  ->  printf
 *
 *  OSPI 四线写时序（参考 projects/drivers/ospi 例子）：
 *    1. csi_ospi_config(&cmd) 配置本次传输的 指令/地址/数据 相位与位宽
 *    2. 软件 CS 拉低
 *    3. csi_ospi_send(...) 发送数据
 *    4. 软件 CS 拉高
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ST77916.h"

/* OSPI 句柄与命令描述符（DW_OSPI0 控制器） */
static csi_ospi_t s_ospi;
static csi_ospi_command_t s_cmd;

/* 控制 IO 用的 GPIO 端口句柄（CS=PA15 / RST=PA7 / 背光=PA4，同属端口 A） */
static csi_gpio_t s_lcd_gpio;

/* 背光当前亮度（开关式，仅区分 0 / 非0） */
uint8_t LCD_Backlight = 70;

/* OSPI 传输超时（ms） */
#define LCD_OSPI_TIMEOUT            (1000)

/* 软件片选：拉低 = 选中，拉高 = 释放 */
#define LCD_CS_LOW()   csi_gpio_write(&s_lcd_gpio, LCD_CS_PIN, GPIO_PIN_LOW)
#define LCD_CS_HIGH()  csi_gpio_write(&s_lcd_gpio, LCD_CS_PIN, GPIO_PIN_HIGH)

/* ===========================================================================
 * ST77916 厂商初始化命令表
 *   内容与原 ESP 版 vendor_specific_init_new[] 完全一致，仅改成本地结构。
 *   {命令, {参数...}, 参数长度, 命令后延时ms}
 * =========================================================================*/
static const st77916_init_cmd_t vendor_specific_init_new[] = {
    {0xF0, {0x28}, 1, 0},
    {0xF2, {0x28}, 1, 0},
    {0x73, {0xF0}, 1, 0},
    {0x7C, {0xD1}, 1, 0},
    {0x83, {0xE0}, 1, 0},
    {0x84, {0x61}, 1, 0},
    {0xF2, {0x82}, 1, 0},
    {0xF0, {0x00}, 1, 0},
    {0xF0, {0x01}, 1, 0},
    {0xF1, {0x01}, 1, 0},
    {0xB0, {0x56}, 1, 0},
    {0xB1, {0x4D}, 1, 0},
    {0xB2, {0x24}, 1, 0},
    {0xB4, {0x87}, 1, 0},
    {0xB5, {0x44}, 1, 0},
    {0xB6, {0x8B}, 1, 0},
    {0xB7, {0x40}, 1, 0},
    {0xB8, {0x86}, 1, 0},
    {0xBA, {0x00}, 1, 0},
    {0xBB, {0x08}, 1, 0},
    {0xBC, {0x08}, 1, 0},
    {0xBD, {0x00}, 1, 0},
    {0xC0, {0x80}, 1, 0},
    {0xC1, {0x10}, 1, 0},
    {0xC2, {0x37}, 1, 0},
    {0xC3, {0x80}, 1, 0},
    {0xC4, {0x10}, 1, 0},
    {0xC5, {0x37}, 1, 0},
    {0xC6, {0xA9}, 1, 0},
    {0xC7, {0x41}, 1, 0},
    {0xC8, {0x01}, 1, 0},
    {0xC9, {0xA9}, 1, 0},
    {0xCA, {0x41}, 1, 0},
    {0xCB, {0x01}, 1, 0},
    {0xD0, {0x91}, 1, 0},
    {0xD1, {0x68}, 1, 0},
    {0xD2, {0x68}, 1, 0},
    {0xF5, {0x00, 0xA5}, 2, 0},
    {0xDD, {0x4F}, 1, 0},
    {0xDE, {0x4F}, 1, 0},
    {0xF1, {0x10}, 1, 0},
    {0xF0, {0x00}, 1, 0},
    {0xF0, {0x02}, 1, 0},
    {0xE0, {0xF0, 0x0A, 0x10, 0x09, 0x09, 0x36, 0x35, 0x33, 0x4A, 0x29, 0x15, 0x15, 0x2E, 0x34}, 14, 0},
    {0xE1, {0xF0, 0x0A, 0x0F, 0x08, 0x08, 0x05, 0x34, 0x33, 0x4A, 0x39, 0x15, 0x15, 0x2D, 0x33}, 14, 0},
    {0xF0, {0x10}, 1, 0},
    {0xF3, {0x10}, 1, 0},
    {0xE0, {0x07}, 1, 0},
    {0xE1, {0x00}, 1, 0},
    {0xE2, {0x00}, 1, 0},
    {0xE3, {0x00}, 1, 0},
    {0xE4, {0xE0}, 1, 0},
    {0xE5, {0x06}, 1, 0},
    {0xE6, {0x21}, 1, 0},
    {0xE7, {0x01}, 1, 0},
    {0xE8, {0x05}, 1, 0},
    {0xE9, {0x02}, 1, 0},
    {0xEA, {0xDA}, 1, 0},
    {0xEB, {0x00}, 1, 0},
    {0xEC, {0x00}, 1, 0},
    {0xED, {0x0F}, 1, 0},
    {0xEE, {0x00}, 1, 0},
    {0xEF, {0x00}, 1, 0},
    {0xF8, {0x00}, 1, 0},
    {0xF9, {0x00}, 1, 0},
    {0xFA, {0x00}, 1, 0},
    {0xFB, {0x00}, 1, 0},
    {0xFC, {0x00}, 1, 0},
    {0xFD, {0x00}, 1, 0},
    {0xFE, {0x00}, 1, 0},
    {0xFF, {0x00}, 1, 0},
    {0x60, {0x40}, 1, 0},
    {0x61, {0x04}, 1, 0},
    {0x62, {0x00}, 1, 0},
    {0x63, {0x42}, 1, 0},
    {0x64, {0xD9}, 1, 0},
    {0x65, {0x00}, 1, 0},
    {0x66, {0x00}, 1, 0},
    {0x67, {0x00}, 1, 0},
    {0x68, {0x00}, 1, 0},
    {0x69, {0x00}, 1, 0},
    {0x6A, {0x00}, 1, 0},
    {0x6B, {0x00}, 1, 0},
    {0x70, {0x40}, 1, 0},
    {0x71, {0x03}, 1, 0},
    {0x72, {0x00}, 1, 0},
    {0x73, {0x42}, 1, 0},
    {0x74, {0xD8}, 1, 0},
    {0x75, {0x00}, 1, 0},
    {0x76, {0x00}, 1, 0},
    {0x77, {0x00}, 1, 0},
    {0x78, {0x00}, 1, 0},
    {0x79, {0x00}, 1, 0},
    {0x7A, {0x00}, 1, 0},
    {0x7B, {0x00}, 1, 0},
    {0x80, {0x48}, 1, 0},
    {0x81, {0x00}, 1, 0},
    {0x82, {0x06}, 1, 0},
    {0x83, {0x02}, 1, 0},
    {0x84, {0xD6}, 1, 0},
    {0x85, {0x04}, 1, 0},
    {0x86, {0x00}, 1, 0},
    {0x87, {0x00}, 1, 0},
    {0x88, {0x48}, 1, 0},
    {0x89, {0x00}, 1, 0},
    {0x8A, {0x08}, 1, 0},
    {0x8B, {0x02}, 1, 0},
    {0x8C, {0xD8}, 1, 0},
    {0x8D, {0x04}, 1, 0},
    {0x8E, {0x00}, 1, 0},
    {0x8F, {0x00}, 1, 0},
    {0x90, {0x48}, 1, 0},
    {0x91, {0x00}, 1, 0},
    {0x92, {0x0A}, 1, 0},
    {0x93, {0x02}, 1, 0},
    {0x94, {0xDA}, 1, 0},
    {0x95, {0x04}, 1, 0},
    {0x96, {0x00}, 1, 0},
    {0x97, {0x00}, 1, 0},
    {0x98, {0x48}, 1, 0},
    {0x99, {0x00}, 1, 0},
    {0x9A, {0x0C}, 1, 0},
    {0x9B, {0x02}, 1, 0},
    {0x9C, {0xDC}, 1, 0},
    {0x9D, {0x04}, 1, 0},
    {0x9E, {0x00}, 1, 0},
    {0x9F, {0x00}, 1, 0},
    {0xA0, {0x48}, 1, 0},
    {0xA1, {0x00}, 1, 0},
    {0xA2, {0x05}, 1, 0},
    {0xA3, {0x02}, 1, 0},
    {0xA4, {0xD5}, 1, 0},
    {0xA5, {0x04}, 1, 0},
    {0xA6, {0x00}, 1, 0},
    {0xA7, {0x00}, 1, 0},
    {0xA8, {0x48}, 1, 0},
    {0xA9, {0x00}, 1, 0},
    {0xAA, {0x07}, 1, 0},
    {0xAB, {0x02}, 1, 0},
    {0xAC, {0xD7}, 1, 0},
    {0xAD, {0x04}, 1, 0},
    {0xAE, {0x00}, 1, 0},
    {0xAF, {0x00}, 1, 0},
    {0xB0, {0x48}, 1, 0},
    {0xB1, {0x00}, 1, 0},
    {0xB2, {0x09}, 1, 0},
    {0xB3, {0x02}, 1, 0},
    {0xB4, {0xD9}, 1, 0},
    {0xB5, {0x04}, 1, 0},
    {0xB6, {0x00}, 1, 0},
    {0xB7, {0x00}, 1, 0},
    {0xB8, {0x48}, 1, 0},
    {0xB9, {0x00}, 1, 0},
    {0xBA, {0x0B}, 1, 0},
    {0xBB, {0x02}, 1, 0},
    {0xBC, {0xDB}, 1, 0},
    {0xBD, {0x04}, 1, 0},
    {0xBE, {0x00}, 1, 0},
    {0xBF, {0x00}, 1, 0},
    {0xC0, {0x10}, 1, 0},
    {0xC1, {0x47}, 1, 0},
    {0xC2, {0x56}, 1, 0},
    {0xC3, {0x65}, 1, 0},
    {0xC4, {0x74}, 1, 0},
    {0xC5, {0x88}, 1, 0},
    {0xC6, {0x99}, 1, 0},
    {0xC7, {0x01}, 1, 0},
    {0xC8, {0xBB}, 1, 0},
    {0xC9, {0xAA}, 1, 0},
    {0xD0, {0x10}, 1, 0},
    {0xD1, {0x47}, 1, 0},
    {0xD2, {0x56}, 1, 0},
    {0xD3, {0x65}, 1, 0},
    {0xD4, {0x74}, 1, 0},
    {0xD5, {0x88}, 1, 0},
    {0xD6, {0x99}, 1, 0},
    {0xD7, {0x01}, 1, 0},
    {0xD8, {0xBB}, 1, 0},
    {0xD9, {0xAA}, 1, 0},
    {0xF3, {0x01}, 1, 0},
    {0xF0, {0x00}, 1, 0},
    {0x21, {0x00}, 1, 0},
    {0x11, {0x00}, 1, 120},   /* SLEEP OUT，需等 120ms */
    {0x29, {0x00}, 1, 0},     /* DISPLAY ON */
};

/* ===========================================================================
 * QSPI 底层读写封装
 * =========================================================================*/

/*
 * 写命令 + 可选参数（命令相位）。
 *
 * 重要：本 SDK 的 csi_ospi_send 有一个特性——只有当 data.bus_width != SINGLE
 * （即 DUAL/QUAD/OCTAL）时，才会把 instruction.value / address.value 压进 FIFO；
 * 数据相位为单线时，instruction/address 不会被发出。
 *
 * ST77916 QSPI 写命令格式：0x02(单线) + 24bit 地址(高8bit=命令字, 单线) + 参数(单线)。
 * 由于上述特性，单线下没法用 instruction/address 相位，因此这里把整条命令帧
 *   [0x02, 0x00, cmd, 0x00, 参数...]
 * 拼成一段连续字节，全部走单线 data 相位手动发出（指令/地址相位 disabled）。
 *   - 字节0：0x02 写命令操作码
 *   - 字节1~3：24bit 地址，高8bit=0x00，中8bit=命令字 cmd，低8bit=0x00
 *     （等效于 esp_lcd_st77916 的 cmd<<8 over 24-bit address）
 *   - 之后跟随参数字节
 */
static int lcd_write_cmd(uint8_t cmd, const uint8_t *data, uint32_t len)
{
    static uint8_t frame[4 + 16];   /* 4 字节帧头 + 最多 16 字节参数 */
    uint32_t total;
    int32_t ret;

    /* 拼命令帧：opcode + 24bit 地址(高8=0, 中8=cmd, 低8=0) */
    frame[0] = LCD_OPCODE_WRITE_CMD;   /* 0x02 */
    frame[1] = 0x00;                   /* 地址 [23:16] */
    frame[2] = cmd;                    /* 地址 [15:8] = 命令字 */
    frame[3] = 0x00;                   /* 地址 [7:0] */
    for (uint32_t i = 0; i < len && i < 16; i++) {
        frame[4 + i] = data[i];
    }
    total = 4 + ((len < 16) ? len : 16);

    /* 整帧单线发送，指令/地址相位关闭，全部走 data 相位 */
    memset(&s_cmd, 0, sizeof(s_cmd));
    s_cmd.instruction.disabled  = true;
    s_cmd.address.disabled      = true;
    s_cmd.alt.disabled          = true;
    s_cmd.dummy_count           = 0;
    s_cmd.data.bus_width        = OSPI_LINE_SINGLE;
    s_cmd.data.frame_len        = 8;
    s_cmd.data.transfer_mode    = OSPI_TRANSFER_SEND_ONLY;
    s_cmd.data.disabled         = false;

    csi_ospi_config(&s_ospi, &s_cmd);

    LCD_CS_LOW();
    ret = csi_ospi_send(&s_ospi, frame, total, LCD_OSPI_TIMEOUT);
    LCD_CS_HIGH();

    if ((uint32_t)ret != total) {
        return -1;
    }
    return 0;
}

/*
 * 写显存色数据。
 * ST77916 QSPI 写色数据格式：0x32(单线) opcode + 24bit 地址(高8bit=0x2C 写显存, 单线)
 * + 像素数据(四线 QUAD)。
 *
 * 利用 SDK 特性：data.bus_width = QUAD 时，csi_ospi_send 会自动先发
 * instruction.value(0x32) 与 address.value(0x2C<<8)，再用四线发数据。
 */
static int lcd_write_color(const uint8_t *data, uint32_t len)
{
    int32_t ret;

    memset(&s_cmd, 0, sizeof(s_cmd));
    s_cmd.instruction.bus_width = OSPI_LINE_SINGLE;
    s_cmd.instruction.size      = OSPI_INSTRUCTION_8_BITS;
    s_cmd.instruction.value     = LCD_OPCODE_WRITE_COLOR;  /* 0x32 */
    s_cmd.instruction.disabled  = false;
    s_cmd.address.bus_width     = OSPI_LINE_SINGLE;
    s_cmd.address.size          = OSPI_ADDRESS_24_BITS;
    s_cmd.address.value         = ((uint32_t)0x2C) << 8;   /* 0x2C: RAMWR 写显存 */
    s_cmd.address.disabled      = false;
    s_cmd.alt.disabled          = true;
    s_cmd.dummy_count           = 0;
    s_cmd.data.bus_width        = OSPI_LINE_QUAD;          /* 色数据走四线 */
    s_cmd.data.frame_len        = 8;
    s_cmd.data.transfer_mode    = OSPI_TRANSFER_SEND_ONLY;
    s_cmd.data.disabled         = false;

    csi_ospi_config(&s_ospi, &s_cmd);

    LCD_CS_LOW();
    ret = csi_ospi_send(&s_ospi, data, len, LCD_OSPI_TIMEOUT);
    LCD_CS_HIGH();

    if ((uint32_t)ret != len) {
        return -1;
    }
    return 0;
}

/* ===========================================================================
 * 复位 / 初始化
 * =========================================================================*/

/* OSPI 数据/时钟引脚复用：D0-D3 = PB4-PB7，SCK = PA28
 *   数据线必须加上拉，否则四线读写时序异常导致屏不亮（参考 ospi 例子）。 */
static void lcd_ospi_pin_init(void)
{
    csi_pin_set_mux(LCD_OSPI_D0_PORT,  LCD_OSPI_D0_PIN,  LCD_OSPI_D0_FUNC);
    csi_pin_set_mux(LCD_OSPI_D1_PORT,  LCD_OSPI_D1_PIN,  LCD_OSPI_D1_FUNC);
    csi_pin_set_mux(LCD_OSPI_D2_PORT,  LCD_OSPI_D2_PIN,  LCD_OSPI_D2_FUNC);
    csi_pin_set_mux(LCD_OSPI_D3_PORT,  LCD_OSPI_D3_PIN,  LCD_OSPI_D3_FUNC);
    csi_pin_set_mux(LCD_OSPI_SCK_PORT, LCD_OSPI_SCK_PIN, LCD_OSPI_SCK_FUNC);

    /* 四条数据线加上拉 */
    csi_pin_mode(LCD_OSPI_D0_PORT, LCD_OSPI_D0_PIN, GPIO_MODE_PULLUP);
    csi_pin_mode(LCD_OSPI_D1_PORT, LCD_OSPI_D1_PIN, GPIO_MODE_PULLUP);
    csi_pin_mode(LCD_OSPI_D2_PORT, LCD_OSPI_D2_PIN, GPIO_MODE_PULLUP);
    csi_pin_mode(LCD_OSPI_D3_PORT, LCD_OSPI_D3_PIN, GPIO_MODE_PULLUP);
}

/* GPIO 控制脚初始化：CS=PA15、RST=PA7、背光=PA4，均为推挽输出 */
static void lcd_gpio_init(void)
{
    /* CS / 复位 / 背光复用为普通 GPIO */
    csi_pin_set_mux(LCD_CS_PORT,  LCD_CS_PIN,  PIN_FUNC_GPIO);
    csi_pin_set_mux(LCD_RST_PORT, LCD_RST_PIN, PIN_FUNC_GPIO);
    csi_pin_set_mux(LCD_BL_PORT,  LCD_BL_PIN,  PIN_FUNC_GPIO);

    /* 三个脚同属端口 A，共用句柄，统一配为输出 */
    csi_gpio_init(&s_lcd_gpio, LCD_CS_PORT);
    csi_gpio_dir(&s_lcd_gpio, LCD_CS_PIN | LCD_RST_PIN | LCD_BL_PIN, GPIO_DIRECTION_OUTPUT);

    /* CS 默认拉高（释放） */
    LCD_CS_HIGH();
}

/* ST77916 硬复位时序：先拉高稳定，再拉低 10ms，最后拉高并等 120ms
 *   ST77916 复位后需要 ~120ms 才能接收命令，否则初始化命令丢失导致屏不亮。 */
void ST7701_Reset(void)
{
    csi_gpio_write(&s_lcd_gpio, LCD_RST_PIN, GPIO_PIN_HIGH);
    mdelay(10);
    csi_gpio_write(&s_lcd_gpio, LCD_RST_PIN, GPIO_PIN_LOW);
    mdelay(10);
    csi_gpio_write(&s_lcd_gpio, LCD_RST_PIN, GPIO_PIN_HIGH);
    mdelay(120);
}

/* 初始化 OSPI 控制器 + 遍历命令表逐条下发 */
static int OSPI_Init(void)
{
    csi_error_t ret;

    /* 1. OSPI 数据/时钟引脚复用 */
    lcd_ospi_pin_init();

    /* 2. 初始化 DW_OSPI0 控制器（主机模式） */
    ret = csi_ospi_init(&s_ospi, LCD_OSPI_IDX);
    if (ret != CSI_OK) {
        printf("ST77916: csi_ospi_init failed (%d)\r\n", ret);
        return 0;
    }
    csi_ospi_mode(&s_ospi, OSPI_MASTER);
    csi_ospi_baud(&s_ospi, LCD_OSPI_BAUD_HZ);
    csi_ospi_cp_format(&s_ospi, OSPI_FORMAT_CPOL0_CPHA0);
    csi_ospi_select_slave(&s_ospi, 0);

    /* 3. 遍历厂商命令表逐条下发 */
    uint32_t n = sizeof(vendor_specific_init_new) / sizeof(vendor_specific_init_new[0]);
    for (uint32_t i = 0; i < n; i++) {
        const st77916_init_cmd_t *c = &vendor_specific_init_new[i];
        if (lcd_write_cmd(c->cmd, c->data, c->data_len) != 0) {
            printf("ST77916: init cmd 0x%02x failed\r\n", c->cmd);
            return 0;
        }
        if (c->delay_ms) {
            mdelay(c->delay_ms);
        }
    }

    printf("ST77916: panel init done (%lu cmds)\r\n", (unsigned long)n);
    return 1;
}

void ST77916_Init(void)
{
    ST7701_Reset();
    if (!OSPI_Init()) {
        printf("ST77916 Failed to be initialized\r\n");
    }
}

/* ===========================================================================
 * 显示窗口与刷屏
 * =========================================================================*/

/*
 * 设置显示窗口 [Xstart,Xend] x [Ystart,Yend] 并写入色数据。
 *   0x2A 列地址(CASET)，0x2B 行地址(RASET)，0x2C 写显存(RAMWR)。
 *   color 为 RGB565 像素数组，像素数 = (Xend-Xstart+1)*(Yend-Ystart+1)。
 */
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t *color)
{
    uint8_t caset[4];
    uint8_t raset[4];
    uint32_t pixels = (uint32_t)(Xend - Xstart + 1) * (uint32_t)(Yend - Ystart + 1);

    /* 列地址范围 */
    caset[0] = (uint8_t)(Xstart >> 8);
    caset[1] = (uint8_t)(Xstart & 0xFF);
    caset[2] = (uint8_t)(Xend >> 8);
    caset[3] = (uint8_t)(Xend & 0xFF);
    lcd_write_cmd(0x2A, caset, 4);

    /* 行地址范围 */
    raset[0] = (uint8_t)(Ystart >> 8);
    raset[1] = (uint8_t)(Ystart & 0xFF);
    raset[2] = (uint8_t)(Yend >> 8);
    raset[3] = (uint8_t)(Yend & 0xFF);
    lcd_write_cmd(0x2B, raset, 4);

    /* 写显存（四线色数据），每像素 2 字节 */
    lcd_write_color((const uint8_t *)color, pixels * 2);
}

/* ===========================================================================
 * 点亮自测：画几条彩条，验证 QSPI 通路与刷屏
 * =========================================================================*/
static void test_draw_bitmap(void)
{
    /* 每次画一行（360 像素），用纯色填充，逐段切换颜色 */
    static uint16_t line[EXAMPLE_LCD_WIDTH];
    const uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000}; /* 红绿蓝白黑 */
    uint32_t bands = sizeof(colors) / sizeof(colors[0]);
    uint16_t band_h = EXAMPLE_LCD_HEIGHT / bands;

    for (uint32_t b = 0; b < bands; b++) {
        for (uint32_t x = 0; x < EXAMPLE_LCD_WIDTH; x++) {
            line[x] = colors[b];
        }
        uint16_t y0 = (uint16_t)(b * band_h);
        uint16_t y1 = (uint16_t)((b == bands - 1) ? (EXAMPLE_LCD_HEIGHT - 1) : (y0 + band_h - 1));
        for (uint16_t y = y0; y <= y1; y++) {
            LCD_addWindow(0, y, EXAMPLE_LCD_WIDTH - 1, y, line);
        }
    }
    printf("LCD: color bar drawn\r\n");
}

/* ===========================================================================
 * 背光（GPIO 开关式，替代原 LEDC PWM）
 * =========================================================================*/
void Backlight_Init(void)
{
    /* GPIO 已在 lcd_gpio_init 配置为输出，这里按默认亮度点亮 */
    Set_Backlight(LCD_Backlight);
}

void Set_Backlight(uint8_t Light)
{
    csi_gpio_pin_state_t on_level, off_level;

    if (Light > Backlight_MAX) {
        Light = Backlight_MAX;
    }
    LCD_Backlight = Light;

    /* 按 LCD_BL_ACTIVE_HIGH 选择点亮电平（兼容高/低有效背光） */
#if LCD_BL_ACTIVE_HIGH
    on_level  = GPIO_PIN_HIGH;
    off_level = GPIO_PIN_LOW;
#else
    on_level  = GPIO_PIN_LOW;
    off_level = GPIO_PIN_HIGH;
#endif
    /* 开关式：Light>0 点亮，Light==0 熄灭 */
    csi_gpio_write(&s_lcd_gpio, LCD_BL_PIN, (Light > 0) ? on_level : off_level);
    printf("LCD: backlight %s (PA4=%d)\r\n",
           (Light > 0) ? "ON" : "OFF",
           (Light > 0) ? on_level : off_level);
}

/* ===========================================================================
 * 对外初始化总入口
 * =========================================================================*/
void LCD_Init(void)
{
    lcd_gpio_init();    /* CS/复位/背光 GPIO 配置 */
    Backlight_Init();   /* 先点亮背光（与显示初始化解耦，便于判断背光是否正常） */
    ST77916_Init();     /* 复位 + OSPI + 命令表 */
    test_draw_bitmap(); /* 彩条自测 */
}

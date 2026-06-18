# GUI 工程引脚对照表（ST77916 OSPI 屏 + CST816 触摸）

> 本文档说明把 ESP-IDF 版 ST77916 / CST816 驱动移植到 **TD1601（T-Head Xuantie E906FD，
> RISC-V RV32 IMAFDC）** 后所使用的引脚分配。
>
> LCD 显示总线采用 **OSPI（DW_OSPI0）四线（QUAD）模式**，参考 `projects/drivers/ospi` 例子；
> 触摸采用 **IIC0**。下表引脚已按用户提供的实际接线定义。

---

## 1. 总引脚对照表

| 功能 | 模块 | TD1601 引脚 | 复用功能(func) | 方向 | 备注 |
|------|------|-------------|----------------|------|------|
| OSPI SDO0 (D0) | LCD | **PB4** | `PB4_SPI1_D0` | — | 四线数据 0 |
| OSPI SDO1 (D1) | LCD | **PB5** | `PB5_SPI1_D1` | — | 四线数据 1 |
| OSPI SDO2 (D2) | LCD | **PB6** | `PB6_SPI1_D2` | — | 四线数据 2 |
| OSPI SDO3 (D3) | LCD | **PB7** | `PB7_SPI1_D3` | — | 四线数据 3 |
| OSPI SCL (SCK) | LCD | **PA28** | `PA28_SPI1_SCK` | — | 时钟 |
| LCD 片选 CS | LCD | **PA15** | `PIN_FUNC_GPIO` | 输出 | 软件 CS，传输前后拉低/拉高 |
| LCD 复位 RESET | LCD | **PA7** | `PIN_FUNC_GPIO` | 输出 | 10ms 低 / 50ms 高 |
| LCD 背光 BKL | LCD | **PA4** | `PIN_FUNC_GPIO` | 输出 | GPIO 开关，开机常亮 |
| TP I2C SDA | 触摸 | **PA9** | `PA9_IIC0_SDA` | — | IIC0 |
| TP I2C SCL | 触摸 | **PA8** | `PA8_IIC0_SCL` | — | IIC0 |
| TP 中断 INT | 触摸 | **PA0** | `PIN_FUNC_GPIO` | 输入 | 下降沿中断，上拉 |
| TP 复位 RST | 触摸 | **PA6** | `PIN_FUNC_GPIO` | 输出 | 10ms 低 / 50ms 高 |
| TP I2C 地址 | 触摸 | 0x15 (7-bit) | — | — | CST816S，400kHz |

> 端口归属：CS/RST/BKL/TP_INT/TP_RST/SCK/SDA/SCL 均在 **PORTA**；OSPI 数据 D0-D3 在 **PORTB**。

---

## 2. LCD（ST77916，OSPI 四线）

- **总线**：`csi_ospi`（**DW_OSPI0** 控制器，索引 0，`DW_OSPI0_BASE = 0x6000C000`）。
- **波特率**：`LCD_OSPI_BAUD_HZ = 40MHz`，格式 `OSPI_FORMAT_CPOL0_CPHA0`。
- **引脚复用**：D0-D3 = PB4-PB7（`PBx_SPI1_Dx`），SCK = PA28（`PA28_SPI1_SCK`），
  通过 `csi_pin_set_mux` 配置；**CS 用软件 GPIO（PA15）** 控制。
- **四线写时序**（参考 ospi 例子）：
  1. `csi_ospi_config(&cmd)` 配置本次传输的 指令/地址/数据 相位与位宽；
  2. 软件 CS 拉低 → `csi_ospi_send(...)` → 软件 CS 拉高。
- **命令格式**（与 esp_lcd_st77916 一致）：
  - 写命令：指令 `0x02`（单线 8bit）+ 地址 24bit（高 8bit = 命令字）+ 参数（单线）。
  - 写显存：指令 `0x32`（单线）+ 地址 24bit（高 8bit = `0x2C` RAMWR）+ 色数据（**四线 QUAD**）。
- **初始化命令表**：`vendor_specific_init_new[]` 内容与原 ESP 版逐字节一致，
  在 `OSPI_Init()` 内手动逐条下发 + `mdelay(delay_ms)`。
- **复位时序**（`ST7701_Reset()`）：PA7 拉低 10ms → 拉高 50ms。
- **背光**（`Backlight_Init/Set_Backlight`）：PA4 GPIO 开关式，`Light>0` 点亮、`==0` 熄灭。

---

## 3. 触摸（CST816S）

- **总线**：`csi_iic`（**IIC0**，索引 0），主机模式 / 7-bit 地址 / **400kHz**（`IIC_BUS_SPEED_FAST`）。
- **I2C 地址**：`0x15`（7-bit）。
- **引脚**：SDA = PA9(`PA9_IIC0_SDA`)，SCL = PA8(`PA8_IIC0_SCL`)。
- **寄存器读写**：`csi_iic_mem_receive` / `csi_iic_mem_send`（8-bit 寄存器地址）。
- **关键寄存器**：数据起始 `0x02`、芯片 ID `0xA7`、自动休眠 `0xFE`。
- **坐标解析**（从 `0x02` 读 5 字节）：
  ```
  byte0 = 触点数
  x = ((byte1 & 0x0F) << 8) | byte2
  y = ((byte3 & 0x0F) << 8) | byte4
  ```
- **复位时序**（`cst816_reset()`）：PA6 拉低 10ms → 拉高 50ms。
- **中断**：PA0 配置为输入 + 上拉 + **下降沿中断**，回调置标志，
  主循环 `Touch_Poll()` 在临界区（`csi_irq_save/restore`）读并清标志后读坐标。

---

## 4. 移植关键改动（ESP-IDF → TD1601 CSI）

| ESP-IDF | TD1601 CSI |
|---------|-----------|
| `esp_lcd_panel_io_spi` / `spi_bus_initialize` | `csi_ospi_init` + `csi_ospi_config` + `csi_ospi_send`（四线） |
| `esp_lcd_new_panel_st77916`（自动下发命令） | 手动遍历 `vendor_specific_init_new[]` 逐条下发 |
| `esp_lcd_panel_draw_bitmap` | 自实现 `LCD_addWindow`（0x2A/0x2B/0x32） |
| 控制器内部 CS | 软件 GPIO CS（PA15，传输前后拉低/拉高） |
| `i2c_param_config` / `i2c_driver_install` | `csi_iic_init` + `csi_iic_mode/addr_mode/speed` |
| `esp_lcd_panel_io_rx/tx_param`（I2C） | `csi_iic_mem_receive` / `csi_iic_mem_send` |
| `Set_EXIO(TCA9554_EXIO1/2)` | `csi_gpio_write(PA6 / PA7, ...)` 直连 |
| LEDC PWM 背光 | `csi_gpio_write(PA4, ...)` 开关 |
| `vTaskDelay(pdMS_TO_TICKS(n))` | `mdelay(n)` |
| `portENTER/EXIT_CRITICAL` | `csi_irq_save` / `csi_irq_restore` |
| `ESP_LOGx` / `ESP_ERROR_CHECK` | `printf` + 返回值判断 |
| `TCA9554PWR` 扩展IO 芯片 | **已移除**（CS/复位/背光改 PA 直连） |
| `lvgl.h` / FreeRTOS / esp_lcd_touch | **已移除** |

---

## 5. 注意事项

1. **OSPI 四线 = SPI1 的 D0-D3 + SCK**：D0-D3 复用在 PB4-PB7，SCK 在 PA28，
   CS 不走控制器内部信号，而是软件 GPIO（PA15）。
2. **OSPI 与 SPI1 共用引脚资源**：PB4-PB7 / PA28 复用为 SPI1 数据/时钟，
   使用 LCD 时这些脚被 OSPI 占用，勿再用于其它 SPI1 功能。
3. **仅完成软件移植与编译**：已 `make` 通过、符号链接正确，但**点亮屏幕、读到真实坐标
   需接好硬件后实测**。彩条自测 `test_draw_bitmap()` 用于上电验证 OSPI 刷屏通路。

---

## 6. 构建

```bash
cd projects/gui
make                    # 编译 -> Obj/gui.elf / .bin / .ihex
bash aft_build_macos.sh # 打包烧录镜像（可选）
```

实测体积：`text 32700 / data 456 / bss 2660`（含 ST77916 命令表 + OSPI/IIC 驱动 + 触摸）。

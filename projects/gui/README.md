# GUI 工程说明

该工程运行在 TD1601 EVB 裸机环境中，当前功能是初始化 ST77916 QSPI LCD、初始化 CST816 触摸控制器，把编译进固件的 JPEG 图片同步到 LittleFS，再通过 TJpgDec 流式解码显示到屏幕。主循环中持续轮询触摸事件并打印坐标。

## 启动流程

`src/main.c` 的当前流程：

1. `board_init()`
   - 执行芯片启动初始化。
   - 当前默认把 UART0 初始化为普通数据串口，波特率为 921600，接收使用中断。
   - 如把 `src/gui_uart.h` 中的 `GUI_UART_MODE` 改成 `GUI_UART_MODE_LOG`，UART0 会恢复为日志串口，`printf()` 正常输出。

2. `LCD_Init()`
   - 初始化 LCD 控制 GPIO：CS=PA15、RST=PA7、背光=PA4。
   - 点亮背光；当前背光是开关式控制，PA4 低电平点亮。
   - 初始化 ST77916：复位屏幕、初始化 DW_OSPI0、下发厂商初始化命令表。
   - 绘制红/绿/蓝/白/黑彩条自测。

3. `Touch_Init()`
   - 初始化 CST816 触摸芯片。
   - I2C 使用 400kHz，寄存器读写走 CSI IIC 接口。
   - RST=PA12，INT=PA11，INT 使用下降沿中断。
   - 中断回调只置位标志，主循环通过 `Touch_Poll()` 读取坐标。

4. `jpg_display_run()`
   - 挂载 LittleFS。
   - 检查 `/img/1.jpg` 是否与当前编译进固件的 `img_1_jpg[]` 一致。
   - 文件不存在、大小不同或内容不同都会重写 LittleFS 中的 `/img/1.jpg`。
   - 使用 TJpgDec 从 LittleFS 文件流式读取 JPEG，解码输出 RGB565。
   - 每个 MCU 块直接调用 `LCD_addWindow()` 写入 LCD，无需整帧 framebuffer。

5. 主循环
   - `GUI_UART_MODE_DATA` 模式下做串口回环测试：上位机下发什么数据，设备就原样回发什么数据。
   - `GUI_UART_MODE_LOG` 模式下，`Touch_Poll()` 有触摸事件时读取并打印 `x/y/points`。

## LCD 通路

LCD 驱动文件：

- `src/ST77916.c`
- `src/ST77916.h`

屏幕参数：

- 分辨率：360 x 360
- 像素格式：RGB565
- 命令写入：单线 SPI，`LCD_OPCODE_WRITE_CMD = 0x02`
- 像素写入：QSPI 四线，`LCD_OPCODE_WRITE_COLOR = 0x32`

OSPI 参数：

- 控制器：DW_OSPI0，`LCD_OSPI_IDX = 0`
- 命令波特率：10MHz，`LCD_OSPI_CMD_BAUD_HZ`
- 像素数据波特率：50MHz，`LCD_OSPI_DATA_BAUD_HZ`
- 数据线：D0=PB4、D1=PB5、D2=PB6、D3=PB7、SCK=PA28
- 高速线配置：D0-D3/SCK 使用 `GPIO_MODE_PULLNONE`，驱动强度 `PIN_DRIVE_LV3`

写像素时序：

1. 低速单线发送帧头：`0x32 0x00 0x2C 0x00`
2. CS 保持低电平，切换到 QUAD + 16-bit frame
3. 逐像素写入 RGB565 数据
4. 等待 FIFO 空和总线空闲，释放 CS，并恢复单线 8-bit 状态

## LittleFS 分区

LittleFS 移植文件：

- `src/lfs_port.c`
- `src/lfs_port.h`

Flash 布局：

- Flash XIP 起始地址：`0x18000000`
- 代码和只读数据：`0x18000000 ~ 0x1806FFFF`
- LittleFS 分区：`0x18070000 ~ 0x1807FFFF`
- LittleFS 分区大小：64KB
- Block/Sector：4KB
- Page：256B
- Block 数量：16

注意事项：

- `lfs_port.c` 复用系统启动时已经初始化好的 `g_spiflash`。
- 不要在 LittleFS 移植层再次调用 `csi_spiflash_qspi_init()`，否则 XIP 执行期间复位 QSPI 控制器会导致取指异常。
- 挂载失败时会自动格式化再挂载。

## JPEG 显示

JPEG 显示文件：

- `src/jpg_display.c`
- `src/jpg_display.h`
- `src/TJpgDec/tjpgd.c`
- `src/TJpgDec/tjpgd.h`
- `src/img_1.c`
- `src/img_1.h`

运行时路径：

- LittleFS 目录：`/img`
- LittleFS 文件：`/img/1.jpg`
- 编译进固件的数组：`img_1_jpg[]`
- 编译进固件的长度：`img_1_jpg_len`

`jpg_display_run()` 的写入策略：

- 文件不存在：写入当前固件内置 JPEG
- 文件大小不同：删除旧文件并重写
- 文件大小相同但内容不同：逐块比较后重写
- 文件内容一致：跳过写入，直接解码显示
- 写入失败时会格式化 LittleFS 并重试一次

## 替换图片

1. 用新的 JPEG 覆盖工程图片：

   ```sh
   cp /path/to/new.jpg projects/gui/1.jpg
   ```

2. 重新生成 C 数组：

   ```sh
   python3 projects/gui/utilities/jpg_to_c.py projects/gui/1.jpg projects/gui/src/img_1.c
   ```

   该脚本会同时更新：

   - `projects/gui/src/img_1.c`
   - `projects/gui/src/img_1.h`

3. 编译 GUI 工程：

   ```sh
   make -C projects/gui
   ```

4. 如需生成烧录镜像，进入 `projects/gui` 后执行：

   ```sh
   bash aft_build_macos.sh
   ```

设备启动后会自动把新的 `img_1_jpg[]` 同步到 LittleFS 的 `/img/1.jpg`，再解码显示。

## UART0 模式开关

GUI 工程当前默认把 UART0 作为普通数据串口使用。串口引脚来自板级配置：

- TX：PA18，`PA18_UART0_TX`
- RX：PA17，`PA17_UART0_RX`
- UART：`CONSOLE_IDX = 0`
- 默认波特率：921600

模式开关位于 `src/gui_uart.h`：

```c
#define GUI_UART_MODE_LOG       0
#define GUI_UART_MODE_DATA      1

#ifndef GUI_UART_MODE
#define GUI_UART_MODE           GUI_UART_MODE_DATA
#endif
```

两种模式：

- `GUI_UART_MODE_DATA`：当前默认模式。`board_init()` 调用 `gui_uart_data_init()`，UART0 初始化为 8N1，RX FIFO 可读中断会把数据搬到 2048 字节软件环形缓冲区；`printf()` 被重定向为空函数，不会输出日志。
- `GUI_UART_MODE_LOG`：日志模式。`board_init()` 调用 `console_init()`，UART0 专门用于日志打印，`printf()` 正常输出。

当前 `src/main.c` 在 DATA 模式下内置了一个串口回环测试：

```c
uint32_t rx_len = gui_uart_read(uart_echo_buf, sizeof(uart_echo_buf));
if (rx_len > 0U) {
    (void)gui_uart_send(uart_echo_buf, rx_len);
}
```

普通串口模式可用接口：

```c
int32_t gui_uart_send(const uint8_t *data, uint32_t len);
uint32_t gui_uart_available(void);
uint32_t gui_uart_read(uint8_t *buf, uint32_t len);
uint8_t gui_uart_rx_overflowed(void);
void gui_uart_clear_rx(void);
```

注意：UART0 只有一路硬件资源。DATA 模式下日志默认关闭，避免日志字节混入普通串口协议。

## 主要源文件

- `src/main.c`：主流程入口。
- `src/gui_uart.c/h`：UART0 日志/普通串口模式开关，普通串口中断接收和发送接口。
- `src/ST77916.c/h`：LCD 初始化、窗口设置、RGB565 像素写入、背光控制。
- `src/CST816.c/h`：触摸初始化、中断标志、坐标读取。
- `src/lfs_port.c/h`：LittleFS 到 SPI Flash 的底层适配。
- `src/jpg_display.c/h`：JPEG 写入 LittleFS、TJpgDec 解码、LCD 显示。
- `utilities/jpg_to_c.py`：把 JPEG 原始字节转换为 C 数组。
- `Makefile`：macOS/xPack RISC-V GCC 构建脚本。

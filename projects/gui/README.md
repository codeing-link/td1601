# GUI 工程说明

该工程运行在 TD1601 EVB 裸机环境中，当前默认功能是初始化 ST77916 QSPI LCD、初始化 CST816 触摸控制器，启动后扫描 LittleFS 中已有 JPG 图片并显示上次停留的图片；也可以通过 UART 接收 JPG 文件，保存到 LittleFS，校验成功后按文件名保存为正式图片，再通过 TJpgDec 从 LittleFS 流式解码显示到屏幕。存在多张图片时，可通过左滑/右滑循环切换。

## 启动流程

`src/main.c` 的当前流程：

1. `board_init()`
   - 执行芯片启动初始化。
   - 当前默认 `GUI_UART_MODE_DATA` 下不初始化日志口，UART0 由 `transport_uart` 在图片更新应用初始化时接管。
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
   - 中断回调只置位标志，主循环通过 `Touch_Poll()` 读取坐标和手势。

4. `image_update_init(&g_uart_transport)`
   - 挂载 LittleFS。
   - 初始化 UART transport，UART RX 中断只负责把字节放入 2048 字节 ring buffer。
   - 初始化文件传输状态机，等待 PC 或未来 BLE App 发送 START/DATA/END 包。
   - 扫描 LittleFS 根目录下的 `.jpg/.jpeg` 图片。
   - 如果存在图片，优先读取 `/.current_image` 并显示上次实际显示的图片；记录不存在或目标图片已删除时显示排序后的第一张图片。
   - 如果不存在图片，则清黑屏，不显示默认图片。

5. 主循环
   - `GUI_UART_MODE_DATA` 模式下调用 `image_update_poll()`，从 transport 取字节，解析协议，写 LittleFS，完成后自动显示刚收到的 JPG。
   - `GUI_UART_MODE_DATA` 模式下同时读取 CST816 手势，左滑显示下一张，右滑显示上一张；切换后会更新 `/.current_image`。
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
- 代码和只读数据：`0x18000000 ~ 0x1805FFFF`
- LittleFS 分区：`0x18060000 ~ 0x1807FFFF`
- LittleFS 分区大小：128KB
- Block/Sector：4KB
- Page：256B
- Block 数量：32

注意事项：

- `lfs_port.c` 复用系统启动时已经初始化好的 `g_spiflash`。
- 不要在 LittleFS 移植层再次调用 `csi_spiflash_qspi_init()`，否则 XIP 执行期间复位 QSPI 控制器会导致取指异常。
- 挂载失败时会自动格式化再挂载。

## JPEG 显示

JPEG 显示文件：

- `src/image_gallery.c`
- `src/image_gallery.h`
- `src/jpeg_viewer.c`
- `src/jpeg_viewer.h`
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

`jpeg_viewer_show_file(const char *path)` 是当前图片接收完成后的统一显示入口。例如收到 `1.jpg` 后保存为 `/1.jpg` 并显示：

```c
jpeg_viewer_show_file("/1.jpg");
```

该接口要求 LittleFS 已经挂载，内部只负责打开指定 JPG 文件、TJpgDec 流式解码、调用 `LCD_addWindow()` 刷屏。

`image_gallery.c/h` 是当前 DATA 模式的图片库入口，负责：

- 扫描 LittleFS 根目录下最多 16 张 `.jpg/.jpeg` 图片，并按文件名排序。
- 上电时读取 `/.current_image`，直接显示上次实际显示的图片。
- 没有图片时清黑屏，不写入或显示默认图片。
- 左滑切换下一张，右滑切换上一张，切换顺序循环。
- 每次成功显示图片后，把当前路径写入 `/.current_image`。

`jpeg_viewer_show_file()` 只负责解码显示单个文件；启动恢复、图片列表、左右滑切换和当前图片持久化都由 `image_gallery` 管理。

## UART/BLE 图片传输

当前默认通过 UART 模拟未来 BLE 串口透传。核心设计是 transport 抽象：

```c
typedef struct {
    int (*init)(void);
    int (*send)(const uint8_t *data, uint32_t len);
    int (*recv)(uint8_t *data, uint32_t max_len);
    int (*set_rx_callback)(void (*cb)(const uint8_t *data, uint32_t len));
} transport_t;
```

当前初始化：

```c
image_update_init(&g_uart_transport);
```

未来 BLE ready 后切换为：

```c
image_update_init(&g_ble_transport);
```

模块边界：

- `transport_if.h`：统一 transport 接口。
- `transport_uart.c/h`：UART transport，封装 `gui_uart_read()` 和 `gui_uart_send()`。
- `transport_ble.c/h`：BLE transport 占位，后续只在这里对接 BLE 透传。
- `file_transfer.c/h`：START/DATA/END 协议解析、ACK/NACK、CRC、seq/offset 校验；不直接调用 UART。
- `fs_image.c/h`：LittleFS 临时文件保存、CRC32 校验、rename。
- `jpeg_viewer.c/h`：从 LittleFS 文件解码显示。
- `image_gallery.c/h`：LittleFS 图片库，负责启动扫描、上次图片恢复、左右滑切换和当前图片记录。
- `app_image_update.c/h`：应用入口，连接 transport、file_transfer、image_gallery 和触摸手势。

文件策略：

- 接收中：`/image_tmp.jpg`
- 正式图：按 START 包里的文件名保存到根目录，例如 `1.jpg` -> `/1.jpg`，`2.jpg` -> `/2.jpg`。
- 只有完整接收并且 CRC32 校验成功后，才执行删除同名旧文件和 rename。
- 接收失败会删除 `/image_tmp.jpg`，不会破坏已存在的正式图片。
- DATA 模式接收开始时会清理旧调试路径 `/img/1.jpg`，避免历史内置图片占用 128KB LittleFS 分区导致新图写入空间不足。
- START 时会先检查 LittleFS 剩余空间。如果空间不足，MCU 返回 `STORAGE_FULL`，PC 脚本会提示是否格式化 LittleFS。

协议字段全部使用小端格式。CRC16 使用 CRC-16/CCITT-FALSE，CRC32 使用标准 ZIP/以太网 CRC32。

限制：

- `FT_MAX_CHUNK_SIZE = 512`
- 默认 PC 发送 chunk 为 240，用于模拟 BLE 透传小包。
- `FS_IMAGE_MAX_FILE_SIZE = 120KB`，给 128KB LittleFS 分区预留元数据空间。

## 串口发送 JPG 测试

安装 Python 依赖：

```sh
python3 -m pip install pyserial
```

编译固件：

```sh
make -C projects/gui
```

macOS 示例：

```sh
python3 projects/gui/utilities/send_jpg_uart.py \
  --port /dev/tty.usbserial-310 \
  --baud 921600 \
  --file test.jpg \
  --chunk 240
```

Windows 示例：

```sh
python3 projects/gui/utilities/send_jpg_uart.py \
  --port COM3 \
  --baud 921600 \
  --file test.jpg \
  --chunk 240
```

可选参数：

- `--timeout`：每包等待 ACK 的超时时间，默认 2 秒。
- `--retry`：每包超时或 NACK 后的重试次数，默认 5 次。
- `--inter-packet-delay`：包间延时，调试低速链路时可使用。
- `--format-on-full`：空间不足时自动发送 FORMAT 命令格式化 LittleFS，然后重新开始传输。会清空 LittleFS 中已有图片，适合自动化测试。
- `--no-format-on-full`：空间不足时直接停止传输，不进行交互提示，也不会格式化，适合需要保留已有图片的场景。

空间不足时的三种用法：

1. 默认交互模式：脚本收到 `STORAGE_FULL` 后询问是否格式化。

   ```sh
   python3 projects/gui/utilities/send_jpg_uart.py \
     --port COM3 \
     --baud 921600 \
     --file 2.jpg \
     --chunk 240
   ```

   提示如下，输入 `y` 或 `yes` 会格式化，直接回车或输入其他内容会停止传输：

   ```text
   MCU reports storage full. Format LittleFS and erase old images? [y/N]:
   ```

2. 自动格式化模式：空间不足时不询问，直接格式化并重传。

   ```sh
   python3 projects/gui/utilities/send_jpg_uart.py \
     --port COM3 \
     --baud 921600 \
     --file 2.jpg \
     --chunk 240 \
     --format-on-full
   ```

3. 禁止格式化模式：空间不足时直接停止，保留板子里已有图片。

   ```sh
   python3 projects/gui/utilities/send_jpg_uart.py \
     --port COM3 \
     --baud 921600 \
     --file 2.jpg \
     --chunk 240 \
     --no-format-on-full
   ```

发送流程：

1. PC 读取 JPG 并计算 CRC32。
2. 发送 START，等待 ACK。
3. 按 chunk 分片发送 DATA，每包等待 ACK。
4. 发送 END，等待最终 ACK。
5. 如果 START 返回 `STORAGE_FULL`，脚本提示是否格式化；确认后发送 FORMAT 命令并重发 START。
6. MCU 校验 `/image_tmp.jpg` 的 CRC32，成功后 rename 为按文件名生成的正式路径。
7. MCU 调用 `image_gallery_show_path()` 刷新图片列表、记录当前图片并自动显示新收到的图片。

## BLE 替换步骤

BLE 硬件 ready 后，只需要实现 `transport_ble.c`：

1. `transport_ble_init()` 初始化 BLE 串口透传服务。
2. `transport_ble_send()` 发送 ACK/NACK 到手机 App。
3. `transport_ble_recv()` 从 BLE RX ring buffer 取数据。
4. 如 BLE SDK 是事件回调模型，可在 `transport_ble_set_rx_callback()` 中接入回调，再由本层缓存到 ring buffer。
5. 在 `main.c` 中把 `image_update_init(&g_uart_transport)` 替换为 `image_update_init(&g_ble_transport)`。

`file_transfer.c`、`fs_image.c`、`image_gallery.c`、`jpeg_viewer.c` 不需要改。

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

该内置图片流程主要用于 `GUI_UART_MODE_LOG` 调试模式。当前默认 `GUI_UART_MODE_DATA` 下，设备启动后会扫描 LittleFS 根目录已有图片；有图则显示上次实际显示的图片，无图则空屏等待 UART/BLE transport 下发 JPG。收到新 JPG 后按文件名保存、加入图片库并显示。

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

- `GUI_UART_MODE_DATA`：当前默认模式。UART0 由 `transport_uart` 初始化为 8N1，RX FIFO 可读中断会把数据搬到 2048 字节软件环形缓冲区；`printf()`、`puts()`、`putchar()`、`fputc()` 被重定向为空函数，不会输出日志。
- `GUI_UART_MODE_LOG`：日志模式。`board_init()` 调用 `console_init()`，UART0 专门用于日志打印，`printf()` 正常输出。

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
- `src/transport_if.h`：UART/BLE 统一 transport 接口。
- `src/transport_uart.c/h`：UART transport。
- `src/transport_ble.c/h`：BLE transport 占位。
- `src/file_transfer.c/h`：图片传输协议状态机、ACK/NACK、CRC 和分片校验。
- `src/fs_image.c/h`：LittleFS 图片临时文件和正式文件管理。
- `src/jpeg_viewer.c/h`：从 LittleFS 文件解码显示。
- `src/image_gallery.c/h`：扫描 LittleFS 图片、恢复上次显示、左右滑切换和记录当前图片。
- `src/app_image_update.c/h`：图片更新应用入口，驱动传输状态机和触摸切图。
- `src/ST77916.c/h`：LCD 初始化、窗口设置、RGB565 像素写入、背光控制。
- `src/CST816.c/h`：触摸初始化、中断标志、坐标读取。
- `src/lfs_port.c/h`：LittleFS 到 SPI Flash 的底层适配。
- `src/jpg_display.c/h`：JPEG 写入 LittleFS、TJpgDec 解码、LCD 显示。
- `utilities/jpg_to_c.py`：把 JPEG 原始字节转换为 C 数组。
- `utilities/send_jpg_uart.py`：通过 UART 发送 JPG 的 PC/macOS 测试工具。
- `Makefile`：macOS/xPack RISC-V GCC 构建脚本。

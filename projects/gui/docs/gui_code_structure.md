# GUI 工程 C 文件结构说明

本文档用于帮助开发者快速读懂 `projects/gui` 工程。当前工程的主要功能是：初始化 LCD 和触摸，默认通过 UART 接收 JPG 图片，保存到 LittleFS，校验成功后从文件系统读取 JPG 并解码显示。未来 BLE 接入后，只替换 transport 层，不重写图片接收和显示核心逻辑。

## 1. 推荐阅读顺序

建议按以下顺序阅读源码：

1. `src/main.c`
2. `src/board_init.c`
3. `src/app_image_update.c`
4. `src/transport_if.h`
5. `src/transport_uart.c`
6. `src/file_transfer.c`
7. `src/fs_image.c`
8. `src/jpeg_viewer.c`
9. `src/ST77916.c`
10. `src/CST816.c`
11. `src/lfs_port.c`

如果只是开发手机 App，重点看：

- `docs/image_transfer_protocol.md`
- `src/file_transfer.h`
- `utilities/send_jpg_uart.py`

如果只是移植 BLE，重点看：

- `src/transport_if.h`
- `src/transport_ble.c`
- `src/transport_uart.c`
- `src/app_image_update.c`

## 2. 主流程

入口文件：

- `src/main.c`

当前默认宏：

```c
#define GUI_UART_MODE GUI_UART_MODE_DATA
```

DATA 模式主流程：

```text
main()
  ↓
board_init()
  ↓
LCD_Init()
  ↓
Touch_Init()
  ↓
image_update_init(&g_uart_transport)
  ↓
while (1) {
    image_update_poll();
    mdelay(1);
}
```

LOG 模式主要用于调试，会恢复 `printf` 日志输出，并执行原来的内置 JPG 显示路径。

## 3. 启动和 UART 模式

### `src/board_init.c`

职责：

- 调用芯片启动初始化 `__ChipInitHandler()`
- LOG 模式下初始化 console 串口
- DATA 模式下不提前初始化 UART，UART 由 transport 层接管

为什么 DATA 模式不在 `board_init()` 初始化 UART：

- 当前 UART 是普通数据通道，不是日志通道
- 未来切换 BLE 时，`board_init()` 不应关心底层传输类型
- `image_update_init(&g_uart_transport)` 或 `image_update_init(&g_ble_transport)` 才是应用层选择 transport 的位置

### `src/gui_uart.c` / `src/gui_uart.h`

职责：

- 管理 UART0 的数据模式
- UART RX 中断只负责把硬件 FIFO 数据搬到软件 ring buffer
- 提供 `gui_uart_read()` 和 `gui_uart_send()`
- DATA 模式下屏蔽日志输出，避免日志字节混入传输协议

关键点：

- RX ring buffer 默认 2048 bytes
- UART 波特率默认 921600
- DATA 模式下 `printf()`、`puts()`、`putchar()`、`fputc()` 被链接器 wrap 成空输出

常用接口：

```c
int32_t gui_uart_data_init(void);
int32_t gui_uart_send(const uint8_t *data, uint32_t len);
uint32_t gui_uart_read(uint8_t *buf, uint32_t len);
uint32_t gui_uart_available(void);
```

## 4. Transport 抽象层

### `src/transport_if.h`

职责：

- 定义统一的 transport 接口
- 让文件传输协议不依赖 UART 或 BLE 的具体 API

接口：

```c
typedef struct {
    int (*init)(void);
    int (*send)(const uint8_t *data, uint32_t len);
    int (*recv)(uint8_t *data, uint32_t max_len);
    int (*set_rx_callback)(void (*cb)(const uint8_t *data, uint32_t len));
} transport_t;
```

设计原则：

- `file_transfer.c` 只能通过 `transport_t` 收发数据
- UART、BLE、USB CDC 等底层链路都可以实现同一个接口
- App 层只需要替换传入 `image_update_init()` 的 transport 实例

### `src/transport_uart.c` / `src/transport_uart.h`

职责：

- 当前 UART transport 实现
- `init()` 调用 `gui_uart_data_init()`
- `send()` 调用 `gui_uart_send()`
- `recv()` 调用 `gui_uart_read()`

对外实例：

```c
extern const transport_t g_uart_transport;
```

### `src/transport_ble.c` / `src/transport_ble.h`

职责：

- BLE transport 占位
- 当前未接 BLE 硬件，函数返回空实现或失败
- 未来 BLE ready 后，只需要在这里对接 BLE 串口透传

对外实例：

```c
extern const transport_t g_ble_transport;
```

未来替换方式：

```c
image_update_init(&g_uart_transport);
```

改为：

```c
image_update_init(&g_ble_transport);
```

## 5. 图片更新应用层

### `src/app_image_update.c` / `src/app_image_update.h`

职责：

- 作为图片更新功能的应用入口
- 挂载 LittleFS
- 初始化文件传输状态机
- 在主循环中驱动 `file_transfer_poll()`

接口：

```c
int image_update_init(const transport_t *transport);
void image_update_poll(void);
```

调用位置：

- `main.c` 中调用 `image_update_init(&g_uart_transport)`
- 主循环中调用 `image_update_poll()`

## 6. 文件传输协议层

### `src/file_transfer.c` / `src/file_transfer.h`

职责：

- 解析 START/DATA/END 二进制协议
- 校验包 CRC16
- 校验 `file_id`、`seq`、`offset`、`len`
- 发送 ACK/NACK
- 调用 `fs_image` 写入临时文件
- END 后触发文件 CRC32 校验和图片显示

状态机：

```text
IDLE
WAIT_START
RECV_DATA
VERIFY_FILE
RENAME_FILE
DECODE_DISPLAY
ERROR
```

当前主要运行状态：

- `WAIT_START`：等待 START 包
- `RECV_DATA`：接收 DATA 包
- `VERIFY_FILE`：END 后进行文件校验
- `DECODE_DISPLAY`：校验成功后显示图片

重要限制：

```c
#define FT_DEFAULT_CHUNK_SIZE 240U
#define FT_MAX_CHUNK_SIZE     512U
#define FT_MAX_FILENAME_LEN   64U
```

关键原则：

- 本文件不直接调用 UART API
- 所有收发都通过 `transport_t`
- 协议字段全部按小端解析
- 接收到的数据先进入临时文件，不直接覆盖正式图片

## 7. LittleFS 图片文件层

### `src/fs_image.c` / `src/fs_image.h`

职责：

- 管理图片文件接收策略
- 挂载 LittleFS
- 创建和写入临时文件
- 计算文件 CRC32
- 校验成功后 rename 成正式文件
- 失败时清理临时文件

路径：

```c
#define FS_IMAGE_TMP_PATH   "/image_tmp.jpg"
```

正式图片路径由 START 包中的文件名决定，例如 `1.jpg` 保存为 `/1.jpg`，`2.jpg` 保存为 `/2.jpg`。

关键接口：

```c
int fs_image_mount(void);
int fs_image_format(void);
int fs_image_prepare_receive(const char *filename, uint32_t file_size,
                             uint32_t *free_bytes);
int fs_image_begin(const char *filename, uint32_t file_size);
int fs_image_write(uint32_t offset, const uint8_t *data, uint32_t len);
int fs_image_finish(uint32_t expected_crc32);
void fs_image_abort(void);
const char *fs_image_get_final_path(void);
```

安全策略：

- 接收开始只删除 `/image_tmp.jpg`
- 接收过程中只写 `/image_tmp.jpg`
- START 时先检查 LittleFS 剩余空间，空间不足会返回 `STORAGE_FULL`
- 只有 CRC32 校验成功后才删除同名旧图片
- 然后 rename `/image_tmp.jpg` 为按文件名生成的正式路径
- 失败不会破坏已存在的正式图片
- 用户确认格式化后，`fs_image_format()` 会清空 LittleFS 中所有图片

当前默认最大图片大小：

```c
#define FS_IMAGE_MAX_FILE_SIZE (120U * 1024U)
```

该限制用于给 128KB LittleFS 分区预留元数据空间。

## 8. JPEG 解码显示层

### `src/jpeg_viewer.c` / `src/jpeg_viewer.h`

职责：

- 从 LittleFS 打开指定 JPG 文件
- 调用 TJpgDec 流式解码
- 每个 MCU block 直接写入 LCD

接口：

```c
int jpeg_viewer_show_file(const char *path);
```

图片接收完成后的调用：

```c
jpeg_viewer_show_file(fs_image_get_final_path());
```

设计特点：

- 不需要整帧 framebuffer
- JPEG 数据从 LittleFS 流式读取
- 解码输出块直接通过 `LCD_addWindow()` 显示

### `src/jpg_display.c` / `src/jpg_display.h`

职责：

- 原有内置 JPG 调试流程
- 把编译进固件的 `img_1_jpg[]` 写入 `/img/1.jpg`
- 从 `/img/1.jpg` 解码显示

当前用途：

- 主要用于 LOG 模式或调试内置图片流程
- DATA 模式下主链路使用 `jpeg_viewer_show_file(fs_image_get_final_path())`

## 9. LCD 驱动层

### `src/ST77916.c` / `src/ST77916.h`

职责：

- 初始化 ST77916 LCD
- 初始化 QSPI/OSPI 和控制 GPIO
- 发送屏幕初始化命令表
- 控制背光
- 设置窗口并写入 RGB565 像素

主要接口：

```c
void LCD_Init(void);
void LCD_addWindow(uint16_t x1, uint16_t y1,
                   uint16_t x2, uint16_t y2,
                   uint16_t *color);
void LCD_SetLight(uint8_t Light);
```

JPEG 显示时，`jpeg_viewer.c` 的输出回调最终调用 `LCD_addWindow()`。

## 10. 触摸驱动层

### `src/CST816.c` / `src/CST816.h`

职责：

- 初始化 CST816 触摸芯片
- I2C 读写寄存器
- 配置 RST/INT GPIO
- INT 中断回调只置位标志
- 主循环通过 `Touch_Poll()` 读取坐标

主要接口：

```c
void Touch_Init(void);
int Touch_Poll(cst816_data_t *out);
```

DATA 模式当前主循环重点是图片接收，触摸初始化仍保留。

## 11. LittleFS 移植层

### `src/lfs_port.c` / `src/lfs_port.h`

职责：

- 把 LittleFS 的 read/prog/erase/sync 映射到 TD1601 SPI Flash
- 提供全局 mount/unmount/get 接口
- 挂载失败时自动格式化

主要接口：

```c
int lfs_port_mount(void);
int lfs_port_format(void);
void lfs_port_unmount(void);
lfs_t *lfs_port_get(void);
```

重要注意事项：

- 不能在 `lfs_port.c` 里重新初始化 QSPI flash 控制器
- 系统启动时已经初始化 `g_spiflash`
- XIP 状态下重置 QSPI 控制器会导致取指异常

## 12. 第三方和生成文件

### `src/TJpgDec/tjpgd.c` / `src/TJpgDec/tjpgd.h`

职责：

- TJpgDec JPEG 解码库
- `jpeg_viewer.c` 和 `jpg_display.c` 都依赖它

通常不需要修改。

### `src/littlefs/lfs.c` / `src/littlefs/lfs.h`

职责：

- LittleFS 源码

通常不需要修改。

### `src/img_1.c` / `src/img_1.h`

职责：

- 由 `utilities/jpg_to_c.py` 生成
- 把 JPG 原始字节编译进固件
- 用于旧的内置图片调试流程

当前 UART/BLE 图片更新主链路不依赖该文件。

## 13. 工具脚本

### `utilities/send_jpg_uart.py`

职责：

- PC/macOS 串口发送 JPG 测试工具
- 构造 START/DATA/END 包
- 计算 CRC16 和 CRC32
- 等待 ACK/NACK
- 支持超时重试

示例：

```sh
python3 projects/gui/utilities/send_jpg_uart.py \
  --port /dev/tty.usbserial-310 \
  --baud 921600 \
  --file test.jpg \
  --chunk 240
```

BLE App 可以参考这个脚本实现同样的协议。

### `utilities/jpg_to_c.py`

职责：

- 把 JPG 文件转换成 C 数组
- 用于旧的内置图片调试流程

## 14. Makefile

### `projects/gui/Makefile`

职责：

- 定义 RISC-V 工具链路径
- 定义 include 路径和编译宏
- 显式列出 GUI 工程源码
- 链接 chip、console、csi、minilibc、mm、littlefs 等静态库

当前新增模块已经加入：

```make
src/transport_uart.c
src/transport_ble.c
src/file_transfer.c
src/fs_image.c
src/jpeg_viewer.c
src/app_image_update.c
```

DATA 模式下为了避免日志污染协议，链接参数包含：

```make
-Wl,--wrap=printf
-Wl,--wrap=vprintf
-Wl,--wrap=puts
-Wl,--wrap=putchar
-Wl,--wrap=fputc
```

## 15. 如何定位问题

### 收不到 ACK

优先检查：

1. UART 波特率是否和脚本一致，当前默认 921600
2. `GUI_UART_MODE` 是否为 `GUI_UART_MODE_DATA`
3. START 包 magic 是否为 `"JPGU"`
4. CRC16 是否按 CCITT-FALSE 计算
5. App 是否等待 ACK 后再发下一包

### START ACK 正常，DATA NACK

优先检查：

1. DATA.seq 是否从 0 开始连续递增
2. DATA.offset 是否等于前面所有 payload 长度之和
3. DATA.len 是否超过 START.chunk_size
4. DATA.len 是否超过 512
5. DATA 包 CRC16 是否包含 payload

### END NACK

优先检查：

1. total_chunks 是否等于 DATA 包数量
2. END.file_crc32 是否等于 START.file_crc32
3. JPG 文件整体 CRC32 是否和 MCU 计算一致
4. LittleFS 剩余空间是否足够

### 传输成功但不显示

优先检查：

1. JPG 是否为 TJpgDec 支持的格式
2. 图片尺寸是否适配当前 LCD
3. `jpeg_viewer_show_file(fs_image_get_final_path())` 是否返回错误
4. LCD 初始化是否成功

## 16. 总体调用关系

```text
main.c
  ├─ board_init.c
  ├─ ST77916.c
  ├─ CST816.c
  └─ app_image_update.c
       ├─ fs_image.c
       │    └─ lfs_port.c
       │         └─ littlefs/lfs.c
       └─ file_transfer.c
            ├─ transport_if.h
            ├─ transport_uart.c
            │    └─ gui_uart.c
            ├─ fs_image.c
            └─ jpeg_viewer.c
                 ├─ TJpgDec/tjpgd.c
                 ├─ lfs_port.c
                 └─ ST77916.c
```

这个结构的核心目的是分层：

- transport 层只解决字节怎么收发
- file_transfer 层只解决协议和可靠传输
- fs_image 层只解决文件安全落盘
- jpeg_viewer 层只解决图片显示
- LCD/Touch/LittleFS port 层只解决硬件和底层适配

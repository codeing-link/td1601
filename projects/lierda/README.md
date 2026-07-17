# lierda 串口工程

本工程从 `projects/gui` 中剥离出串口打印和普通串口收发功能，并增加 PA24 LED 闪灯，
不包含 LCD、触摸、LittleFS、JPEG、图片传输或 BLE 适配代码。

PA24 上电后先输出低电平，之后每隔 1000 ms 翻转一次。闪灯使用非阻塞轮询实现，
不会影响 UART1 的接收和回显。

## 串口分配

| 用途 | 控制器 | TX/RX | 参数 |
| --- | --- | --- | --- |
| `printf` 日志 | UART0 | PA18 / PA17 | 921600, 8N1 |
| 普通数据收发 | UART1 | PA28 / PA27 | 115200, 8N1 |

UART1 使用接收中断和软件环形缓冲区，发送为带超时的阻塞发送。示例程序会在 UART1
启动时发送提示字符串，之后把 UART1 收到的所有字节原样回显。

## macOS 命令行编译与打包

```sh
cd projects/lierda
make clean && make
bash aft_build_macos.sh
```

可烧录文件为 `Obj/lierda_0x18000000_V1.0.0.bin`。

## 下载

使用已有的 Ubuntu ISP 共享工程时：

```sh
./build_patch_download.sh                  # 默认使用 /dev/ttyUSB0
./build_patch_download.sh /dev/ttyUSB1     # 也可以显式指定 Ubuntu 串口
```

如果固件已经编译和打包完成，只需要重新下载，可使用：

```sh
SKIP_BUILD=1 ./build_patch_download.sh
```

也可以用 CDK 打开 `project.cdkproj`，工程已配置 `utilities/flash.init` 和
`utilities/td1601_flash_cdk.elf`，编译后执行 Download。

## API

- `lierda_uart_init()`：初始化 UART1 和 RX 中断。
- `lierda_uart_send()`：发送任意字节。
- `lierda_uart_available()`：查询 RX 缓冲区字节数。
- `lierda_uart_read()`：非阻塞读取 RX 缓冲区。
- `lierda_uart_rx_overflowed()`：查询溢出状态。
- `lierda_uart_clear_rx()`：清空接收缓冲和溢出状态。

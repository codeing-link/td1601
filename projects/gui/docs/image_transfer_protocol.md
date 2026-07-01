# JPG 图片传输协议文档

本文档描述当前 GUI 固件使用的 JPG 图片传输协议。协议设计目标是让 PC 串口工具和未来 BLE App 使用同一套上层传输格式：当前底层是 UART，未来 BLE 只替换 transport，不修改文件接收、CRC 校验、LittleFS 保存和 JPEG 显示逻辑。

## 1. 适用范围

该协议用于把一张 JPG 图片从上位机、手机 App 或其他主机传输到板子。

板子收到完整图片后会执行：

1. 写入临时文件 `/image_tmp.jpg`
2. 校验整文件 CRC32
3. 校验通过后按文件名替换正式文件，例如 `1.jpg` 保存为 `/1.jpg`
4. 调用 JPEG 解码显示接口显示刚收到的文件

同名旧图片只有在新文件完整接收并校验成功后才会被替换。不同文件名的图片会同时保存在 LittleFS 中，直到空间不足或用户选择格式化。

兼容旧调试流程：当前 DATA 模式开始接收新图时，固件会清理历史内置图片路径 `/img/1.jpg`，用于释放 LittleFS 空间。该文件不是正式图片文件；正式图片路径由 START 包中的文件名决定。

多图片保存规则：

- `filename = "1.jpg"` 保存为 `/1.jpg`
- `filename = "2.jpg"` 保存为 `/2.jpg`
- 文件名不同且空间足够时，多张图片会同时保存
- 文件名相同但内容不同，只有新图片完整接收并 CRC32 校验成功后才覆盖旧同名图片
- 空间不足时，Receiver 不会创建 `/image_tmp.jpg`，也不会破坏已有图片

## 2. 传输角色

- Sender：PC 串口工具或未来手机 BLE App。
- Receiver：TD1601 板子固件。

Sender 负责主动发送 `START`、多个 `DATA`、最后发送 `END`。

Receiver 每收到一个包后返回 `ACK` 或 `NACK`。Sender 必须等待 ACK 后再发送下一包。如果超时或收到 NACK，Sender 应重发当前包。

## 3. 字节序

所有多字节整数均使用小端格式。

示例：`uint32_t 0x12345678` 在包中按以下字节发送：

```text
78 56 34 12
```

## 4. 基本常量

```c
magic   = "JPGU"        // 4 bytes: 0x4A 0x50 0x47 0x55
version = 1

START   = 0x01
DATA    = 0x02
END     = 0x03
FORMAT  = 0x04
ACK     = 0x80
NACK    = 0x81

FT_DEFAULT_CHUNK_SIZE = 240
FT_MAX_CHUNK_SIZE     = 512
FT_MAX_FILENAME_LEN   = 64
```

当前建议 BLE App 默认使用 240 bytes payload，原因是后续 BLE 透传通常需要较小 MTU 或应用层分包。

## 5. CRC 算法

### 5.1 包 CRC16

`START`、`DATA`、`END` 包末尾带 `crc16`。

算法：

- 名称：CRC-16/CCITT-FALSE
- 初值：`0xFFFF`
- 多项式：`0x1021`
- 输入反转：否
- 输出反转：否
- 结果异或：`0x0000`

CRC16 覆盖范围：

```text
从 magic 第 1 字节开始，到 crc16 字段之前的最后 1 字节结束
```

即不包含末尾 `crc16` 自身。

### 5.2 文件 CRC32

`START.file_crc32` 和 `END.file_crc32` 使用标准 CRC32。

算法：

- 标准 ZIP/以太网 CRC32
- 多项式反射形式：`0xEDB88320`
- 初值：`0xFFFFFFFF`
- 结束异或：`0xFFFFFFFF`

Python 可直接使用：

```python
file_crc32 = zlib.crc32(jpg_bytes) & 0xFFFFFFFF
```

## 6. 包格式

### 6.1 START 包

Sender 发送，用于声明即将传输的 JPG 文件。

固定字段长度为 19 bytes，后跟文件名和 2 bytes CRC16。

| Offset | Size | Field | Type | Description |
|---:|---:|---|---|---|
| 0 | 4 | magic | bytes | 固定 `"JPGU"` |
| 4 | 1 | version | uint8 | 当前为 `1` |
| 5 | 1 | cmd | uint8 | `START = 0x01` |
| 6 | 2 | file_id | uint16 LE | 本次传输 ID，Sender 自定义 |
| 8 | 4 | file_size | uint32 LE | JPG 文件总字节数 |
| 12 | 4 | file_crc32 | uint32 LE | JPG 文件 CRC32 |
| 16 | 2 | chunk_size | uint16 LE | 后续 DATA payload 最大长度 |
| 18 | 1 | filename_len | uint8 | 文件名长度，最大 64 |
| 19 | N | filename | bytes | UTF-8 或 ASCII 文件名 |
| 19+N | 2 | header_crc16 | uint16 LE | START 包 CRC16 |

约束：

- `file_size > 0`
- `chunk_size > 0`
- `chunk_size <= 512`
- `filename_len <= 64`
- 当前固件还限制 JPG 文件大小不超过 LittleFS 可用空间，默认最大约 56KB。

Receiver 校验通过后：

- 先根据 `file_size` 和 LittleFS 当前剩余空间判断是否足够保存新图
- 空间不足时返回 `NACK STORAGE_FULL`，不创建临时文件
- 空间足够时删除旧 `/image_tmp.jpg`
- 创建新的 `/image_tmp.jpg`
- 初始化接收状态
- 返回 `ACK(file_id, seq=0)`

如果空间不足，Receiver 返回：

```text
NACK(file_id, seq=0, error_code=FT_ERR_STORAGE_FULL)
```

Sender 收到该错误后不能继续发送 DATA，必须让用户选择是否格式化 LittleFS。

### 6.2 DATA 包

Sender 发送，携带 JPG 文件分片。

固定字段长度为 18 bytes，后跟 payload 和 2 bytes CRC16。

| Offset | Size | Field | Type | Description |
|---:|---:|---|---|---|
| 0 | 4 | magic | bytes | 固定 `"JPGU"` |
| 4 | 1 | version | uint8 | 当前为 `1` |
| 5 | 1 | cmd | uint8 | `DATA = 0x02` |
| 6 | 2 | file_id | uint16 LE | 必须等于 START.file_id |
| 8 | 4 | seq | uint32 LE | 从 0 开始递增 |
| 12 | 4 | offset | uint32 LE | 当前 payload 在文件中的偏移 |
| 16 | 2 | len | uint16 LE | payload 字节数 |
| 18 | N | payload | bytes | JPG 原始数据 |
| 18+N | 2 | packet_crc16 | uint16 LE | DATA 包 CRC16 |

约束：

- `file_id` 必须等于当前 START 的 `file_id`
- `seq` 必须等于 Receiver 期望的下一个序号
- `offset` 必须等于 Receiver 当前已接收字节数
- `len > 0`
- `len <= START.chunk_size`
- `len <= 512`
- `offset + len <= START.file_size`

Receiver 校验通过后：

- 把 payload 写入 `/image_tmp.jpg`
- 更新已接收字节数
- 返回 `ACK(file_id, seq)`

### 6.3 END 包

Sender 发送，用于结束本次传输。

固定长度 18 bytes。

| Offset | Size | Field | Type | Description |
|---:|---:|---|---|---|
| 0 | 4 | magic | bytes | 固定 `"JPGU"` |
| 4 | 1 | version | uint8 | 当前为 `1` |
| 5 | 1 | cmd | uint8 | `END = 0x03` |
| 6 | 2 | file_id | uint16 LE | 必须等于 START.file_id |
| 8 | 4 | total_chunks | uint32 LE | DATA 包总数 |
| 12 | 4 | file_crc32 | uint32 LE | JPG 文件 CRC32 |
| 16 | 2 | packet_crc16 | uint16 LE | END 包 CRC16 |

Receiver 校验通过后：

1. 确认 `total_chunks` 等于已接收 DATA 包数
2. 确认已接收字节数等于 START.file_size
3. 关闭 `/image_tmp.jpg`
4. 重新读取 `/image_tmp.jpg` 并计算 CRC32
5. CRC32 正确则删除同名旧正式图片
6. rename `/image_tmp.jpg` 为正式路径，例如 `/1.jpg`
7. 返回 `ACK(file_id, seq=total_chunks)`
8. 调用 `jpeg_viewer_show_file()` 显示新图

如果任一步失败，Receiver 返回 NACK，并删除 `/image_tmp.jpg`。

### 6.4 FORMAT 包

Sender 发送，用于在 Receiver 报告 `STORAGE_FULL` 后格式化 LittleFS。

固定长度 10 bytes。

| Offset | Size | Field | Type | Description |
|---:|---:|---|---|---|
| 0 | 4 | magic | bytes | 固定 `"JPGU"` |
| 4 | 1 | version | uint8 | 当前为 `1` |
| 5 | 1 | cmd | uint8 | `FORMAT = 0x04` |
| 6 | 2 | file_id | uint16 LE | 当前传输 ID |
| 8 | 2 | packet_crc16 | uint16 LE | FORMAT 包 CRC16 |

Receiver 收到 FORMAT 后：

1. 删除临时接收状态
2. 卸载 LittleFS
3. 格式化 LittleFS
4. 重新挂载 LittleFS
5. 返回 `ACK(file_id, seq=0)`

FORMAT 会清空 LittleFS 中所有图片。App 必须在用户确认后才能发送。

App 交互建议：

```text
设备存储空间不足，无法保存新图片。
是否清空设备中的所有图片并继续传输？
[取消] [清空并继续]
```

用户选择“取消”：

- 不发送 FORMAT
- 停止当前传输
- 保留设备中已有图片

用户选择“清空并继续”：

- 发送 FORMAT 包
- 等待 `ACK(file_id, seq=0)`
- 重新发送 START
- 然后继续 DATA/END 流程

### 6.5 ACK 包

Receiver 返回，表示包处理成功。

当前 ACK 固定 13 bytes，不带 CRC16。

| Offset | Size | Field | Type | Description |
|---:|---:|---|---|---|
| 0 | 4 | magic | bytes | 固定 `"JPGU"` |
| 4 | 1 | version | uint8 | 当前为 `1` |
| 5 | 1 | cmd | uint8 | `ACK = 0x80` |
| 6 | 2 | file_id | uint16 LE | 对应文件 ID |
| 8 | 4 | seq | uint32 LE | 对应包序号 |
| 12 | 1 | status | uint8 | 固定 `0` |

ACK 的 `seq` 规则：

- START 成功：`seq = 0`
- DATA 成功：`seq = DATA.seq`
- END 成功：`seq = total_chunks`

### 6.6 NACK 包

Receiver 返回，表示包处理失败。

当前 NACK 固定 13 bytes，不带 CRC16。

| Offset | Size | Field | Type | Description |
|---:|---:|---|---|---|
| 0 | 4 | magic | bytes | 固定 `"JPGU"` |
| 4 | 1 | version | uint8 | 当前为 `1` |
| 5 | 1 | cmd | uint8 | `NACK = 0x81` |
| 6 | 2 | file_id | uint16 LE | 对应文件 ID |
| 8 | 4 | seq | uint32 LE | 对应包序号 |
| 12 | 1 | error_code | uint8 | 错误码 |

错误码：

```c
FT_ERR_CRC       = 1
FT_ERR_SEQ       = 2
FT_ERR_OFFSET    = 3
FT_ERR_FS_WRITE  = 4
FT_ERR_FILE_CRC  = 5
FT_ERR_PARAM     = 6
FT_ERR_TIMEOUT   = 7
FT_ERR_STORAGE_FULL = 8
```

## 7. Sender 状态机建议

BLE App 或 PC 工具建议实现以下发送流程：

1. 读取 JPG 文件到内存或分块读取
2. 计算文件 CRC32
3. 选择 `file_id`
4. 发送 START
5. 等待 ACK
   - 如果收到 `NACK STORAGE_FULL`，提示用户是否格式化文件系统
   - 用户拒绝：停止传输
   - 用户确认：发送 FORMAT，等待 ACK，然后重新发送 START
6. 按 `chunk_size` 切分 JPG
7. 对每个 DATA：
   - 设置 `seq`
   - 设置 `offset`
   - 发送 DATA
   - 等待 ACK
   - 超时或 NACK 则重发当前 DATA
8. 发送 END
9. 等待最终 ACK
10. 显示传输成功

推荐参数：

```text
chunk_size = 240
ack_timeout = 2s
retry = 5
```

如果 BLE 链路更慢，可以增加 `ack_timeout` 或在 DATA 包之间增加短延时。

空间不足处理必须发生在 START 阶段。收到 `STORAGE_FULL` 后，Sender 不应发送任何 DATA 包，因为 Receiver 还没有创建临时文件，也没有进入 `RECV_DATA` 状态。

## 8. Receiver 行为摘要

Receiver 当前在主循环中调用：

```c
image_update_poll();
```

内部处理链路：

```text
UART/BLE ISR or RX callback
        ↓
transport ring buffer
        ↓
file_transfer_poll()
        ↓
START/DATA/END parser
        ↓
fs_image_write()
        ↓
fs_image_finish()
        ↓
jpeg_viewer_show_file(received_path)
```

`file_transfer.c` 不依赖 UART API，只依赖 `transport_t`。

## 9. Python 参考实现

当前工程提供 PC/macOS 测试工具：

```text
projects/gui/utilities/send_jpg_uart.py
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

BLE App 开发时可以直接参考该脚本的包构造、CRC16、CRC32、ACK/NACK 等逻辑。

脚本中与空间不足相关的参数：

- 默认：收到 `STORAGE_FULL` 后在命令行询问用户是否格式化。
- `--format-on-full`：收到 `STORAGE_FULL` 后自动发送 FORMAT，并重新发送 START。
- `--no-format-on-full`：收到 `STORAGE_FULL` 后直接停止传输，不发送 FORMAT。

BLE App 中建议映射为：

- 默认交互模式：弹出确认框，由用户决定是否清空图片。
- 自动格式化模式：仅用于内部测试，不建议作为正式 App 默认行为。
- 禁止格式化模式：适用于 App 只允许追加图片、不允许清空设备图片的业务场景。

## 10. BLE App 实现注意事项

1. App 层应该把 BLE 当成字节流，不要假设一次 BLE write 等于一个 MCU 包。
2. 如果 BLE MTU 小于一个 DATA 包，App 可以把同一个协议包拆成多次 BLE write；MCU 端会按字节流重新组包。
3. App 必须等待 ACK 后再发送下一包，避免 MCU flash 写入期间 ring buffer 堆积过多。
4. 推荐默认 payload 240 bytes；如果实际 BLE 稳定，也可以逐步增加，但不能超过 512。
5. NACK 后重发当前包，不要跳过 seq。
6. 收到 `STORAGE_FULL` 后必须提示用户是否格式化。用户不同意时停止传输。
7. FORMAT 会清空 LittleFS 中所有已保存图片，App UI 必须明确提示。
8. END ACK 收到后，MCU 可能正在解码显示图片；App 可提示“传输完成，设备正在显示”。

App 不应把 FORMAT 做成静默行为。除非是工厂测试或开发者模式，否则必须有用户确认。

## 11. 最小伪代码

```python
jpg = read_file("test.jpg")
file_crc32 = crc32(jpg)
file_id = 1
chunk = 240

send_start(file_id, len(jpg), file_crc32, chunk, "test.jpg")
wait_ack(file_id, 0)

offset = 0
seq = 0
while offset < len(jpg):
    payload = jpg[offset:offset + chunk]
    send_data(file_id, seq, offset, payload)
    wait_ack(file_id, seq)
    offset += len(payload)
    seq += 1

send_end(file_id, seq, file_crc32)
wait_ack(file_id, seq)
```

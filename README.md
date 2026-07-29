# TD1601 EVB 命令行生成、编译与下载指南

本文以 `projects/lierda` 工程为例，介绍如何通过命令行生成工程配置、编译固件、签名打包、下载到开发板并验证运行结果。

以下命令默认在 SDK 根目录执行：

```text
td1601_evb_v0.2.0_cdk/
```

## 1. 功能概览

完整流程如下：

```text
project.cdkproj + package.yaml
            |
            v
generate_td1601_makefiles.py
            |
            v
Makefile / 打包脚本 / 下载脚本
            |
            v
编译 ELF、BIN -> 签名与 CRC 打包 -> ISP 下载 -> 上电运行
```

`projects/lierda` 下载运行后提供以下演示功能：

| 功能 | 引脚/参数 | 说明 |
| --- | --- | --- |
| 调试串口 UART0 | TX：PA18，RX：PA17，921600，8N1 | 输出启动信息和运行日志 |
| 普通串口 UART1 | TX：PA28，RX：PA27，115200，8N1 | 接收数据并回传，用于串口收发测试 |
| LED | PA24 | 每隔约 1000 ms 翻转一次 |

## 2. Windows（Git Bash）使用

Windows 下直接在 Git Bash 中完成生成、编译、打包和串口烧写，不需要 SSH、Ubuntu 或
Samba 共享目录。工具链默认位于工作区同级的 `opt/xpack-riscv`，ISP 下载器随 SDK
统一管理于 `tools/ISP/BROM_DL.exe`。

### 2.1 安装 xPack RISC-V 工具链

[点击这里下载 xPack RISC-V](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v14.3.0-1/xpack-riscv-none-elf-gcc-14.3.0-1-win32-x64.zip)
下载 `xpack-riscv-none-elf-gcc-14.3.0-1-win32-x64.zip` 后，解压并确保工具链目录为：

```text
<工作区>/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1
```

例如，工作区为 `F:\mcu\ninestar\github` 时，工具链应位于：

```text
F:\mcu\ninestar\github\opt\xpack-riscv\xpack-riscv-none-elf-gcc-14.3.0-1
```

部分现有脚本会从当前 Windows 用户目录下的 `opt` 查找工具链。为兼容这类脚本，可在
**命令提示符（cmd）** 中创建目录软连接：

```cmd
mklink /J "%USERPROFILE%\opt" "F:\mcu\ninestar\github\opt"
```

`%USERPROFILE%` 会自动展开为当前用户目录，例如
`C:\Users\Administrator`；不同电脑上的用户名不同，不需要手工修改目标用户目录。
将右侧源路径替换为实际工作区的 `opt` 目录即可。若 C 盘空间足够，也可以直接将工具链
解压到 `%USERPROFILE%\opt\xpack-riscv\xpack-riscv-none-elf-gcc-14.3.0-1`，此时不需要创建软连接。

### 2.2 生成 Windows 工程文件

首次为 Windows 生成工程文件时，在 SDK 根目录执行：

```bash
python tools/generate_td1601_makefiles.py projects/drivers/gpio/gpio_toggle --platform windows --force
```

在工程目录编译、打包并烧写：

```bash
cd projects/drivers/gpio/gpio_toggle
make
bash aft_build_windows.sh
bash build_patch_download_windows.sh \
  -p COM3 \
  -f ./Obj/gpio_toggle_0x18000000_V1.0.0.bin
```

`-d` 可省略，脚本默认使用 `../../../../tools/ISP/BROM_DL.exe`。如需指定其他下载器，
从上述工程目录传入相对路径：

```bash
bash build_patch_download_windows.sh \
  -p COM3 \
  -f ./Obj/gpio_toggle_0x18000000_V1.0.0.bin \
  -d ../../../../tools/ISP/BROM_DL.exe
```

## 3. macOS 使用

macOS 保留原有的“本地编译和签名 + Ubuntu 远程 ISP 烧写”流程。生成器默认识别 macOS；
也可以显式指定平台：

```bash
python3 tools/generate_td1601_makefiles.py projects/lierda --platform macos --force
```

在 SDK 根目录执行完整流程：

```bash
make -C projects/lierda clean
make -C projects/lierda
bash projects/lierda/aft_build_macos.sh
projects/lierda/build_patch_download.sh /dev/ttyUSB0
```

其中 `/dev/ttyUSB0` 是 Ubuntu 下载机上的串口。macOS 的环境要求、远程下载配置与排障说明见后续章节。

## 4. macOS 环境准备

### 4.1 Python 3 和 GNU Make

检查命令是否可用：

```bash
python3 --version
make --version
```

### 4.2 RISC-V 交叉编译工具链

生成的 `Makefile` 默认使用：

```text
~/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1
```

检查编译器：

```bash
~/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1/bin/riscv-none-elf-gcc --version
```

TD1601 工程需要支持 `rv32imafdc_zicsr/ilp32d` multilib，可用下面的命令确认：

```bash
~/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1/bin/riscv-none-elf-gcc \
  -march=rv32imafdc_zicsr -mabi=ilp32d -print-multi-directory
```

工具链安装在其他位置时，可以在编译命令中指定：

```bash
make -C projects/lierda \
  TOOLCHAIN_DIR=/absolute/path/to/xpack-riscv-none-elf-gcc-14.3.0-1
```

### 4.3 Wine

`aft_build_macos.sh` 使用 SDK 内的 Windows 工具完成固件签名和封装，因此 macOS 上需要安装 Wine：

```bash
brew install --cask wine-stable
wine64 --version
```

Apple Silicon 设备如提示缺少 Rosetta，可执行：

```bash
softwareupdate --install-rosetta --agree-to-license
```

### 4.4 下载环境

当前下载脚本采用“macOS 本地编译 + Ubuntu 远程 ISP 下载”的方式，需要：

- macOS 已挂载共享目录 `/Volumes/mpushare/mpushare/macos_workspace/isp-9star`；
- Ubuntu 下载机能够连接 TD1601 开发板；
- Ubuntu 下载机存在对应串口，例如 `/dev/ttyUSB0`；
- `projects/lierda/utilities/macos-copy-and-download.sh` 中的远程主机配置正确。

注意：传给下载脚本的 `/dev/ttyUSB0` 是 **Ubuntu 下载机上的设备路径**，不是 macOS 本地串口路径。

## 5. macOS 一次完成生成、编译和下载

在 SDK 根目录依次执行：

```bash
python3 tools/generate_td1601_makefiles.py projects/lierda --force
make -C projects/lierda clean
make -C projects/lierda
bash projects/lierda/aft_build_macos.sh
projects/lierda/build_patch_download.sh /dev/ttyUSB0
```

其中最后一条下载命令默认也会重新编译和打包。因此，日常开发时通常只需要：

```bash
projects/lierda/build_patch_download.sh /dev/ttyUSB0
```

如果已经完成编译和打包，只希望下载现有固件：

```bash
SKIP_BUILD=1 projects/lierda/build_patch_download.sh /dev/ttyUSB0
```

## 6. macOS 生成工程配置文件

生成命令：

```bash
python3 tools/generate_td1601_makefiles.py projects/lierda
```

工具会读取工程中的 `project.cdkproj`、`package.yaml` 及依赖组件配置，生成以下文件：

| 文件 | 用途 |
| --- | --- |
| `projects/lierda/Makefile` | 命令行编译工程 |
| `projects/lierda/aft_build_macos.sh` | 固件签名、CRC 和镜像封装 |
| `projects/lierda/build_patch_download.sh` | 编译、打包和远程下载入口 |
| `projects/lierda/generated/td1601_xpack_compat.h` | xPack GCC 与原工程头文件的兼容配置 |

如果目标文件已经存在，生成器会停止以避免误覆盖。确认需要重新生成时使用：

```bash
python3 tools/generate_td1601_makefiles.py projects/lierda --force
```

`--force` 会覆盖上述自动生成的配置文件。应用源码、`package.yaml` 和 `project.cdkproj` 不会被生成器修改，但仍建议在重新生成前提交或备份手工修改过的生成文件。

## 7. macOS 编译工程

### 7.1 编译

```bash
make -C projects/lierda
```

### 7.2 清理后重新编译

```bash
make -C projects/lierda clean
make -C projects/lierda
```

编译成功后，主要产物位于：

```text
projects/lierda/Obj/lierda.elf
projects/lierda/Obj/lierda.bin
projects/lierda/Obj/lierda.ihex
projects/lierda/Lst/lierda.map
projects/lierda/Lst/lierda.asm
```

其中：

- `lierda.elf` 用于调试和检查程序入口；
- `lierda.bin` 是尚未完成签名封装的原始二进制；
- `lierda.map` 可用于分析链接地址和符号占用；
- 最终 ISP 下载文件需要继续执行打包步骤生成。

## 8. macOS 签名和打包固件

执行：

```bash
bash projects/lierda/aft_build_macos.sh
```

脚本会根据 `package.yaml` 中的版本号和工程配置完成签名、CRC 计算及镜像封装。当前 `lierda` 示例的最终固件为：

```text
projects/lierda/Obj/lierda_0x18000000_V1.0.0.bin
```

文件名中的字段含义：

- `lierda`：工程/包名称；
- `0x18000000`：Flash 下载地址；
- `V1.0.0`：来自 `projects/lierda/package.yaml` 的版本号。

修改版本号后需要重新执行打包，下载脚本会自动寻找新版本对应的文件名。

## 9. macOS 下载固件

### 9.1 使用默认串口

不传参数时，远程串口默认为 `/dev/ttyUSB0`：

```bash
projects/lierda/build_patch_download.sh
```

### 9.2 指定远程串口

```bash
projects/lierda/build_patch_download.sh /dev/ttyUSB1
```

该脚本会自动执行以下操作：

1. 编译 `projects/lierda`；
2. 调用 `aft_build_macos.sh` 生成最终固件；
3. 将固件复制到 macOS 与 Ubuntu 的共享目录；
4. 通过远程下载脚本调用 Ubuntu 上的 ISP 工具；
5. 把固件写入 TD1601 的 `0x18000000` 地址。

如需使用另一份 macOS 下载辅助脚本，可通过环境变量指定：

```bash
ISP_TOOL=/absolute/path/to/macos-copy-and-download.sh \
  projects/lierda/build_patch_download.sh /dev/ttyUSB0
```

下载前请让开发板进入 Boot/ISP 模式；下载完成后复位或重新上电，使程序从 Flash 启动。

## 10. 运行验证

### 10.1 调试串口日志

使用 USB 转串口连接：

```text
TD1601 PA18 (UART0 TX) -> 串口模块 RX
TD1601 PA17 (UART0 RX) -> 串口模块 TX
TD1601 GND             -> 串口模块 GND
```

串口参数：

```text
921600 baud, 8 data bits, no parity, 1 stop bit
```

复位开发板后应看到 `lierda` 工程的启动信息和 UART/LED 初始化日志。

### 10.2 普通串口收发

连接 UART1：

```text
TD1601 PA28 (UART1 TX) -> 串口模块 RX
TD1601 PA27 (UART1 RX) -> 串口模块 TX
TD1601 GND             -> 串口模块 GND
```

串口参数：

```text
115200 baud, 8 data bits, no parity, 1 stop bit
```

向 UART1 发送数据，开发板应通过 UART1 回传收到的数据。

### 10.3 LED

PA24 上的 LED 每隔约 1000 ms 翻转一次。LED 正常闪烁说明程序已进入主循环且系统延时工作正常。

## 11. 用于其他工程

其他 TD1601 CDK 工程只要包含有效的 `project.cdkproj` 和 `package.yaml`，也可以使用同一生成器。例如：

```bash
python3 tools/generate_td1601_makefiles.py projects/drivers/gpio/gpio_toggle
make -C projects/drivers/gpio/gpio_toggle clean
make -C projects/drivers/gpio/gpio_toggle
bash projects/drivers/gpio/gpio_toggle/aft_build_macos.sh
```

如果该工程需要使用一键下载脚本，还应在工程目录准备可用的：

```text
utilities/macos-copy-and-download.sh
```

然后执行生成的：

```bash
projects/drivers/gpio/gpio_toggle/build_patch_download.sh /dev/ttyUSB0
```

## 12. macOS 常见问题

### 生成器提示文件已存在

这是防止覆盖已有文件的保护机制。确认需要重建配置后执行：

```bash
python3 tools/generate_td1601_makefiles.py projects/lierda --force
```

### 找不到 RISC-V 编译器

检查默认工具链路径，或指定实际安装目录：

```bash
make -C projects/lierda TOOLCHAIN_DIR=/absolute/path/to/toolchain
```

### 提示找不到正确的 multilib 或 ABI

确认工具链支持：

```bash
riscv-none-elf-gcc -march=rv32imafdc_zicsr -mabi=ilp32d -print-multi-directory
```

输出应包含或指向 `rv32imafdc_zicsr/ilp32d` 对应目录。

### 编译成功但打包失败

依次检查：

- `wine64 --version` 是否正常；
- `projects/lierda/Obj/lierda.elf` 是否存在；
- xPack 工具链中的 `riscv-none-elf-objcopy` 是否可执行；
- SDK 的签名工具、私钥和配置文件是否完整。

### 下载脚本提示固件不存在

先重新编译和打包：

```bash
make -C projects/lierda clean
make -C projects/lierda
bash projects/lierda/aft_build_macos.sh
```

然后确认最终文件：

```bash
ls -lh projects/lierda/Obj/lierda_0x18000000_*.bin
```

### 下载脚本无法访问共享目录

检查共享目录是否已挂载：

```bash
ls -ld /Volumes/mpushare/mpushare/macos_workspace/isp-9star
```

同时确认 Ubuntu 下载机可访问共享目录中的固件文件。

### Ubuntu 找不到串口或没有权限

在 Ubuntu 下载机上检查：

```bash
ls -l /dev/ttyUSB*
```

确认下载命令传入的是实际存在的设备，并确保执行 ISP 工具的用户具有串口访问权限。

### 工程能编译，但下载后不能启动

先确保配置文件由当前版本生成器重新生成：

```bash
python3 tools/generate_td1601_makefiles.py projects/lierda --force
make -C projects/lierda clean
make -C projects/lierda
```

再检查 ELF 入口地址：

```bash
~/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1/bin/riscv-none-elf-readelf \
  -h projects/lierda/Obj/lierda.elf | grep 'Entry point'
```

`lierda` 工程的入口应位于 `0x18000000` Flash 区域内。随后重新打包并下载，不要直接把未封装的 `lierda.bin` 当作最终 ISP 固件下载。

## 13. macOS 推荐的日常开发命令

修改源码后，一键编译、打包并下载：

```bash
projects/lierda/build_patch_download.sh /dev/ttyUSB0
```

只验证编译：

```bash
make -C projects/lierda clean
make -C projects/lierda
```

工程配置发生变化后重新生成：

```bash
python3 tools/generate_td1601_makefiles.py projects/lierda --force
```

只下载已经打包好的固件：

```bash
SKIP_BUILD=1 projects/lierda/build_patch_download.sh /dev/ttyUSB0
```

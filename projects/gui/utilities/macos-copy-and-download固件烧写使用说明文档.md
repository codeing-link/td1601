# ISP 固件烧写工具

本工程用于在 macOS 终端发起操作，由 Ubuntu 主机访问实际串口并完成 ISP 固件烧写。下载程序最终在 Ubuntu 上以独立 Linux 可执行文件运行，不依赖 Ubuntu 系统环境中的 `python` 命令。

## 工作原理

```text
macOS 本地固件
     │
     │ macos-copy-and-download.sh：复制到 Samba 共享目录
     ▼
工程目录 /Volumes/mpushare/mpushare/macos_workspace/isp-9star
     │                         ▲
     │ Samba 映射              │ /home/qinbo/.../isp-9star
     ▼                         │
Ubuntu 项目目录 ── isp-remote-download.sh ──> dist/brom-isp
                                                       │
                                                       ▼
                                              Ubuntu 串口（例如 /dev/ttyUSB0）
                                                       │
                                                       ▼
                                                     目标芯片
```

- macOS 保存或生成固件，并执行入口脚本。
- 工程目录通过 Samba 共享；Ubuntu 看到的对应目录为 `/home/qinbo/mpushare/macos_workspace/isp-9star`（实际解析到 `/mnt/share/...`）。
- macOS 脚本通过 SSH 登录 Ubuntu；`dist/brom-isp` 在 Ubuntu 上打开串口、下载 `imgwriter.bin`，再下载应用固件。
- `brom-isp` 会自动忽略 macOS 生成的 `._*` AppleDouble 元数据文件，避免将其误判为第二份固件。

## 工程文件说明

| 文件 | 作用 |
| --- | --- |
| `BROM_DL.py` | ISP 下载器源代码。 |
| `imgwriter.bin` | 首先下载到目标芯片的 imgwriter 文件。 |
| `build-ubuntu-isp.sh` | 从 macOS 通过 SSH 在 Ubuntu 上将 Python 源码打包为 Linux 可执行文件。 |
| `dist/brom-isp` | Ubuntu 上实际运行的独立 ISP 下载程序。 |
| `isp-remote-download.sh` | 从 macOS SSH 调用 Ubuntu 下载程序。 |
| `macos-copy-and-download.sh` | 推荐的 macOS 入口：复制本地固件到 Samba 共享目录后开始烧写。 |

## 工程内的固件识别规则

下载器始终在 Ubuntu 项目根目录查找应用固件：

```text
*0x18000000_V1.0.0.bin
```

必须只有一个真实文件符合该规则。`._*` 开头的 macOS 元数据文件会被忽略。

`macos-copy-and-download.sh` 每次运行会：

1. 先将用户指定的 macOS 固件暂存，确保本地复制可用；
2. 删除共享工程根目录内已有的 `*0x18000000_V1.0.0.bin`；
3. 复制新固件为 `macos_0x18000000_V1.0.0.bin`；
4. 调用 Ubuntu 下载器烧写该固件。

因此，同一时刻工程根目录只保留一份被自动识别的应用固件。

## macOS 使用方法

推荐从任意 macOS 工作目录执行以下命令。第一个参数是 **Ubuntu 上实际存在的串口设备路径**，第二个参数是 **macOS 本地固件路径**。

```bash
/Volumes/mpushare/mpushare/macos_workspace/isp-9star/macos-copy-and-download.sh \
  /dev/ttyUSB0 \
  /path/on/macos/firmware.bin
```

例如，在其他工程目录中：

```bash
/Volumes/mpushare/mpushare/macos_workspace/isp-9star/macos-copy-and-download.sh \
  /dev/ttyUSB0 \
  ../Obj/gui_0x18000000_V1.0.0.bin
```

第二个参数可以是绝对路径或相对于当前 macOS 终端目录的相对路径。该脚本不依赖当前工作目录；它固定使用工程所在的 Samba 路径。

如果固件本来已在共享工程目录中，也可以直接调用远程下载器：

```bash
/Volumes/mpushare/mpushare/macos_workspace/isp-9star/isp-remote-download.sh \
  /dev/ttyUSB0
```

此方式不会复制固件，要求工程根目录中已经存在唯一的 `*0x18000000_V1.0.0.bin` 文件。

## Ubuntu 可执行文件构建

首次使用或修改 `BROM_DL.py` 后，在 macOS 执行：

```bash
cd /Volumes/mpushare/mpushare/macos_workspace/isp-9star
./build-ubuntu-isp.sh
```

该脚本会通过 SSH 进入 Ubuntu，并在工程目录下创建或复用 `.venv-isp` 构建环境，使用 PyInstaller 生成：

```text
/home/qinbo/mpushare/macos_workspace/isp-9star/dist/brom-isp
```

运行烧写时只调用 `brom-isp`，不需要在 Ubuntu 上直接执行 `BROM_DL.py` 或手动运行 Python。

## 配置与排查

- 默认 Ubuntu SSH 地址为 `qinbo@192.168.1.100`。如有变化，可在调用命令前设置 `UBUNTU_HOST=user@host`。
- 默认 Ubuntu 项目目录为 `/home/qinbo/mpushare/macos_workspace/isp-9star`。如有变化，可设置 `REMOTE_PROJECT=/path/to/isp-9star`。
- macOS 共享工程目录默认是 `/Volumes/mpushare/mpushare/macos_workspace/isp-9star`。可通过 `PROJECT_DIR=/path/to/project` 覆盖。
- Ubuntu 用户须具有访问指定串口（例如 `/dev/ttyUSB0`）的权限。
- 烧写前确认目标板已进入正确的 BootROM/下载模式，并且没有其他程序占用该串口。

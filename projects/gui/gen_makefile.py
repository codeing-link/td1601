#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
gen_makefile.py - TD1601 通用 Makefile 生成器

自动适配逻辑：
  - 工程名取自文件夹名
  - 工程源文件自动扫描 src/ 下所有 .c 和根目录 *.c
  - 核心组件（chip、minilibc、mm、csi、console）始终编译
  - 可选组件（lvgl、freertos 等）通过扫描 #include 自动检测

用法：在工程目录下运行
    python gen_makefile.py
"""

import os
import re
import glob
import sys

# ============================================================================
# 路径定义
# ============================================================================

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJ_DIR = SCRIPT_DIR
REPO_DIR = os.path.abspath(os.path.join(PROJ_DIR, '..', '..'))
OUTPUT = os.path.join(PROJ_DIR, 'Makefile')

# ============================================================================
# 平台配置（TD1601 E906FD 固定参数）
# ============================================================================

TOOLCHAIN_DIR = 'D:/toolchain/xpack-riscv-none-elf-gcc-14.3.0-1'
PYTHON_PATH = 'D:/sort/miniconda3/envs/ai310/python.exe'
GIT_FIND = 'D:/sort/Git/usr/bin/find.exe'

# 基础宏定义（始终生效）
BASE_DEFINES = [
    ('CONFIG_KERNEL_NONE', '1'),
    ('CONFIG_ARCH_INTERRUPTSTACK', '16384'),
    ('CONFIG_DEBUG', '1'),
    ('CONFIG_DEBUG_MODE', '0'),
    ('CONFIG_XIP', '1'),
    ('CONFIG_CHIP_TD1601', '1'),
    ('CONFIG_SUPPORT_TSPEND', '1'),
    ('CONFIG_CPU_E906FD', '1'),
]

# LVGL 专用宏（检测到 LVGL 时追加）
LVGL_DEFINES = [
    ('LV_LVGL_H_INCLUDE_SIMPLE', '1'),
    ('LV_CONF_INCLUDE_SIMPLE', '1'),
]

# FreeRTOS 专用宏（检测到 FreeRTOS 时替换 KERNEL_NONE）
FREERTOS_DEFINES = [
    ('CONFIG_KERNEL_FREERTOS', '1'),
]

# ============================================================================
# 组件注册表
# ============================================================================

COMPONENTS = {
    # --- 核心组件（始终编译） ---
    'chip_td1601': {
        'path': 'components/chips/chip_td1601/v1.0',
        'always': True,
        'src_dirs': ['drivers', 'drivers/ll', 'sys'],
        'asm_dirs': ['sys'],
        'includes': ['include', 'sys'],
    },
    'console': {
        'path': 'components/console/v1.0',
        'always': True,
        'src_dirs': ['src'],
        'includes': ['.', 'console'],
    },
    'csi': {
        'path': 'components/csi/v1.0',
        'always': True,
        'src_dirs': ['src'],
        'includes': ['include', 'include/core', 'include/drv'],
    },
    'minilibc': {
        'path': 'components/minilibc/v1.0',
        'always': True,
        'src_dirs': ['src'],
        'includes': ['include'],
    },
    'mm': {
        'path': 'components/mm/v1.0',
        'always': True,
        'src_dirs': ['src'],
        'includes': ['include'],
    },

    # --- 可选组件（自动检测） ---
    'lvgl': {
        'path': 'components/lvgl',
        'always': False,
        'detect_headers': ['lvgl.h', 'lv_conf.h', 'lvgl/lvgl.h'],
        'use_shell_find': True,    # 文件太多，Makefile 中用 $(shell find)
        'includes': ['.', 'src'],
        'extra_includes': ['components'],  # -I$(REPO)/components
    },
    'freertos': {
        'path': 'components/freertos/v1.0',
        'always': False,
        'detect_headers': ['FreeRTOS.h', 'task.h', 'semphr.h'],
        'src_dirs': [
            'FreeRTOS/Source',
            'FreeRTOS/Source/portable/MemMang',
            'FreeRTOS/Source/portable/GCC/riscv/rv32fd_32gpr/tspend',
            'adapter',
        ],
        'asm_dirs': [
            'FreeRTOS/Source/portable/GCC/riscv/rv32fd_32gpr/tspend',
        ],
        'includes': [
            'include',
            'FreeRTOS/Source/include',
            'FreeRTOS/Source/portable/GCC/riscv/rv32fd_32gpr/tspend',
        ],
    },
}

# 板级支持（始终需要，提供链接脚本和 board_init）
BOARD = {
    'path': 'boards/td1601_evb/v1.0',
    'includes': ['include'],
    'ldscript': 'gcc_flash.ld',
}


# ============================================================================
# 工具函数
# ============================================================================

def to_rel(path):
    """转为相对于工程目录的 POSIX 路径"""
    rel = os.path.relpath(path, PROJ_DIR)
    return rel.replace('\\', '/')


def obj_name(rel_path):
    """从相对路径生成不冲突的 .o 文件名"""
    name = rel_path.replace('../', '').replace('./', '')
    name = name.replace('/', '_')
    name = re.sub(r'\.(c|cpp|S)$', '.o', name)
    return name


def collect_c_files(directory):
    """递归收集目录下所有 .c 文件"""
    results = []
    for root, _, files in os.walk(directory):
        for f in sorted(files):
            if f.endswith('.c'):
                results.append(os.path.normpath(os.path.join(root, f)))
    return results


def collect_asm_files(directory):
    """收集目录下所有 .S 文件"""
    results = []
    if not os.path.isdir(directory):
        return results
    for f in sorted(os.listdir(directory)):
        if f.endswith('.S'):
            results.append(os.path.normpath(os.path.join(directory, f)))
    return results


def scan_project_includes():
    """扫描工程所有 .c/.h 文件中的 #include，返回头文件名集合"""
    headers = set()
    exclude_dirs = {'Obj', 'Lst', '.cache', '__workspace_pack__'}
    scan_dirs = [PROJ_DIR]
    for scan_dir in scan_dirs:
        if not os.path.isdir(scan_dir):
            continue
        for root, dirs, files in os.walk(scan_dir):
            dirs[:] = [d for d in dirs if d not in exclude_dirs]
            for f in files:
                if not (f.endswith('.c') or f.endswith('.h')):
                    continue
                filepath = os.path.join(root, f)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as fh:
                        for line in fh:
                            m = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', line)
                            if m:
                                headers.add(m.group(1))
                except (IOError, OSError):
                    pass
    return headers


def detect_optional_components(includes_set):
    """根据工程 #include 集合判断哪些可选组件需要编译"""
    detected = []
    for name, info in COMPONENTS.items():
        if info.get('always'):
            continue
        detect_headers = info.get('detect_headers', [])
        for h in detect_headers:
            if h in includes_set:
                detected.append(name)
                break
    return detected


def discover_components():
    """自动发现 components/ 下未注册的组件目录，返回新增组件的字典"""
    components_dir = os.path.join(REPO_DIR, 'components')
    if not os.path.isdir(components_dir):
        return {}

    registered_paths = set()
    for info in COMPONENTS.values():
        registered_paths.add(os.path.normpath(os.path.join(REPO_DIR, info['path'])))

    discovered = {}
    for entry in sorted(os.listdir(components_dir)):
        entry_path = os.path.join(components_dir, entry)
        if not os.path.isdir(entry_path):
            continue

        # 跳过已注册组件的父目录（如 chips、lvgl 等）
        is_registered = False
        for rp in registered_paths:
            if os.path.normpath(entry_path) == rp or rp.startswith(os.path.normpath(entry_path) + os.sep):
                is_registered = True
                break
        if is_registered:
            continue

        # 推断组件结构
        comp_path = f'components/{entry}'
        src_dirs = []
        includes = []
        detect_headers = []

        # 查找源文件目录
        for candidate in ['src', 'source', '.']:
            d = os.path.join(entry_path, candidate) if candidate != '.' else entry_path
            if os.path.isdir(d):
                has_c = any(f.endswith('.c') for f in os.listdir(d) if os.path.isfile(os.path.join(d, f)))
                if has_c:
                    src_dirs.append(candidate if candidate != '.' else '.')
                    break

        # 如果根目录和 src/ 都没有 .c，递归找
        if not src_dirs:
            for root, _, files in os.walk(entry_path):
                if any(f.endswith('.c') for f in files):
                    rel = os.path.relpath(root, entry_path).replace('\\', '/')
                    src_dirs.append(rel)
            if not src_dirs:
                continue  # 没有源文件，跳过

        # 查找头文件目录并推断 detect_headers
        for candidate in ['include', 'inc', '.']:
            d = os.path.join(entry_path, candidate) if candidate != '.' else entry_path
            if os.path.isdir(d):
                h_files = [f for f in os.listdir(d)
                           if f.endswith('.h') and os.path.isfile(os.path.join(d, f))]
                if h_files:
                    includes.append(candidate if candidate != '.' else '.')
                    detect_headers.extend(h_files)
                    break

        if not detect_headers:
            continue  # 没有公开头文件，无法被检测到

        discovered[entry] = {
            'path': comp_path,
            'always': False,
            'detect_headers': detect_headers,
            'src_dirs': src_dirs,
            'includes': includes,
        }

    return discovered


# ============================================================================
# 主逻辑
# ============================================================================

def main():
    # 工程名 = 文件夹名
    project_name = os.path.basename(PROJ_DIR)
    print(f"工程名: {project_name}")

    # ---- 收集工程源文件（递归扫描工程目录，排除构建目录）----
    proj_sources = []
    exclude_dirs = {'Obj', 'Lst', '.cache', '__workspace_pack__'}
    for root, dirs, files in os.walk(PROJ_DIR):
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for f in sorted(files):
            if f.endswith('.c') or f.endswith('.cpp'):
                proj_sources.append(os.path.normpath(os.path.join(root, f)))

    print(f"工程源文件: {len(proj_sources)} 个")

    # ---- 自动发现未注册组件 ----
    discovered = discover_components()
    if discovered:
        print(f"自动发现组件: {list(discovered.keys())}")
        COMPONENTS.update(discovered)

    # ---- 检测可选组件 ----
    includes_set = scan_project_includes()
    optional = detect_optional_components(includes_set)

    # 构建活跃组件列表
    active_components = []
    for name, info in COMPONENTS.items():
        if info.get('always') or name in optional:
            active_components.append(name)

    print(f"活跃组件: {active_components}")
    if optional:
        print(f"  (自动检测到: {optional})")

    # ---- 确定 defines ----
    use_freertos = 'freertos' in active_components
    use_lvgl = 'lvgl' in active_components

    defines = []
    for k, v in BASE_DEFINES:
        if k == 'CONFIG_KERNEL_NONE' and use_freertos:
            continue  # FreeRTOS 下不要 KERNEL_NONE
        defines.append((k, v))
    if use_freertos:
        defines.extend(FREERTOS_DEFINES)
    if use_lvgl:
        defines.extend(LVGL_DEFINES)

    # ---- 收集 include 路径 ----
    all_includes = []

    # 工程自身
    all_includes.append('include')
    all_includes.append('include/sys')
    all_includes.append('src')
    all_includes.append('src/eez/gui/src/ui')

    # 板级
    board_abs = os.path.join(REPO_DIR, BOARD['path'])
    for inc in BOARD['includes']:
        all_includes.append(to_rel(os.path.join(board_abs, inc)))

    # 各组件
    for name in active_components:
        info = COMPONENTS[name]
        comp_abs = os.path.join(REPO_DIR, info['path'])
        for inc in info.get('includes', []):
            all_includes.append(to_rel(os.path.join(comp_abs, inc)))
        for inc in info.get('extra_includes', []):
            all_includes.append(to_rel(os.path.join(REPO_DIR, inc)))

    # ---- 收集组件源文件 ----
    comp_data = {}  # name -> {'c': [...], 'asm': [...]}
    for name in active_components:
        info = COMPONENTS[name]
        comp_abs = os.path.join(REPO_DIR, info['path'])

        if info.get('use_shell_find'):
            comp_data[name] = {'shell_find': True, 'path': comp_abs}
            continue

        c_files = []
        asm_files = []

        for sd in info.get('src_dirs', []):
            d = os.path.join(comp_abs, sd)
            if os.path.isdir(d):
                # 只收集当前目录，不递归（子目录由单独条目处理）
                for f in sorted(os.listdir(d)):
                    full = os.path.join(d, f)
                    if f.endswith('.c') and os.path.isfile(full):
                        c_files.append(full)

        for sd in info.get('asm_dirs', []):
            d = os.path.join(comp_abs, sd)
            asm_files.extend(collect_asm_files(d))

        comp_data[name] = {'c': c_files, 'asm': asm_files}
        print(f"  {name}: {len(c_files)} .c, {len(asm_files)} .S")

    # ==================================================================
    # 生成 Makefile
    # ==================================================================
    print(f"\n生成 Makefile ...")

    with open(OUTPUT, 'w', encoding='utf-8', newline='\n') as f:
        write_makefile(f, project_name, proj_sources, active_components,
                       comp_data, all_includes, defines)

    print(f"\n{'='*50}")
    print(f"Makefile 已生成")
    print(f"{'='*50}")
    print(f"\n使用方法:")
    print(f"  make            # 增量编译")
    print(f"  make image      # 编译 + 签名 + 打包烧录文件")
    print(f"  make clean      # 清除编译产物")


# ============================================================================
# Makefile 生成
# ============================================================================

def write_makefile(f, project_name, proj_sources, active_components,
                   comp_data, all_includes, defines):
    """写入完整 Makefile"""

    board_rel = to_rel(os.path.join(REPO_DIR, BOARD['path']))
    ldscript_rel = f"{board_rel}/{BOARD['ldscript']}"

    # ---- 头部 ----
    f.write(f"""\
# ============================================================================
# Makefile - 由 gen_makefile.py 自动生成
#
# 用法:
#   make            增量编译
#   make image      编译 + 签名 + 打包烧录文件
#   make clean      清除编译产物
# ============================================================================

.DEFAULT_GOAL := all
SHELL := D:/sort/Git/usr/bin/sh.exe
FIND  := {GIT_FIND}

PROJECT  := {project_name}

# ---- 工具链 (xPack riscv-none-elf-gcc) ------------------------------------
TOOLCHAIN_DIR ?= {TOOLCHAIN_DIR}
CROSS    := $(TOOLCHAIN_DIR)/bin/riscv-none-elf-
CC       := $(CROSS)gcc
CXX      := $(CROSS)g++
AS       := $(CROSS)gcc
AR       := $(CROSS)ar
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
SIZE     := $(CROSS)size

# ---- 路径 -----------------------------------------------------------------
REPO     := ../..
OBJ      := Obj
LST      := Lst
LDSCRIPT := {ldscript_rel}

# ---- 架构参数 (Xuantie E906FD) --------------------------------------------
ARCH_FLAGS := -march=rv32imafdc_zicsr_xtheadcmo -mabi=ilp32d

# ---- T-Head 汇编兼容头 ----------------------------------------------------
COMPAT := -include include/thead_compat.h

""")

    # ---- Include 路径 ----
    f.write("# ---- Include 路径 "
            "--------------------------------------------------------\n")
    f.write("INCLUDES := \\\n")
    for i, inc in enumerate(all_includes):
        tail = ' \\' if i < len(all_includes) - 1 else ''
        f.write(f"\t-I{inc}{tail}\n")
    f.write('\n')

    # ---- 宏定义 ----
    f.write("# ---- 预处理宏定义 "
            "--------------------------------------------------------\n")
    f.write("DEFINES := \\\n")
    for i, (k, v) in enumerate(defines):
        tail = ' \\' if i < len(defines) - 1 else ''
        f.write(f"\t-D{k}={v}{tail}\n")
    f.write('\n')

    # ---- 编译参数 ----
    f.write("""\
# ---- 编译/链接参数 ---------------------------------------------------------
COMMON_FLAGS := $(ARCH_FLAGS) -Os -g $(COMPAT) $(INCLUDES) $(DEFINES) \\
\t-ffunction-sections -fdata-sections

CFLAGS   := $(COMMON_FLAGS) -Wall -std=gnu99
CXXFLAGS := $(COMMON_FLAGS) -Wall -std=gnu++14 -fno-exceptions -fno-rtti \\
\t-DEEZ_DISABLE_DATE_NOW_DEFAULT_IMPLEMENTATION
ASFLAGS  := $(COMMON_FLAGS) -x assembler-with-cpp
DEPFLAGS = -MMD -MP

LDFLAGS := -nostartfiles --specs=nano.specs -Wl,--gc-sections \\
\t-T$(LDSCRIPT) $(ARCH_FLAGS) -L$(OBJ) -Wl,-zmax-page-size=1024 \\
\t-Wl,-Map=$(LST)/$(PROJECT).map

""")

    # ---- 工程源文件 ----
    f.write("# ============================================================"
            "================\n")
    f.write("# 工程源文件\n")
    f.write("# ============================================================"
            "================\n")

    proj_objs = []
    for src in proj_sources:
        rel = to_rel(src)
        proj_objs.append(obj_name(rel))

    f.write("PROJ_OBJS := \\\n")
    for i, o in enumerate(proj_objs):
        tail = ' \\' if i < len(proj_objs) - 1 else ''
        f.write(f"\t$(OBJ)/{o}{tail}\n")
    f.write('\n')

    for src in proj_sources:
        rel = to_rel(src)
        oname = obj_name(rel)
        ext = os.path.splitext(src)[1]
        if ext == '.S':
            compiler, flags = '$(AS)', '$(ASFLAGS)'
        elif ext == '.cpp':
            compiler, flags = '$(CXX)', '$(CXXFLAGS)'
        else:
            compiler, flags = '$(CC)', '$(CFLAGS)'
        f.write(f"$(OBJ)/{oname}: {rel}\n")
        f.write("\t@mkdir -p $(dir $@)\n")
        f.write(f"\t{compiler} -c {flags} $(DEPFLAGS)"
                f" -MF $(@:.o=.d) -o $@ $<\n\n")

    # ---- 各组件 ----
    lib_targets = []
    lib_names = []

    for name in active_components:
        data = comp_data[name]
        info = COMPONENTS[name]
        comp_rel = to_rel(os.path.join(REPO_DIR, info['path']))

        f.write("# ============================================================"
                "================\n")
        f.write(f"# 组件: {name}\n")
        f.write("# ============================================================"
                "================\n")

        lib_file = f"lib{name}.a"
        lib_targets.append(f"$(OBJ)/{lib_file}")
        lib_names.append(name)

        if data.get('shell_find'):
            # LVGL 等大型组件用 $(shell find)
            var = name.upper()
            f.write(f"{var}_SRCS := $(shell $(FIND) {comp_rel}/src"
                    f" -name \"*.c\")\n")
            f.write(f"{var}_OBJS := $(patsubst {comp_rel}/src/%.c,"
                    f"$(OBJ)/{name}/%.o,$({var}_SRCS))\n\n")
            f.write(f"$(OBJ)/{name}/%.o: {comp_rel}/src/%.c\n")
            f.write("\t@mkdir -p $(dir $@)\n")
            f.write("\t$(CC) -c $(CFLAGS) $(DEPFLAGS)"
                    " -MF $(@:.o=.d) -o $@ $<\n\n")
            f.write(f"$(OBJ)/{lib_file}: $({var}_OBJS)\n")
            f.write("\t$(AR) rcs $@ $^\n\n")
            continue

        # 常规组件：列出所有 .o
        objs = []
        all_srcs = []

        for src in data.get('c', []):
            rel = to_rel(src)
            base = os.path.splitext(os.path.basename(src))[0]
            oname = f"{name}_{base}.o"
            objs.append(oname)
            all_srcs.append((rel, oname, '$(CC)', '$(CFLAGS)'))

        for src in data.get('asm', []):
            rel = to_rel(src)
            base = os.path.splitext(os.path.basename(src))[0]
            oname = f"{name}_{base}.o"
            objs.append(oname)
            all_srcs.append((rel, oname, '$(AS)', '$(ASFLAGS)'))

        if not objs:
            # 无源文件的组件（如 board），不生成 lib
            lib_targets.pop()
            lib_names.pop()
            f.write("# (仅提供头文件和链接脚本，无源文件)\n\n")
            continue

        var = name.upper().replace('-', '_')
        f.write(f"{var}_OBJS := \\\n")
        for i, o in enumerate(objs):
            tail = ' \\' if i < len(objs) - 1 else ''
            f.write(f"\t$(OBJ)/{o}{tail}\n")
        f.write('\n')

        for rel, oname, compiler, flags in all_srcs:
            f.write(f"$(OBJ)/{oname}: {rel}\n")
            f.write("\t@mkdir -p $(dir $@)\n")
            f.write(f"\t{compiler} -c {flags} $(DEPFLAGS)"
                    f" -MF $(@:.o=.d) -o $@ $<\n\n")

        f.write(f"$(OBJ)/{lib_file}: $({var}_OBJS)\n")
        f.write("\t$(AR) rcs $@ $^\n\n")

    # ---- 链接 ----
    f.write("# ============================================================"
            "================\n")
    f.write("# 链接\n")
    f.write("# ============================================================"
            "================\n")

    f.write("LIBS := \\\n")
    for i, lt in enumerate(lib_targets):
        tail = ' \\' if i < len(lib_targets) - 1 else ''
        f.write(f"\t{lt}{tail}\n")
    f.write('\n')

    lib_flags = ' '.join(f'-l{n}' for n in lib_names)

    # 检测是否有 C++ 源文件，需要链接 libstdc++
    has_cpp = any(s.endswith('.cpp') for s in proj_sources)
    if has_cpp:
        lib_flags += ' -lstdc++'
    lib_flags += ' -lm'
    ld_path = os.path.join(REPO_DIR, BOARD['path'], BOARD['ldscript'])
    flash_addr = '0x18000000'
    with open(ld_path, 'r', encoding='utf-8') as lf:
        for line in lf:
            m = re.search(r'SPIFLASH\s*:\s*ORIGIN\s*=\s*(0x[0-9a-fA-F]+)', line)
            if m:
                flash_addr = m.group(1)
                break
    img_offset = int(flash_addr, 16) - 0x18000000

    pkg_yaml = os.path.join(PROJ_DIR, 'package.yaml')
    version = 'V1.0.0'
    if os.path.isfile(pkg_yaml):
        with open(pkg_yaml, 'r', encoding='utf-8') as pf:
            for line in pf:
                m = re.match(r'^version:\s*(.+)', line)
                if m:
                    version = m.group(1).strip()
                    break

    f.write(f"""\
ELF  := $(OBJ)/$(PROJECT).elf
BIN  := $(OBJ)/$(PROJECT).bin
IHEX := $(OBJ)/$(PROJECT).ihex

# ---- 签名打包参数（由 gen_makefile.py 解析生成）-----------------------------
BOARD_DIR    := {board_rel}
PRODUCT_EXE  := $(BOARD_DIR)/bootimgs/product.exe
CRC32_EXE    := $(BOARD_DIR)/bootimgs/crc32.exe
PRIVATE_KEY  := $(BOARD_DIR)/bootimgs/private.ecc.pem
FLASH_ADDR   := {flash_addr}
IMG_OFFSET   := {img_offset}
VERSION      := {version}

.PHONY: all clean size image
all: $(BIN) $(IHEX) size

image: all
\t@echo "==== 签名打包: $(PROJECT) $(VERSION) @$(FLASH_ADDR) ===="
\t$(OBJCOPY) -O binary $(ELF) $(OBJ)/$(PROJECT)_$(FLASH_ADDR).bin
\t$(PRODUCT_EXE) sig $(OBJ)/$(PROJECT)_$(FLASH_ADDR).bin \\
\t\t-n $(VERSION) -iver 0x58 \\
\t\t-s $(PRIVATE_KEY) \\
\t\t-o $(OBJ)/$(PROJECT)_$(FLASH_ADDR)-mnt.bin \\
\t\t-t SPI_Flash -a $(IMG_OFFSET) \\
\t\t-tr SPI_Flash -ar $(IMG_OFFSET) \\
\t\t-ss ECC160r1 -ds SHA1 -d
""")
    # 步骤1: 计算 raw.bin 的 CRC32，追加到 mnt.bin 尾部（104 -> 108 字节）
    f.write('\t@CRC=$$($(CRC32_EXE) $(OBJ)/$(PROJECT)_$(FLASH_ADDR).bin'
            ' | awk \'{print $$NF}\'); \\\n')
    f.write('\tHEX=$${CRC#0x}; \\\n')
    f.write('\tprintf "\\\\x$${HEX:6:2}\\\\x$${HEX:4:2}'
            '\\\\x$${HEX:2:2}\\\\x$${HEX:0:2}" \\\n')
    f.write('\t\t>> $(OBJ)/$(PROJECT)_$(FLASH_ADDR)-mnt.bin; \\\n')
    f.write('\techo "raw.bin CRC32: $$CRC -> 已追加到 mnt.bin 尾部"\n')
    # 步骤2: fill.bin 填充（mnt 108 + fill 3988 = 4096 页对齐）
    f.write('\tdd if=/dev/zero of=$(OBJ)/fill.bin bs=3988 count=1 2>/dev/null\n')
    # 步骤3: 拼接 mnt + fill + raw
    f.write('\tcat $(OBJ)/$(PROJECT)_$(FLASH_ADDR)-mnt.bin $(OBJ)/fill.bin \\\n')
    f.write('\t\t$(OBJ)/$(PROJECT)_$(FLASH_ADDR).bin \\\n')
    f.write('\t\t> $(OBJ)/$(PROJECT)_$(FLASH_ADDR)_$(VERSION).bin\n')
    # 步骤4: 计算最终文件 CRC32，追加到自身尾部
    f.write('\t@CRC=$$($(CRC32_EXE) $(OBJ)/$(PROJECT)_$(FLASH_ADDR)_$(VERSION).bin'
            ' | awk \'{print $$NF}\'); \\\n')
    f.write('\tHEX=$${CRC#0x}; \\\n')
    f.write('\tprintf "\\\\x$${HEX:6:2}\\\\x$${HEX:4:2}'
            '\\\\x$${HEX:2:2}\\\\x$${HEX:0:2}" \\\n')
    f.write('\t\t>> $(OBJ)/$(PROJECT)_$(FLASH_ADDR)_$(VERSION).bin; \\\n')
    f.write('\techo "final CRC32: $$CRC -> 已追加到文件尾部"\n')
    f.write(f"""\
\t@echo "==== 输出: $(OBJ)/$(PROJECT)_$(FLASH_ADDR)_$(VERSION).bin ===="

$(ELF): $(PROJ_OBJS) $(LIBS)
\t@mkdir -p $(LST)
\t$(CC) $(PROJ_OBJS) \\
\t\t-Wl,--start-group \\
\t\t{lib_flags} \\
\t\t-Wl,--end-group \\
\t\t$(LDFLAGS) -o $@
\t$(OBJDUMP) -d $@ > $(LST)/$(PROJECT).asm

$(BIN): $(ELF)
\t$(OBJCOPY) -O binary $< $@

$(IHEX): $(ELF)
\t$(OBJCOPY) -O ihex $< $@

size: $(ELF)
\t$(SIZE) $(ELF)

clean:
\trm -rf $(OBJ) $(LST) Makefile fill.bin \
\t\t.cache .cdk __workspace_pack__ \
\t\tcdkws.mk $(PROJECT).mk $(PROJECT).modify.bat $(PROJECT).txt

# ---- 自动依赖 -------------------------------------------------------------
-include $(wildcard $(OBJ)/*.d $(OBJ)/**/*.d)
""")


if __name__ == '__main__':
    main()

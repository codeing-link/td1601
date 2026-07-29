#!/usr/bin/env python3
"""Generate host-specific xPack build and download scripts for CDK solutions.

The CDK project file tells us where packages live; package.yaml supplies the
source globs, public includes, flags and configuration.  This intentionally
has no PyYAML dependency so it can be used on a fresh macOS or Windows
installation.
"""
from __future__ import annotations

import argparse
import glob
import os
import platform
import re
import stat
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


def unquote(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] in "'\"" and value[-1] == value[0]:
        return value[1:-1]
    return value


def yaml_subset(path: Path) -> dict:
    """Read the mapping/list subset used by CDK package.yaml files."""
    root: dict = {}
    stack: list[tuple[int, object]] = [(-1, root)]
    lines = path.read_text(encoding="utf-8").splitlines()
    i = 0
    while i < len(lines):
        raw = lines[i]
        i += 1
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        indent = len(raw) - len(raw.lstrip())
        text = raw.strip()
        while len(stack) > 1 and indent <= stack[-1][0]:
            stack.pop()
        parent = stack[-1][1]
        if text.startswith("- "):
            if isinstance(parent, list):
                parent.append(unquote(text[2:].split(" #", 1)[0]))
            continue
        if ":" not in text or not isinstance(parent, dict):
            continue
        key, value = text.split(":", 1)
        key, value = key.strip(), value.strip()
        value = value.split(" #", 1)[0].rstrip()
        if value.startswith("#"):
            value = ""
        if value in ("", "#"):
            # Look ahead: CDK uses a mapping or a list here.
            kind = dict
            for next_raw in lines[i:]:
                if not next_raw.strip() or next_raw.lstrip().startswith("#"):
                    continue
                if len(next_raw) - len(next_raw.lstrip()) <= indent:
                    break
                kind = list if next_raw.strip().startswith("- ") else dict
                break
            child = kind()
            parent[key] = child
            stack.append((indent, child))
        elif value in (">", "|"):
            parts: list[str] = []
            while i < len(lines):
                nxt = lines[i]
                nxt_indent = len(nxt) - len(nxt.lstrip())
                if nxt.strip() and nxt_indent <= indent:
                    break
                i += 1
                if nxt.strip():
                    parts.append(nxt.strip())
            parent[key] = " ".join(parts)
        else:
            parent[key] = unquote(value)
    return root


def parse_dep(item: str) -> str:
    return item.split(":", 1)[0].strip()


def version_dir(base: Path, package: str, version: str) -> Path:
    version = version.strip().lstrip("=<> ")
    exact = base / package / version
    if exact.is_dir():
        return exact.resolve()
    matches = sorted((base / package).glob("[Vv]*")) if (base / package).is_dir() else []
    for match in matches:
        if match.name.lower() == version.lower():
            return match.resolve()
    if len(matches) == 1:
        return matches[0].resolve()
    raise FileNotFoundError(f"cannot locate {package} {version} below {base}")


@dataclass
class Package:
    name: str
    path: Path
    data: dict
    kind: str


def add_package(packages: dict[str, Package], path: Path, kind: str) -> Package:
    data = yaml_subset(path / "package.yaml")
    name = data.get("name")
    if not name:
        raise ValueError(f"{path}/package.yaml has no name")
    pkg = Package(name, path.resolve(), data, kind)
    old = packages.get(name)
    if old and old.path != pkg.path:
        raise ValueError(f"duplicate package {name}: {old.path} and {pkg.path}")
    packages[name] = pkg
    return pkg


def write_generated(path: Path, content: str) -> None:
    """Write generated text with LF endings on every host platform."""
    path.write_bytes(content.encode("utf-8"))


COMPAT_HEADER = r'''#ifndef TD1601_XPACK_COMPAT_H
#define TD1601_XPACK_COMPAT_H
#define TD1601_COMPAT_ASM \
".set mxstatus, 0x7c0\n" \
".set mhcr, 0x7c1\n" \
".set mhint, 0x7c5\n" \
".set mtvt, 0x307\n" \
".macro dcache.call\n th.dcache.call\n.endm\n" \
".macro dcache.ciall\n th.dcache.ciall\n.endm\n" \
".macro dcache.iall\n th.dcache.iall\n.endm\n" \
".macro icache.iall\n th.icache.iall\n.endm\n" \
".macro dcache.cpa rs1\n th.dcache.cpa \\rs1\n.endm\n" \
".macro dcache.cipa rs1\n th.dcache.cipa \\rs1\n.endm\n" \
".macro dcache.ipa rs1\n th.dcache.ipa \\rs1\n.endm\n" \
".macro icache.ipa rs1\n th.icache.ipa \\rs1\n.endm\n"
#ifdef __ASSEMBLER__
.set mxstatus, 0x7c0
.set mhcr, 0x7c1
.set mhint, 0x7c5
.set mtvt, 0x307
.macro dcache.call
 th.dcache.call
.endm
.macro dcache.ciall
 th.dcache.ciall
.endm
.macro dcache.iall
 th.dcache.iall
.endm
.macro icache.iall
 th.icache.iall
.endm
.macro dcache.cpa rs1
 th.dcache.cpa \rs1
.endm
.macro dcache.cipa rs1
 th.dcache.cipa \rs1
.endm
.macro dcache.ipa rs1
 th.dcache.ipa \rs1
.endm
.macro icache.ipa rs1
 th.icache.ipa \rs1
.endm
#else
__asm__(TD1601_COMPAT_ASM);
#endif
#endif
'''


AFTER_BUILD = r'''#!/bin/bash
# Generated by generate_td1601_makefiles.py. Run after `make`.
set -euo pipefail
PROJECT="@PROJECT@"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/@REPO_REL@" && pwd)"
BOARD="$REPO/boards/@BOARD@/@BOARD_VERSION@"
OBJ="$HERE/Obj"
ELF="$OBJ/$PROJECT.elf"
TC_DIR="${TOOLCHAIN_DIR:-$HOME/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1}"
OBJCOPY="$TC_DIR/bin/riscv-none-elf-objcopy"
VERSION="$(sed -n 's/^version:[[:space:]]*V//p' "$HERE/package.yaml" | head -n 1 | tr -d '\r')"
ADDR="$(grep 'SPIFLASH : ORIGIN = ' "$BOARD/gcc_flash.ld" | grep -oE '0x[0-9a-fA-F]+' | head -n 1)"
[ -n "$VERSION" ] && [ -n "$ADDR" ] && [ -f "$ELF" ] || { echo 'missing ELF, version, or flash address' >&2; exit 1; }
RAW="$OBJ/${PROJECT}_${ADDR}.bin"
MNT="$OBJ/${PROJECT}_${ADDR}-mnt.bin"
OUT="$OBJ/${PROJECT}_${ADDR}_V${VERSION}.bin"
"$OBJCOPY" -O binary "$ELF" "$RAW"
offset=$((16#${ADDR#0x} - 16#18000000))
wine_bin="$(command -v wine || command -v wine64 || true)"
[ -n "$wine_bin" ] || { echo 'wine/wine64 is required for product.exe signing' >&2; exit 1; }
"$wine_bin" "$BOARD/bootimgs/product.exe" sig "$RAW" -n "$VERSION" -iver 0x58 -s "$BOARD/bootimgs/private.ecc.pem" -o "$MNT" -t SPI_Flash -a "$offset" -tr SPI_Flash -ar "$offset" -ss ECC160r1 -ds SHA1 -d
append_crc() { python3 - "$1" "$2" <<'PY'
import sys, zlib
crc = zlib.crc32(open(sys.argv[1], 'rb').read()) & 0xffffffff
open(sys.argv[2], 'ab').write(crc.to_bytes(4, 'little'))
PY
}
append_crc "$RAW" "$MNT"
dd if=/dev/zero of="$OBJ/fill.bin" bs=3988 count=1 status=none
cat "$MNT" "$OBJ/fill.bin" "$RAW" > "$OUT"
append_crc "$OUT" "$OUT"
echo "Flashable image: $OUT"
'''


AFTER_BUILD_WINDOWS = r'''#!/bin/bash
# Generated by generate_td1601_makefiles.py. Run after `make` in Git Bash.
set -euo pipefail

PROJECT="@PROJECT@"
SCRIPT_DIR="${BASH_SOURCE[0]%/*}"
[ "$SCRIPT_DIR" = "${BASH_SOURCE[0]}" ] && SCRIPT_DIR="."
HERE="$(cd "$SCRIPT_DIR" && pwd)"
REPO="$(cd "$HERE/@REPO_REL@" && pwd)"
WORKSPACE="$(cd "$REPO/.." && pwd)"
BOARD="$REPO/boards/@BOARD@/@BOARD_VERSION@"
OBJ="$HERE/Obj"
ELF="$OBJ/$PROJECT.elf"

TC_DIR_RAW="${TOOLCHAIN_DIR:-$WORKSPACE/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1}"
TC_DIR="$(cygpath -u "$TC_DIR_RAW")"
OBJCOPY="$TC_DIR/bin/riscv-none-elf-objcopy"
VERSION="$(sed -n 's/^version:[[:space:]]*V//p' "$HERE/package.yaml" | head -n 1 | tr -d '\r')"
ADDR="$(grep 'SPIFLASH : ORIGIN = ' "$BOARD/gcc_flash.ld" | grep -oE '0x[0-9a-fA-F]+' | head -n 1)"
[ -n "$VERSION" ] && [ -n "$ADDR" ] && [ -f "$ELF" ] || { echo 'missing ELF, version, or flash address' >&2; exit 1; }

RAW="$OBJ/${PROJECT}_${ADDR}.bin"
MNT="$OBJ/${PROJECT}_${ADDR}-mnt.bin"
OUT="$OBJ/${PROJECT}_${ADDR}_V${VERSION}.bin"
"$OBJCOPY" -O binary "$ELF" "$RAW"
offset=$((16#${ADDR#0x} - 16#18000000))

WIN_PRODUCT_EXE="$(cygpath -w "$BOARD/bootimgs/product.exe")"
WIN_RAW="$(cygpath -w "$RAW")"
WIN_PEM="$(cygpath -w "$BOARD/bootimgs/private.ecc.pem")"
WIN_MNT="$(cygpath -w "$MNT")"
"$WIN_PRODUCT_EXE" sig "$WIN_RAW" -n "$VERSION" -iver 0x58 -s "$WIN_PEM" -o "$WIN_MNT" -t SPI_Flash -a "$offset" -tr SPI_Flash -ar "$offset" -ss ECC160r1 -ds SHA1 -d

append_crc() { env python - "$1" "$2" <<'PY'
import sys, zlib
crc = zlib.crc32(open(sys.argv[1], 'rb').read()) & 0xffffffff
open(sys.argv[2], 'ab').write(crc.to_bytes(4, 'little'))
PY
}

append_crc "$RAW" "$MNT"
dd if=/dev/zero of="$OBJ/fill.bin" bs=3988 count=1 status=none
cat "$MNT" "$OBJ/fill.bin" "$RAW" > "$OUT"
append_crc "$OUT" "$OUT"
echo "Flashable image: $OUT"
'''


BUILD_PATCH_DOWNLOAD = r'''#!/usr/bin/env bash
# Generated by generate_td1601_makefiles.py.
# Build, sign/package, copy to the shared ISP project, then burn over UART.
# Usage: ./build_patch_download.sh [ubuntu-serial-port]
set -euo pipefail
PROJECT="@PROJECT@"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
case "$#" in
    0) PORT="/dev/ttyUSB0" ;;
    1) PORT="$1" ;;
    *)
        echo "Usage: $0 [/dev/ttyUSB0]" >&2
        exit 2
        ;;
esac

# Use the project's verified staging helper.  It copies the firmware to the
# Samba share and forwards an explicit Ubuntu-side path to the ISP downloader.
ISP_TOOL="${ISP_TOOL:-$HERE/utilities/macos-copy-and-download.sh}"

echo "Download port: $PORT"

if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
    make -C "$HERE"
    "$HERE/aft_build_macos.sh"
fi

VERSION="$(sed -n 's/^version:[[:space:]]*V//p' "$HERE/package.yaml" | head -n 1 | tr -d '\r')"
ADDR="$(grep 'SPIFLASH : ORIGIN = ' "$HERE/@REPO_REL@/boards/@BOARD@/@BOARD_VERSION@/gcc_flash.ld" | grep -oE '0x[0-9a-fA-F]+' | head -n 1)"
FIRMWARE="$HERE/Obj/${PROJECT}_${ADDR}_V${VERSION}.bin"
[ -f "$FIRMWARE" ] || { echo "firmware was not generated: $FIRMWARE" >&2; exit 1; }
[ -x "$ISP_TOOL" ] || { echo "ISP downloader not found/executable: $ISP_TOOL" >&2; exit 1; }
exec "$ISP_TOOL" "$PORT" "$FIRMWARE"
'''


MACOS_COPY_AND_DOWNLOAD = r'''#!/usr/bin/env bash
# Generated by generate_td1601_makefiles.py.
# Copy a macOS-local firmware into the Samba-shared ISP project, then burn it
# from Ubuntu. This script can be launched from any macOS working directory.
#
# Usage:
#   ./utilities/macos-copy-and-download.sh /dev/ttyUSB0 /path/to/firmware.bin

set -euo pipefail

PROJECT_DIR="${PROJECT_DIR:-/Volumes/mpushare/mpushare/macos_workspace/isp-9star}"
REMOTE_DOWNLOADER="${REMOTE_DOWNLOADER:-${PROJECT_DIR}/isp-remote-download.sh}"
if (( $# != 2 )); then
    echo "Usage: $0 /dev/ttyUSB0 /path/to/firmware.bin" >&2
    exit 2
fi

serial_port="$1"
source_file="$2"

if [[ ! -d "$PROJECT_DIR" ]]; then
    echo "ISP project directory not found: $PROJECT_DIR" >&2
    exit 1
fi
if [[ ! -x "$REMOTE_DOWNLOADER" ]]; then
    echo "Remote downloader not found or not executable: $REMOTE_DOWNLOADER" >&2
    exit 1
fi
if [[ ! -f "$source_file" ]]; then
    echo "Firmware file not found: $source_file" >&2
    exit 1
fi

# Stage first so a failed local copy never removes the current shared firmware.
staged_file="$(mktemp "${PROJECT_DIR}/.isp-firmware.XXXXXX")"
cleanup_stage() {
    rm -f "$staged_file"
}
trap cleanup_stage EXIT
cp "$source_file" "$staged_file"

# Use the source name and pass its path explicitly to the remote downloader.
# This avoids a hard-coded firmware version and ambiguous auto-detection.
target_file="${PROJECT_DIR}/$(basename "$source_file")"
cp "$staged_file" "$target_file"
rm -f "$staged_file"
trap - EXIT

echo "Copied firmware to shared project: $target_file"
exec "$REMOTE_DOWNLOADER" "$serial_port" "$target_file"
'''


BUILD_PATCH_DOWNLOAD_WINDOWS = r'''#!/bin/bash
# Generated by generate_td1601_makefiles.py. Run in Git Bash.
set -euo pipefail

usage() {
    printf '%s\n' \
        'Usage: build_patch_download_windows.sh -p COMx -f firmware.bin [-d BROM_DL.exe]' \
        '' \
        '  -p  Windows serial port, for example COM3.' \
        '  -f  Flashable firmware generated by aft_build_windows.sh.' \
        '  -d  Downloader path. Defaults to tools/ISP/BROM_DL.exe in this SDK.' \
        '  -h  Show this help.'
}

SCRIPT_DIR="${BASH_SOURCE[0]%/*}"
[ "$SCRIPT_DIR" = "${BASH_SOURCE[0]}" ] && SCRIPT_DIR="."
HERE="$(cd "$SCRIPT_DIR" && pwd)"
REPO="$(cd "$HERE/@REPO_REL@" && pwd)"
PORT=""
FIRMWARE=""
DOWNLOADER="$REPO/tools/ISP/BROM_DL.exe"

while getopts ":p:f:d:h" option; do
    case "$option" in
        p) PORT="$OPTARG" ;;
        f) FIRMWARE="$OPTARG" ;;
        d) DOWNLOADER="$OPTARG" ;;
        h)
            usage
            exit 0
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            usage >&2
            exit 2
            ;;
        \?)
            echo "Unknown option: -$OPTARG" >&2
            usage >&2
            exit 2
            ;;
    esac
done

[ -n "$PORT" ] || { echo "Serial port is required." >&2; usage >&2; exit 2; }
[ -n "$FIRMWARE" ] || { echo "Firmware path is required." >&2; usage >&2; exit 2; }
[ -f "$FIRMWARE" ] || { echo "Firmware was not found: $FIRMWARE" >&2; exit 1; }
[ -f "$DOWNLOADER" ] || { echo "Downloader was not found: $DOWNLOADER" >&2; exit 1; }

FIRMWARE_PARENT="${FIRMWARE%/*}"
[ "$FIRMWARE_PARENT" = "$FIRMWARE" ] && FIRMWARE_PARENT="."
FIRMWARE_ABS="$(cd "$FIRMWARE_PARENT" && pwd)/${FIRMWARE##*/}"

DOWNLOADER_PARENT="${DOWNLOADER%/*}"
[ "$DOWNLOADER_PARENT" = "$DOWNLOADER" ] && DOWNLOADER_PARENT="."
DOWNLOADER_ABS="$(cd "$DOWNLOADER_PARENT" && pwd)/${DOWNLOADER##*/}"
TOOL_DIR="${DOWNLOADER_ABS%/*}"

WIN_DOWNLOADER="$(cygpath -aw "$DOWNLOADER_ABS")"
WIN_FIRMWARE="$(cygpath -aw "$FIRMWARE_ABS")"

echo "Downloader: $WIN_DOWNLOADER"
echo "Firmware: $WIN_FIRMWARE"

pushd "$TOOL_DIR" >/dev/null
PYTHONIOENCODING=utf-8 PYTHONUTF8=1 "$WIN_DOWNLOADER" "$PORT" "$WIN_FIRMWARE"
popd >/dev/null
'''


def makefile(project: Package, packages: list[Package], board: Package, sources: list[tuple[str, Path]], includes: list[Path], defines: dict, cflags: str, ldflags: str, libs: list[str], target_platform: str, workspace_rel: str) -> str:
    # Shadow the board metadata with a project-relative path so the generated
    # Makefile remains portable when the SDK workspace is moved.
    board = Package(board.name, Path(os.path.relpath(board.path, project.path)), board.data, board.kind)
    obj_entries: list[tuple[str, Path]] = []
    used: set[str] = set()
    for label, source in sources:
        # gcc_flash.ld matches *startup.o when placing Reset_Handler/vectors.
        # Using source.name produced startup_S.o and broke that convention.
        stem = re.sub(r"[^A-Za-z0-9_]", "_", f"{label}_{source.stem}")
        name = stem
        n = 2
        while name in used:
            name = f"{stem}_{n}"; n += 1
        used.add(name)
        source_rel = Path(os.path.relpath(source, project.path))
        obj_entries.append((f"$(OBJ)/{name}.o", source_rel))
    lines = ["# Generated by tools/generate_td1601_makefiles.py; do not edit.", ".DEFAULT_GOAL := all", f"PROJECT := {project.name}", "OBJ := Obj", "LST := Lst"]
    if target_platform == "windows":
        lines.append(f"TOOLCHAIN_DIR ?= $(abspath {workspace_rel}/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1)")
    else:
        lines.append("TOOLCHAIN_DIR ?= $(HOME)/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1")
    lines += ["CROSS := $(TOOLCHAIN_DIR)/bin/riscv-none-elf-", "CC := $(CROSS)gcc", "AR := $(CROSS)ar", "OBJCOPY := $(CROSS)objcopy", "OBJDUMP := $(CROSS)objdump", "SIZE := $(CROSS)size", "ARCH_FLAGS := -march=rv32imafdc_zicsr_xtheadcmo -mabi=ilp32d", "COMPAT := -include generated/td1601_xpack_compat.h", f"CFLAGS := $(ARCH_FLAGS) -Wall -std=gnu99 -ffunction-sections -fdata-sections -MMD -MP $(COMPAT) {cflags}", "ASFLAGS := $(ARCH_FLAGS) -x assembler-with-cpp -ffunction-sections -fdata-sections -MMD -MP $(COMPAT)"]
    lines.append("INCLUDES := " + " ".join(f"-I{Path(os.path.relpath(p, project.path)).as_posix()}" for p in includes))
    lines.append("DEFINES := " + " ".join(f"-D{k}" if str(v).lower() in ("y", "yes", "true", "") else f"-D{k}={v}" for k, v in defines.items()))
    lines += ["CFLAGS += $(INCLUDES) $(DEFINES)", "ASFLAGS += $(INCLUDES) $(DEFINES)", f"LDSCRIPT := {(board.path / 'gcc_flash.ld').as_posix()}", f"LDFLAGS := -nostartfiles --specs=nano.specs -Wl,--gc-sections -T$(LDSCRIPT) $(ARCH_FLAGS) -Wl,-zmax-page-size=1024 -Wl,-Map=$(LST)/$(PROJECT).map {ldflags}", "", "OBJS := \\"]
    lines += [f"  {obj} \\" if i + 1 < len(obj_entries) else f"  {obj}" for i, (obj, _) in enumerate(obj_entries)]
    lines += ["DEPS := $(OBJS:.o=.d)", "ELF := $(OBJ)/$(PROJECT).elf", "BIN := $(OBJ)/$(PROJECT).bin", "IHEX := $(OBJ)/$(PROJECT).ihex", ".PHONY: all clean size", "all: $(BIN) $(IHEX) size", ""]
    for obj, source in obj_entries:
        flag = "$(ASFLAGS)" if source.suffix.lower() in (".s", ".asm") or source.suffix == ".S" else "$(CFLAGS)"
        lines += [f"{obj}: {source.as_posix()}", "\t@mkdir -p $(dir $@)", f"\t$(CC) -c {flag} -MF $(@:.o=.d) -o $@ $<", ""]
    link_libs = " ".join(f"-l{x}" for x in libs)
    lines += ["$(ELF): $(OBJS)", "\t@mkdir -p $(LST)", f"\t$(CC) $(OBJS) -Wl,--start-group {link_libs} -Wl,--end-group $(LDFLAGS) -o $@", "\t$(OBJDUMP) -d $@ > $(LST)/$(PROJECT).asm", "$(BIN): $(ELF)", "\t$(OBJCOPY) -O binary $< $@", "$(IHEX): $(ELF)", "\t$(OBJCOPY) -O ihex $< $@", "size: $(ELF)", "\t$(SIZE) $(ELF)", "clean:", "\trm -rf $(OBJ) $(LST)", "-include $(DEPS)", ""]
    return "\n".join(lines)


def target_platform(value: str) -> str:
    if value != "auto":
        return value
    host = platform.system()
    if host == "Darwin":
        return "macos"
    if host == "Windows":
        return "windows"
    raise SystemExit(f"unsupported host platform: {host}; pass --platform macos or --platform windows")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate platform-specific Makefile, packaging, and ISP scripts from CDK metadata.")
    parser.add_argument("project", type=Path, help="solution directory containing package.yaml and project.cdkproj")
    parser.add_argument("--platform", choices=("auto", "macos", "windows"), default="auto", help="target script platform; default: detect the host system")
    parser.add_argument("--force", action="store_true", help="overwrite all generated build, packaging, and burn scripts")
    args = parser.parse_args()
    build_platform = target_platform(args.platform)
    root = args.project.resolve()
    repo = next((parent for parent in (root, *root.parents) if (parent / "components").is_dir() and (parent / "boards").is_dir()), None)
    if repo is None:
        raise SystemExit("cannot find SDK root (expected sibling components/ and boards/ directories)")
    proj_file = root / "project.cdkproj"
    packages: dict[str, Package] = {}
    project = add_package(packages, root, "solution")
    tree = ET.parse(proj_file)
    refs: list[tuple[str, str, str, str]] = []
    for tag, kind in (("Chip", "chip"), ("Board", "board"), ("Package", "package")):
        for node in tree.findall(f".//{tag}"):
            refs.append((node.attrib["ID"], node.attrib["Path"], node.attrib["Version"], kind))
    ref_bases: dict[str, Path] = {}
    board: Package | None = None
    for name, rel, version, kind in refs:
        base = (root / rel).resolve(); ref_bases[name] = base
        pkg = add_package(packages, version_dir(base, name, version), kind)
        if kind == "board": board = pkg
    if board is None:
        raise ValueError("project.cdkproj has no Board")
    # Resolve transitive dependencies through the nearest component repository.
    pending = list(packages.values())
    while pending:
        pkg = pending.pop()
        for dep_entry in pkg.data.get("depends", []) or []:
            dep = parse_dep(str(dep_entry))
            if dep in packages: continue
            base = ref_bases.get(dep, root.parents[3] / "components")
            dep_pkg = add_package(packages, version_dir(base, dep, str(dep_entry).split(":", 1)[-1]), "package")
            ref_bases[dep] = base; pending.append(dep_pkg)
    ordered = list(packages.values())
    sources: list[tuple[str, Path]] = []
    source_paths: set[Path] = set()
    includes: list[Path] = []
    defines: dict = {}
    libs: list[str] = []
    for pkg in ordered:
        config = pkg.data.get("build_config", {}) or {}
        for inc in (config.get("include", []) or []) + (config.get("internal_include", []) or []):
            candidate = (pkg.path / str(inc)).resolve()
            if candidate.is_dir() and candidate not in includes: includes.append(candidate)
        for pat in pkg.data.get("source_file", []) or []:
            for filename in sorted(glob.glob(str(pkg.path / str(pat)), recursive=True)):
                source = Path(filename).resolve()
                if source.suffix in (".c", ".S", ".s", ".cc", ".cpp") and source not in source_paths:
                    sources.append((pkg.name, source))
                    source_paths.add(source)
        if pkg is project:
            defines.update(pkg.data.get("def_config", {}) or {})
            libs += list(config.get("libs", []) or [])
    # Required by old CDK source trees that include headers by repository path.
    components = repo / "components"
    if components.exists(): includes.append(components.resolve())
    project_config = project.data.get("build_config", {}) or {}
    cflags = str(project_config.get("cflag", ""))
    ldflags = str(project_config.get("ldflag", ""))
    if build_platform == "macos":
        outputs = [
            root / "Makefile",
            root / "aft_build_macos.sh",
            root / "build_patch_download.sh",
            root / "utilities/macos-copy-and-download.sh",
        ]
    else:
        outputs = [
            root / "Makefile",
            root / "aft_build_windows.sh",
            root / "build_patch_download_windows.sh",
        ]
    existing = [p for p in outputs if p.exists()]
    if existing and not args.force:
        raise SystemExit("refusing to overwrite " + ", ".join(map(str, existing)) + "; pass --force")
    (root / "generated").mkdir(exist_ok=True)
    write_generated(root / "generated/td1601_xpack_compat.h", COMPAT_HEADER)
    board_version = board.path.name
    repo_rel = os.path.relpath(repo, root).replace("\\", "/")
    workspace_rel = os.path.relpath(repo.parent, root).replace("\\", "/")
    write_generated(outputs[0], makefile(project, ordered, board, sources, includes, defines, cflags, ldflags, libs, build_platform, workspace_rel))
    # project.cdkproj may spell this as V1.0.0 while the on-disk SDK uses v1.0.
    if build_platform == "macos":
        aft = AFTER_BUILD.replace("@PROJECT@", project.name).replace("@REPO_REL@", repo_rel).replace("@BOARD@", board.name).replace("@BOARD_VERSION@", board_version)
    else:
        aft = AFTER_BUILD_WINDOWS.replace("@PROJECT@", project.name).replace("@REPO_REL@", repo_rel).replace("@BOARD@", board.name).replace("@BOARD_VERSION@", board_version)
    write_generated(outputs[1], aft)
    outputs[1].chmod(outputs[1].stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    if build_platform == "macos":
        burn = BUILD_PATCH_DOWNLOAD.replace("@PROJECT@", project.name).replace("@REPO_REL@", repo_rel).replace("@BOARD@", board.name).replace("@BOARD_VERSION@", board_version)
    else:
        burn = BUILD_PATCH_DOWNLOAD_WINDOWS.replace("@REPO_REL@", repo_rel)
    write_generated(outputs[2], burn)
    outputs[2].chmod(outputs[2].stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    if build_platform == "macos":
        outputs[3].parent.mkdir(parents=True, exist_ok=True)
        write_generated(outputs[3], MACOS_COPY_AND_DOWNLOAD)
        outputs[3].chmod(outputs[3].stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    print(f"generated {outputs[0]} ({len(sources)} sources; platform={build_platform})")
    print(f"generated {outputs[1]}")
    print(f"generated {outputs[2]}")
    if build_platform == "macos":
        print(f"generated {outputs[3]}")


if __name__ == "__main__":
    main()

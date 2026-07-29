#!/bin/bash
# Build the flashable firmware image after `make` has generated the ELF.
set -euo pipefail

PROJECT="lierda"
SCRIPT_DIR="${BASH_SOURCE[0]%/*}"
[ "$SCRIPT_DIR" = "${BASH_SOURCE[0]}" ] && SCRIPT_DIR="."
HERE="$(cd "$SCRIPT_DIR" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
WORKSPACE="$(cd "$REPO/.." && pwd)"
BOARD="$REPO/boards/td1601_evb/v1.0"
OBJ="$HERE/Obj"
ELF="$OBJ/$PROJECT.elf"

# TOOLCHAIN_DIR may be set explicitly; otherwise use the workspace's opt tree.
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

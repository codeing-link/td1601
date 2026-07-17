#!/usr/bin/env bash
set -euo pipefail

case "$#" in
    0) SERIAL_PORT="/dev/ttyUSB0" ;;
    1) SERIAL_PORT="$1" ;;
    *)
        echo "Usage: $0 [/dev/ttyUSB0]" >&2
        exit 2
        ;;
esac

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ISP_TOOL="${ISP_TOOL:-$HERE/utilities/macos-copy-and-download.sh}"
VERSION="$(sed -n 's/^version:[[:space:]]*V//p' "$HERE/package.yaml" | head -n 1 | tr -d '\r')"
FIRMWARE="$HERE/Obj/lierda_0x18000000_V${VERSION}.bin"

echo "Download port: $SERIAL_PORT"

if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
    make -C "$HERE"
    bash "$HERE/aft_build_macos.sh"
fi

if [[ ! -f "$FIRMWARE" ]]; then
    echo "Firmware not found: $FIRMWARE" >&2
    exit 1
fi

exec "$ISP_TOOL" "$SERIAL_PORT" "$FIRMWARE"

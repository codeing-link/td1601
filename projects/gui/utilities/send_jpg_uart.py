#!/usr/bin/env python3
"""
通过 UART 发送 JPG 到板子。

协议字段全部使用小端格式：
  START -> DATA... -> END
  MCU 每包返回 ACK/NACK。
"""

import argparse
import os
import struct
import sys
import time
import zlib

try:
    import serial
except ImportError:
    serial = None


MAGIC = b"JPGU"
VERSION = 1

CMD_START = 0x01
CMD_DATA = 0x02
CMD_END = 0x03
CMD_FORMAT = 0x04
CMD_ACK = 0x80
CMD_NACK = 0x81

ERR_NAMES = {
    1: "CRC",
    2: "SEQ",
    3: "OFFSET",
    4: "FS_WRITE",
    5: "FILE_CRC",
    6: "PARAM",
    7: "TIMEOUT",
    8: "STORAGE_FULL",
}

RETRYABLE_ERRORS = {1, 7}  # CRC/timeout 可重发当前包；其他错误通常需要重新开始传输或修复板端状态。


class PacketError(RuntimeError):
    def __init__(self, packet_name: str, seq: int, offset: int, payload_len: int, err: int):
        self.packet_name = packet_name
        self.seq = seq
        self.offset = offset
        self.payload_len = payload_len
        self.err = err
        name = ERR_NAMES.get(err, f"ERR_{err}")
        super().__init__(f"packet {packet_name} seq={seq} offset={offset} len={payload_len} failed: {name}")


class StorageFullError(PacketError):
    pass


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def add_crc16(body: bytes) -> bytes:
    return body + struct.pack("<H", crc16_ccitt(body))


def build_start(file_id: int, payload: bytes, chunk_size: int, filename: str) -> bytes:
    name = os.path.basename(filename).encode("utf-8")
    if len(name) > 64:
        name = name[:64]
    file_crc = zlib.crc32(payload) & 0xFFFFFFFF
    body = (
        MAGIC
        + struct.pack("<BBHIIHB", VERSION, CMD_START, file_id, len(payload), file_crc, chunk_size, len(name))
        + name
    )
    return add_crc16(body)


def build_data(file_id: int, seq: int, offset: int, chunk: bytes) -> bytes:
    body = MAGIC + struct.pack("<BBHIIH", VERSION, CMD_DATA, file_id, seq, offset, len(chunk)) + chunk
    return add_crc16(body)


def build_end(file_id: int, total_chunks: int, payload: bytes) -> bytes:
    file_crc = zlib.crc32(payload) & 0xFFFFFFFF
    body = MAGIC + struct.pack("<BBHII", VERSION, CMD_END, file_id, total_chunks, file_crc)
    return add_crc16(body)


def build_format(file_id: int) -> bytes:
    body = MAGIC + struct.pack("<BBH", VERSION, CMD_FORMAT, file_id)
    return add_crc16(body)


def read_exact(port, size: int, timeout_s: float) -> bytes:
    deadline = time.monotonic() + timeout_s
    data = bytearray()
    while len(data) < size:
        remain = deadline - time.monotonic()
        if remain <= 0:
            break
        port.timeout = min(0.05, remain)
        part = port.read(size - len(data))
        if part:
            data.extend(part)
    return bytes(data)


def wait_ack(port, file_id: int, seq: int, timeout_s: float) -> tuple[bool, int]:
    deadline = time.monotonic() + timeout_s
    window = bytearray()

    while time.monotonic() < deadline:
        b = read_exact(port, 1, max(0.01, deadline - time.monotonic()))
        if not b:
            continue
        window.extend(b)
        if len(window) > 13:
            del window[0 : len(window) - 13]

        if len(window) >= 4 and bytes(window[-4:]) == MAGIC:
            window = bytearray(MAGIC)

        if len(window) == 13 and bytes(window[0:4]) == MAGIC:
            version, cmd, ack_file_id, ack_seq, status = struct.unpack("<BBHIB", window[4:13])
            if version != VERSION or ack_file_id != file_id or ack_seq != seq:
                window = bytearray()
                continue
            if cmd == CMD_ACK and status == 0:
                return True, 0
            if cmd == CMD_NACK:
                return False, status
            window = bytearray()

    return False, 7


def send_with_retry(
    port,
    packet: bytes,
    file_id: int,
    seq: int,
    timeout: float,
    retry: int,
    *,
    packet_name: str,
    offset: int = 0,
    payload_len: int = 0,
    verbose: bool = False,
) -> int:
    for attempt in range(retry + 1):
        port.write(packet)
        port.flush()
        ok, err = wait_ack(port, file_id, seq, timeout)
        if ok:
            if verbose:
                print(f"ACK {packet_name} seq={seq} offset={offset} len={payload_len} attempt={attempt + 1}")
            return attempt
        name = ERR_NAMES.get(err, f"ERR_{err}")
        print(
            f"NACK/TIMEOUT {packet_name} seq={seq} offset={offset} len={payload_len} "
            f"attempt={attempt + 1}/{retry + 1} err={name}"
        )
        if err not in RETRYABLE_ERRORS:
            if err == 8:
                raise StorageFullError(packet_name, seq, offset, payload_len, err)
            raise PacketError(packet_name, seq, offset, payload_len, err)
        if attempt >= retry:
            raise PacketError(packet_name, seq, offset, payload_len, err)
    return retry


def should_format_on_full(args) -> bool:
    if args.format_on_full:
        return True
    if args.no_format_on_full:
        return False

    answer = input("MCU reports storage full. Format LittleFS and erase old images? [y/N]: ").strip().lower()
    return answer in ("y", "yes")


def main() -> int:
    parser = argparse.ArgumentParser(description="Send JPG to MCU over UART")
    parser.add_argument("--port", required=True, help="COM3 or /dev/tty.usbserial-310")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--file", required=True)
    parser.add_argument("--chunk", type=int, default=240)
    parser.add_argument("--timeout", type=float, default=2.0)
    parser.add_argument("--retry", type=int, default=5)
    parser.add_argument("--file-id", type=int, default=1)
    parser.add_argument("--inter-packet-delay", type=float, default=0.0)
    parser.add_argument("--verbose", action="store_true", help="print every ACK and retry detail")
    parser.add_argument("--format-on-full", action="store_true", help="format LittleFS automatically when MCU reports storage full")
    parser.add_argument("--no-format-on-full", action="store_true", help="stop immediately when MCU reports storage full")
    args = parser.parse_args()

    if serial is None:
        print("pyserial is required: python3 -m pip install pyserial", file=sys.stderr)
        return 2

    if args.chunk <= 0 or args.chunk > 512:
        print("--chunk must be 1..512", file=sys.stderr)
        return 2

    if args.format_on_full and args.no_format_on_full:
        print("--format-on-full and --no-format-on-full cannot be used together", file=sys.stderr)
        return 2

    with open(args.file, "rb") as f:
        payload = f.read()

    if not payload:
        print("empty file", file=sys.stderr)
        return 2

    file_id = args.file_id & 0xFFFF
    total_chunks = (len(payload) + args.chunk - 1) // args.chunk
    file_crc = zlib.crc32(payload) & 0xFFFFFFFF

    print(f"file={args.file} size={len(payload)} crc32=0x{file_crc:08x} chunks={total_chunks} chunk={args.chunk}")

    retries = 0
    start_time = time.monotonic()

    with serial.Serial(args.port, args.baud, timeout=0.05, write_timeout=args.timeout) as port:
        port.reset_input_buffer()
        port.reset_output_buffer()

        while True:
            try:
                retries += send_with_retry(
                    port,
                    build_start(file_id, payload, args.chunk, args.file),
                    file_id,
                    0,
                    args.timeout,
                    args.retry,
                    packet_name="START",
                    payload_len=len(payload),
                    verbose=args.verbose,
                )
                break
            except StorageFullError:
                if not should_format_on_full(args):
                    raise RuntimeError("MCU storage is full, transfer stopped by user")

                print("formatting MCU LittleFS...")
                retries += send_with_retry(
                    port,
                    build_format(file_id),
                    file_id,
                    0,
                    args.timeout,
                    args.retry,
                    packet_name="FORMAT",
                    verbose=args.verbose,
                )
                print("format done, restart transfer")

        offset = 0
        for seq in range(total_chunks):
            chunk = payload[offset : offset + args.chunk]
            retries += send_with_retry(
                port,
                build_data(file_id, seq, offset, chunk),
                file_id,
                seq,
                args.timeout,
                args.retry,
                packet_name="DATA",
                offset=offset,
                payload_len=len(chunk),
                verbose=args.verbose,
            )
            offset += len(chunk)
            if args.inter_packet_delay > 0:
                time.sleep(args.inter_packet_delay)
            if (seq + 1) % 20 == 0 or (seq + 1) == total_chunks:
                print(f"sent {seq + 1}/{total_chunks}")

        retries += send_with_retry(
            port,
            build_end(file_id, total_chunks, payload),
            file_id,
            total_chunks,
            args.timeout,
            args.retry,
            packet_name="END",
            payload_len=len(payload),
            verbose=args.verbose,
        )

    elapsed = time.monotonic() - start_time
    speed = len(payload) / elapsed if elapsed > 0 else 0
    print(f"done elapsed={elapsed:.2f}s speed={speed:.1f} B/s retries={retries}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

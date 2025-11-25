#!/usr/bin/env python3
import sys
import re

BYTES_PER_LINE = 16
START_OFFSET   = 0x0100  # DEBUG のロードアドレス

def print_crlf(s=""):
    """CR+LF で行を出力する"""
    sys.stdout.write(s + "\r\n")

def load_hex_bytes(path: str):
    with open(path, "r") as f:
        text = f.read()

    hex_only = re.sub(r"[^0-9a-fA-F]", "", text)

    if len(hex_only) == 0:
        raise ValueError("HEX データが空です。")

    if len(hex_only) % 2 != 0:
        raise ValueError("HEX データの桁数が奇数です（壊れている可能性）。")

    data = []
    for i in range(0, len(hex_only), 2):
        data.append(int(hex_only[i:i+2], 16))

    return data

def main():
    if len(sys.argv) != 3:
        sys.stderr.write("Usage: python3 make_debug_script.py input.hex OUTPUT.BIN\n")
        sys.exit(1)

    hex_path = sys.argv[1]
    out_name = sys.argv[2]

    data = load_hex_bytes(hex_path)
    size = len(data)

    if size > 0xFF00:
        sys.stderr.write(
            f"警告: バイナリサイズ {size} bytes は DEBUG の一度の書き込み上限付近です。\n"
        )

    # === DEBUG スクリプト出力 ===

    print_crlf(f"n {out_name}")

    offset = START_OFFSET
    idx = 0

    while idx < size:
        chunk = data[idx : idx + BYTES_PER_LINE]
        bytes_str = " ".join(f"{b:02X}" for b in chunk)
        print_crlf(f"e {offset:04X} {bytes_str}")
        offset += len(chunk)
        idx += len(chunk)

    print_crlf("rcx")
    print_crlf(f"{size:04X}")
    print_crlf("w")
    print_crlf("q")

if __name__ == "__main__":
    main()

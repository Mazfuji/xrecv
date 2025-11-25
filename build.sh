#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: ./build.sh [options]
  --debug-script       Also generate xrecv.hex and xrecv_dbg.txt
  -o, --output FILE    Output exe name (default: xrecv.exe)
  --debug-name NAME    File name used inside the DEBUG script (default: XRECV.EXE)
  -h, --help           Show this help

Environment:
  MINGW (optional)     Compiler command (default: i686-w64-mingw32-gcc)
EOF
}

want_debug=0
out_exe="xrecv.exe"
debug_name="XRECV.EXE"
compiler="${MINGW:-i686-w64-mingw32-gcc}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --debug-script)
      want_debug=1
      shift
      ;;
    -o|--output)
      out_exe="$2"
      shift 2
      ;;
    --debug-name)
      debug_name="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

echo "Compiling $out_exe with $compiler ..."
"$compiler" -Os -s -o "$out_exe" xrecv.c
echo "Done: $out_exe"

if [[ $want_debug -eq 1 ]]; then
  hex_file="${out_exe%.*}.hex"
  dbg_file="${out_exe%.*}_dbg.txt"
  echo "Generating DEBUG script artifacts ..."
  xxd -p "$out_exe" > "$hex_file"
  python3 make_debug_script.py "$hex_file" "$debug_name" > "$dbg_file"
  echo "Done: $hex_file, $dbg_file"
  echo "Use 'debug < $dbg_file' on Windows 98 to reconstruct $debug_name"
fi

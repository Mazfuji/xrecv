# xrecv.exe README

`xrecv.exe` is a minimal **XMODEM Checksum receiver** for Windows 98, designed specifically for recovery scenarios where the system can only operate in **Safe Mode** and lacks access to normal file transfer methods (no USB, no LAN, no CD/DVD, no floppy). It enables transferring binary files (DLL/EXE) over a serial port using a second machine (e.g., Ubuntu/Linux).

This document describes:

* What `xrecv.exe` does
* How to compile `xrecv.c` using mingw-w64 on Linux
* How to deliver the EXE to Windows 98 using a DEBUG script
* How to perform XMODEM transfers from Linux to Win98

---

## 1. Overview

`xrecv.exe` provides a lightweight implementation of **XMODEM (128-byte, checksum mode)** and writes the received binary data to a file.

### Supported protocol features

* **SOH (0x01)** for block start (128-byte)
* **EOT (0x04)** for transfer end
* **ACK (0x06)** on valid block
* **NAK (0x15)** on checksum or header error
* Block number and inverse (0xFF - blk)
* Simple checksum

### Serial settings (fixed)

* **38400 baud**
* **8 data bits, no parity, 1 stop bit (8N1)**
* **Flow Control Off (no RTS/CTS, no XON/XOFF)**

### Target use case

This tool is intended for scenarios such as:

* Windows 98 system boots only in Safe Mode
* Network stack is broken (e.g., missing `msnp32.dll`)
* No working floppy, CD-R, USB, PCMCIA
* Only COM1/COM2 works

In such cases, `xrecv.exe` allows recovery of critical system files.

---

## 2. Usage (on Windows 98)

Command format:

```bat
xrecv COM1 output.bin
xrecv COM2 msnp32.dll
```

* **Argument 1:** COM port name (`COM1`–`COM4`)
* **Argument 2:** Output file name (binary)

Example:

```bat
xrecv COM1 msnp32.dll
```

This puts `xrecv.exe` into **waiting mode**, where it repeatedly sends `NAK` until the sender begins transmitting.

---

## 3. Compiling on Ubuntu/Linux

### 3.1 Install mingw-w64

```bash
sudo apt update
sudo apt install mingw-w64
```

### 3.2 Compile

```bash
i686-w64-mingw32-gcc -Os -s -o xrecv.exe xrecv.c
```

* `-Os`: Optimize for size
* `-s` : Strip symbols to minimize EXE size

### 3.3 Verify

```bash
ls -l xrecv.exe
```

The resulting binary is safe to reconstruct via Windows 98 `debug.exe`.

### 3.4 Compile via helper script

You can also use the bundled helper:

```bash
./build.sh               # builds xrecv.exe
./build.sh --debug-script    # builds xrecv.exe + xrecv_dbg.txt
```

* `--debug-script` additionally produces `xrecv.hex` and a DEBUG script (`xrecv_dbg.txt`) using `make_debug_script.py`. The script defaults to emitting `XRECV.EXE` inside the DEBUG recipe.

---

## 4. Delivering xrecv.exe to Windows 98 Using DEBUG

If you cannot copy the file normally, you can re-create the binary using a DEBUG script.

1. Convert binary to hex text:

   ```bash
   xxd -p xrecv.exe > xrecv.hex
   ```

2. Generate a DEBUG script (CR+LF required):

   ```bash
   python3 make_debug_script.py xrecv.hex XRECV.EXE > xrecv_dbg.txt
   ```

3. Transfer `xrecv_dbg.txt` to Windows 98 via plain ASCII serial transfer (e.g., using `COPY COM1:`).

4. On Windows 98:

   ```bat
   debug < xrecv_dbg.txt
   ```

This reconstructs `XRECV.EXE` in the current directory.

### 4.1 About `make_debug_script.py`

`make_debug_script.py` converts a plain hex dump (e.g., `xxd -p` output) into a DEBUG script that:

* writes the bytes starting at `0100h` (DEBUG load address)
* sets `CX` to the binary size
* saves the file with the name you pass as the second argument
* emits all lines with CR+LF endings (as required by DEBUG)

Usage:

```bash
python3 make_debug_script.py INPUT.hex OUTPUT.EXE > script.txt
```

Use the generated `script.txt` with `debug < script.txt` on Windows 98 to re-create the executable.

---

## 5. Sending Files from Ubuntu using XMODEM

### 5.1 Configure the serial port

```bash
sudo stty -F /dev/ttyUSB0 38400 cs8 -parenb -cstopb -ixon -ixoff -crtscts
```

### 5.2 Start minicom

```bash
sudo minicom -s
```

Set:

* Serial port: `/dev/ttyUSB0`
* 38400 8N1
* Hardware Flow Control: **No**
* Software Flow Control: **No**

### 5.3 Run xrecv on Windows 98

```bat
xrecv COM1 msnp32.dll
```

### 5.4 Send file from Ubuntu (inside minicom)

1. Press **Ctrl+A → S** (Send)
2. Choose **xmodem** (checksum mode)
3. Select `msnp32.dll`
4. Press **Enter**

When the transfer completes, Win98 will show:

```
Completed.
```

A new file `msnp32.dll` will appear in the current directory.

---

## 6. Verification

### 6.1 File size check

Ubuntu:

```bash
ls -l msnp32.dll
```

Windows 98:

```bat
dir msnp32.dll
```

Sizes must match.

### 6.2 Optional: double-transfer comparison

```bat
xrecv COM1 msnp32a.dll
xrecv COM1 msnp32b.dll
fc /b msnp32a.dll msnp32b.dll
```

If `FC /B` reports no differences, your transfer is fully reliable.

### 6.3 Optional: use a lower baud rate

If errors occur, use **9600 baud** on both sides.

---

## 7. Limitations

`xrecv.exe` is intentionally minimal. It lacks:

* CRC XMODEM
* XMODEM-1k support
* CAN-based session abort
* Advanced block number handling (only basic validation)

However, for short-range cable transfers in a recovery scenario, it is sufficiently robust.

---

## 8. Typical Full Recovery Workflow

1. Build `xrecv.exe` on Ubuntu
2. Convert to HEX
3. Generate DEBUG script (CR+LF)
4. Send script to Win98
5. Rebuild `XRECV.EXE` using DEBUG
6. Run `xrecv COM1 target.dll`
7. Send DLL via XMODEM from Ubuntu
8. Place DLL into the correct Win98 system directory and reboot

This restores broken components (like `msnp32.dll`) while avoiding all non-working devices.

---

## 9. License

This project is released under the MIT License (see `MIT LICENSE.txt`).

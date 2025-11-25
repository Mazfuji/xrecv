/* xrecv.c - Minimal XMODEM receiver for Windows 9x/NT
 *
 * Usage:
 *   xrecv COM1 output.bin
 *   xrecv COM2 recv.dat
 *
 * COM1～COM4 のいずれかを argv[1] に指定する。
 *
 */

#include <windows.h>
#include <stdio.h>

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

static HANDLE open_serial(const char *portname) {
    HANDLE h;
    DCB dcb;
    COMMTIMEOUTS to;

    h = CreateFileA(portname,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);
    if (h == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Cannot open %s\n", portname);
        return INVALID_HANDLE_VALUE;
    }

    SetupComm(h, 2048, 2048);

    GetCommState(h, &dcb);
    dcb.BaudRate = CBR_38400;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fParity  = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_ENABLE;
    dcb.fRtsControl  = RTS_CONTROL_ENABLE;
    SetCommState(h, &dcb);

    to.ReadIntervalTimeout         = 1000;
    to.ReadTotalTimeoutMultiplier  = 0;
    to.ReadTotalTimeoutConstant    = 1000;
    to.WriteTotalTimeoutMultiplier = 0;
    to.WriteTotalTimeoutConstant   = 1000;
    SetCommTimeouts(h, &to);

    return h;
}

static int read_byte(HANDLE h, unsigned char *b) {
    DWORD n = 0;
    if (!ReadFile(h, b, 1, &n, NULL)) return 0;
    return (n == 1);
}

static int write_byte(HANDLE h, unsigned char b) {
    DWORD n = 0;
    if (!WriteFile(h, &b, 1, &n, NULL)) return 0;
    return (n == 1);
}

int main(int argc, char *argv[]) {
    HANDLE hCom, hOut;
    unsigned char block[128];
    unsigned char hdr[2];
    unsigned char c;
    int expecting = 1;
    int retry = 0;
    DWORD wrote;
    int i;
    unsigned char sum;

    if (argc < 3) {
        fprintf(stderr,
            "Usage: xrecv COM1 output.bin\n"
            "       xrecv COM2 recv.dat\n");
        return 1;
    }

    /* argv[1] = COMポート ("COM1" など) */
    hCom = open_serial(argv[1]);
    if (hCom == INVALID_HANDLE_VALUE) return 1;

    /* argv[2] = 出力ファイル */
    hOut = CreateFileA(argv[2],
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       0,
                       NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Cannot create %s\n", argv[2]);
        CloseHandle(hCom);
        return 1;
    }

    /* kick starter: send NAK until SOH/EOT arrives */
    for (;;) {
        write_byte(hCom, NAK);
        if (!read_byte(hCom, &c)) {
            if (++retry > 20) {
                fprintf(stderr, "No response from sender.\n");
                goto done;
            }
            continue;
        }
        if (c == SOH || c == EOT) break;
    }

    while (1) {
        if (c == EOT) {
            write_byte(hCom, ACK);
            printf("Completed.\n");
            break;
        }

        /* Already consumed SOH */
        if (!read_byte(hCom, &hdr[0]) ||
            !read_byte(hCom, &hdr[1])) {
            fprintf(stderr, "Header read error.\n");
            goto done;
        }

        /* 128 bytes data + checksum */
        for (i = 0; i < 128; i++) {
            if (!read_byte(hCom, &block[i])) {
                fprintf(stderr, "Data read error.\n");
                goto done;
            }
        }
        if (!read_byte(hCom, &c)) {
            fprintf(stderr, "Checksum read error.\n");
            goto done;
        }

        /* Validate block number and checksum */
        if ((unsigned char)(hdr[0] + hdr[1]) != 0xFF) {
            write_byte(hCom, NAK);
            fprintf(stderr, "Block number mismatch.\n");
        } else {
            sum = 0;
            for (i = 0; i < 128; i++) sum += block[i];
            if (sum != c) {
                write_byte(hCom, NAK);
                fprintf(stderr, "Checksum error.\n");
            } else {
                WriteFile(hOut, block, 128, &wrote, NULL);
                write_byte(hCom, ACK);
                expecting++;
            }
        }

        /* Wait next SOH/EOT */
        if (!read_byte(hCom, &c)) {
            fprintf(stderr, "Timeout waiting next block.\n");
            goto done;
        }
        if (c != SOH && c != EOT) {
            /* Ignore unexpected bytes */
            continue;
        }
    }

done:
    CloseHandle(hOut);
    CloseHandle(hCom);
    return 0;
}

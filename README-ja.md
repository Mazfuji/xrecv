# xrecv.exe README

Windows 98 (特に Safe Mode 環境) で、シリアルポート経由の **XMODEM チェックサムモード**を用いてファイルを受信するための最小限ユーティリティです。

この README は、

* `xrecv.c` の概要
* Ubuntu 上でのコンパイル方法
* Windows 98 への配置方法（debug スクリプト経由を前提）
* 実際のファイル受信手順（Ubuntu → Win98）

をまとめたものです。

---

## 1. 機能概要

`xrecv.exe` は以下の動作を行います。

* Windows のシリアルポート (`COM1`〜`COM4`) をオープン
* ボーレート **38400 / 8N1 / FlowControl 無し** に設定
* **XMODEM (128 バイト・チェックサム版)** でファイルを受信
* 受信データを指定されたファイルにそのままバイナリで書き込む

### 対応しているプロトコル仕様

* ブロックサイズ：128 バイト
* チェック方式：単純チェックサム（CRC ではない）
* 制御コード：

  * `SOH (0x01)` ブロック開始
  * `EOT (0x04)` 転送終了
  * `ACK (0x06)` 正常受信応答
  * `NAK (0x15)` 再送要求
* ブロック番号 + 反転値 (0xFF) の整合性チェックあり

### 想定用途

* 壊れかけの Windows 98 環境（Safe Mode のみ起動、ネットワーク不可など）に対し、
  COM ポート＋別マシン (Ubuntu など) を使って DLL や EXE を救出転送するレスキュー用途。

---

## 2. 使い方（Windows 98 側）

コマンドライン形式：

```bat
xrecv COM1 output.bin
xrecv COM2 msnp32.dll
```

* 第 1 引数: 使用する COM ポート名 (`COM1`〜`COM4`)
* 第 2 引数: 受信した内容を書き出すファイル名

例：

```bat
xrecv COM1 msnp32.dll
```

この状態で xrecv は送信開始を待機し、定期的に `NAK` を送信します。

別マシン側から XMODEM チェックサムモードで `msnp32.dll` を送信すると、
受信完了後、カレントディレクトリに `msnp32.dll` が生成されます。

---

## 3. Ubuntu 上でのコンパイル方法

ここでは Ubuntu (または他の Linux) 上で **mingw-w64** を用いて
Windows 98 用の 32bit コンソール EXE をビルドする手順を示します。

### 3.1 依存パッケージのインストール

```bash
sudo apt update
sudo apt install mingw-w64
```

### 3.2 ビルドコマンド

`xrecv.c` がカレントディレクトリにある前提で：

```bash
i686-w64-mingw32-gcc -Os -s -o xrecv.exe xrecv.c
```

* `-Os` : サイズ最適化
* `-s`  : シンボルストリップ（EXE を小さくする）
* 出力ファイル: `xrecv.exe`

### 3.3 生成物の確認

```bash
ls -l xrecv.exe
```

サイズが数十 KB 程度のコンソール EXE が生成されます。
Windows 98 上の `debug.exe` で復元しやすいよう、
できるだけ小さいサイズになるように設計されています。

### 3.4 付属ビルドスクリプトを使う

手軽にビルドしたい場合は付属のスクリプトも利用できます。

```bash
./build.sh                       # xrecv.exe をビルド
./build.sh --debug-script        # ビルド＋xrecv_dbg.txt も生成
```

* `--debug-script` を付けると、`xrecv.hex` と DEBUG 用スクリプト `xrecv_dbg.txt` を `make_debug_script.py` で自動生成します（スクリプト内の出力ファイル名はデフォルトで `XRECV.EXE`）。

---

## 4. debug スクリプト経由で Win98 に配置する（概要）

Windows 98 側に直接 EXE をコピーできない場合、
以下の方法で `xrecv.exe` をシリアル経由で送り込むことができます。

1. Ubuntu で `xrecv.exe` を 16 進テキスト化：

   ```bash
   xxd -p xrecv.exe > xrecv.hex
   ```

2. `xrecv.hex` から **MS-DOS DEBUG 用スクリプト**を生成：

   ```bash
   python3 make_debug_script.py xrecv.hex XRECV.EXE > xrecv_dbg.txt
   ```

   * `make_debug_script.py` は、

     * 入力: `xxd -p` 形式の 16 進テキスト
     * 出力: `debug < xrecv_dbg.txt` で実行可能なスクリプト
       を生成するユーティリティです。

3. `xrecv_dbg.txt` をシリアル経由で Windows 98 に転送
   （例：Ubuntu 側で `/dev/ttyUSB0` に対して ASCII 送信、Win98 側で `COPY COM1: XRECV_DBG.TXT`）。

4. Win98 側で `xrecv_dbg.txt` を `debug` に食わせて EXE を再構成：

   ```bat
   debug < XRECV_DBG.TXT
   ```

   正常終了すれば、カレントディレクトリに `XRECV.EXE`（実体は xrecv.exe）が生成されます。

> ⚠ 注意: debug 用スクリプトは **CR+LF (DOS 形式の改行)** である必要があります。
> `make_debug_script.py` は CR+LF を出力するように実装しておくと安全です。

### 4.1 `make_debug_script.py` について

`make_debug_script.py` は、`xxd -p` などで得たプレーンな 16 進テキストを、DEBUG が読み込めるスクリプトに変換します。特徴は以下の通りです。

* 0100h（DEBUG のロードアドレス）から順にバイトを書き込む `e` 行を生成
* `CX` にバイナリサイズを設定
* 第二引数で指定したファイル名で `w` 保存
* すべて CR+LF で出力

使い方:

```bash
python3 make_debug_script.py INPUT.hex OUTPUT.EXE > script.txt
```

Windows 98 側では `debug < script.txt` として実行すると `OUTPUT.EXE` が再構成されます。

---

## 5. Ubuntu 側での XMODEM 送信手順（minicom 使用）

Win98 側で `xrecv` が待機している状態で、
Ubuntu 側から XMODEM でファイルを送信する典型例です。

### 5.1 Ubuntu でシリアルポート設定

```bash
sudo stty -F /dev/ttyUSB0 38400 cs8 -parenb -cstopb -ixon -ixoff -crtscts
```

### 5.2 minicom の起動と設定

```bash
sudo minicom -s
```

* Serial port setup:

  * A: Serial Device → `/dev/ttyUSB0`
  * E: Bps/Par/Bits → `38400 8N1`
  * F: Hardware Flow Control → `No`
  * G: Software Flow Control → `No`

設定後、`Save setup as dfl` で保存しておくと便利です。

### 5.3 Win98 側で xrecv を起動

```bat
xrecv COM1 msnp32.dll
```

### 5.4 Ubuntu 側で XMODEM 送信

minicom の画面で：

1. `Ctrl + A` → `S`（Send）
2. プロトコルとして `xmodem` を選択

   * **xmodem-1k / xmodem-crc ではなく、チェックサム版 xmodem を選ぶこと**
3. 送信ファイルとして `msnp32.dll` を選択
4. Enter で送信開始

転送が完了すると、Win98 側の `xrecv` が `Completed.` と表示し、
カレントディレクトリに `msnp32.dll` が作成されます。

---

## 6. 受信結果の確認と運用上の注意

1. **ファイルサイズの確認**

   Ubuntu 側で元ファイルのサイズを控えておきます：

   ```bash
   ls -l msnp32.dll
   ```

   Win98 側で：

   ```bat
   dir msnp32.dll
   ```

   サイズが一致していることを確認します。

2. **必要に応じて二重受信＋バイナリ比較**

   信頼性をさらに上げるには、2回受信して比較します。

   ```bat
   xrecv COM1 msnp32a.dll
   xrecv COM1 msnp32b.dll
   fc /b msnp32a.dll msnp32b.dll
   ```

   `FC /B` で差分なしなら、実質的に転送は成功と言ってよいレベルです。

3. **ボーレートを下げる選択肢**

   環境によっては 38400bps より 9600bps の方が安定する場合があります。
   xrecv.c / minicom の両方の設定を揃えてボーレートを変更してください。

---

## 7. 制限事項・既知の簡略化

`xrecv.exe` はレスキュー用途を想定した **最小限の XMODEM 実装**であり、
一般的な完全実装に比べて以下のような簡略・制限があります。

* 128 バイトブロック＋チェックサム方式のみ対応（CRC / 1K ブロック非対応）
* ブロック番号のロールバックや二重受信に対する高度な処理は未実装
* CAN (0x18) によるセッション中断処理などは簡略化

とはいえ、

* 室内での短いクロスケーブル
* ボーレートを適切に設定
* 数十 KB 程度の DLL を 1〜2 回転送

といった状況では、実用上十分な信頼性が期待できます。

---

## 8. 典型的なレスキューフロー例

1. Ubuntu で `xrecv.exe` をビルド
2. `xxd -p` ＋ `make_debug_script.py` で DEBUG スクリプト生成
3. スクリプトをシリアル経由で Win98 に転送
4. Win98 の `debug` で `XRECV.EXE` を再構築
5. `xrecv COM1 msnp32.dll` を実行
6. Ubuntu から minicom の XMODEM 送信で `msnp32.dll` を送信
7. 受信完了後、`msnp32.dll` を `C:\WINDOWS\SYSTEM\` にコピーして再起動

この流れで、物理メディアやネットワークが使えない Windows 98 マシンでも、
COM ポートだけを頼りに DLL / EXE を救出転送することができます。

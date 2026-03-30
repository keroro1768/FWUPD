# Virtual HID over I2C Device

**專案狀態：** Phase 1 完成 ✅
**作者：** Keroro 小隊研究組
**日期：** 2026-03-30

---

## 專案目標

實作一個可在 Linux 上掛載的虛擬 HID over I2C 裝置，用於測試 FWUPD Plugin 的枚舉和 HID 通信功能。

---

## 目錄結構

```
virtual-device/
├── vhidi2c-daemon/           # Userspace daemon (Phase 1)
│   ├── vhidi2c-daemon.c      # 主程式
│   ├── hid-i2c-protocol.c    # HID over I2C 協定實作
│   ├── hid-i2c-protocol.h    # 協定頭文件
│   ├── Makefile              # 建置配置
│   └── README.md             # 詳細說明文件
│
├── i2c-stub-config/         # i2c-stub 設定
│   └── setup.sh              # 自動設定腳本
│
├── tests/                    # 測試腳本
│   ├── test-plugin.sh        # FWUPD Plugin 測試腳本
│   └── expected-output.txt   # 預期輸出
│
└── README.md                 # 本文件
```

---

## 快速開始

### 1. 建置 Daemon

```bash
cd vhidi2c-daemon
make
```

需要先安裝依賴：
```bash
# Debian/Ubuntu
sudo apt-get install build-essential libi2c-dev i2c-tools

# Fedora/RHEL
sudo dnf install gcc make i2c-tools-devel

# Arch Linux
sudo pacman -S base-devel i2c-tools
```

### 2. 設定 i2c-stub

```bash
cd ../i2c-stub-config
sudo ./setup.sh
```

或手動：
```bash
sudo modprobe i2c-dev
sudo modprobe i2c-stub chip_addr=0x2A
```

### 3. 執行 Daemon

```bash
sudo ../vhidi2c-daemon/vhidi2c-daemon
```

### 4. 執行測試

```bash
cd ../tests
sudo ./test-plugin.sh
```

---

## HID over I2C 協定實作

### HID Descriptor（30 bytes）

| 位元組 | 欄位 | 說明 |
|--------|------|------|
| 0-1 | wHIDDescLength | 描述符長度（30） |
| 2-3 | bcdVersion | HID 版本（1.11 = 0x0101） |
| 4-5 | wReportDescLength | 報告描述符長度 |
| 6-7 | wReportDescRegister | 報告描述符寄存器地址 |
| 8-9 | wInputRegister | 輸入寄存器地址 |
| 10-11 | wMaxInputLength | 最大輸入長度 |
| 12-13 | wOutputRegister | 輸出寄存器地址 |
| 14-15 | wMaxOutputLength | 最大輸出長度 |
| 16-17 | wCommandRegister | 命令寄存器地址 |
| 18-19 | wDataRegister | 數據寄存器地址 |
| 20-21 | wVendorID | 廠商 ID（0x1234） |
| 22-23 | wProductID | 產品 ID（0x5678） |
| 24-25 | wVersionID | 版本 ID（0x0001） |
| 26-29 | reserved | 保留（必須為 0） |

### 命令操作碼

| Opcode | 名稱 | 說明 |
|--------|------|------|
| 0x01 | RESET | 重置裝置 |
| 0x02 | GET_REPORT | 主機讀取報告 |
| 0x03 | SET_REPORT | 主機寫入報告 |
| 0x04 | GET_IDLE | 讀取閒置速率 |
| 0x05 | SET_IDLE | 設定閒置速率 |
| 0x06 | GET_PROTOCOL | 讀取協定版本 |
| 0x07 | SET_PROTOCOL | 設定協定版本 |
| 0x08 | SET_POWER | 設定電源狀態 |

---

## 架構圖

```
┌─────────────────────────────────────────────────────────┐
│  i2c-stub (kernel module)                              │
│  - 模擬 I2C EEPROM/晶片                                 │
│  - 提供 SMBus registers                                 │
└─────────────────┬───────────────────────────────────────┘
                  │ I2C Bus (/dev/i2c-0)
                  │
┌─────────────────▼───────────────────────────────────────┐
│  vhidi2c-daemon (userspace)                             │
│                                                         │
│  - 使用 i2c-dev 存取 I2C bus                           │
│  - 程式 HID Descriptor 到 i2c-stub                      │
│  - 程式 HID Report Descriptor                          │
│  - 處理 HID 命令 (GET/SET_REPORT, RESET 等)           │
│  - 維護裝置狀態                                         │
└─────────────────────────────────────────────────────────┘
                  │
                  │ (Host 可以枚舉和控制虛擬裝置)
                  ▼
┌─────────────────────────────────────────────────────────┐
│  FWUPD Plugin                                           │
│  - 枚舉 I2C HID 裝置                                    │
│  - 讀取 HID Descriptor                                 │
│  - 發送 HID 命令                                        │
└─────────────────────────────────────────────────────────┘
```

---

## 限制

**Phase 1 (Userspace Daemon)**：
- 使用 i2c-stub 作為虛擬 I2C 匯流排
- 裝置是被動的 - 主機必須主動發起所有通信
- 真正的中斷驅動行為尚未完全模擬
- 適用於離線測試 FWUPD Plugin

**命令處理限制（Phase 1）：**
- 由於 i2c-stub 為被動 EEPROM 模擬，daemon 無法接收 i2c-hid kernel driver 發送的 HID 命令（RESET/SET_REPORT/GET_REPORT 等）
- 此設計**僅適用於驗證 FWUPD Plugin 的枚舉流程**
- 如需完整命令處理，需升級至 Phase 2（Kernel Module + I2C Slave API）

**Phase 2 (Kernel Module)**（後期規劃）：
- 使用 Linux I2C Slave API
- 真正的 I2C slave 驅動程式
- 完整的中斷和電源管理
- 可被 i2c-hid 核心模組直接識別

---

## 參考資源

- [HID over I2C Protocol Specification (Microsoft)](https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/hid-over-i2c-guide)
- [Linux i2c-hid driver](https://www.kernel.org/doc/Documentation/hid/i2c-hid.txt)
- [Linux i2c-stub documentation](https://www.kernel.org/doc/Documentation/i2c/i2c-stub)
- [Linux kernel i2c-hid-core.c](drivers/hid/i2c-hid/i2c-hid-core.c)

---

## 授權

GPL-2.0-or-later

---

## Keroro 小隊

- **Keroro** - 專案總指揮官 🐱
- **Giroro** - 實作者（本次任務）
- **Dororo** - 驗證者

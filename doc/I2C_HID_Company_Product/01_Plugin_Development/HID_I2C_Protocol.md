# HID I2C Protocol 說明

## 1. HID I2C 協定概覽

HID I2C（HID over I2C）是 HID 協定在 I2C 傳輸層上的實現。
這是一個標準化的方式，讓 HID 裝置（如觸控板、觸控螢幕、鍵盤等）可以透過 I2C 介面與主機通訊，而不是傳統的 USB。

### 1.1 為何使用 HID I2C？

- **低針腳數**：只需要 SCL 和 SDA 兩條線（加上 power 和 ground）
- **標準化**：微軟主導的 Windows 標準，Linux kernel 有完整支援
- **節省成本**：不需要 USB 實體層晶片
- **相容性**：作業系統原生支援（Windows 8+、Linux kernel 4.14+）

## 2. Linux Kernel 中的 HID I2C 實作

### 2.1 核心架構

```
HID I2C 硬體
    │
    │ (I2C bus)
    ▼
┌─────────────────┐
│ i2c_adapter     │  ← Linux I2C 子系統
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ i2c_hid driver  │  ← i2c-hid-core.c (HID I2C 傳輸層核心)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ hid-generic     │  ← 標準 HID core (hid-core.c)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ hidraw driver   │  ← /dev/hidrawX 節點（使用者空間存取）
└─────────────────┘
```

### 2.2 核心驅動程式位置

- **i2c-hid-core.c**：HID I2C 傳輸層核心（負責 I2C 與 HID protocol 轉換）
  - 路徑：`drivers/hid/i2c-hid/i2c-hid-core.c`
- **i2c-hid-dmi.c**：DMI table 匹配表，用於有問題的硬體
  - 路徑：`drivers/hid/i2c-hid/i2c-hid-dmi.c`
- **hid-core.c**：通用 HID core
  - 路徑：`drivers/hid/hid-core.c`
- **hid-raw.c**：hidraw 節點驅動
  - 路徑：`drivers/hid/hidraw.c`

### 2.3 HID I2C Protocol Details

#### 2.3.1 I2C Header Format

每次 I2C 交易包含一個 HID Descriptor：

```c
struct i2c_hid_desc {
  __le16 wHIDDescLength;        // 整個 descriptor 的長度
  __le16 bcdVersion;            // HID 版本 (1.00 = 0x0100)
  __le16 wReportDescLength;    // Report Descriptor 長度
  __le16 wReportDescRegister;  // Report Descriptor 的 I2C register address
  __le16 wInputRegister;       // Input report 的 I2C register address
  __le16 wMaxInputLength;      // 最大 Input report 長度
  __le16 wOutputRegister;      // Output report 的 I2C register address
  __le16 wMaxOutputLength;     // 最大 Output report 長度
  __le16 wCommandRegister;     // HID command/feature register address
  __le16 wDataRegister;         // HID data register address
  __le16 wVendorID;            // Vendor ID
  __le16 wProductID;           // Product ID
  __le16 wVersionID;           // Version ID
} __attribute__((packed));
```

#### 2.3.2 Report Descriptor

HID Report Descriptor 定義了：
- Input Reports（裝置→主機）
- Output Reports（主機→裝置）
- Feature Reports（雙向）
- 各 report 的格式和大小

#### 2.3.3 讀取 Input Report

```c
// 讀取 input report 的 I2C transaction:
// 1. 寫入 input register address 到 I2C (無 data)
// 2. 從 I2C 讀取 response

// I2C write: [Input Register Address]
// I2C read:  [Report ID][Data...]

ssize_t ret;
char cmd = input_register_addr;
char buf[64];

// Step 1: 通知裝置要讀取
ret = write(i2c_fd, &cmd, 1);

// Step 2: 讀取 input report
ret = read(i2c_fd, buf, sizeof(buf));
```

#### 2.3.4 寫入 Output/Feature Report

```c
// 寫入 output report:
// 1. 寫入 output register address + data 到 I2C

char packet[65];  // [register][data], 最多 64 bytes data
packet[0] = output_register_addr;
memcpy(&packet[1], data, len);

ret = write(i2c_fd, packet, len + 1);
```

## 3. HID I2C 與 fwupd 的整合

### 3.1 為何 HID I2C 裝置需要客製 Plugin？

標準 HID 裝置可以使用 `hidraw` 節點讓 fwupd 通用處理，
但**韌體更新**涉及：

1. **自定義更新 Protocol**：各家廠商有自己的 flash protocol
2. **Bootloader 模式切換**：需要發送特定 HID Feature Report
3. **韌體格式解析**：非標準 binary 格式
4. **I2C fallback recovery**：當 HID 更新失敗時透過 i2c-dev 修復

因此需要一個客製 plugin 來實作這些細節。

### 3.2 兩種溝通路徑

```
路徑 1: HID 模式（正常运行时）
─────────────────────────────────────
  Plugin → hidraw (/dev/hidrawX) → hid-core → i2c-hid-core → I2C bus → 裝置
  速度: 快，適合大資料傳輸
  限制: 依賴 Linux HID 子系統

路徑 2: 原始 I2C 模式（更新/Recovery）
─────────────────────────────────────
  Plugin → i2c-dev (/dev/i2c-X) → I2C bus → 裝置
  速度: 慢，但更直接可靠
  用途: firmware flash, device recovery
```

### 3.3 自定義 HID Report 用於韌體更新

典型設計（以 Elan TouchPad 為例）：

```c
/* HID Feature Report 0x01: 進入 IAP (In-Application Programming) 模式 */
typedef struct __attribute__((packed)) {
  guint8 report_id;    /* 0x01 */
  guint8 command;      /* 0x00 = enter IAP */
  guint8 param[14];    /* 額外參數 */
} ElanIapCommand;

/* HID Output Report 0x02: 傳送韌體資料 */
typedef struct __attribute__((packed)) {
  guint8 report_id;    /* 0x02 */
  guint8 address_hi;  /* 高位址 */
  guint8 address_lo;   /* 低位址 */
  guint8 data[60];    /* 最多 60 bytes 資料 */
} ElanFirmwareData;
```

## 4. HID I2C 在 Windows 的實作

### 4.1 Windows 驅動程式堆疊

```
HID I2C 裝置
    │
    │ (ACPI _HID 和 _CID)
    ▼
┌─────────────────────────┐
│ Microsoft I2C HID Driver │  ← i2chid.sys (系統內建)
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│ HID Class Driver (hidclass.sys) │
└────────────┬────────────┘
             │
             ▼
┌─────────────────────────┐
│ HID Minidriver (hidi2c.sys) │  ← 轉換
└────────────┬────────────┘
             │
             ▼
         使用者應用程式 (ReadFile/WriteFile)
```

### 4.2 在 Windows 上使用 HID API

```c
#include <windows.h>
#include <hidusage.h>
#include <hidpi.h>

// 開啟 HID 裝置
HANDLE hDevice = CreateFile(devicePath,
  GENERIC_READ | GENERIC_WRITE,
  FILE_SHARE_READ | FILE_SHARE_WRITE,
  NULL, OPEN_EXISTING,
  FILE_FLAG_OVERLAPPED, NULL);

// 發送 Feature Report
 HIDP_CAPS caps;
 PHIDP_PREPARSED_DATA preparsedData;
 HidD_GetPreparsedData(hDevice, &preparsedData);
 HidP_GetCaps(preparsedData, &caps);

 HID_COLLECTION_INFORMATION info;
 HidD_GetCollectionInformation(hDevice, &info);

// 發送客製 Command (用於韌體更新)
BOOLEAN result = HidD_SetFeature(hDevice,
  (PVOID)firmware_cmd, caps.FeatureReportByteLength);
```

## 5. 自家產品的 Protocol 設計建議

### 5.1 定義自家 Protocol ID

在 `metainfo.xml` 和 plugin 中使用一致的 Protocol ID：

```xml
<custom>
  <value key="LVFS::UpdateProtocol">com.company.i2chid</value>
</custom>
```

### 5.2 韌體區塊格式建議

```c
/* 建議的韌體檔案格式 */
typedef struct __attribute__((packed)) {
  guint32 magic;           // 魔數，如 0xC0mpany  (4 bytes)
  guint16 header_version;   // Header 版本 (2 bytes)
  guint16 firmware_version; // 韌體版本 (2 bytes)
  guint32 total_length;     // 總長度 (4 bytes)
  guint32 checksum;         // CRC32 校驗 (4 bytes)
  guint16 block_size;       // 每區塊大小 (2 bytes)
  guint16 num_blocks;       // 區塊數量 (2 bytes)
  // 以下是各區塊資料
} CompanyFirmwareHeader;
```

### 5.3 更新流程的 HID Command 建議

| Command ID | 功能 | 說明 |
|---|---|---|
| 0x01 | `FW_GET_INFO` | 取得當前韌體版本、IC 型號 |
| 0x02 | `FW_ENTER_BOOT` | 進入 bootloader 模式 |
| 0x03 | `FW_WRITE_BLOCK` | 寫入一個區塊 |
| 0x04 | `FW_VERIFY_BLOCK` | 驗證區塊寫入 |
| 0x05 | `FW_EXIT_BOOT` | 離開 bootloader 並重置 |
| 0x06 | `FW_READ_BLOCK` | 讀取一個區塊（用於驗證） |
| 0x07 | `FW_GET_STATUS` | 取得晶片狀態 |

### 5.4 重要安全考量

1. **通訊加密**：建議在 HID Report 中加入簡單的 XOR 或滾動密鑰
2. **Bootloader 鎖定**：確保只有正確的 command sequence 才能進入更新模式
3. **韌體驗證**：燒錄完成後主動讀回驗證 CRC/checksum
4. **防磚機制**：斷電保護 — 確保 bootloader 可以從上次失敗中恢復

## 6. 實用資源

### 6.1 Linux Kernel Documentation
- `Documentation/hid/i2c-hid.rst` — HID I2C 子系統說明
- `Documentation/hid/` — HID 子系統整體架構

### 6.2 USB-IF HID 規範
- Device Class Definition for HID (HID1.11)
- HID Usage Tables
- HID over I2C Protocol Specification (Microsoft)

### 6.3 fwupd 中的 HID 相關 Plugin
- `plugins/hidpp/` — Logitech HID++ (深度參考)
- `plugins/elantp/` — Elan TouchPad (I2C fallback recovery)
- `plugins/focal-fp/` — Fingerprint reader (I2C recovery)
- `plugins/cros-ec/` — ChromeOS EC (自定義 HID protocol)

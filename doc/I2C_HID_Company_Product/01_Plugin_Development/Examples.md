# 現有 Plugin 範例研究

本文件分析 fwupd 中與 HID I2C 產品最相關的幾個現有 Plugin，
作為自家產品 plugin 開發的參考。

---

## 1. Elan TouchPad Plugin (`elantp`)

**位置**：`https://github.com/fwupd/fwupd/tree/main/plugins/elantp`  
**Protocol ID**：`tw.com.emc.elantp`  
**HID I2C 相關性**：★★★★★（最高）

### 1.1 為何重要

Elan TouchPad plugin 是**最完整的 HID + I2C 雙模式**實作參考。
它同時支援 HID 模式（`hidraw`）和原始 I2C 模式（`i2c-dev`），
並且 I2C 模式用於 recovery（當 HID 更新失敗時）。

### 1.2 核心設計

**GUID 生成策略**（多層次匹配）：

```c
// 標準 HID DeviceInstanceId
HIDRAW\\VEN_04F3&DVN_3010

// 加入模組 ID
HIDRAW\\VEN_04F3&DVN_3010&MOD_1234

// 自定義 IC 型號 GUID
ELANTP\\ICTYPE_09

// IC + 模組
ELANTP\\ICTYPE_09&MOD_1234

// IC + 模組 + Part
ELANTP\\ICTYPE_09&MOD_1234&PART_MCU

// IC + 模組 + Driver 區分（HID vs I2C）
ELANTP\\ICTYPE_09&MOD_1234&DRIVER_HID   // HID 模式
ELANTP\\ICTYPE_09&MOD_1234&DRIVER_ELAN_I2C  // I2C 模式
```

**I2C Fallback Recovery**（最重要！）：

```c
static gboolean
fu_elantp_device_write_firmware(FuDevice *device,
                                 GBytes *blob_fw,
                                 FuProgress *progress,
                                 GError **error)
{
  // 嘗試使用 HID 介面寫入（快速）
  ret = fu_elantp_device_write_firmware_hid(device, blob_fw, error);
  if (!ret) {
    // HID 失敗，使用 I2C fallback（緩慢但可靠）
    g_debug("HID update failed, using I2C recovery...");
    ret = fu_elantp_device_write_firmware_i2c(device, blob_fw, error);
  }
  return ret;
}
```

**使用 Plugin Quirks**：

```
# elantp.quirk
[iantp]
Plugin = elantp
DeviceInstanceId=HIDRAW\VEN_04F3&DVN_3010
DeviceInstanceId=ELANTP\ICTYPE_09
DeviceInstanceId=ELANTP\ICTYPE_09&MOD_1234
ElantpIcPageCount=0x1000
ElantpIapPassword=0x1234
```

### 1.3 可借鑒的點

- [ ] 多層次 GUID 生成（`fu_device_add_instance_id`）
- [ ] I2C fallback recovery 機制
- [ ] Plugin quirks 檔案的格式和內容
- [ ] IAP (In-Application Programming) 模式切換流程
- [ ] 韌體版本解析（IC page count 等）

---

## 2. ChromeOS EC Plugin (`cros-ec`)

**位置**：`https://github.com/fwupd/fwupd/tree/main/plugins/cros-ec`  
**Protocol ID**：`com.google.usb.crosec`  
**HID I2C 相關性**：★★★☆☆

### 2.1 為何重要

ChromeOS EC (Embedded Controller) plugin 展示如何實作一個**完全自定義 HID protocol**。
它不是標準 DFU 或 UEFI Capsule，而是自行定義的更新方式。

### 2.2 核心設計

**Protocol ID 定義**：
```c
/* This plugin supports the following protocol ID: */
static const gchar *const
fu_cros_ec_plugin_protocol_ids[] = {
  "com.google.usb.crosec",
  NULL
};
```

**USB + 未来 I2C 支援**：
```c
static gboolean
fu_cros_ec_plugin_coldplug(FuPlugin *plugin, FuProgress *progress, GError **error)
{
  // 目前支援 USB，未來可擴展 I2C
  // 使用 REPLUG_MATCH_GUID 處理 runtime/bootloader 模式切換
  fu_device_add_flag(device, FU_DEVICE_STATE_FLAG_REPLUG_MATCH_GUID);
}
```

**Firmware Format**：
- 使用 Google fmap (flash map) 格式
- 標準 `.cab` archive 內含 fmap binary

### 2.3 可借鑒的點

- [ ] 自定義 Protocol ID 的命名方式 (`com.google.xxx` → `com.company.i2chid`)
- [ ] 韌體格式的封裝方式（fmap 可作參考）
- [ ] runtime ↔ bootloader 模式切換的 `REPLUG_MATCH_GUID` 使用

---

## 3. Focal Point Fingerprint Plugin (`focal-fp`)

**位置**：`https://github.com/fwupd/fwupd/tree/main/plugins/focal-fp`  
**HID I2C 相關性**：★★★★☆

### 3.1 為何重要

這個 plugin 展示指紋辨識器如何同時使用 HID 和 I2C 介面。
指紋辨識器與自家 HID I2C 產品的最大公約數：
- 都需要安全性考量
- 都可能需要 I2C fallback recovery

### 3.2 核心設計

**HID 模式下更新**：
```c
static gboolean
fu_focalfp_device_write_firmware_hid(FuDevice *device,
                                      GBytes *blob_fw,
                                      GError **error)
{
  // HID 模式快速寫入
  // 切換到 Flash mode → 傳送資料 → 驗證
}
```

**I2C Recovery**：
```c
static gboolean
fu_focalfp_device_recover_i2c(FuDevice *device, GError **error)
{
  // 當 HID 更新失敗磚化後
  // 使用 i2c-dev 開啟原始 I2C 存取
  // 重新燒錄 bootloader + firmware
  // 速度慢但可靠
}
```

---

## 4. Logitech HID++ Plugin (`hidpp`)

**位置**：`https://github.com/fwupd/fwupd/tree/main/plugins/logitech-hidpp`  
**Protocol ID**：`com.logitech.unifying`  
**HID I2C 相關性**：★★★☆☆

### 4.1 為何重要

Logitech HID++ 是最複雜的 HID-based 更新 Plugin，
處理了非常多的 Logitech 裝置型號，展示了如何管理大量裝置相容性。

### 4.2 核心設計

**使用 FuBluetoothDevice**：
```c
fu_device_add_instance_id_full(device, instance_ids[1],
  FU_DEVICE_INSTANCE_FLAG_QUIRK);
```

**多種 runtime/bootloader 模式**：
```c
static const gchar *
fu_logitech_hidpp_peripheral_get_plugin(FuLogitechHidppPeripheral *self)
{
  if (fu_device_has_flag(device, FWUPD_DEVICE_FLAG_IS_BOOTLOADER))
    return "logitech-hidpp-runtime";
  return "logitech-hidpp-runtime-bolt";
}
```

### 4.3 可借鑒的點

- [ ] 大量裝置的 GUID 管理
- [ ] 跨型號的通用更新框架
- [ ] Battery level monitoring (作為輔助功能)

---

## 5. USB DFU Plugin (`dfu`)

**位置**：`https://github.com/fwupd/fwupd/tree/main/plugins/dfu`  
**HID I2C 相關性**：★★☆☆☆

### 5.1 為何重要

DFU (Device Firmware Update) 是 USB 論壇的標準更新協定，
被大量 USB 裝置採用。了解 DFU 有助於理解 fwupd 如何支援標準協定。

### 5.2 可借鑒的點

- [ ] DFU 狀態機（`dfu-tool` 命令列工具）
- [ ] Manifest 格式處理
- [ ] UFD (USB Device Firmware) footer 格式

---

## 6. Plugin 結構對照表

| 功能需求 | elantp | cros-ec | focal-fp | 建議 |
|---|---|---|---|---|
| HID 介面溝通 | ✅ | ✅ (USB HID) | ✅ | 參考 elantp |
| 原始 I2C fallback | ✅ | ❌ | ✅ | 必須實作 |
| 自定義 Protocol | ✅ | ✅ | ✅ | 建議 |
| Bootloader 模式切換 | ✅ | ✅ | ✅ | 必備 |
| 版本比對/防降級 | ✅ | ✅ | ✅ | 必備 |
| Quirk 檔案 | ✅ | ❌ | ❌ | 建議 |
| I2C 設備發現 | ✅ (udev) | ❌ | ✅ (i2c-dev) | FuUdevDevice |

---

## 7. 重點範例：elantp Plugin 的 coldplug 流程

```c
static gboolean
fu_elantp_plugin_coldplug(FuPlugin *plugin, FuProgress *progress, GError **error)
{
  g_autoptr(FuUdevDevice) device = NULL;
  FuContext *ctx = fu_plugin_get_context(plugin);

  /* 1. 使用 udev 發現 hidraw 節點 */
  device = FU_UDEV_DEVICE(fu_context_udev_device_new(ctx,
    "hidraw",
    "HID_NAME", "Elan TouchPad",
    NULL));

  /* 2. 讀取 VID/PID */
  vendor_id = fu_udev_device_get_vendor_id(device);  /* e.g. "HIDRAW:0x04F3" */
  fu_device_set_vendor_id(device, vendor_id);

  /* 3. 讀取韌體版本 */
  version = fu_udev_device_get_sysfs_attr(device, "device/version", error);
  fu_device_set_version(device, version);

  /* 4. 加入 instance IDs (GUIDs) */
  fu_device_add_instance_id(device, "HIDRAW\\VEN_04F3&DVN_3010");
  fu_device_add_instance_id_full(device,
    "ELANTP\\ICTYPE_09",  /* IC type */
    FU_DEVICE_INSTANCE_FLAG_QUIRK);

  /* 5. 設定為可更新 */
  fu_device_add_flag(device, FWUPD_DEVICE_FLAG_UPDATABLE);
  fu_device_add_flag(device, FWUPD_DEVICE_FLAG_NEEDS_BOOTLOADER);

  /* 6. 加入 I2C fallback 支援 */
  fu_device_add_internal_flag(device,
    FU_DEVICE_INTERNAL_FLAG_RETRY_OPEN);

  fu_plugin_add_device(plugin, FU_DEVICE(device));
  return TRUE;
}
```

---

## 8. 快速上手：複製 elantp 作為起點

若要快速啟動，建議以 elantp plugin 為基礎進行修改：

```bash
# 1. 複製 elantp plugin
cp -r fwupd/plugins/elantp fwupd/plugins/company-i2c-hid

# 2. 替換所有 elantp → company_i2c_hid
find . -type f -exec sed -i 's/elantp/company_i2c_hid/g' {} \;
find . -type f -exec sed -i 's/Elantp/CompanyI2cHid/g' {} \;
find . -type f -exec sed -i 's/ELANTP/COMPANY_I2C_HID/g' {} \;
find . -type f -exec sed -i 's/ELAN_TP/COMPANY_I2C_HID/g' {} \;
find . -type f -exec sed -i 's/tw.com.emc/com.company/g' {} \;

# 3. 調整 Protocol ID
# 4. 實作自家的 read/write I2C/HID functions
# 5. 調整 firmware parsing logic
```

---

## 9. 學習路徑建議

1. **第一天**：細讀 `elantp` plugin 完整程式碼
   - 了解 FuUdevDevice 的使用方式
   - 理解 I2C fallback recovery 機制
   - 看懂 quirk 檔案格式

2. **第二天**：看 `hidpp` plugin
   - 了解 HID Report 的讀寫方式
   - 理解多型號支援框架

3. **第三天**：看 `cros-ec` plugin
   - 自定義 protocol ID 設計
   - runtime/bootloader 切換

4. **第四天**：建立自家 plugin 骨架
   - 複製 elantp 並替換
   - 編譯測試 (meson + ninja)
   - 用 `fwupdmgr get-devices` 確認偵測到

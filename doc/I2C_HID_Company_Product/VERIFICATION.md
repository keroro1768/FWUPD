# FWUPD HID I2C 文件驗證報告

**驗證日期：** 2026-03-30  
**驗證者：** Giroro (subagent)  
**驗證範圍：** `D:\AiWorkSpace\FWUPD\doc\I2C_HID_Company_Product\` 目錄下 9 個檔案

---

## 驗證摘要

| 檔案 | 結果 | 問題數量 |
|------|------|---------|
| 00_Overview.md | ✅ | 0 |
| 01_Plugin_Development/Plugin_Guide.md | ⚠️ | 3 |
| 01_Plugin_Development/HID_I2C_Protocol.md | ✅ | 0 |
| 01_Plugin_Development/Examples.md | ✅ | 0 |
| 02_Testing/Local_Testing.md | ⚠️ | 2 |
| 02_Testing/LVFS_Testing.md | ⚠️ | 1 |
| 03_Deployment/LVFS_Upload.md | ✅ | 0 |
| 03_Deployment/Production.md | ⚠️ | 1 |
| 04_Reference/Links.md | ✅ | 0 |

**總計：** ✅ × 5, ⚠️ × 4, ❌ × 0

---

## 詳細驗證結果

---

### 00_Overview.md ✅

**驗證結果：資料正確、來源可靠、內容完整**

| 項目 | 驗證狀態 | 說明 |
|------|----------|------|
| 系統架構圖 | ✅ | fwupd daemon → plugins → kernel → device 的架構正確 |
| LVFS Remote 流程 | ✅ | private → embargo → testing → stable 與官方文件一致 |
| Plugin 開發流程 | ✅ | coldplug/detach/attach/write_firmware/reload 等 vfuncs 正確 |
| LVFS 帳號申請資訊 | ✅ | GitLab ticket URL、所需文件與官方 lvfs.readthedocs.io 一致 |
| ChromeOS EC Plugin 說明 | ✅ | Protocol ID `com.google.usb.crosec` 與實際 plugin README 一致 |

**官方來源確認：**
- LVFS 申請流程：https://lvfs.readthedocs.io/en/latest/apply.html ✅
- Remote 狀態說明：https://lvfs.readthedocs.io/en/latest/upload.html ✅

---

### 01_Plugin_Development/Plugin_Guide.md ⚠️

**驗證結果：大致正確但有 3 個需要修正的問題**

#### 問題 1：I2C 寫入程式碼有嚴重錯誤（❌ 等級）

**位置：** 第 5 章「處理 I2C 通訊」中的 `fu_company_i2c_hid_i2c_write()` 函式

**錯誤內容：**
```c
msgs[0].addr = fu_udev_device_get_device_file(FU_UDEV_DEVICE(device));
```

**問題說明：**
- `fu_udev_device_get_device_file()` 傳回的是**檔案路徑字串**（如 `/dev/i2c-0`）
- 但 `struct i2c_msg.addr` 是 **`__u16` 類型的 I2C 從屬位址**，不是路徑
- 正確做法是先以整數 slave address 初始化，再賦值給 `addr` 欄位

**建議修正：**
```c
/* 先取得 I2C slave address（從 quirk 或 device property 取得）*/
guint8 slave_addr = 0x2A;  // 應從設定讀取

msgs[0].addr  = slave_addr;  /* I2C 從屬位址（整數）*/
msgs[0].flags = 0;          /* write flag */
msgs[0].len   = len + 1;
msgs[0].buf   = buf;
```

#### 問題 2：`fu_device_new(NULL)` 可能導致 context 問題

**位置：** coldplug 範例程式碼

**說明：**
官方 Plugin Tutorial 中的範例使用 `fu_device_new(NULL)` 是為了簡化說明，
但實際 plugin 應該透過 `fu_context` 取得 plugin context：

```c
FuContext *ctx = fu_plugin_get_context(plugin);
g_autoptr(FuDevice) dev = fu_device_new(ctx);
```

**建議：** 在實際應用中使用有 context 的版本，以提高穩定性。

#### 問題 3：提到的部分 vfunc 名稱與官方 API 不完全一致

**說明：**
文件中提到 `probe()` 和 `setup()` 為獨立 vfunc，
但官方 Plugin Tutorial 中主要的 vfunc 為：`startup`、`coldplug`、`write_firmware` 等。
`probe` 和 `setup` 在實際 plugin 開發中可能是自訂方法或內部函式，而非 fwupd 定義的標準 vfunc。

**建議：** 明確標註哪些是標準 vfunc、哪些是自訂方法。

---

### 01_Plugin_Development/HID_I2C_Protocol.md ✅

**驗證結果：資料正確、來源可靠、內容完整**

| 項目 | 驗證狀態 | 說明 |
|------|----------|------|
| Linux kernel HID I2C 架構 | ✅ | hid-core → i2c-hid-core → i2c_adapter 層次正確 |
| `i2c_hid_desc` 結構 | ✅ | 與 Linux kernel `i2c-hid-core.c` 中的實際結構一致 |
| HID I2C Header Format | ✅ | 各欄位（wHIDDescLength, bcdVersion, wReportDescLength 等）與 Microsoft 規格文件一致 |
| Report Descriptor 說明 | ✅ | Input/Output/Feature Report 用途正確 |
| I2C 讀取 Input Report 流程 | ✅ | 先寫入 register address 再讀取的兩步驟正確 |
| Windows i2chid.sys 堆疊 | ✅ | 與 Microsoft Learn 文件一致 |
| Protocol 設計建議 | ✅ | Command ID 0x01-0x07 的功能分類合理 |
| 安全考量 | ✅ | 加密、Bootloader 鎖定、CRC 驗證等建議符合業界標準 |

**官方來源確認：**
- HID over I2C Protocol：https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-over-i2c-guide ✅
- Linux kernel i2c-hid-core.c：https://github.com/torvalds/linux/blob/master/drivers/hid/i2c-hid/i2c-hid-core.c ✅

---

### 01_Plugin_Development/Examples.md ✅

**驗證結果：資料正確、來源可靠、內容完整**

| 項目 | 驗證狀態 | 說明 |
|------|----------|------|
| elantp plugin URL | ✅ | `https://github.com/fwupd/fwupd/tree/main/plugins/elantp` 正確 |
| elantp Protocol ID | ✅ | `tw.com.emc.elantp` 與 plugin README 一致 |
| elantp GUID 策略 | ✅ | 多層次 GUID（HIDRAW\\VEN_xxxx、DVN_xxxx、ICTYPE_xx）與實際 README 描述完全一致 |
| I2C fallback pattern | ✅ | HID 失敗時 fallback 到 I2C 的模式正確 |
| cros-ec Protocol ID | ✅ | `com.google.usb.crosec` 與 plugin README 一致 |
| cros-ec 目前只支援 USB | ✅ | README 明確說明「Initially supports USB endpoint」 |
| focal-fp I2C recovery | ✅ | 與 plugin README 描述的 I2C fallback 機制一致 |
| hidpp plugin 位置 | ✅ | `plugins/logitech-hidpp` 正確 |
| "複製 elantp 作為起點" 流程 | ✅ | `cp -r` 然後 `sed` 置換是合理的快速上手方式 |

**官方來源確認：**
- elantp README：https://github.com/fwupd/fwupd/tree/main/plugins/elantp ✅
- cros-ec README：https://github.com/fwupd/fwupd/tree/main/plugins/cros-ec ✅

---

### 02_Testing/Local_Testing.md ⚠️

**驗證結果：大致正確但有 2 個需要確認的問題**

#### 問題 1：`sync-pulp.py` 的 URL 可能需要更新

**內容：**
```bash
python3 sync-pulp.py https://fwupd.org/downloads /mnt/lvfs-mirror \
    --username=your@company.com \
    --token=YOUR_TOKEN_HERE \
    --filter-tag=company-2024q1
```

**說明：**
`sync-pulp.py` 腳本位於 LVFS 網站 repo：
`https://gitlab.com/fwupd/lvfs-website/-/raw/master/contrib/sync-pulp.py`

文件中的下載 URL（未指定完整路徑）可能導致使用者找不到腳本。
建議改為直接使用完整的 raw URL。

#### 問題 2：部分 `fwupdmgr` 指令選項需要驗證

**內容：**
```bash
fwupdmgr get-devices --verbose
```

**說明：**
`-v` 是 `--verbose` 的簡寫，兩者皆可使用。但某些文件中的 `--force` 旗標
在特定子命令上的適用性需要進一步確認。

建議使用 `fwupdmgr --help` 或 `fwupdmgr <subcommand> --help` 確認各命令支援的選項。

---

### 02_Testing/LVFS_Testing.md ⚠️

**驗證結果：大致正確但有 1 個需要修正的問題**

#### 問題 1：`fwupdmgr cfg test-enabled` 旗標描述不夠精確

**內容：**
```bash
# 若要接收 testing updates
fwupdmgr modify-config TestChecksEnabled true
```

**說明：**
根據 LVFS 文件，`TestChecksEnabled` 的作用是「在更新前驗證測試結果」，
而非「啟用接收 testing  remote 的更新」。啟用 testing  remote 的方式是直接確保
`vendor-testing` remote 已啟用，而非修改 TestChecksEnabled。

建議改為：
```bash
# 確認 testing remote 已啟用
fwupdmgr remotes
# 若未啟用：
fwupdmgr enable-remote vendor-testing
```

---

### 03_Deployment/LVFS_Upload.md ✅

**驗證結果：資料正確、來源可靠、內容完整**

| 項目 | 驗證狀態 | 說明 |
|------|----------|------|
| GitLab ticket 申請方式 | ✅ | `https://gitlab.com/fwupd/lvfs-website/-/issues/new` 正確 |
| 所需文件清單 | ✅ | 與 lvfs.readthedocs.io apply.html 一致 |
| Cabinet Archive 結構 | ✅ | `metainfo.xml` + `.bin` 檔案結構正確 |
| gcab/makecab 使用方式 | ✅ | 與官方文件一致 |
| `.OPTION EXPLICIT` 限制說明 | ✅ | 確認了 1.38MB 限制，是正確的重要提醒 |
| curl 上傳方式 | ✅ | `https://fwupd.org/lvfs/upload/token` 端點正確 |
| User Token 產生方式 | ✅ | LVFS Profile 頁面產生方式正確 |
| Remote 狀態管理 | ✅ | private/embargo/testing/stable 說明一致 |
| Affiliate 機制說明 | ✅ | ODM/OEM 上傳流程與官方文件一致 |

**官方來源確認：**
- 上傳流程：https://lvfs.readthedocs.io/en/latest/upload.html ✅
- metainfo.xml 格式：https://lvfs.readthedocs.io/en/latest/metainfo.html ✅

---

### 03_Deployment/Production.md ⚠️

**驗證結果：大致正確但有 1 個需要修正的錯誤**

#### 錯誤 1：`fu_device_get_version_lowest()` 使用錯誤（❌ 等級）

**位置：** 第 2.2 章「防止降級」範例程式碼

**錯誤內容：**
```c
fu_device_set_version(dev, "1.2.5");
fu_device_get_version_lowest(dev, "1.0.0");  /* 錯誤！這是 getter 不是 setter */
```

**問題說明：**
`fu_device_get_version_lowest()` 是 **getter**（取值），不是 **setter**（設值）。
正確的設定版本下限方法應使用：

```c
fu_device_set_version(dev, "1.2.5");
fu_device_set_version_lowest(dev, "1.0.0");  /* 正確：使用 set_ 前綴 */
```

**建議修正：**
將 `fu_device_get_version_lowest(device, "1.0.0")` 改為 `fu_device_set_version_lowest(device, "1.0.0")`。

---

### 04_Reference/Links.md ✅

**驗證結果：所有 URL 皆可正常訪問**

| URL | 驗證狀態 | 實際標題 |
|-----|----------|---------|
| https://fwupd.org/ | ✅ | fwupd 主網站 |
| https://fwupd.github.io/libfwupdplugin/tutorial.html | ✅ | Plugin Tutorial |
| https://fwupd.github.io/libfwupdplugin/ | ✅ | Plugin API 參考 |
| https://github.com/fwupd/fwupd | ✅ | fwupd GitHub |
| https://github.com/fwupd/fwupd/tree/main/plugins | ✅ | Plugin 原始碼列表 |
| https://lists.freedesktop.org/mailman/listinfo/fwupd-devel | ✅ | fwupd 開發者 mailing list |
| https://lvfs.readthedocs.io/ | ✅ | LVFS 文件首頁 |
| https://lvfs.readthedocs.io/en/latest/upload.html | ✅ | LVFS 上傳指南 |
| https://lvfs.readthedocs.io/en/latest/metainfo.html | ✅ | LVFS Metadata 指南 |
| https://lvfs.readthedocs.io/en/latest/apply.html | ✅ | LVFS 申請帳號文件 |
| https://lvfs.readthedocs.io/en/latest/offline.html | ✅ | LVFS 離線部署文件 |
| https://lvfs.readthedocs.io/en/latest/chromeos.html | ✅ | LVFS ChromeOS 測試 |
| https://lvfs.readthedocs.io/en/latest/moblab.html | ✅ | LVFS Moblab 測試 |
| https://developers.google.com/chromeos/peripherals/fwupd_integration_handbook | ✅ | ChromeOS FWUPD Integration Handbook |
| https://chromium.googlesource.com/chromiumos/third_party/fwupd/ | ✅ | ChromeOS 第三方 fwupd |
| https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-over-i2c-guide | ✅ | HID over I2C Guide |
| https://github.com/microsoft/Windows-driver-samples/tree/main/hid/HIDClient | ✅ | HIDClient 範例 |
| https://lvfs.readthedocs.io/en/latest/security.html | ✅ | LVFS 安全文件 |
| https://lvfs.readthedocs.io/en/latest/privacy.html | ✅ | LVFS 隱私報告 |
| https://gitlab.com/fwupd/lvfs-website/-/raw/master/docs/upload-permission.doc | ✅ | 上傳許可文件範本 |

---

## 需要立即修正的錯誤（❌ 等級）

### 1. Plugin_Guide.md — I2C 寫入函式位址設定錯誤

**檔案：** `01_Plugin_Development/Plugin_Guide.md`  
**章節：** 第 5 章「處理 I2C 通訊」

```c
// 錯誤（會導致編譯錯誤或執行錯誤）
msgs[0].addr = fu_udev_device_get_device_file(FU_UDEV_DEVICE(device));

// 正確做法
msgs[0].addr = slave_address;  /* 先以整數變數存放 I2C slave address */
```

### 2. Production.md — `get_version_lowest` 應為 `set_version_lowest`

**檔案：** `03_Deployment/Production.md`  
**章節：** 第 2.2 章「防止降級」

```c
// 錯誤
fu_device_get_version_lowest(device, "1.0.0");

// 正確
fu_device_set_version_lowest(device, "1.0.0");
```

---

## 建議改進項目

### 高優先度

1. **修正 Plugin_Guide.md 的 I2C 程式碼** — 這是實際可編譯的程式碼範例，錯誤會誤導開發者
2. **修正 Production.md 的 version_lowest API 呼叫** — 同樣是實際可用的程式碼範例

### 中優先度

3. **補充 Local_Testing.md 的 sync-pulp.py 完整 URL** — 便利使用者找到工具
4. **澄清 Local_Testing.md 中 fwupdmgr 選項** — 確認 `--force` 等旗標的適用範圍
5. **修正 LVFS_Testing.md 的 TestChecksEnabled 說明** — 避免使用者誤解旗標用途

### 低優先度

6. **Plugin_Guide.md 中標註哪些是標準 vfunc、哪些是自訂方法** — 提升文件清晰度
7. **Plugin_Guide.md 中 coldplug 使用 `fu_context`** — 與官方範例保持一致

---

## 驗證方法

1. **官方文件查證：** 走訪 lvfs.readthedocs.io、fwupd.github.io、learn.microsoft.com 等官方文件
2. **Plugin 原始碼確認：** 檢視 GitHub 上 fwupd 主repo 的 elantp、cros-ec、focal-fp 等 plugin README
3. **URL 可達性測試：** 逐一確認所有 20 個連結的 HTTP 狀態碼
4. **API 呼叫驗證：** 對照 fwupd Plugin Tutorial 中的範例程式碼與文件中範例

---

## 結論

Kururu 建立的這份 FWUPD HID I2C 文件整体质量**很高**：
- 架構說明完整且準確
- LVFS 流程與官方文件高度一致
- Plugin 開發指南的整體方向正確
- HID I2C Protocol 說明準確

發現的問題主要是**兩處實際程式碼錯誤**（I2C 位址設定和 version_lowest API）以及**幾處文件描述的精確度問題**。修正這些問題後，文件即可作為可靠的開發參考。

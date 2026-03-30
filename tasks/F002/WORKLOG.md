# WORKLOG.md — F002 工作日誌

## 任務資訊

| 欄位 | 內容 |
|------|------|
| 任務 ID | F002 |
| 任務名稱 | FWUPD Plugin 開發實作 |
| 開始日期 | 2026-03-30 |
| 結束日期 | 2026-03-30 |

---

## 更新日誌

### 2026-03-30

#### 完成的工作

1. **交叉比對驗證**
   - 將 `company-i2c-hid` Plugin 與 `elantp` Plugin 原始碼進行全面比對
   - 完成驗證報告 (`tasks/F002/VERIFY.md`)

2. **識別的問題**
   - ❌ Plugin 缺少 udev 子系統註冊
   - ❌ HID 通信完全未實作
   - ⚠️ Device 基類選擇錯誤 (應使用 FuHidrawDevice/FuI2cDevice)
   - ⚠️ Device 生命週期鉤子位置錯誤
   - ⚠️ Quirk key 名稱不一致
   - ⚠️ Firmware parsing 變數名錯誤 (CHECKSUM vs checksum)

3. **已修正的問題**
   - ✅ 修正 `fu_company_i2c_hid_plugin_init()` 中不應調用 `fu_plugin_add_device_gtype()`
   - ✅ 在 `fu_company_i2c_hid_plugin_constructed()` 中添加 udev 子系統註冊
   - ✅ 在 constructed() 中添加 quirk key 註冊
   - ✅ 修正 `FU_COMPANY_I2C_HID_FIRMWARE_checksum_OFFSET` 為大寫 `FU_COMPANY_I2C_HID_FIRMWARE_CHECKSUM_OFFSET`
   - ✅ 修正 quirk 檔案中的 key 名稱 (`QuirkCompanyI2cHidDriver` → `CompanyI2cHidDriver`)
   - ✅ 統一 quirk key 名稱格式

#### 待完成的工作

1. **P0 - Critical**
   - [ ] 實作 HID 通信功能 (使用 fu_hidraw_device_set_feature/get_feature)
   - [ ] 考慮拆分為兩個 device 類型 (HID 和 I2C)

2. **P1 - High**
   - [ ] 調整 Device 類型繼承 (使用 FuHidrawDevice 或 FuI2cDevice)
   - [ ] 將 detach 等方法移至 DeviceClass 而非 PluginClass

3. **P2 - Medium**
   - [ ] 完成後續測試
   - [ ] 驗證 I2C fallback recovery 機制

---

## 技術筆記

### elantp Plugin 架構

elantp 使用分離的 HID 和 I2C 設備類型：
- `FuElantpHidDevice` → 繼承 `FU_TYPE_HIDRAW_DEVICE`
- `FuElantpI2cDevice` → 繼承 `FU_TYPE_I2C_DEVICE`

Plugin 只負責：
1. 註冊 quirk keys
2. 添加 udev 子系統監聽
3. 添加 device/firmware 類型
4. 設備創建鉤子 (device_created)

實際設備通信由 DeviceClass 方法完成：
- `probe` - 設備探測
- `open` - 打開設備
- `setup` - 設備初始化
- `attach/detach` - 模式切換
- `write_firmware` - 韌體寫入
- `check_firmware` - 韌體驗證

### company-i2c-hid 當前架構問題

1. 單一 device 類型同時處理 HID 和 I2C
2. 在 PluginClass 中實現設備通信
3. coldplug 手動創建設備而非依賴 udev + quirk 自動匹配

---

## 提交記錄

| 日期 | 提交訊息 |
|------|----------|
| 2026-03-30 | fix: plugin structure - add udev subsystem registration |
| 2026-03-30 | fix: firmware - correct CHECKSUM macro naming |
| 2026-03-30 | fix: quirk file - standardize key naming convention |
| 2026-03-30 | docs: add verification report (VERIFY.md) |

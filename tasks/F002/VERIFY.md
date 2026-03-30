# FWUPD Plugin 交叉比對驗證報告 (二次驗證)

**比對目標：** `company-i2c-hid` Plugin vs `elantp` Plugin (fwupd 官方)  
**驗證日期：** 2026-03-30 (二次驗證)  
**Plugin 版本：** elantp (main branch), company-i2c-hid (v1.0.0)  

---

## 📋 比對摘要

| 項目 | elantp Plugin | company-i2c-hid Plugin | 狀態 |
|------|---------------|------------------------|------|
| Plugin 結構 | ✅ 正確 | ✅ 已修正 | **PASS** |
| Device 枚舉 | ✅ udev 自動偵測 | ✅ udev 自動偵測 | **PASS** |
| HID 模式實作 | ✅ 完整 (FuHidrawDevice) | ✅ 完整實作 | **PASS** |
| Device 基類 | ✅ FU_TYPE_HIDRAW_DEVICE | ✅ FU_TYPE_HIDRAW_DEVICE | **PASS** |
| 生命週期鉤子 | ✅ FuDeviceClass | ✅ FuDeviceClass | **PASS** |
| Firmware 格式 | ✅ 結構化解析 | ✅ 結構化解析 | **PASS** |
| Quirk 處理 | ✅ 正確 | ✅ 正確 | **PASS** |

---

## ✅ P0 Critical 驗證結果

### 1. HID 通信完整實作 ✅ PASS

**elantp 實作：** 使用 `fu_hidraw_device_set_feature()` / `fu_hidraw_device_get_feature()`

**company-i2c-hid 實作：**
```c
// fu-company-i2c-hid-device.c:165
return fu_hidraw_device_set_feature(FU_HIDRAW_DEVICE(self), buf, buflen, ...);

// fu-company-i2c-hid-device.c:206
if (!fu_hidraw_device_get_feature(FU_HIDRAW_DEVICE(self), res_buf, sizeof(res_buf), ...))

// fu-company-i2c-hid-device.c:249 (firmware block write)
return fu_hidraw_device_set_feature(FU_HIDRAW_DEVICE(self), array->data, array->len, ...);
```

**結論：** HID 通信已完整實作，使用正確的 `fu_hidraw_device_set/get_feature()` API。

---

### 2. udev 子系統註冊 ✅ PASS

**elantp 實作：**
```c
fu_plugin_add_udev_subsystem(plugin, "i2c");
fu_plugin_add_udev_subsystem(plugin, "i2c-dev");
fu_plugin_add_udev_subsystem(plugin, "hidraw");
```

**company-i2c-hid 實作：** (`fu-company-i2c-hid-plugin.c:63-65`)
```c
fu_plugin_add_udev_subsystem(plugin, "i2c");
fu_plugin_add_udev_subsystem(plugin, "i2c-dev");
fu_plugin_add_udev_subsystem(plugin, "hidraw");
```

**結論：** udev 子系統註冊完全符合 elantp 模式。

---

## ✅ P1 High 驗證結果

### 3. Device 基類改為 `FU_TYPE_HIDRAW_DEVICE` ✅ PASS

**elantp 實作：**
```c
G_DEFINE_TYPE(FuElantpHidDevice, fu_elantp_hid_device, FU_TYPE_HIDRAW_DEVICE)
```

**company-i2c-hid 實作：** (`fu-company-i2c-hid-device.c:30`)
```c
G_DEFINE_TYPE(FuCompanyI2cHidDevice, fu_company_i2c_hid_device, FU_TYPE_HIDRAW_DEVICE)
```

**結論：** Device 基類已從 `FU_TYPE_DEVICE` 正確改為 `FU_TYPE_HIDRAW_DEVICE`。

---

### 4. 生命週期鉤子從 FuPluginClass 移到 FuDeviceClass ✅ PASS

**company-i2c-hid 實作：** (`fu-company-i2c-hid-device.c:834-844`)
```c
device_class->to_string = fu_company_i2c_hid_device_to_string;
device_class->set_quirk_kv = fu_company_i2c_hid_device_set_quirk_kv;
device_class->setup = fu_company_i2c_hid_device_setup;
device_class->reload = fu_company_i2c_hid_device_setup;
device_class->attach = fu_company_i2c_hid_device_attach;
device_class->detach = NULL;  /* handled by write_firmware */
device_class->write_firmware = fu_company_i2c_hid_device_write_firmware;
device_class->check_firmware = fu_company_i2c_hid_device_check_firmware;
device_class->probe = fu_company_i2c_hid_device_probe;
device_class->set_progress = fu_company_i2c_hid_device_set_progress;
device_class->convert_version = fu_company_i2c_hid_device_convert_version;
```

**說明：** `device_class->detach = NULL` 是因為 Plugin 選擇在 `write_firmware` 內部處理 bootloader 進入邏輯（`fu_company_i2c_hid_device_detach()`），而非透過標準的 `detach` 鉤子。這是非標準但功能正確的設計。

**結論：** 所有生命週期鉤子已從 FuPluginClass 移到 FuDeviceClass。

---

## ⚠️ P2/P3 殘留項目（非阻塞）

### P2: Quirk Key 名稱風格

company-i2c-hid 使用 `CompanyI2cHid*` 前綴（如 `CompanyI2cHidIcType`），elantp 使用 `Elantp*` 前綴。這是各自公司的命名約定，不影響功能。

### P3: I2C Fallback 模式

company-i2c-hid 主要基於 `FU_TYPE_HIDRAW_DEVICE`，透過 HID interface 進行 IAP programming，而非使用 elantp 的 `FU_TYPE_I2C_DEVICE` + I2C ioctl 模式。這是架構選擇差異，非缺失。

---

## 📊 驗證總結

| 優先級 | 項目 | 狀態 |
|--------|------|------|
| P0 | HID 通信完整實作 | ✅ PASS |
| P0 | udev 子系統註冊 | ✅ PASS |
| P1 | Device 基類改為 FU_TYPE_HIDRAW_DEVICE | ✅ PASS |
| P1 | 生命週期鉤子在 FuDeviceClass | ✅ PASS |

**所有 P0/P1 問題已修復。Plugin 通過驗證。**

---

## 📝 修正清單 (更新)

- [x] P0: Plugin udev 子系統註冊
- [x] P0: HID 通信完整實作 (fu_hidraw_device_set/get_feature)
- [x] P1: Device 基類改為 FU_TYPE_HIDRAW_DEVICE
- [x] P1: 生命週期鉤子移到 FuDeviceClass
- [x] P2: Firmware parsing 變數名 (已自行修正)
- [x] P2: Plugin constructed() vs init() (已自行修正)
- [x] P3: I2C device open 方式 (架構選擇，非錯誤)

---

## 🔗 參考文檔

- [fwupd elantp plugin](https://github.com/fwupd/fwupd/tree/main/plugins/elantp)
- [FWUPD Plugin 開發文檔](https://github.com/fwupd/fwupd/blob/main/docs/building-plugin.md)
- [Plugin 類型參考](https://github.com/fwupd/fwupd/tree/main/docs/plugin-types.md)

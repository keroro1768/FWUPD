# FWUPD Plugin 交叉比對驗證報告

**比對目標：** `company-i2c-hid` Plugin vs `elantp` Plugin (fwupd 官方)  
**驗證日期：** 2026-03-30  
**Plugin 版本：** elantp (main branch), company-i2c-hid (v1.0.0)  

---

## 📋 比對摘要

| 項目 | elantp Plugin | company-i2c-hid Plugin | 狀態 |
|------|---------------|------------------------|------|
| Plugin 結構 | ✅ 正確 | ⚠️ 需要調整 | 需修改 |
| Device 枚舉 | ✅ udev 自動偵測 | ⚠️ 手動 coldplug | 需修改 |
| HID 模式實作 | ✅ 完整 (FuHidrawDevice) | ❌ 未實作 | 需實作 |
| I2C 模式實作 | ✅ 完整 (FuI2cDevice) | ⚠️ 基本但需改進 | 需修改 |
| Firmware 格式 | ✅ 結構化解析 | ⚠️ 簡化實作 | 需改進 |
| Quirk 處理 | ✅ 正確 | ⚠️ Key 名稱不一致 | 需修改 |
| Device 生命週期 | ✅ FuDeviceClass 鉤子 | ⚠️ FuPluginClass 鉤子 | 需修改 |

---

## ✅ 符合 elantp 實作的部分

### 1. Plugin 基本結構
- ✅ 使用 `G_DEFINE_TYPE` 正確声明类型
- ✅ Plugin class 继承 `FU_PLUGIN_CLASS`
- ✅ 注册 protocol ID (`com.company.i2c-hid`)
- ✅ 包含必要的 quirk key 定义

### 2. Device 基本属性
- ✅ 有 `ic_type`、`module_id`、`i2c_slave_addr`、`iap_password`、`page_count`、`block_size` 等属性
- ✅ 使用 `FU_DEVICE_FLAG_UPDATABLE`、`FU_DEVICE_FLAG_NEEDS_BOOTLOADER` 等标志
- ✅ 支持多层级 GUID 生成 (ICTYPE, MOD, DRIVER)

### 3. I2C 通信基本框架
- ✅ 使用标准 I2C ioctl (`I2C_RDWR`)
- ✅ 实现分块写入逻辑
- ✅ 有 IAP password 机制

### 4. Quirk 文件格式
- ✅ 使用 `[section]` 格式定义设备
- ✅ 包含 `Plugin`、`DeviceInstanceId`、`I2cSlaveAddress` 等字段

---

## ⚠️ 需要調整的部分

### 1. Plugin 枚舉方式 (Critical)

**elantp 實作：**
```c
// 插件构造函数中
fu_plugin_add_udev_subsystem(plugin, "i2c");
fu_plugin_add_udev_subsystem(plugin, "i2c-dev");
fu_plugin_add_udev_subsystem(plugin, "hidraw");
fu_plugin_add_device_gtype(plugin, FU_TYPE_ELANTP_I2C_DEVICE);
fu_plugin_add_device_gtype(plugin, FU_TYPE_ELANTP_HID_DEVICE);
```

**company-i2c-hid 實作：**
```c
// 错误：在 coldplug 中手动创建设备
fu_plugin_add_device_gtype(plugin, FU_TYPE_COMPANY_I2C_HID_DEVICE);
// 并在 coldplug 中手动创建设备对象...
```

**問題：** elantp 使用 udev 子系统自动枚举设备，而不是手动 coldplug

**調整方向：**
1. 移除或简化 coldplug 中的手动设备创建
2. 使用 `fu_plugin_add_udev_subsystem()` 注册需要监听的子系统
3. 设备类型应该通过 `GType` 和 quirk 文件自动匹配

### 2. Device 基類選擇 (Critical)

**elantp 實作：**
- HID 设备：`G_DEFINE_TYPE(FuElantpHidDevice, fu_elantp_hid_device, FU_TYPE_HIDRAW_DEVICE)`
- I2C 设备：`G_DEFINE_TYPE(FuElantpI2cDevice, fu_elantp_i2c_device, FU_TYPE_I2C_DEVICE)`

**company-i2c-hid 實作：**
```c
G_DEFINE_TYPE(FuCompanyI2cHidDevice, fu_company_i2c_hid_device, FU_TYPE_DEVICE)
```

**問題：** 直接继承 `FU_TYPE_DEVICE` 而不是 `FU_TYPE_HIDRAW_DEVICE` 或 `FU_TYPE_I2C_DEVICE`

**調整方向：**
- HID 模式设备应继承 `FU_TYPE_HIDRAW_DEVICE`
- I2C 模式设备应继承 `FU_TYPE_I2C_DEVICE`
- 或者保持 `FU_TYPE_DEVICE` 但实现正确的 `probe` 和 `open` 方法

### 3. Device 生命週期鉤子位置 (Critical)

**elantp 實作：** (在 FuDeviceClass 中)
```c
device_class->probe = fu_elantp_i2c_device_probe;
device_class->open = fu_elantp_i2c_device_open;
device_class->setup = fu_elantp_i2c_device_setup;
device_class->reload = fu_elantp_i2c_device_setup;
device_class->attach = fu_elantp_i2c_device_attach;
device_class->detach = fu_elantp_i2c_device_detach;  // 注意: detach 是 device 方法
device_class->write_firmware = fu_elantp_i2c_device_write_firmware;
device_class->check_firmware = fu_elantp_i2c_device_check_firmware;
```

**company-i2c-hid 實作：** (在 FuPluginClass 中)
```c
plugin_class->device_prepare = fu_company_i2c_hid_plugin_device_prepare;
plugin_class->device_attach = fu_company_i2c_hid_plugin_device_attach;
plugin_class->device_detach = fu_company_i2c_hid_plugin_device_detach;
plugin_class->write_firmware = fu_company_i2c_hid_plugin_write_firmware;
```

**問題：** 
- elantp 的 `detach` 是 FuDeviceClass 方法，不是 FuPluginClass 方法
- elantp 使用 `device_class->detach` 而不是 `plugin_class->device_detach`

**調整方向：**
- 将 `detach` 实现移到 FuDeviceClass
- 使用 device 级别的 `setup`、`reload` 等方法

### 4. HID 通信未實作 (Critical)

**company-i2c-hid 實作：**
```c
static gboolean
fu_company_i2c_hid_hid_write(FuCompanyI2cHidDevice *self, ...)
{
    // 返回錯誤，表示未實作
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED,
                "HID write not implemented - use I2C fallback");
    return FALSE;
}
```

**問題：** HID 写入/读取完全未实现，总是回退到 I2C

**調整方向：**
- 使用 `fu_hidraw_device_set_feature()` 和 `fu_hidraw_device_get_feature()` 实现 HID 通信
- 参考 elantp 的 `fu_elantp_hid_device_read_cmd()` 和 `fu_elantp_hid_device_write_cmd()`

### 5. Quirk Key 名稱不一致

**elantp quirk keys：**
- `ElantpI2cTargetAddress`
- `ElantpIapPassword`
- `ElantpIcPageCount`

**company-i2c-hid quirk keys：**
- `CompanyI2cHidIcType`
- `CompanyI2cHidModuleId`
- `CompanyI2cHidIapPassword`
- `CompanyI2cHidPageCount`
- `CompanyI2cHidBlockSize`
- `QuirkCompanyI2cHidDriver` (前缀不一致)

**問題：** 命名风格不统一

**調整方向：**
- 统一使用 `CompanyI2cHid` 前缀
- 修正 `QuirkCompanyI2cHidDriver` 为 `CompanyI2cHidDriver`

---

## ❌ 錯誤需要修正的部分

### 1. Plugin 缺少 udev 子系統註冊

**錯誤：**
```c
// company-i2c-hid-plugin.c - constructed() 中
// 缺少 fu_plugin_add_udev_subsystem() 調用
```

**修正：**
```c
static void
fu_company_i2c_hid_plugin_constructed(GObject *obj)
{
    FuPlugin *plugin = FU_PLUGIN(obj);
    FuContext *ctx = fu_plugin_get_context(plugin);
    
    /* 註冊感興趣的 udev 子系統 */
    fu_plugin_add_udev_subsystem(plugin, "hidraw");
    fu_plugin_add_udev_subsystem(plugin, "i2c-dev");
    
    /* 註冊 firmware 和 device 類型 */
    fu_plugin_add_firmware_gtype(plugin, FU_TYPE_COMPANY_I2C_HID_FIRMWARE);
    fu_plugin_add_device_gtype(plugin, FU_TYPE_COMPANY_I2C_HID_DEVICE);
    
    /* 鏈到父類 */
    G_OBJECT_CLASS(fu_company_i2c_hid_plugin_parent_class)->constructed(obj);
}
```

### 2. Firmware parsing 中的變數名錯誤

**錯誤：** (fu-company-i2c-hid-firmware.c)
```c
checksum = fu_common_read_uint32(&fw_data[offset + FU_COMPANY_I2C_HID_FIRMWARE_checksum_OFFSET], G_BIG_ENDIAN);
//                                                          ^^^^^^^ 大寫
```

**修正：**
```c
checksum = fu_common_read_uint32(&fw_data[offset + FU_COMPANY_I2C_HID_FIRMWARE_CHECKSUM_OFFSET], G_BIG_ENDIAN);
//                                                          ^^^^^^^ 大寫
```

### 3. Quirk 檔案中的實例 ID 格式不一致

**錯誤：** (fu-company-i2c-hid.quirk)
```ini
[company-i2c-hid-generic]
DeviceInstanceId=HIDRAW\VEN_COMPANY&PID_0001
DeviceInstanceId=HID\VEN_COMPANY&PID_0001
```

**問題：** elantp 使用 `HIDRAW\VEN_04F3&DEV_3010` 格式，但公司使用 `HIDRAW\VEN_COMPANY&PID_0001`

**調整方向：**
- 確認公司的 DeviceInstanceId 格式
- 建議使用 `COMPANY_I2C\ICTYPE_XX` 格式與 elantp 一致

### 4. Plugin init 中的錯誤

**錯誤：**
```c
static void
fu_company_i2c_hid_plugin_init(FuCompanyI2cHidPlugin *self)
{
    FuCompanyI2cHidPluginPrivate *priv = fu_company_i2c_hid_plugin_get_instance_private(self);
    self->priv = priv;
    
    // ...
    
    fu_plugin_add_device_gtype(plugin_class, FU_TYPE_COMPANY_I2C_HID_DEVICE);  // ❌ 這裡應該在 constructed 中
}
```

**問題：** `plugin_class` 在 `init` 中不可用，應在 `constructed` 中調用

### 5. I2C device open 方式錯誤

**錯誤：**
```c
// company-i2c-hid 尝试打开 I2C 设备
self->fd_i2c = open("/dev/i2c-0", O_RDWR);  // ❌ 硬编码总线编号
```

**elantp 實作：**
```c
// elantp-i2c-device.c
static gboolean
fu_elantp_i2c_device_open(FuDevice *device, GError **error)
{
    FuElantpI2cDevice *self = FU_ELANTP_I2C_DEVICE(device);
    
    /* FuUdevDevice->open */
    if (!FU_DEVICE_CLASS(fu_elantp_i2c_device_parent_class)->open(device, error))
        return FALSE;
    
    /* set target address */
    if (!fu_i2c_device_set_address(FU_I2C_DEVICE(self), self->i2c_addr, TRUE, error))
        return FALSE;
    
    // ...
}
```

**調整方向：**
- 使用 `FuI2cDevice` 基类和 `fu_i2c_device_set_address()` 而不是手动 open

---

## 📊 修正優先級

| 優先級 | 項目 | 嚴重程度 |
|--------|------|----------|
| P0 | Plugin 缺少 udev 子系統註冊 | Critical - 設備無法枚舉 |
| P0 | HID 通信完全未實作 | Critical - 只能使用 I2C fallback |
| P1 | Device 基類選擇錯誤 | High - 影響設備通信 |
| P1 | Device 生命週期鉤子位置錯誤 | High - 架構不符合規範 |
| P2 | Quirk key 名稱不一致 | Medium - 配置匹配可能失敗 |
| P2 | Firmware parsing 變數名錯誤 | Medium - 編譯可能失敗 |
| P3 | I2C device open 方式 | Low - 已有 workaround |

---

## 📝 修正清單

- [x] 識別所有不符合 elantp 實作的部分
- [ ] 修正 plugin 結構（添加 udev 子系統註冊）
- [ ] 實現 HID 通信功能
- [ ] 調整 Device 類型繼承
- [ ] 修正 quirk key 名稱
- [ ] 修正 firmware parsing 變數名
- [ ] 更新 WORKLOG.md 並 commit

---

## 🔗 參考文檔

- [fwupd elantp plugin](https://github.com/fwupd/fwupd/tree/main/plugins/elantp)
- [FWUPD Plugin 開發文檔](https://github.com/fwupd/fwupd/blob/main/docs/building-plugin.md)
- [Plugin 類型參考](https://github.com/fwupd/fwupd/tree/main/docs/plugin-types.md)

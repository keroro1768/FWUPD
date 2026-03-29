# FWUPD Plugin 開發指南（適用於 HID I2C 裝置）

## 1. 前置準備

### 必備知識
- **C 語言**（fwupd 本身使用 C + GLib/GObject）
- **GLib/GObject** 基礎（理解 GError、GObject 繼承機制）
- **Git 和 GitHub** 操作（pull request 流程）
- **Linux 系統**（udev、/dev、I2C 設備節點）

### 開發環境建置

```bash
# Ubuntu/Debian
sudo apt install build-essential git meson ninja-build libglib2.0-dev \
  libusb-1.0-0-dev libudev-dev libcurl4-openssl-dev libgcab-1.0-dev \
  libfwupd-dev

# 取得 fwupd 原始碼
git clone https://github.com/fwupd/fwupd.git
cd fwupd
meson setup build
ninja -C build
```

## 2. Plugin 目錄結構

在 `fwupd/plugins/` 下建立自己的 plugin 目錄：

```
plugins/
└── company-i2c-hid/           ← 建議目錄名稱
    ├── meson.build            ← 編譯設定
    ├── fu-company-i2c-hid-plugin.h
    ├── fu-company-i2c-hid-plugin.c
    ├── fu-company-i2c-hid-device.h
    ├── fu-company-i2c-hid-device.c
    ├── fu-company-i2c-hid-firmware.h  (視需要)
    ├── fu-company-i2c-hid-firmware.c   (視需要)
    ├── fu-company-i2c-hid-quirki2c.quirk
    └── README.md
```

### 2.1 meson.build 範例

```meson
plugin_define('company-i2c-hid', sources: [
  'fu-company-i2c-hid-plugin.c',
  'fu-company-i2c-hid-device.c',
], dependencies: [
  dependency('libfwupd', version: '>= 1.0.0'),
  dependency('libusb-1.0'),
  dependency('libudev'),
])

# 加入到 plugins 設定
fwupd_src_plugins = [
  ...
  'company-i2c-hid',
]
```

## 3. Plugin 核心實作

### 3.1 Plugin 標頭檔 (fu-company-i2c-hid-plugin.h)

```c
#pragma once

#include <fwupdplugin.h>

/* Plugin 自己的資料結構（可選） */
typedef struct {
  FuPlugin *plugin;
  gpointer  proxy;        /* 若需要與其他子系統溝通 */
} FuCompanyI2cHidPluginData;

G_DECLARE_FINAL_TYPE(FuCompanyI2cHidPlugin,
                      fu_company_i2c_hid_plugin,
                      FU, COMPANY_I2C_HID_PLUGIN,
                      FuPlugin)
```

### 3.2 Plugin 實作檔 (fu-company-i2c-hid-plugin.c)

```c
#include "config.h"
#include "fu-company-i2c-hid-plugin.h"

struct _FuCompanyI2cHidPlugin {
  FuPlugin parent_instance;
  gpointer proxy;
};

G_DEFINE_TYPE(FuCompanyI2cHidPlugin,
               fu_company_i2c_hid_plugin,
               FU_TYPE_PLUGIN)

/*--------------------------------------------------------------
 * coldplug: 枚舉系統中符合條件的 HID I2C 裝置
 *--------------------------------------------------------------*/
static gboolean
fu_company_i2c_hid_plugin_coldplug(FuPlugin *plugin,
                                    FuProgress *progress,
                                    GError **error)
{
  g_autoptr(FuDevice) dev = NULL;
  /* 1. 嘗試開啟 I2C 節點或 hidraw 節點 */
  /* 2. 讀取 VID, PID, 韌體版本 */
  /* 3. 產生 GUID */
  /* 4. 註冊裝置 */
  dev = fu_device_new(NULL);
  fu_device_set_id(dev, "CompanyI2CHid-1:2:3");
  fu_device_add_instance_id(dev, "HIDRAW\\VEN_ABCD&PID_1234");
  fu_device_set_version(dev, "1.2.3");
  fu_device_add_flag(dev, FWUPD_DEVICE_FLAG_UPDATABLE);
  fu_device_set_summary(dev, "Company I2C HID Device");
  fu_device_set_plugin(plugin, "company-i2c-hid");
  fu_plugin_add_device(plugin, dev);
  return TRUE;
}

/*--------------------------------------------------------------
 * write_firmware: 將韌體寫入裝置
 *--------------------------------------------------------------*/
static gboolean
fu_company_i2c_hid_plugin_write_firmware(FuPlugin *plugin,
                                         FuDevice *dev,
                                         GBytes *blob_fw,
                                         FuProgress *progress,
                                         FwupdInstallFlags flags,
                                         GError **error)
{
  gsize sz = 0;
  guint8 *buf = g_bytes_get_data(blob_fw, &sz);

  /* 1. 將裝置切換到 bootloader 模式 (detach) */
  /* 2. 透過 I2C 或 HID 介面分塊寫入韌體 */
  /*    - 使用 fu_progress_set_steps() 分階段 */
  /*    - 使用 fu_device_write_i2c() 或 hidraw write */
  /* 3. 驗證寫入成功 */
  /* 4. 重置裝置 */

  return TRUE;
}

/*--------------------------------------------------------------
 * attach / detach: 處理 bootloader 模式切換
 *--------------------------------------------------------------*/
static gboolean
fu_company_i2c_hid_plugin_detach(FuPlugin *plugin,
                                  FuDevice *device,
                                  FuProgress *progress,
                                  GError **error)
{
  /* 發送 HID feature report 或寫入 I2C register
   * 使裝置進入 bootloader 模式 */
  return TRUE;
}

static gboolean
fu_company_i2c_hid_plugin_attach(FuPlugin *plugin,
                                  FuDevice *device,
                                  FuProgress *progress,
                                  GError **error)
{
  /* 重置裝置，使其回到 runtime 模式 */
  return TRUE;
}

static gboolean
fu_company_i2c_hid_plugin_reload(FuPlugin *plugin,
                                  FuDevice *device,
                                  GError **error)
{
  g_autofree gchar *version = NULL;
  /* 重新讀取新韌體版本 */
  version = fu_company_i2c_hid_get_version(device, error);
  if (version == NULL)
    return FALSE;
  fu_device_set_version(device, version);
  return TRUE;
}

/*--------------------------------------------------------------
 * Class Init: 註冊所有 vfuncs
 *--------------------------------------------------------------*/
static void
fu_company_i2c_hid_plugin_class_init(FuCompanyI2cHidPluginClass *klass)
{
  FuPluginClass *plugin_class = FU_PLUGIN_CLASS(klass);
  plugin_class->coldplug       = fu_company_i2c_hid_plugin_coldplug;
  plugin_class->write_firmware = fu_company_i2c_hid_plugin_write_firmware;
  plugin_class->detach         = fu_company_i2c_hid_plugin_detach;
  plugin_class->attach         = fu_company_i2c_hid_plugin_attach;
  plugin_class->reload         = fu_company_i2c_hid_plugin_reload;
}

static void
fu_company_i2c_hid_plugin_init(FuCompanyI2cHidPlugin *self)
{
}
```

## 4. HID I2C 裝置的特殊考量

### 4.1 雙模式支援（HID + 原始 I2C）

HID I2C 裝置通常有兩種溝通方式：

**模式 A — HID 模式（生產模式）**：
- 透過 Linux 的 `hidraw` 節點 (`/dev/hidrawX`)
- 韌體更新速度較快
- 使用標準 HID Report Protocol

**模式 B — 原始 I2C 模式（修復模式）**：
- 透過 `/dev/i2c-X` 節點
- 當 HID 模式更新失敗（磚化）時的救命手段
- 速度較慢，但更可靠

```c
/* 若 HID 更新失敗，使用 I2C fallback recovery */
if (fu_device_has_flag(device, FWUPD_DEVICE_FLAG_WAIT_FOR_REPLUG)) {
  /* 嘗試開啟 i2c-dev 節點 */
  fd = open("/dev/i2c-0", O_RDWR);
  /* 執行 I2C 原始寫入 recovery */
}
```

**重要借鏡**：Elan TouchPad plugin (`elantp`) 完整實作了這個雙模式機制，
是其最佳實踐之一。

### 4.2 GUID 生成策略

建議為自家裝置產生多層級 GUID：

```
HIDRAW\\VEN_ABCD&PID_1234              ← 廣泛匹配（所有同型號）
HIDRAW\\VEN_ABCD&PID_1234&MOD_5678      ← 中等匹配（模組 ID）
HID\\VEN_ABCD&PID_1234&MOD_5678&PART_MCU  ← 精確匹配（IC+模組+Part）
COMPANY_I2C\\ICTYPE_XX&MOD_5678&DRIVER_HID  ← 自定義 I2C 格式
```

### 4.3 udev 規則（讓非 root 可存取）

建立 `/etc/udev/rules.d/99-company-i2c-hid.rules`：

```
# 讓 plugdev 群組的成員可以存取 hidraw 節點
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="abcd",
  MODE="0664", GROUP="plugdev", TAG+="uaccess"

# 讓 plugdev 群組的成員可以存取 I2C 節點
SUBSYSTEM=="i2c-dev", ATTRS{idVendor}=="abcd",
  MODE="0664", GROUP="plugdev", TAG+="uaccess"
```

## 5. 處理 I2C 通訊

fwupd 沒有內建高階 I2C helper，但可以使用標準 Linux I2C API：

```c
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/* 正確的 I2C 寫入方式：msgs[0].addr 必須是 I2C slave address（整數），不是路徑 */
int
fu_company_i2c_hid_i2c_write(FuDevice *device,
                              guint8 reg_addr,
                              const guint8 *data,
                              gsize len)
{
  int fd = fu_udev_device_get_fd(FU_UDEV_DEVICE(device));
  guint8 buf[256];
  struct i2c_msg msgs[1];
  struct i2c_rdwr_ioctl_data data_struct;
  guint8 slave_addr = 0x2A;  /* I2C slave address，應從 device quirk 或屬性取得 */

  /* 若使用 FuUdevDevice，可透過 udev property 或 sysfs 屬性取得 slave address */
  /* 例如：slave_addr = fu_udev_device_get_sysfs_attr_as_u8(device, "addr", NULL); */

  buf[0] = reg_addr;
  memcpy(&buf[1], data, len);

  msgs[0].addr  = slave_addr;  /* I2C slave address（__u16 整數），不是路徑！ */
  msgs[0].flags = 0;           /* write flag */
  msgs[0].len   = len + 1;
  msgs[0].buf   = buf;

  data_struct.msgs = msgs;
  data_struct.nmsgs = 1;

  if (ioctl(fd, I2C_RDWR, &data_struct) < 0) {
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_WRITE,
                "I2C write failed: %s", g_strerror(errno));
    return FALSE;
  }
  return TRUE;
}
```

更好的方式是使用 `FuUdevDevice` 作為基底類別，
它已經處理了 `udev` 節點的發現和開啟。

## 6. Firmware 格式處理

若自家裝置使用非標準二進位格式，需要 subclass `FuFirmware`：

```c
G_DECLARE_FINAL_TYPE(FuCompanyI2cHidFirmware,
                      fu_company_i2c_hid_firmware,
                      FU, COMPANY_I2C_HID_FIRMWARE,
                      FuFirmware)

struct _FuCompanyI2cHidFirmware {
  FuFirmware parent_instance;
};

G_DEFINE_TYPE(FuCompanyI2cHidFirmware,
               fu_company_i2c_hid_firmware,
               FU_TYPE_FIRMWARE)

static gboolean
fu_company_i2c_hid_firmware_parse(FuFirmware *firmware,
                                   GBytes *blob,
                                   guint64 offset,
                                   FwupdInstallFlags flags,
                                   GError **error)
{
  /* 解析自定義韌體格式 */
  /* 提取 header, body, checksum 等 */
  /* 建立 FuFirmwareImage */
  return TRUE;
}
```

## 7. Quirk 檔案

建立 `fu-company-i2c-hid-quirk.quirk` 讓硬體可以快速被匹配：

```
[i2chid]
Plugin = company_i2c_hid
DeviceInstanceId=HIDRAW\VEN_ABCD&PID_1234
DeviceInstanceId=HID\VEN_ABCD&PID_1234
I2cSlaveAddress=0x2A
VersionFormat=triplet
Summary = Company I2C HID Device

[company-i2c-hid-hwid]
QuirkCompanyI2cHidDriver=HID
QuirkCompanyI2cHidDriver=I2C
```

## 8. Plugin 開發的重點原則

1. **最小職責**：每個 plugin 只做一件事
2. **錯誤復原**：I2C flash 可能失敗，實作 fallback recovery
3. **版本追蹤**：支援 rollback（設定 `version_lowest`）
4. **跨平台溝通**：優先使用標準機制（udev、hidraw），而非私有 IOCTL
5. **貢獻上游**：Plugin 最終目標是 merge 到 fwupd 主線（LGPLv2+）

## 9. 與現有 Plugin 的對照

| 功能 | 最佳參考 Plugin | 說明 |
|---|---|---|
| HID + I2C 雙模式 | `elantp` (Elan TouchPad) | 同時支援 HID 和 I2C recovery |
| HID Protocol | `hidpp` (Logitech) | 使用 HID Report 通訊 |
| USB + I2C 混合 | `focal-fp` (Fingerprint) | I2C fallback recovery |
| 自定義 Protocol | `cros-ec` (ChromeOS EC) | 自定義 USB HID 更新協定 |
| Udev 設備發現 | `uefi-cod` (UEFI) | FuUdevDevice 使用方式 |

## 10. 提交 Plugin 到上游

流程：
1. 在 `fwupd/fwupd` GitHub 開 issue 討論設計
2. 參考 Google ChromeOS Integration Handbook 中的要求：
   - 開放原始碼 (LGPLv2+)
   - 韌體可在 LVFS 發布
   - 願意提供硬體樣本給 fwupd 維護者
3. 使用 `git формат commit` 拆分成小型 reviewable commits
4. 開 Pull Request，維護者會幫忙 code review

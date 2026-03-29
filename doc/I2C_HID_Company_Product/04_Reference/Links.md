# 重要連結

## 1. 官方文件

### FWUPD
- **主網站**: https://fwupd.org/
- **Plugin 開發教程**: https://fwupd.github.io/libfwupdplugin/tutorial.html
- **Plugin API 參考**: https://fwupd.github.io/libfwupdplugin/
- **fwupd GitHub**: https://github.com/fwupd/fwupd
- **Plugin 原始碼**: https://github.com/fwupd/fwupd/tree/main/plugins
- **fwupd 開發者 mailing list**: https://lists.freedesktop.org/mailman/listinfo/fwupd-devel

### LVFS
- **LVFS 主站**: https://fwupd.org/lvfs/
- **LVFS 文件**: https://lvfs.readthedocs.io/
- **LVFS 上傳指南**: https://lvfs.readthedocs.io/en/latest/upload.html
- **LVFS Metadata 指南**: https://lvfs.readthedocs.io/en/latest/metainfo.html
- **LVFS 申請帳號**: https://lvfs.readthedocs.io/en/latest/apply.html
- **LVFS 離線部署**: https://lvfs.readthedocs.io/en/latest/offline.html
- **LVFS ChromeOS 測試**: https://lvfs.readthedocs.io/en/latest/chromeos.html
- **LVFS Moblab 測試**: https://lvfs.readthedocs.io/en/latest/moblab.html

### Google ChromeOS Integration
- **ChromeOS FWUPD Integration Handbook**: https://developers.google.com/chromeos/peripherals/fwupd_integration_handbook
- **ChromeOS 第三方 fwupd**: https://chromium.googlesource.com/chromiumos/third_party/fwupd/

## 2. Plugin 範例原始碼

| Plugin | GitHub 路徑 | 用途 |
|---|---|---|
| Elan TouchPad | `fwupd/fwupd/tree/main/plugins/elantp` | HID I2C + I2C fallback (最重要參考) |
| Focal Point FP | `fwupd/fwupd/tree/main/plugins/focal-fp` | Fingerprint, I2C recovery |
| ChromeOS EC | `fwupd/fwupd/tree/main/plugins/cros-ec` | 自定義 HID protocol |
| Logitech HID++ | `fwupd/fwupd/tree/main/plugins/logitech-hidpp` | HID Report 通訊 |
| USB DFU | `fwupd/fwupd/tree/main/plugins/dfu` | 標準 DFU 協定 |
| UEFI Capsule | `fwupd/fwupd/tree/main/plugins/uefi-codec` | UEFI 更新 |

## 3. Linux Kernel 文件

- **HID I2C 子系統**: `Documentation/hid/i2c-hid.rst` (在 kernel 原始碼中)
- **HID 子系統整體**: `Documentation/hid/` (在 kernel 原始碼中)
- **Linux I2C 子系統**: `Documentation/i2c/` (在 kernel 原始碼中)

## 4. HID I2C 協定標準

- **HID over I2C Protocol Specification** (Microsoft): https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/hid-over-i2c-guide
- **HID Descriptor Tool** (Microsoft): 用於驗證 HID Report Descriptor
- **USB-IF HID 規範**: https://www.usb.org/document-library/device-class-definition-hid-111
- **HID Usage Tables**: https://www.usb.org/document-library/hid-usage-tables-112

## 5. Windows 驅動程式開發

- **HID I2C Windows 驅動程式**: https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/
- **HIDClient 範例 (UMDF 2)**: https://github.com/microsoft/Windows-driver-samples/tree/main/hid/HIDClient
- **I2C HID 故障排除**: https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/troubleshoot-hid-over-i2c-device-issues

## 6. 工具下載

### Linux
- **gcab** (Cabinet archive tool): `sudo apt install gcab`
- **fwupd** (daemon + fwupdmgr): `sudo apt install fwupd`
- **meson + ninja** (build system): `sudo apt install meson ninja-build`

### Windows
- **makecab.exe**: Windows 內建（無需安裝）
- **fwupd for Windows**: https://github.com/fwupd/fwupd (可 cross-compile)

### LVFS 工具
- **sync-pulp.py**: https://gitlab.com/fwupd/lvfs-website/-/raw/master/contrib/sync-pulp.py
- **local.py** (建立 local remote metadata): https://gitlab.com/fwupd/lvfs-website/-/blob/master/local.py

## 7. 安全與漏洞相關

- **LVFS 安全文件**: https://lvfs.readthedocs.io/en/latest/security.html
- **fwupd Security Announcement**: https://github.com/fwupd/fwupd/releases
- **CVE Database**: https://cve.mitre.org/

## 8. 申請 LVFS 帳號所需的 Template

- **上傳許可文件範本**: https://gitlab.com/fwupd/lvfs-website/-/raw/master/docs/upload-permission.doc
- **LVFS 隱私報告**: https://lvfs.readthedocs.io/en/latest/privacy.html

## 9. DeepWiki (fwupd 架構分析)

- **fwupd 架構**: https://deepwiki.com/fwupd/fwupd/
- **Device-Specific Plugins**: https://deepwiki.com/fwupd/fwupd/4-device-specific-plugins

## 10. 版本格式參考

LVFS 支援的版本格式：
- **triplet**: `1.2.3` (最常用)
- **quad**: `1.2.3.4` (Intel ME)
- **intel-me**: Intel ME 專用
- **bcd**: `0x1234`
- **decimal**: 小數格式

詳見：https://fwupd.org/lvfs/docs/metainfo/version

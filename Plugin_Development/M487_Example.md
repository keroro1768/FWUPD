# Nuvoton M487 FWUPD Plugin 實作分析

## M487 微控制器概覽

### 晶片基本資訊

| 項目 | 規格 |
|------|------|
| **核心** | ARM Cortex-M4 |
| **時脈** | 最大 192 MHz |
| **Flash** | 512 KB（雙bank可選） |
| **SRAM** | 160 KB |
| **USB** | USB 2.0 OTG（內建 PHY） |
| **VID/PID** | VID=0x0416（量產預設）|
| **封裝** | LQFP-64 |
| **開發板** | M487 ETH USE（NuMaker）|

### M487 啟動模式

M487 具備多種啟動模式選擇：

| 模式 | 說明 | 用途 |
|------|------|------|
| **USB Download** | 透過 USB 进入 bootloader 下载韌體 | 量产/调试 |
| **UART Download** | 透過 UART 进入 bootloader | 無USB時 |
| **Flash Boot** | 從內部 Flash 正常啟動 | 正常運行 |
| **SPIM Boot** | 從外部 SPI Flash 加載 | 外部儲存 |

進入 USB Download 模式的方式：
1. 將 **PA2 (USB D+)** 拉低後復位晶片
2. 或使用 ISP（In-System Programming）工具點擊進入

---

## M487 FWUPD 支援策略

### 目標

讓 M487 系列晶片能夠透過標準的 `fwupdmgr` 工具進行韌體更新，無需專屬工具。

### 實現層級

```
┌─────────────────────────────────────────────────────┐
│  應用層：fwupdmgr CLI 或 GNOME Software             │
├─────────────────────────────────────────────────────┤
│  Daemon 層：fwupd daemon                            │
│     └─ Plugin Manager → Nuvoton M487 Plugin         │
├─────────────────────────────────────────────────────┤
│  硬體存取層：                                       │
│     ├─ USB HID（USB Download 模式）                 │
│     ├─ USB Bulk（UART-to-USB Bridge）                │
│     └─ 客製序列協定（取決於產品設計）                │
├─────────────────────────────────────────────────────┤
│  M487 硬體                                          │
│     └─ Boot ROM / User Flash                        │
└─────────────────────────────────────────────────────┘
```

### Plugin 類型選擇

M487 適合使用 **Desktop-style Plugin**（亦可作為 Emulated Plugin）：

- ✅ 可在系統運行時進入 USB Download 模式燒錄
- ✅ 燒錄期間裝置停止運作（可接受）
- ✅ 無需開機搶佔（不需要 Boot-time）
- ✅ 有標準 USB 介面（易於整合）

---

## M487 USB Download Protocol

### 通訊協定概述

M487 USB Download 模式使用 USB HID 或 USB Bulk 端點傳輸資料。Nuvoton 提供參考協定文件，通常包含以下指令：

| 指令 | 功能 | 方向 |
|------|------|------|
| `CMD_GET_DEVICE_INFO` | 取得晶片型號、版本 | Host → Device |
| `CMD_ERASE_FLASH` | 擦除指定範圍 | Host → Device |
| `CMD_PROGRAM_FLASH` | 燒錄資料到 Flash | Host → Device |
| `CMD_VERIFY_FLASH` | 驗證燒錄結果 | Host → Device |
| `CMD_RESET` | 重啟晶片 | Host → Device |

### VID/PID 配置

出廠預設：
- **VID**: `0x0416`（Nuvoton）
- **PID**: `0xC487`（M487 USB Bootloader）

> ⚠️ 量產時建議客製化 VID/PID 以便區分不同產品，可在 plugin 中添加多個 ID 組合。

---

## Plugin 實作架構

### 目錄結構

```
src/plugins/nuvoton_m487/
├── meson.build
├── fu-nuvoton-m487-device.c
├── fu-nuvoton-m487-device.h
├── fu-nuvoton-m487-plugin.c       ← 主要邏輯
├── fu-nuvoton-m487-plugin.h
└── fu-nuvoton-m487-firmware.c     ← 韌體格式解析
```

### 核心程式碼分析

#### 1. Plugin 初始化

```c
#include "fu-nuvoton-m487-plugin.h"
#include "fu-nuvoton-m487-device.h"

struct _FuNuvotonM487Plugin {
    FuPlugin parent;
    guint16 vid;
    guint16 pid;
};

static void
fu_nuvoton_m487_plugin_init (FuNuvotonM487Plugin *self)
{
    fu_plugin_set_build_hash (FU_PLUGIN (self), BUILD_HASH);
    fu_plugin_add_flag (FU_PLUGIN (self), FWUPD_PLUGIN_FLAG_NONE);
    fu_plugin_add_device_gtype (FU_PLUGIN (self), FU_TYPE_NUVOTON_M487_DEVICE);
}
```

#### 2. 裝置探測（device_probe）

```c
static gboolean
fu_nuvoton_m487_plugin_device_probe (FuPlugin *plugin, FuDevice *device, GError **error)
{
    // 檢查 VID
    if (fu_device_get_vid (device) != 0x0416) {
        g_set_error_literal (error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED,
                             "not a Nuvoton device");
        return FALSE;
    }
    
    guint16 pid = fu_device_get_pid (device);
    if (pid != 0xc487 && pid != 0xb487) {
        g_set_error_literal (error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED,
                             "not a supported M487 variant");
        return FALSE;
    }
    
    // 設定識別資訊
    fu_device_add_guid (device, "ae4e3d10-05c8-5b5e-ae8e-0416c0c487");
    fu_device_set_name (device, "Nuvoton M487 Bootloader");
    fu_device_set_summary (device, "M487 series USB bootloader mode");
    fu_device_set_firmware_size (device, 512 * 1024);
    fu_device_add_icon (device, "computer");
    
    // 標誌：需要使用者中斷連接
    fu_device_add_flag (device, FWUPD_DEVICE_FLAG_UPDATABLE);
    fu_device_add_flag (device, FWUPD_DEVICE_FLAG_NEEDS_ATTACH);
    
    return TRUE;
}
```

#### 3. detach（進入更新模式）

```c
static gboolean
fu_nuvoton_m487_plugin_detach (FuPlugin *plugin, FuDevice *device, GError **error)
{
    g_autoptr(FuUsbDevice) usb_device = FU_USB_DEVICE (device);
    FuNuvotonM487Plugin *self = FU_NUVOTON_M487_PLUGIN (plugin);
    
    // 嘗試發送進入 bootloader 指令
    // 如果已經在 bootloader 模式，這步可能直接成功
    guint8 cmd[] = { 0x41, 0x76, 0x01, 0x01 };  // Nuvoton bootloader magic
    
    if (!fu_usb_device_control_transfer (usb_device,
                                          FU_USB_DIRECTION_HOST_TO_DEVICE,
                                          FU_USB_REQUEST_TYPE_VENDOR,
                                          0x01, 0x00, 0x00,
                                          cmd, sizeof (cmd),
                                          NULL, 1000, error)) {
        // 可能需要手動操作：長按 boot pin + 復位
        fu_device_add_flag (device, FWUPD_DEVICE_FLAG_NEEDS_USER_ACTION);
        g_set_error_literal (error, FWUPD_ERROR, FWUPD_ERROR_NEEDS_USER_ACTION,
                             "Please unplug and reconnect while holding BOOT pin");
        return FALSE;
    }
    
    // 等待重新枚舉（Bootloader 會以新 PID 出現）
    g_usleep (1000000);
    
    return TRUE;
}
```

#### 4. write_firmware（燒錄韌體）

```c
static gboolean
fu_nuvoton_m487_plugin_write_firmware (FuPlugin *plugin,
                                        FuDevice *device,
                                        GBytes *blob_fw,
                                        FwupdInstallFlags flags,
                                        GError **error)
{
    FuUsbDevice *usb_device = FU_USB_DEVICE (device);
    const guint8 *data = g_bytes_get_data (blob_fw, NULL);
    gsize len = g_bytes_get_size (blob_fw);
    guint32 address = 0x00000000;  // M487 Flash 起始地址
    
    // 步驟 1：擦除
    g_debug ("Erasing %" G_GSIZE_FORMAT " bytes...", len);
    if (!fu_nuvoton_m487_send_command (usb_device,
                                        M487_CMD_ERASE,
                                        address, len, error))
        return FALSE;
    
    // 步驟 2：燒錄（分塊）
    const guint page_size = 2048;  // M487 Page Size = 2KB
    guint32 offset = 0;
    
    while (offset < len) {
        guint32 chunk = MIN (page_size, len - offset);
        
        if (!fu_nuvoton_m487_send_command (usb_device,
                                            M487_CMD_PROGRAM,
                                            address + offset,
                                            data + offset,
                                            chunk,
                                            error))
            return FALSE;
        
        offset += chunk;
        fu_plugin_set_progress_full (plugin, offset, len);
    }
    
    // 步驟 3：驗證
    g_debug ("Verifying...");
    if (!fu_nuvoton_m487_verify (usb_device, address, data, len, error))
        return FALSE;
    
    return TRUE;
}
```

#### 5. attach（離開更新模式）

```c
static gboolean
fu_nuvoton_m487_plugin_attach (FuPlugin *plugin, FuDevice *device, GError **error)
{
    FuUsbDevice *usb_device = FU_USB_DEVICE (device);
    
    // 發送復位指令，離開 bootloader 並進入正常模式
    if (!fu_nuvoton_m487_send_command (usb_device,
                                        M487_CMD_RESET,
                                        0, 0, error))
        return FALSE;
    
    // 等待重新枚舉為正常 PID
    g_usleep (2000000);
    
    return TRUE;
}
```

### 指令傳輸輔助函式

```c
static gboolean
fu_nuvoton_m487_send_command (FuUsbDevice *usb_device,
                                guint8 cmd,
                                guint32 addr,
                                const guint8 *data,
                                guint32 data_len,
                                GError **error)
{
    guint8 buf[64] = { 0 };
    guint8 reply[64] = { 0 };
    
    // 封裝指令格式（依據 M487 USB Protocol）
    buf[0] = 0x4E;  // Nuvoton Sync
    buf[1] = cmd;   // 命令碼
    buf[2] = (addr >> 0) & 0xFF;
    buf[3] = (addr >> 8) & 0xFF;
    buf[4] = (addr >> 16) & 0xFF;
    buf[5] = (addr >> 24) & 0xFF;
    buf[6] = (data_len >> 0) & 0xFF;
    buf[7] = (data_len >> 8) & 0xFF;
    
    if (data && data_len > 0)
        memcpy (&buf[8], data, MIN (data_len, 56));  // 64-8=56 bytes max per transfer
    
    // USB 控制傳輸
    if (!fu_usb_device_control_transfer (usb_device,
                                          FU_USB_DIRECTION_HOST_TO_DEVICE,
                                          FU_USB_REQUEST_TYPE_VENDOR,
                                          cmd, 0, 0,
                                          buf, sizeof (buf),
                                          reply, sizeof (reply),
                                          5000, error))
        return FALSE;
    
    // 檢查回覆狀態
    if (reply[0] != 0x00) {
        g_set_error (error, FWUPD_ERROR, FWUPD_ERROR_WRITE,
                     "device returned error 0x%02x", reply[0]);
        return FALSE;
    }
    
    return TRUE;
}
```

---

## 韌體格式（Firmware Blob）

### 預期格式

M487 FWUPD 插件預期的韌體 blob 為原始二進制格式（`.bin`），不做特殊封裝。

```
┌──────────────────────────────────────┐
│   M487 Firmware Binary (.bin)        │
│                                      │
│   Offset 0x00000 ───┐                │
│                     │                │
│   燒錄到 Flash      │                │
│                     │                │
│   Offset 0x7FFFF ───┘                │
│                                      │
└──────────────────────────────────────┘
```

在 metainfo.xml 中的對應設定：

```xml
<firmware type="flashed">
  <release version="1.2.0">
    <checksum type="sha256" target="content" filename="m487_fw.bin">
      <!-- 自動計算 -->
    </checksum>
    <location filename="m487_fw.bin"/>
  </release>
</firmware>
```

### 雙 Bank 支援（可選）

M487 部分型號支援雙 Bank Flash（A/B 分區），可做無損更新：

```
Bank A (0x00000-0x7FFFF)    Bank B (0x80000-0xFFFFF)
┌─────────────────────┐    ┌─────────────────────┐
│   Running Firmware  │    │   New Firmware      │
│                     │    │   (being updated)   │
└─────────────────────┘    └─────────────────────┘
     Normal boot                Receiving update
     
更新完成後，設定下次從 Bank B 啟動，並交換。
```

---

## Quirks 配置

在 `data/quirks.d/nuvoton-m487.quirk` 中添加：

```ini
[Plugin]
Name=nuvoton-m487
Module=nuvoton_m487

[DeviceInstance]
Name=Nuvoton M487 USB Bootloader (Default PID)
Plugin=nuvoton_m487
USB\VID_0416&PID_C487
Flags=updatable|signed|needs-attach
FirmwareSize=524288
VersionFormat=triplet

[DeviceInstance]
Name=Nuvoton M487 USB Bootloader (Alt PID)
Plugin=nuvoton_m487
USB\VID_0416&PID_B487
Flags=updatable|signed|needs-attach
FirmwareSize=524288
VersionFormat=triplet
```

---

## 驗證流程

### 本地測試

```bash
# 1. 確認插件已編譯並安裝
ls /usr/lib/fwupd-2.0/plugins/ | grep nuvoton

# 2. 確認 M487 設備已進入 USB Bootloader 模式
fwupdmgr get-devices

# 3. 檢查設備詳細資訊
fwupdmgr dump --device <DEVICE_ID>

# 4. 本地安裝（非 LVFS）
fwupdmgr install --allow-reinstall --force m487_v1.2.0.cab

# 5. 查看更新歷史
fwupdmgr get-history
```

### 測試矩陣

| 測試項目 | 預期結果 |
|----------|----------|
| `get-devices` 識別到 M487 | ✅ 顯示正確名稱與 VID/PID |
| `get-updates` 找到新版本 | ✅ LVFS 或本地 CAB 解析正確 |
| 韌體燒錄成功率 | ✅ 100% 成功率 |
| 燒錄後裝置正常啟動 | ✅ 進入正常運行模式 |
| 驗證簽章 | ✅ LVFS 公鑰驗證通過 |
| 錯誤處理（中途拔除） | ✅ 正確報錯、不破壞晶片 |

---

## 與 LVFS 整合

### 上架流程

1. 完成 Plugin 開發與本地測試
2. 聯繫 LVFS 管理員（Richard Hughes）開通 Nuvoton 供應商帳號
3. 上傳 CAB 檔案（包含 metainfo.xml）
4. 填寫版本資訊、發布說明、緊急程度
5. LVFS 審核通過後，使用者即可透過 `fwupdmgr get-updates` 看到更新

### metainfo.xml 範例

```xml
<?xml version="1.0" encoding="UTF-8"?>
<component type="firmware">
  <id>com.nuvoton.m487-firmware</id>
  <name>Nuvoton M487 Series</name>
  <summary>Firmware for M487 series ARM Cortex-M4 microcontrollers</summary>
  
  <provides>
    <firmware type="flashed">ae4e3d10-05c8-5b5e-ae8e-0416c0c487</firmware>
  </provides>
  
  <description>
    <p>
      This package contains the latest bootloader and application firmware
      for the Nuvoton M487 series microcontrollers.
    </p>
    <p>This release includes:</p>
    <ul>
      <li>Security fix for CVE-2024-XXXX</li>
      <li>Improved USB enumeration stability</li>
      <li>Updated peripheral driver library</li>
    </ul>
  </description>
  
  <categories>
    <category>X-DeviceDriver</category>
  </categories>
  
  <keywords>
    <keyword>nuvoton</keyword>
    <keyword>m487</keyword>
    <keyword>firmware</keyword>
    <keyword>arm</keyword>
    <keyword>cortex-m</keyword>
  </keywords>
</component>
```

---

## 已知挑戰與解決方案

| 挑戰 | 說明 | 解決方案 |
|------|------|----------|
| **USB PID 多樣性** | 不同產品可能使用不同 PID | 在 plugin 中註冊多個 PID，或使用動態 VID/PID 匹配 |
| **沒有統一下載協定** | Nuvoton 官方可能使用 ISP 或 NuLink | 評估是否可逆向或申請官方通訊協定文件 |
| **dual-bank 不普及** | 並非所有 M487 型號都支援 | 預設單bank，告知使用者更新期間裝置會中斷 |
| **簽章金鑰管理** | 需要安全地管理供應商私鑰 | 使用 HSM 或安全的離線金鑰管理 |
| **LVFS 審核延遲** | 首次上架可能需要數天 | 提前準備完整文件，減少來回 |

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

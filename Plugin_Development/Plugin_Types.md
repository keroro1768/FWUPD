# FWUPD Plugin 類型說明

## 三種 Plugin 類型總覽

FWUPD 插件系統根據硬體特性和更新模式，分為三種主要類型：

| 類型 | 名稱 | 特點 | 典型應用 |
|------|------|------|----------|
| **Emulated Plugin** | Emulated Plugin | 在現有作業系統環境下直接讀寫硬體 | USB 裝置、EMMC、 NOR Flash |
| **Desktop-style Plugin** | Desktop Plugin | 由使用者觸發，需中斷裝置使用 | 網卡、顯示器、擴充塢 |
| **Boot-time Plugin** | Boot-time Plugin | 系統開機時搶佔式執行 | UEFI BIOS、SPI Flash |

---

## 1. Emulated Plugin（插件式模擬插件）

### 設計理念

Emulated Plugin 的「emulated」並非「模擬」之意，而是指「在作業系統環境中直接存取硬體」的插件模式。此類插件在系統正常運行時即時讀寫硬體，無需特殊的開機環境或裝置重置。

### 特性

- **無需中斷服務**：更新時裝置可保持連接狀態
- **依賴作業系統**：需要作業系統提供對應的硬體驅動程式（如 USB HID、I2C、SPI）
- **熱更新支援**：部分裝置支援在運行時更新（如 USB 網卡）
- **權限模型**：通常需要 root 權限或 D-Bus Polkit 授權

### 執行流程

```
系統運行中
    │
    ▼
Plugin.device_probe()  ──► 枚舉並識別裝置
    │
    ▼
Plugin.device_open()   ──► 開啟硬體溝通管道
    │
    ▼
Plugin.write_firmware() ──► 直接燒錄（無需離開系統）
    │
    ▼
Plugin.verify()        ──► 驗證燒錄結果
    │
    ▼
更新完成，裝置持續運作
```

### 適用硬體

- USB 裝置（透過 libusb/libhid）
- EMMC / eMMC（透過 Linux MTD 子系統）
- I2C/SPI 感測器（透過 /dev/i2c-*、/dev/spidev*）
- 藍芽控制器
- Webcam、網卡等外設

### 開發重點

```c
// Emulated Plugin 核心結構
static gboolean
fu_example_plugin_write_firmware (FuPlugin *plugin,
                                  FuDevice *device,
                                  GBytes *blob_fw,
                                  FwupdInstallFlags flags,
                                  GError **error)
{
    // 1. 讀取裝置識別
    guint16 vid = fu_usb_device_get_vid (FU_USB_DEVICE (device));
    guint16 pid = fu_usb_device_get_pid (FU_USB_DEVICE (device));

    // 2. 進入更新模式（如需要）
    if (!fu_device_has_flag (device, FWUPD_DEVICE_FLAG_NEEDS_BOOTLOADER)) {
        // 一般模式下燒錄
    }

    // 3. 分塊寫入（避免記憶體不足）
    const guint8 *data = g_bytes_get_data (blob_fw, &len);
    for (guint32 offset = 0; offset < len; offset += chunk_size) {
        if (!fu_device_write_buffer (device, offset, chunk, error))
            return FALSE;
    }

    // 4. 驗證
    return fu_device_verify (device, error);
}
```

---

## 2. Desktop-style Plugin（桌面樣式插件）

### 設計理念

Desktop-style Plugin 專為需要在更新期間「獨占裝置」的硬體設計。這類裝置平時作為標準周邊運作（由系統驅動程式管理），更新時需要進入特殊的「更新模式」，此時會暫時脫離常規功能。

### 特性

- **獨占硬體存取**：更新期間該裝置無法正常運作
- **需要使用者確認**：通常需要使用者中斷使用、等待更新完成
- **進度回饋**：需要即時進度條向使用者回饋狀態
- **更新模式切換**：需要支援 detach/attach 生命週期

### detach / attach 機制

這是 Desktop-style Plugin 的核心概念：

```c
// detach: 將裝置從正常模式切換到更新模式
static gboolean
fu_example_plugin_detach (FuPlugin *plugin, FuDevice *device, GError **error)
{
    // 發送特定指令或控制信號，使裝置進入 bootloader 模式
    // 例如：USB 重新列舉、發送 0xFF 復位指令等
    
    g_usb_device_reset (fu_usb_device_get_dev (FU_USB_DEVICE (device)));
    g_usleep (100000);  // 等待重新列舉
    
    return TRUE;
}

// attach: 將裝置從更新模式恢復到正常模式
static gboolean
fu_example_plugin_attach (FuPlugin *plugin, FuDevice *device, GError **error)
{
    // 發送離開更新模式的指令
    // 讓裝置重新以正常模式枚舉
    return TRUE;
}
```

### 執行流程

```
1. detach()
   └─ 發送控制指令，切換到更新模式
   └─ 裝置可能重新 USB 枚舉（換VID/PID）

2. write_firmware()
   └─ 在更新模式下寫入新韌體
   └─ 通常分塊傳輸

3. attach()
   └─ 發送指令讓裝置回到正常模式
   └─ 驗證新韌體是否正常啟動
```

### 適用硬體

- USB 網卡（更新無線、ethernet 控制器韌體）
- 擴充塢（DisplayPort/USB-C 控制器）
- Thunderbolt 裝置
- 高階鍵盤/滑鼠（內建韌體）
- 指紋辨識器

### 知名 Desktop-style Plugin

| Plugin 名 | 支援硬體 |
|-----------|----------|
| `synaptics_mst` | Synaptics Multi-Touch Trackpad MST 接收器 |
| `wacom_usb` | Wacom 繪圖板 |
| `dfu` | DFU 相容 USB 裝置 |
| `dfu-util` | 開源 DFU 工具对应装置 |

---

## 3. Boot-time Plugin（開機插件）

### 設計理念

Boot-time Plugin 的核心特點是：**在系統啟動階段搶佔執行**，在作業系統完全載入之前完成韌體更新。這是更新某些關鍵底層元件（如 UEFI BIOS、SPI Flash）的唯一安全方式。

### 特性

- **開機搶佔**：在 `fwupd` daemon 啟動的最早期執行
- **ESRT 整合**：與 UEFI ESRT（EFI System Resource Table）深度整合
- **獨立性**：不依賴標準 Linux 驅動程式，直接與 UEFI 固件服務通訊
- **高風險**：錯誤可能導致系統無法啟動，通常需要 dual-bank 保護

### ESRT 機制

ESRT（EFI System Resource Table）是 UEFI 提供的韌體資源表：

```c
// ESRT 條目結構
typedef struct {
    EFI_GUID  FwClass;           // 韌體類型 GUID
    UINT32    FwType;            // 韌體類型（0=BIOS, 1=Firmware Volume）
    UINT32    FwVersion;        // 目前韌體版本
    UINT32    LowestSupportedFwVersion; // 最低支援版本
    UINT32    CapsuleFlags;     // 更新膠囊標誌
    UINT32    LastAttemptVersion;// 上次嘗試版本
    UINT32    LastAttemptStatus; // 上次嘗試狀態
} EFI_FW_RESOURCE_ENTRY;
```

### 執行時機

Boot-time Plugin 的執行順序在所有插件中最優先：

```c
static void
fu_uefi_capsule_plugin_startup (FuPlugin *plugin)
{
    // 這個鉤子在 daemon 啟動時最早被呼叫
    // 用於讀取/寫入 ESRT 
    
    // 檢查 UEFI 環境是否可用
    if (!fu_efivar_supported (error))
        return;
    
    // 讀取 ESRT 表
    g_autoptr(GPtrArray) entries = fu_efivar_get_entries (error);
    for (guint i = 0; i < entries->len; i++) {
        FuEfivarEntry *entry = g_ptr_array_index (entries, i);
        // 處理每個 ESRT 條目
    }
}
```

### UEFI Update Capsule

UEFI 韌體更新的標準機制：

```bash
# 更新流程示意
1. fwupdmgr update
   │
   ▼
2. fwupd daemon 排程 UEFI 更新
   │
   ▼
3. 系統下次重啟時，UEFI 執行 Capsule 更新
   │
   ▼
4. 膠囊（Capsule）被載入並驗證簽章
   │
   ▼
5. UEFI fwUpdate 服務執行實際更新
   │
   ▼
6. 系統重啟，新 BIOS 生效
```

### 適用硬體

- 主機板 BIOS/UEFI 韌體
- SPI Flash 中的嵌入式控制器
- Intel ME（Management Engine）相關更新
- BMC（Baseboard Management Controller）韌體

### 知名 Boot-time Plugin

| Plugin 名 | 支援硬體 |
|-----------|----------|
| `uefi_capsule` | 所有 UEFI BIOS 更新（標準方式） |
| `uefi_pkinotes` | UEFI PKI 資料庫更新 |
| `linux_lockdown` | 核心 lockdown 模式整合 |
| `linux_suspend` | 系統睡眠/喚醒相關韌體 |

---

## 三種插件類型比較

| 特性 | Emulated Plugin | Desktop-style Plugin | Boot-time Plugin |
|------|-----------------|---------------------|-----------------|
| **執行時機** | 系統運行中 | 系統運行中（需停止使用裝置） | 開機階段/重啟時 |
| **需要重啟** | 通常不需要 | 通常不需要 | 通常需要 |
| **作業系統依賴** | 高（需要驅動程式） | 中 | 極低（UEFI 環境） |
| **風險等級** | 低 | 中 | 高 |
| **典型更新時間** | 秒～分鐘 | 分鐘 | 分鐘～十分鐘 |
| **dual-bank 保護** | 可選 | 可選 | 強烈建議 |
| **開發複雜度** | 中 | 中 | 高 |

---

## Plugin 類型選擇指南

```
選擇流程：

1. 硬體是否在系統運行時可以被直接存取？
   │
   ├── YES → 該硬體是否有正常運作的 Linux 驅動程式？
   │         │
   │         ├── YES → Emulated Plugin
   │         │
   │         └── NO → 該硬體是否支持更新模式切換（detach/attach）？
   │                   │
   │                   ├── YES → Desktop-style Plugin
   │                   │
   │                   └── NO → 該硬體是否為 UEFI/BIOS 等底層元件？
   │                               │
   │                               ├── YES → Boot-time Plugin
   │                               │
   │                               └── NO → 需要評估是否支援 FWUPD
   │
   └── NO（如需要離開作業系統才能更新）
       │
       ▼
       Boot-time Plugin
```

---

## Plugin 開發前置要求

不論類型，所有 Plugin 開發都需要：

1. **編譯環境**：
   ```bash
   # Fedora
   sudo dnf install fwupd-devel meson ninja-build
   
   # Debian/Ubuntu
   sudo apt install fwupd-dev meson ninja-build
   ```

2. **獲取 fwupd 原始碼**：
   ```bash
   git clone https://github.com/fwupd/fwupd.git
   cd fwupd
   ```

3. **了解 FuPlugin 結構**：所有插件都繼承 `FuPlugin` 結構，定義於 `src/fu-plugin.h`

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

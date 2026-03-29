# Linux FWUPD 韌體更新 — 知識庫

> 最後更新：2026-03-28
> 位置：`D:\AiWorkSpace\KM\KM-collect\FWUPD\`

---

## 📚 什麼是 FWUPD？

**FWUPD（Linux Vendor Firmware Update）** 是一個開源的 Linux 韌體更新框架，讓硬體廠商可以透過標準化的方式發布韌體更新。

### 核心組成

| 組件 | 說明 |
|------|------|
| **fwupd** | Daemon（背景服務），執行實際的韌體更新 |
| **fwupdmgr** | 命令列工具，使用者透過它查看/安裝更新 |
| **LVFS** | Linux Vendor Firmware Service，韌體資料庫網站 |
| **GNOME Software** | 圖形化介面，整合 fwupd 顯示更新 |

### 支援的傳輸層

| 傳輸 | 說明 |
|------|------|
| USB | USB DFU 協定 |
| SPI | 透過 EC/BMC |
| Network | 遠端更新（SSH/WEB）|

---

## 🏗️ 系統架構

```
┌─────────────────────────────────────────────┐
│            GNOME Software / fwupdmgr        │
└────────────────┬────────────────────────────┘
                 │ D-Bus
                 ▼
┌─────────────────────────────────────────────┐
│                 fwupd daemon                │
│  ┌─────────┐  ┌─────────┐  ┌─────────────┐  │
│  │ Plugin1 │  │ Plugin2 │  │   Plugin N   │  │
│  │  (USB) │  │  (SPI) │  │  (Custom)   │  │
│  └────┬────┘  └────┬────┘  └──────┬──────┘  │
│       │            │               │          │
└───────┼────────────┼───────────────┼──────────┘
        │            │               │
        ▼            ▼               ▼
     [Device]    [Device]       [Device]
        │            │               │
        └────────────┴───────────────┘
                     │
                     ▼
         ┌──────────────────────┐
         │   LVFS (lvfs.fwupd.org)│
         └──────────────────────┘
```

---

## 📋 Plugin 開發類型

根據 LVFS 文件，Plugin 分為 3 類：

| 類型 | 說明 | 範例 |
|------|------|------|
| **Quirk-only** | 只需編輯 quirk 檔案，無需寫程式 | 已有驅動的標準設備 |
| **Simple Plugin** | 新協定/傳輸，但有 helper 可用 | USB DFU |
| **Complex Plugin** | 完全自訂，需要完整實作 | 新 IC |

---

## 🔧 Plugin 開發流程

### 1. 分析設備協定
- 確認設備的燒錄協定
- 確認是否已有類似 Plugin 可參考
- 評估需要實作的程度

### 2. 選擇 Plugin 類型
```
設備已支援?
    │
    ├─ 是 → 只需要 quirk 檔案
    │
    └─ 否 → 評估協定複雜度
             │
             ├─ 有 helper → Simple Plugin
             │
             └─ 完全自訂 → Complex Plugin
```

### 3. 實作 Plugin
```c
// 需要的函式
fu_device_activate()      // 啟動更新模式
fu_device_write_firmware() // 寫入韌體
fu_device_dump_firmware()  // 讀取韌體
```

### 4. 建立 Firmware CAB
```xml
<!-- example.metainfo.xml -->
<?xml version="1.0" encoding="UTF-8"?>
<component type="firmware">
  <id>com.company.device.firmware</id>
  <name>Device Firmware</name>
  <summary>Firmware for Company Device</summary>
  <provides>
    <firmware type="flashed">XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX</firmware>
  </provides>
  <metadata>
    <value key="BootloaderVersion">1.0</value>
  </metadata>
</component>
```

---

## 📦 LVFS 上架流程

### 1. 申請 LVFS 帳號
https://fwupd.org/vendor

### 2. 上傳 Firmware CAB
```bash
# 使用 LVFS 網頁介面上傳
# 或使用 lvfs-cli 工具
```

### 3. 等待審核
- LVFS 管理員審核韌體格式
- 確認 metadata 正確

### 4. 發布
- 測試環境驗證 → 正式發布
- 各 Linux 發行版自動取得更新

---

## 🎯 以 M487 為例

M487 作為第一個驗證 IC：

### 評估
- M487 已支援 USB DFU（透過 BSP FMC）
- 已有 MSC + HID Interface
- 可評估建立 DFU Plugin

### 選項
1. **使用現有 USB DFU Plugin**（如果支援）
2. **建立新 Plugin**（如果協定特殊）
3. **使用 quirk**（如果只需 meta 資訊）

### 驗證步驟
```bash
# 1. 安裝 fwupd
sudo apt install fwupd

# 2. 檢查設備
fwupdmgr get-devices

# 3. 安裝更新
fwupdmgr install firmware.cab

# 4. 查看歷史
fwupdmgr history
```

---

## 📁 目錄結構

```
FWUPD/
├── README.md                    ← 本檔案
├── Basics/                     ← 基礎知識
│   ├── What_is_FWUPD.md
│   ├── Architecture.md
│   └── LVFS_Explained.md
├── Plugin_Development/          ← Plugin 開發
│   ├── Plugin_Types.md
│   ├── Development_Guide.md
│   └── M487_Example.md          ← 以 M487 為例
├── Firmware_CAB/               ← CAB 檔案製作
│   ├── CAB_Creation.md
│   └── Metainfo_XML.md
└── Deployment/                  ← 上線部署
    ├── LVFS_Upload.md
    └── Enterprise_Deployment.md
```

---

## 🔗 參考資源

| 資源 | 連結 |
|------|------|
| FWUPD 官網 | https://fwupd.org/ |
| LVFS | https://lvfs.readthedocs.io/ |
| Plugin 教程 | https://fwupd.github.io/libfwupdplugin/tutorial.html |
| FWUPD GitHub | https://github.com/fwupd/fwupd |
| ChromeOS Peripheral Guide | https://developers.google.com/chromeos/peripherals/fwupd-guide-for-peripherals |
| ArchWiki FWUPD | https://wiki.archlinux.org/title/Fwupd |

---

## ❓ 常見問題

### Q: FWUPD 支援哪些設備？
A: 官網有設備清單：https://fwupd.org/device-list

### Q: 設備已支援 USB DFU，還需要 Plugin 嗎？
A: 可能只需要 quirk 檔案即可，視情況而定

### Q: LVFS 上架需要費用嗎？
A: **完全免費**，這是 Linux Foundation 支援的開源服務

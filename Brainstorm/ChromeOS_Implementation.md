# Brainstorm 會議：Chromebook 上的 FWUPD 實作分析

> **日期：** 2026-03-30  
> **參與：** Keroro 小隊（Caro 主持）  
> **目的：** 探討 ChromeOS 如何實作 FWUPD，理解其架構以供未來參考

---

## 一、ChromeOS 韌體更新架構總覽

ChromeOS 的韌體更新採用**兩層架構**：

```
┌─────────────────────────────────────────────────────────┐
│                    ChromeOS                             │
├─────────────────────────────────────────────────────────┤
│  1. 主機韌體更新（Host/BIOS/EC）                         │
│     → chromeos-firmwareupdate（self-extracting archive） │
│     → 由 update_engine 觸發（A/B 分區更新）               │
│                                                         │
│  2. 周邊裝置韌體更新                                     │
│     → fwupd（Linux 開源 daemon）                        │
│     → 透過 LVFS 分發 .cab 檔                            │
└─────────────────────────────────────────────────────────┘
```

---

## 二、周邊裝置韌體更新（fwupd）實作方式

### 2.1 核心流程

```
周邊裝置（USB HID/MSC/其他）
        ↓
    ChromeOS fwupd daemon
        ↓
   LVFS（Linux Vendor Firmware Service）
        ↓
   OEM 上傳 .cab 檔至 LVFS
        ↓
   ChromeOS 團隊驗證 + allowlist
        ↓
   使用者可在 ChromeOS UI 看到更新通知
```

### 2.2 關鍵特性

| 特性 | 說明 |
|------|------|
| **更新觸發** | 使用者主動（不自動） |
| **通知機制** | 根據 LVFS 設定的 urgency 等級決定是否通知 |
| **驗證流程** | 需 ChromeOS 團隊单独驗證 + allowlist |
| **支援前提** | fwupd 必須已支援該周邊裝置 plugin |

### 2.3 兩種情況

**情況 A：fwupd 尚不支援該周邊**
- ODM/OEM 需與晶片廠商合作
- 提交 plugin 改動至 fwupd 程式碼庫
- 等待 fwupd 新版 release 並被 ChromeOS 採用

**情況 B：fwupd 已支援該周邊**
- 將韌體上傳至 LVFS
- 確保有 signed reports 可用
- 由 ChromeOS 團隊驗證後加入 allowlist

---

## 三、ChromeOS EC（嵌入式控制器）Plugin

### 3.1 基本資訊

fwupd 有專門的 **ChromeOS EC plugin**（`cros-ec`），用於支援 Chrome OS EC 專案裝置的韌體更新。

- **支援版本：** fwupd 1.5.0 起
- **更新方式：** USB endpoint updater（目前主要）
- **底層基於：** `chromeos-ec` 專案的 `usb_updater2` 應用程式

### 3.2 USB 更新協定

文件位置：https://chromium.googlesource.com/chromiumos/platform/ec/+/master/docs/usb_updater.md

### 3.3 韌體格式

- daemon 解壓縮 `.cab` 檔
- 取出 **fmap 格式**的韌體 blob（Google fmap 是 ChromeOS 使用的 flash map 格式）
- 詳細規格：https://www.chromium.org/chromium-os/firmware-porting-guide/fmap

### 3.4 支援的 Protocol ID

```
com.google.usb.crosec
```

### 3.5 GUID 生成

標準 USB DeviceInstanceId：
```
USB\VID_18D1&PID_501A
```

額外加入 board name：
```
USB\VID_18D1&PID_501A&BOARDNAME_gingerbread
```

### 3.6 Update Behavior（更新行為）

ChromeOS EC 裝置的特點：
- **Runtime mode：** 正常運行模式（locked）
- **Unlocked mode：** 更新時进不去 mode，USB PID 不變但功能不同
- **Attach：** 更新完成後重新連接，回到 runtime locked mode

因此使用 `REPLUG_MATCH_GUID` flag，讓 bootloader 和 runtime mode 被視為同一裝置。

### 3.7 Vendor ID

USB VID = `0x18D1`（Google）

---

## 四、主機韌體更新（chromeos-firmwareupdate）

### 4.1 架構

不同於一般 Linux，ChromeOS 的主機（AP/BIOS/EC）韌體更新是透過 **self-extracting archive** 方式執行：

```
chromeos-firmwareupdate（/usr/sbin/）
    ├── 韌體 images（RO + RW）
    ├── 更新邏輯
    └── 工具程式
```

### 4.2 更新的關鍵特性

- **Verified Boot：** 所有 Chromebook 都有 verified boot 實作，韌體必須能在背景默默更新
- **保留資料：** updater 會保留 VPD 區段（RO_VPD, RW_VPD）和 GBB 區段（如 HWID）
- **A/B 分區更新：** 由 `update_engine` 驅動，模擬 ChromeOS 自動更新流程

### 4.3 更新模式

```bash
# 查看內容（machine-friendly）
chromeos-firmwareupdate --manifest

# 人類可讀形式
chromeos-firmwareupdate -V

# recovery mode（更新 RO+RW，若 write protection 未啟用）
chromeos-firmwareupdate --mode=recovery

# 只更新 RW（write protection 已啟用）
chromeos-firmwareupdate --mode=recovery --wp=1

# 工廠模式（強制更新 RO+RW，並檢查 WP 狀態）
chromeos-firmwareupdate --mode=factory
```

---

## 五、LVFS 與 ChromeOS 的整合

### 5.1 上傳流程

```
OEM/ODM 上傳 .cab 至 LVFS
        ↓
LVFS 管理員審核
        ↓
等待 ChromeOS 團隊驗證
        ↓
加入 ChromeOS allowlist
        ↓
發布給使用者
```

### 5.2 驗證要求

- ChromeOS 團隊會单独驗證每個韌體更新
- 確保使用者體驗最佳化
- 確認與 ChromeOS 相容

### 5.3 隱私與回報

- fwupd 會鼓勵使用者回報更新成功/失敗至 LVFS
- 這是可選功能，但有助於 OEM 開發者了解更新流程效能

---

## 六、與一般 Linux 的差異

| 項目 | 一般 Linux | ChromeOS |
|------|-----------|----------|
| **更新觸發** | 可自動（fwupd-refresh.timer） | 使用者主動 |
| **LVFS 整合** | 直接可用 | 需額外 allowlist |
| **韌體格式** | 多樣 | 主要 fmap |
| **系統整合** | GNOME Software/KDE Discover | ChromeOS 原生 UI |
| **EC 更新** | 無 | chromeos-firmwareupdate |

---

## 七、對我們的啟示

### 7.1 可借鑒之處

1. **Plugin 架構：** fwupd 的 plugin 機制值得學習，我們的 M487 裝置也可考慮類似架構
2. **fmap 格式：** Google 的 flash map 格式公開，值得研究
3. **USB update protocol：** ChromeOS EC 的 USB 更新協定適用於嵌入式裝置
4. **兩層更新架構：** 主機韌體 + 周邊韌體分開處理，職責清晰

### 7.2 差異點

- ChromeOS 主要用於 **USB 周邊**，M487 目前是 **MSC + HID I2C Bridge**（USB Host + USB Device）
- 我們的更新情境更接近：**微控制器透過 USB 直接燒錄**，而非透過 fwupd daemon

### 7.3 未來探索方向

1. **LVFS 對嵌式裝置的支援程度** — 目前 LVFS 主要針對 USB/UEFI 裝置
2. **fwupd plugin for M487** — 是否可能開發一個 fwupd plugin 來支援 M487？
3. **fmap vs 我們的 flash layout** — 比較雙方設計優劣

---

## 八、關鍵資源

- [fwupd 官網](https://fwupd.org/)
- [ChromeOS fwupd Peripheral Guide](https://developers.google.com/chromeos/peripherals/fwupd-guide-for-peripherals)
- [LVFS ChromeOS 文件](https://lvfs.readthedocs.io/en/latest/chromeos.html)
- [ChromeOS EC USB Updater 文件](https://chromium.googlesource.com/chromiumos/platform/ec/+/master/docs/usb_updater.md)
- [fmap 格式規格](https://www.chromium.org/chromium-os/firmware-porting-guide/fmap)
- [fwupd ChromeOS EC Plugin](https://fwupd.github.io/libfwupdplugin/cros-ec-README.html)

---

*會議結論：由於我們的 M487 專案是 USB 複合裝置（MSC + HID I2C Bridge），與 ChromeOS 周邊應用場景有部分重疊。建議未來可研究 fwupd plugin 開發的可能性，以及 LVFS 對嵌入式裝置的支援程度。此會議記錄供後續評估參考。*

# FWUPD 基本介紹

## 什麼是 FWUPD？

FWUPD（Firmware Update Daemon）是一個開源的 Linux 韌體更新守護進程，由 **Richard Hughes** 開發並維護，旨在為 Linux 系統提供統一的韌體更新框架。它透過 LVFS（Linux Vendor Firmware Service）整合各家硬體供應商的韌體，為使用者提供安全、自動化且供應商中立的韌體更新體驗。

FWUPD 的核心設計理念是：**將韌體更新的复杂性抽象化，讓硬體供應商專注於提供韌體，而系統管理員和使用者無需理會底層細節。**

---

## 歷史背景

在 FWUPD 出現之前，Linux 系統的韌體更新幾乎完全依賴各家供應商提供的專屬工具：

- Intel 的 `fwupdate`
- Dell 的 `dellBoss`
- Lenovo 的專屬更新程式
- 各 IC 供應商的自定義燒錄工具

這些工具各自為政，沒有統一介面，無法跨供應商管理，也難以整合到企業的自動化流程中。

**FWUPD** 的誕生扭轉了這個局面。它首先在 Dell 系統上獲得大規模驗證，隨後被主流 Linux 發行版（Fedora、Ubuntu、Debian、Arch）廣泛採用，並成為 UEFI 韌體更新的標準解決方案之一。

---

## 核心特性

### 1. 統一介面
所有支援的裝置透過同一套 CLI 工具（`fwupdmgr`）和 D-Bus API 進行管理。

### 2. LVFS 整合
與 LVFS（Linux Vendor Firmware Service）深度整合，供應商可上傳韌體供全球 Linux 使用者下載。

### 3. 安全機制
- **憑證驗證**：韌體必須經過 LVFS 或本地金鑰簽章才能安裝
- **版本迴滾保護**：可防止降級到不安全版本
- **原子更新**：更新失敗時自動復原

### 4. 插件式架構
透過插件系統支援各類裝置，包括 UEFI、 USB、 SPI Flash 等。

### 5. 自動化排程
支援 `fwupd-refresh.timer` 定時檢查更新，結合系統通知被動或主動告知使用者。

---

## 常見使用情境

### 消費者場景
```bash
# 檢查所有可用更新
fwupdmgr get-updates

# 安裝所有更新
fwupdmgr update

# 安裝特定裝置更新
fwupdmgr update --device <DEVICE_ID>
```

### 企業 IT 管理
```bash
# 離線更新（不依賴 LVFS）
fwupdmgr get-devices
fwupdmgr clear-cache
fwupdmgr install <CAB_FILE>

# 查看裝置詳細資訊
fwupdmgr dump
```

### 開發者場景
```bash
# 以唯讀模式重新整理 LVFS 元資料
fwupdmgr refresh

# 取得裝置資訊
fwupdmgr get-devices

# 本地簽章測試
fwupdmgr verify --device <DEVICE_ID>
```

---

## 支援的韌體類型

| 類型 | 說明 | 範例 |
|------|------|------|
| **UEFI/BIOS** | 透過 ESRT 更新 BIOS/UEMI | 桌上型、筆記型主機板 |
| **USB** | USB 裝置韌體 | 擴充塢、網卡、隨身碟 |
| **SPI** | SPI Flash 晶片 | 嵌入式控制器 |
| **EMMC** |嵌入式多媒體卡| 嵌入式裝置 |
| **Thunderbolt** | Thunderbolt 控制器 | 認證擴充塢 |
| **SCBI** | 安全元件 | TPM、相關安全晶片 |

---

## 誰在使用 FWUPD？

### 作業系統層級採用
- **Fedora**：默認啟用，深度整合 GNOME Software
- **Ubuntu**：透過 `fwupd` 套件支援，配合 Ubuntu Pro
- **Debian**：官方套件庫提供
- **Arch Linux**：官方 extra 套件庫
- **Windows**：2020 年起，微軟在 Windows 10/11 中整合 LVFS/fwupd 概念（透過 Windows Update）

### 硬體供應商
- **Intel**：幾乎所有 Intel 平台都透過 FWUPD 更新
- **Dell**：標配 FWUPD，支援 BIOS、驅動程式、纇體更新
- **Lenovo**：ThinkPad 系列廣泛支援
- **Samsung**：Galaxy Book 系列
- **Nuvoton**：M487 等 Cortex-M 系列微控制器

---

## 與其他方案的比較

| 特性 | FWUPD | 供應商專屬工具 | 手動燒錄 |
|------|-------|--------------|----------|
| 統一的 CLI/D-Bus | ✅ | ❌ | ❌ |
| LVFS 整合 | ✅ | ❌ | ❌ |
| 自動化檢查 | ✅ | 部分 | ❌ |
| 開源免費 | ✅ | 通常否 | ✅ |
| 插件式擴展 | ✅ | ❌ | ❌ |
| 跨發行版通用 | ✅ | ❌ | N/A |

---

## 快速參考

```bash
# 安裝（以 Fedora 為例）
sudo dnf install fwupd

# 查看服務狀態
systemctl status fwupd

# 開機自動啟動
sudo systemctl enable --now fwupd-refresh.timer

# 基本操作流程
fwupdmgr get-devices        # 列出所有可更新裝置
fwupdmgr get-updates        # 檢查 LVFS 可用更新
fwupdmgr update             # 安裝更新
```

---

## 相關資源

- **官網**：https://fwupd.org/
- **GitHub**：https://github.com/fwupd/fwupd
- **LVFS**：https://fwupd.org/lvfs
- **文件**：https://fwupd.github.io/

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

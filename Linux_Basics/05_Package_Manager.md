# 第5章：套件管理 — Linux 的軟體安裝術

> **本章目標**：學會使用 apt / dnf 安裝、更新、移除軟體  
> **前置知識**：了解 sudo 權限（第四章）

---

## 🎯 先想一下 Windows 怎麼裝軟體

你在 Windows 安裝軟體的流程大概是：

1. 去軟體官網下載 `.exe` 或 `.msi` 安裝程式
2. 執行安裝程式
3. 一路按「下一步」
4. 等安裝完成
5. 如果軟體需要更新，通常要再去官網下載新版本

**問題**：
- 不同軟體有不同的安裝方式
- 軟體之間可能有「依賴關係」（A 軟體需要 B 程式庫才能跑）
- 更新麻煩，要一個一個檢查

---

## 📦 什麼是套件管理器？

### 概念介紹

**套件管理器（Package Manager）**就像一個「軟體應用商店」：

```
┌──────────────────────────────────────────────────┐
│                                                  │
│   Windows Store (想像簡化版)                       │
│   ┌────────────┐ ┌────────────┐ ┌────────────┐  │
│   │ Spotify    │ │ VS Code    │ │ Firefox    │  │
│   │ 安裝 │ 更新 │ │ 安裝 │ 更新 │ │ 安裝 │ 更新 │  │
│   └────────────┘ └────────────┘ └────────────┘  │
│                    ▲                             │
│                    │                             │
│         一次按鈕，全部搞定                         │
└────────────────────────────────────────────────────┘
```

### 套件管理器的功能

| 功能 | 說明 | 類比 |
|------|------|------|
| **安裝（Install）** | 一指令安裝軟體和所有依賴 | 一鍵安裝 |
| **移除（Remove）** | 乾淨移除軟體和相關檔案 | 解除安裝 |
| **更新（Update）** | 更新套件清單 | 檢查更新 |
| **升級（Upgrade）** | 升級到最新版本 | 安裝更新 |
| **搜尋（Search）** | 搜尋可安裝的軟體 | 應用商店搜尋 |
| **依賴處理（Dependencies）** | 自動安裝需要的程式庫 | 自動安裝必要元件 |

---

## 🐧 Debian/Ubuntu：apt / apt-get

### 背景

**apt** = **A**dvanced **P**ackaging **T**ool

Debian 家族的發行版（Debian、Ubuntu、Linux Mint 等）使用 apt 作為套件管理器。

> **歷史**：`apt-get` 是較舊的低階指令，`apt` 是後來推出的較新介面。日常使用中兩者功能差不多，但 `apt` 輸出更友善，建議使用 `apt`。

### apt 常用指令

```bash
# 更新套件清單（檢查哪些軟體可以更新）
sudo apt update

# 升級所有套件（安裝更新）
sudo apt upgrade

# 安裝軟體
sudo apt install firefox

# 安裝多個軟體
sudo apt install firefox gcc make

# 移除軟體（保留設定檔）
sudo apt remove firefox

# 完全移除軟體（包括設定檔）
sudo apt purge firefox

# 搜尋軟體
apt search "firefox"

# 查看套件資訊
apt show firefox

# 列出已安裝的套件
apt list --installed
```

### 升級作業系統

```bash
# 升級作業系統到新版本（Ubuntu）
sudo do-release-upgrade

# 完整升級（包含移除舊套件）
sudo apt full-upgrade
```

### 實例：安裝編譯工具鏈

```bash
# 更新套件清單
sudo apt update

# 安裝 gcc、make、g++（C/C++ 編譯工具）
sudo apt install build-essential

# 安裝 Git（版本控制）
sudo apt install git

# 安裝 CMake（建構系統）
sudo apt install cmake
```

---

## 🔴 Fedora/RHEL：dnf / yum

### 背景

**dnf** = **D**andified **Y**um（Yum 的新一代）

Fedora 從版本 22 開始預設使用 dnf。RHEL/CentOS 8+ 也用 dnf。舊版 RHEL/CentOS 用 `yum`。

| 指令 | 發行版 |
|------|--------|
| `apt` / `apt-get` | Debian, Ubuntu, Linux Mint |
| `dnf` | Fedora, RHEL 8+, CentOS Stream |
| `yum` | 舊版 RHEL/CentOS 6/7 |

> **好消息**：dnf 和 apt 語法非常相似，學會一個另一個幾乎都適用。

### dnf 常用指令

```bash
# 更新套件清單
sudo dnf check-update

# 升級所有套件
sudo dnf upgrade

# 安裝軟體
sudo dnf install firefox

# 移除軟體
sudo dnf remove firefox

# 搜尋軟體
dnf search "firefox"

# 列出已安裝
dnf list --installed

# 列出可更新的套件
dnf list updates
```

### 實例：安裝編譯工具鏈（Fedora）

```bash
# 更新套件清單
sudo dnf check-update

# 安裝編譯工具（Development Tools 是群組套件）
sudo dnf groupinstall "Development Tools"

# 或個別安裝
sudo dnf install gcc gcc-c++ make cmake git
```

---

## 🔧 其他套件管理器

### pacman（Arch Linux）

```bash
# 安裝軟體
sudo pacman -S firefox

# 移除軟體
sudo pacman -R firefox

# 更新系統
sudo pacman -Syu
```

### zypper（openSUSE）

```bash
# 安裝
sudo zypper install firefox

# 移除
sudo zypper remove firefox

# 更新
sudo zypper update
```

---

## 📦 套件格式 — .deb 和 .rpm

不同 Linux 家族使用不同的套件格式：

| 格式 | 套件管理器 | 發行版 |
|------|-----------|--------|
| **.deb** | apt / dpkg | Debian, Ubuntu |
| **.rpm** | dnf / yum / rpm | Fedora, RHEL, CentOS, openSUSE |

```
.deb 套件 = Debian Package
         安裝方式：dpkg -i package.deb
                  或 apt install package.deb（自動處理依賴）

.rpm 套件 = RPM Package Manager
         安裝方式：rpm -i package.rpm
                  或 dnf install package.rpm（自動處理依賴）
```

> **一般使用者不需要知道 .deb / .rpm 的差別**，用 apt/dnf 指令時會自動處理。

---

## 💾 硬碟空間管理

### 查看磁碟空間

```bash
# 查看整體使用情況
df -h

# 查看目前目錄的磁碟使用（人类可讀）
du -sh .

# 查看前10大的資料夾
du -h --max-depth=1 / | sort -hr | head -10
```

### 清理套件快取

```bash
# Debian/Ubuntu：清理下載的套件快取
sudo apt clean

# 移除不再需要的依賴套件
sudo apt autoremove

# Fedora：清理快取
sudo dnf clean all
```

---

## 🎯 實戰：安裝 fwupd

fwupd 是 Linux 的韌體更新工具（類比 Windows 的 Windows Update）。

### Ubuntu/Debian 安裝 fwupd

```bash
# 1. 更新套件清單
sudo apt update

# 2. 安裝 fwupd
sudo apt install fwupd

# 3. 檢查 fwupd 版本
fwupdmgr --version

# 4. 刷新可用韌體列表
sudo fwupdmgr refresh

# 5. 查看可更新的裝置
fwupdmgr get-devices

# 6. 更新所有韌體
sudo fwupdmgr update
```

### Fedora 安裝 fwupd

```bash
# 1. 更新套件清單
sudo dnf check-update

# 2. 安裝 fwupd
sudo dnf install fwupd

# 3. 後續操作相同
sudo fwupdmgr refresh
sudo fwupdmgr get-devices
```

### Arch Linux 安裝 fwupd

```bash
sudo pacman -S fwupd
```

---

## 🔍 疑難排解

### 「無法取得鎖」錯誤

```
E: Could not get lock /var/lib/dpkg/lock
```

**原因**：另一個套件管理程式正在執行，或者前一次安裝異常中斷。

**解決**：

```bash
# 等幾秒再試，或確認沒有其他 apt 程序在跑
ps aux | grep apt

# 如果確定沒有其他程式，可以移除鎖檔（最後手段）
sudo rm /var/lib/dpkg/lock
sudo rm /var/lib/dpkg/lock-frontend
sudo rm /var/lib/apt/lists/lock
sudo dpkg --configure -a
```

### 「依賴問題」錯誤

```
The following packages have unmet dependencies:
```

**解決**：

```bash
# 嘗試自動修復
sudo apt install -f
# 或
sudo dnf install -f

# 如果不行，更新系統後再試
sudo apt update && sudo apt upgrade
```

### 「軟體來源」問題

如果安裝失敗，說「找不到這個套件」：

```bash
# 檢查軟體來源設定
cat /etc/apt/sources.list
# 或
sudo apt edit-sources

# 如果是 Fedora
cat /etc/dnf/dnf.conf
```

---

## 🔑 重點整理

1. **套件管理器**讓軟體安裝、更新、移除都是一行指令
2. **apt**（Debian/Ubuntu）和 **dnf**（Fedora/RHEL）是最常見的套件管理器
3. `apt update` + `apt upgrade` 是基本更新流程
4. **fwupd** 安裝：`apt install fwupd` 或 `dnf install fwupd`
5. 出現鎖檔錯誤時，通常是另一個程式正在跑，等一下就好

---

## ▶️ 下一步

現在你會安裝軟體了！接下來 [第6章：服務與 Daemon](./06_Service_Daemon.md)，了解 Linux 的背景程式，特別是 fwupd daemon！

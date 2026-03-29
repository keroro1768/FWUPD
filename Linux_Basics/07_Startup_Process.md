# 第7章：開機流程 — Linux 開機時發生了什麼？

> **本章目標**：理解 Linux 的開機流程，學會在除錯時快速定位問題  
> **前置知識**：了解 systemd 和服務（第六章）

---

## 🎬 先想一下 Windows 開機流程

當你按下電源鍵，Windows 電腦大約發生這些事：

```
1. 電源供電 → 主機板初始化
2. BIOS/UEFI 自我測試（POST）
3. BIOS/UEFI 讀取啟動裝置（硬碟）
4. 載入 Windows 開機管理員 (Boot Manager)
5. 載入 Windows 核心 (ntoskrnl.exe)
6. 初始化驅動程式
7. 啟動 Windows 服務（如 Windows Update、Defender）
8. 顯示登入畫面
```

整個過程大約 10-30 秒，你只看到「轉圈圈」和「歡迎使用」。

---

## 🐧 Linux 開機流程（鳥瞰圖）

Linux 的開機流程可以分成**五個大階段**：

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│   1. 硬體初始化（BIOS/UEFI）                                 │
│          ↓                                                  │
│   2. 開機載入器（Boot Loader，通常是 GRUB）                   │
│          ↓                                                  │
│   3. 核心載入（Linux Kernel）                                │
│          ↓                                                  │
│   4. 系統初始化（systemd init）                               │
│          ↓                                                  │
│   5. 使用者環境（Login + Desktop）                            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 階段 1：BIOS/UEFI — 硬體的自我檢查

### 什麼是 BIOS？

**BIOS** = **B**asic **I**nput/**O**utput **S**ystem

這是一段存在主機板上的「韌體（Firmware）」，是硬體的第一個軟體。

**BIOS 的工作**：
1. 開機時進行**自我測試（POST）**，確認硬體正常
2. 偵測可開機的裝置（硬碟、USB、光碟）
3. 根據設定，選擇**第一個開機裝置**
4. 讀取該裝置的**開機載入器**，把控制權交給它

### BIOS vs UEFI

| 項目 | BIOS | UEFI |
|------|------|------|
| 發明時間 | 1980 年代 | 2000 年代 |
| 支援硬碟分割 | 只能 MBR | 可以 MBR 或 GPT |
| 支援大硬碟 | 最多 2TB | 超過 2TB |
| 開機速度 | 較慢 | 較快 |
| 安全性 | 無 Secure Boot | 有 Secure Boot |
| 現代電腦 | 舊電腦 | 新電腦（2012年後）|

> **Secure Boot**：UEFI 的一個安全功能，只允許有「數位簽章」的作業系統開機，防止惡意軟體在開機時入侵。

---

## 階段 2：GRUB — 開機載入器

### 什麼是 Boot Loader？

**Boot Loader（開機載入器）**是一個小程式，存在硬碟的特定區塊。它的任務是：

> 「把作業系統的核心（Kernel）載入記憶體，然後把電腦的控制權交給它。」

### 什麼是 GRUB？

**GRUB** = **GR**and **U**nifier **B**ootloader

GRUB 是 Linux 最常見的開機載入器。它顯示一個選單，讓你選擇：

```
┌─────────────────────────────────────────┐
│   GNU GRUB                             │
├─────────────────────────────────────────┤
│   Ubuntu                               │  ← 一般選擇這個
│   Ubuntu (advanced options)            │
│   Memory test (memtest86+)             │
│                                         │
│   Windows Boot Manager (on /dev/sda1)  │  ← 雙系統時會出現
│                                         │
└─────────────────────────────────────────┘
        按 Enter 啟動，或按 e 編輯選項
```

### GRUB 的設定檔

```bash
# GRUB 設定檔位置
/etc/default/grub                # 預設設定
/boot/grub/grub.cfg              # 實際使用的設定（自動產生，不要直接編輯）

# 編輯 GRUB 設定後，需要更新
sudo update-grub                # Debian/Ubuntu
# 或
sudo grub2-mkconfig -o /boot/grub2/grub.cfg  # Fedora/RHEL
```

### GRUB 常見操作

```bash
# 查看 GRUB 選單（開機時自動顯示）
# 按 Shift 或 Esc 鍵可手動叫出

# 編輯開機參數（臨時修改，開機後失效）
# 開機時按 e，進入編輯模式

# 常見開機參數：
#   quiet          → 隱藏開機訊息
#   splash         → 顯示開機畫面
#   text           → 進入文字模式（不啟動圖形介面）
#   single         → 進入單人維護模式
#   recovery       → 進入復原模式
```

---

## 階段 3：Linux Kernel — 核心載入

### Kernel 是什麼？

**Kernel（核心）**是 Linux 作業系統的「大腦」。

```
Kernel 的功能：
┌─────────────────────────────────────────┐
│                                         │
│  CPU 排程    → 決定哪個程式什麼時候跑      │
│  記憶體管理   → 分配/回收記憶體            │
│  檔案系統    → 讀寫檔案                   │
│  硬體驅動   → 和硬體溝通                 │
│  程序管理    → 建立/結束程式              │
│  網路功能   → 網路通訊                   │
│                                         │
└─────────────────────────────────────────┘
```

### 開機時發生了什麼？

```
1. GRUB 把 Kernel 映像（vmlinuz）載入記憶體
2. GRUB 把 initramfs（初始檔案系統）載入記憶體
3. Kernel 解壓縮並開始執行
4. Kernel 偵測硬體、載入驅動程式
5. Kernel 掛載根目錄檔案系統（/）
6. Kernel 啟動第一個程式（PID 1）
```

### initramfs 是什麼？

**initramfs** = **init**ial **RAM** **F**ile**s**ystem

這是一個「臨時的小型檔案系統」，用來在真正的根目錄掛載之前，幫助 Kernel 完成一些前置作業。

**什麼時候需要 initramfs？**
- 磁碟需要額外驅動程式才能存取（如 RAID、LVM、加密）
- 你的根目錄在網路磁碟或特殊裝置上

### 查看目前的 Kernel 版本

```bash
# 查看已安裝的 Kernel 版本
ls /boot/vmlinuz-*

# 查看目前使用的 Kernel 版本
uname -r

# 輸出範例：
# 5.15.0-46-generic
```

---

## 階段 4：systemd — 系統初始化

### PID 1 — 第一個程式

當 Kernel 載入完成後，它啟動的第一個程式是 **PID 1**（Process ID = 1）。

在傳統 Linux 是 `init`，在現代 Linux 是 **`systemd`**。

> **PID**：Process ID，每個正在跑的程式都有一個編號。PID 1 是第一個程式，是所有其他程式的「祖先」。

### systemd 的開機過程

```
systemd 啟動流程：
┌─────────────────────────────────────────┐
│                                         │
│  1. 讀取設定檔 /etc/systemd/system/     │
│                                         │
│  2. 解析依賴關係                         │
│     (服務 A 需要先跑完服務 B 才能跑)      │
│                                         │
│  3. 平行啟動儘可能多的服務               │
│     (加快開機速度)                       │
│                                         │
│  4. 等待每個服務就緒                      │
│                                         │
│  5. 啟動 getty（讓你能登入）             │
│                                         │
└─────────────────────────────────────────┘
```

### systemd 的 Target（目標）

**Target** = 一組服務的組合，相當於 Windows 的「執行等級」。

| Target | 等同 | 用途 |
|--------|------|------|
| `poweroff.target` | 關機 | 關閉系統 |
| `rescue.target` | 安全模式 | 單人維護模式 |
| `multi-user.target` | 文字模式 | 多人文字模式（無圖形） |
| `graphical.target` | 圖形模式 | 圖形介面 + 多人模式 |
| `reboot.target` | 重新啟動 | 重開機 |

```bash
# 切換到文字模式
sudo systemctl isolate multi-user.target

# 切換到圖形模式
sudo systemctl isolate graphical.target

# 設定開機預設模式
sudo systemctl set-default graphical.target    # 開機進圖形介面
sudo systemctl set-default multi-user.target   # 開機進文字模式
```

### 查看開機時間

```bash
# 查看上一次開機的時間分析
systemd-analyze

# 輸出範例：
# Startup finished in 5.2s (kernel) + 3.1s (userspace) = 8.3s

# 查看每個服務的啟動時間
systemd-analyze blame

# 輸出（最慢的排在前面）：
# 30.500s network-manager.service
#  2.100s docker.service
#  ...
```

---

## 階段 5：使用者環境 — 登入與桌面

### 最後一步

```
1. systemd 啟動 display manager（如 gdm, sddm, lightdm）
2. 顯示登入畫面
3. 使用者輸入帳號密碼
4. 啟動 Desktop Environment（GNOME、KDE、XFCE）
5. 使用者看到桌面！
```

### 常見的 Desktop Environment

| 名稱 | 特色 | 資源需求 |
|------|------|---------|
| **GNOME** | Ubuntu 預設，現代簡潔 | 中等 |
| **KDE Plasma** | 功能豐富，高度客製化 | 中等 |
| **XFCE** | 輕量級，老爺機適用 | 輕量 |
| **MATE** | 經典 GNOME 2 風格 | 輕量 |

---

## 📊 Windows vs Linux 開機流程對照

| 階段 | Windows | Linux |
|------|---------|-------|
| 1 | BIOS/UEFI → 自我測試 | BIOS/UEFI → 自我測試 |
| 2 | Boot Manager (BCD) | GRUB (Boot Loader) |
| 3 | Winload.exe → ntoskrnl.exe | Kernel (vmlinuz) → PID 1 |
| 4 | Session Manager (smss) | systemd / init |
| 5 | Services (services.exe) | Services (systemd units) |
| 6 | Winlogon → 登入畫面 | getty → 登入畫面 |
| 7 | Explorer (桌面) | Desktop Environment (GNOME/KDE) |

---

## 🔧 常見開機問題排除

### 忘記 root 密碼（進入單人維護模式）

```
1. 開機時，在 GRUB 選單按 e
2. 找到 "linux /boot/vmlinuz..." 那行
3. 在行尾加上 "single" 或 "init=/bin/bash"
4. 按 Ctrl + x 或 F10 啟動
5. 掛載根目錄：mount -o remount,rw /
6. 設定密碼：passwd
7. 重開機：reboot -f
```

### GRUB 選單沒出現（雙系統選單不見）

```
1. 開機時長按 Shift 鍵（Ubuntu）
2. 或長按 Esc 鍵
3. 進入 GRUB 後，按 e 編輯
```

### 開機停在某一服務

```bash
# 查看開機卡在哪裡
systemd-analyze blame

# 跳過失敗的服務（強制開機）
sudo systemctl start <failed-service> --force
```

### 進入 emergency mode（緊急模式）

如果系統開機進入 emergency mode（根目錄變成唯讀）：

```
1. 輸入 root 密碼
2. 掛載為可寫入：mount -o remount,rw /
3. 檢查日誌：journalctl -xb
4. 嘗試修復或重啟：reboot
```

---

## 🔑 重點整理

1. **BIOS/UEFI** → 硬體自我測試，選擇開機裝置
2. **GRUB** → 開機選單，選擇要啟動的作業系統
3. **Kernel（核心）** → Linux 的大腦，載入記憶體後接管系統
4. **systemd (PID 1)** → 啟動所有系統服務
5. **登入畫面 + Desktop** → 使用者可以開始使用電腦

---

## ▶️ 下一步

了解開機流程之後，接下來學習 [第8章：網路基礎](./08_Network_Command.md)，掌握基本網路指令！

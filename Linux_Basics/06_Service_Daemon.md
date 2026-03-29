# 第6章：服務與 Daemon — Linux 的背景程式

> **本章目標**：理解 systemd、systemctl、Daemon 的概念，以及 fwupd daemon 的運作方式  
> **前置知識**：了解套件管理和基本指令（第四、五章）

---

## 🌊 先想一下 Windows 的「服務」

在 Windows，工作管理員（`Ctrl + Shift + Esc`）有一個「服務」分頁：

```
Windows 服務（Service）列表：
名稱                    狀態   啟動類型
─────────────────────────────────────────
Windows Update         執行中  自動
Windows Defender       執行中  自動
Spooler (列印服務)     執行中  自動
Apache                  已停止  手動
MySQL                   已停止  手動
...
```

**服務（Service）**就是「一直在背景運行的程式」，不需要使用者登入就會執行。

**例子**：
- Windows Update：自動在背景檢查更新
- 防火牆：一直在監聽網路流量
- 遠端桌面：等待別人連線進來

---

## 👻 什麼是 Daemon？

### Linux 的 Daemon = Windows 的 Service

**Daemon** 是 Linux 的術語，相當於 Windows 的「服務」（Service）。

> **詞源**：Daemon 源自希臘神話的「精靈」或「守護者」，在這裡是指「一直在背景默默守護系統的程式」。

**Daemon 的特點**：
1. **長期執行**：啟動後就一直跑，直到系統關機
2. **不需要互動**：沒有圖形介面，在背景默默工作
3. **由系統啟動**：開機時自動啟動，不需要使用者登入

### 常見的 Linux Daemon

| Daemon 名 | 用途 | 類比 Windows 服務 |
|-----------|------|------------------|
| `systemd` | 系統和服務管理器 | 類似「Windows 開機管理員」|
| `sshd` | SSH 遠端登入服務 | 類似「遠端桌面」|
| `httpd` / `apache2` | 網頁伺服器 | IIS |
| `mysqld` | 資料庫 | SQL Server |
| `docker` | 容器引擎 | Docker Desktop |
| **`fwupd`** | 韌體更新服務 | Windows Update（韌體版）|

### Daemon 的命名慣例

注意觀察：Linux Daemon 的名字通常**以「d」結尾**：

```
systemd   → system daemon
sshd      → secure shell daemon
httpd     → HTTP daemon
mysqld    → MySQL daemon
firewalld → firewall daemon
fwupd     → firmware update daemon
```

> 這個「d」就是「daemon」的意思。`systemd` 是「system daemon」的簡稱，是 Linux 系統的第一個Daemon（PID 1）。

---

## ⚙️ systemd — Linux 的服務管理系統

### 什麼是 systemd？

**systemd** 是現代 Linux 發行版（Ubuntu 15.04+、Fedora、RHEL 7+）採用的「系統和服務管理器」。

**主要功能**：
1. **管理系統服務（Daemon）**：啟動、停止、重啟、查看狀態
2. **管理系統資源**：設定開機自動執行的程式
3. **處理系統事件**：插入 USB、網路變更等

### systemd 的歷史

```
早期的 Linux（SysV init）：
- 用一串 shell script 啟動服務
- 速度慢、難以管理

現在的 Linux（systemd）：
- 用平行啟動，速度快
- 統一的指令和設定方式
- 所有主要發行版都在用
```

### systemd 的核心概念

| 概念 | 說明 | 類比 |
|------|------|------|
| **unit（單元）** | systemd 管理的對象（服務、掛載點、計時器等） | 汽車的各個零件 |
| **unit file（單元檔）** | 設定檔，定義一個單元如何運作 | 零件的規格書 |
| **target（目標）** | 一群單元的組合，相當於「執行等級」 | 汽車的駕駛模式 |
| **service（服務）** | 一種 unit type（服務類型的單元） | 引擎 |

### systemd 的 unit 檔案位置

```bash
/etc/systemd/system/       # 系統管理員設定的單元
/run/systemd/system/       # 運行時產生的單元
/lib/systemd/system/       # 套件預設的單元（Debian/Ubuntu）
/usr/lib/systemd/system/   # 套件預設的單元（Fedora/RHEL）
```

---

## 🔧 systemctl — 操控 systemd 的指令

### 基本語法

```bash
systemctl [指令] [單元名]
```

### 常用的 systemctl 指令

```bash
# 查看服務狀態
systemctl status sshd

# 啟動服務
sudo systemctl start fwupd

# 停止服務
sudo systemctl stop fwupd

# 重啟服務（停止後再啟動）
sudo systemctl restart fwupd

# 重新載入設定（不改變服務狀態，只重讀設定檔）
sudo systemctl reload nginx

# 設定開機自動啟動
sudo systemctl enable sshd

# 取消開機自動啟動
sudo systemctl disable sshd

# 檢查是否開機啟動
systemctl is-enabled sshd

# 查看所有執行中的服務
systemctl list-units --type=service --state=running

# 查看所有服務（含已停止的）
systemctl list-units --type=service
```

### 實例：管理 fwupd 服務

```bash
# 查看 fwupd 服務狀態
systemctl status fwupd

# 如果輸出是這樣，代表正在執行：
# ● fwupd.service - Firmware Update Daemon
#    Loaded: loaded (/lib/systemd/system/fwupd.service; enabled)
#    Active: active (running) since ...
```

```bash
# 停止 fwupd 服務（需要管理員）
sudo systemctl stop fwupd

# 再次查看狀態
systemctl status fwupd
# Active: inactive (dead)

# 啟動 fwupd 服務
sudo systemctl start fwupd

# 設定開機自動啟動
sudo systemctl enable fwupd

# 取消開機自動啟動
sudo systemctl disable fwupd
```

### systemctl 狀態詳解

```bash
systemctl status fwupd
```

輸出：
```
● fwupd.service - Firmware Update Daemon
     Loaded: loaded (/lib/systemd/system/fwupd.service; enabled; vendor preset: enabled)
     Active: active (running) since Sat 2026-03-28 10:00:00 CST; 1 day 2h ago
       Docs: man:fwupd(8)
   Main PID: 1234 (fwupd)
      Tasks: 5 (limit: 4915)
     Memory: 15.2M
        CPU: 234ms
     CGroup: /system.slice/fwupd.service
             └─1234 /usr/libexec/fwupd/fwupd
```

**欄位解讀**：

| 欄位 | 意義 |
|------|------|
| `Loaded` | 單元檔已載入；`enabled`=開機啟動，`disabled`=不自動啟動 |
| `Active: active (running)` | 服務正在執行 |
| `Active: inactive (dead)` | 服務已停止 |
| `Main PID` | 主要程序的程序 ID（Process ID） |
| `vendor preset: enabled` | 發行版預設開機啟動 |

---

## 🔍 Daemon 運作原理

### 傳統 Daemon 啟動方式

```
1. 系統啟動，init/systemd 讀取設定
2. 根據設定，fork() 分支出背景程序
3. 背景程序脫離終端（detached），成為 Daemon
4. Daemon 開始監聽/處理任務
```

### systemd 的改進

```
1. systemd 啟動，直接用 fork() + 設定檔啟動
2. 支援平行啟動（更快）
3. 用 cgroup 追蹤程序（更安全）
4. 統一的日誌（journald）
5. 按需啟動（需要的時候才啟動）
```

### 查看 Daemon 的程序

```bash
# 用 ps 查看系統程序
ps aux | grep fwupd

# 輸出：
# root  1234  ...  /usr/libexec/fwupd/fwupd
#          ↑
# PID 1234，擁有者是 root

# 用 pstree 查看程序樹
pstree -p | grep systemd
```

---

## 📊 Windows 服務 vs Linux systemd 對照

| 動作 | Windows | Linux systemd |
|------|---------|--------------|
| 查看狀態 | 工作管理員 → 服務 | `systemctl status` |
| 啟動服務 | 啟動服務 | `sudo systemctl start` |
| 停止服務 | 停止服務 | `sudo systemctl stop` |
| 重啟服務 | 重新啟動服務 | `sudo systemctl restart` |
| 開機啟動 | 屬性 → 啟動類型 → 自動 | `sudo systemctl enable` |
| 關閉開機啟動 | 屬性 → 啟動類型 → 手動 | `sudo systemctl disable` |
| 查看所有服務 | services.msc | `systemctl list-units --type=service` |
| 查看即時日誌 | 事件檢視器 | `journalctl -u 服務名` |

---

## 📝 systemd 管理指令速查

```bash
# ===== 基本操作 =====
sudo systemctl start <service>      # 啟動
sudo systemctl stop <service>        # 停止
sudo systemctl restart <service>    # 重啟
sudo systemctl reload <service>      # 重新載入設定
sudo systemctl status <service>      # 查看狀態

# ===== 開機設定 =====
sudo systemctl enable <service>     # 開機啟動
sudo systemctl disable <service>    # 取消開機啟動
sudo systemctl is-enabled <service>  # 檢查是否開機啟動

# ===== 查看服務 =====
systemctl list-units --type=service --state=running    # 列出所有執行中的服務
systemctl list-units --type=service --state=failed     # 列出所有失敗的服務

# ===== 日誌 =====
journalctl -u <service>             # 查看服務的日誌
journalctl -u <service> -f          # 即時追蹤服務日誌（等同一 tail -f）
journalctl --since "1 hour ago"     # 查看最近一小時的日誌
```

---

## 🔑 重點整理

1. **Daemon** = Linux 的背景服務，等同 Windows 的 Service
2. **systemd** 是現代 Linux 的服務管理系統（PID 1）
3. **systemctl** 是操控 systemd 的指令
4. **fwupd daemon** 是韌體更新服務，管理系統的韌體更新
5. `systemctl status` 查看狀態，`start/stop/restart` 控制執行
6. `systemctl enable/disable` 控制開機是否自動啟動

---

## ▶️ 下一步

了解服務之後，接下來學習 [第7章：開機流程](./07_Startup_Process.md)，知道 Linux 開機時發生了什麼事！

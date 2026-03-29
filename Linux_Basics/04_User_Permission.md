# 第4章：使用者與權限 — 為什麼你不是管理員也能做事？

> **本章目標**：理解 Linux 的使用者模型、root、sudo、使用者群組  
> **前置知識**：了解檔案系統和權限概念（第三章）

---

## 👤 先想一下 Windows 的使用者

在 Windows，你的帳號類型通常是：

| 類型 | 能做什麼 | 範例 |
|------|----------|------|
| **管理員** | 完全控制電腦、刪改系統檔案、安裝軟體 | Administrator |
| **標準使用者** | 只能改自己的檔案、不能動系統 | 你的日常帳號 |

**Windows 的問題**：很多人日常用「管理員帳號」，所以任何病毒或錯誤操作都可以直接破壞系統。

---

## 🛡️ Linux 的使用者模型

Linux 設計更保守：**平時用普通帳號，需要時才臨時取得系統管理員權限。**

### 三種使用者身份

每個檔案/資料夾都有三組權限，分別給三種「身份」：

```
┌─────────────────────────────────────────────┐
│  drwxr-xr-x  3 caro caro 4096 Mar 28 Documents/
├─────────────────────────────────────────────┤
│  │        │    │    │
│  │        │    │    └── 其他人（Others）: r-x
│  │        │    └─────── 群組（Group）: r-x
│  │        └─────────── 擁有者（Owner）: rwx
│  └─────────────────── 檔案類型
└─────────────────────────────────────────────┘
```

| 身份 | 意義 | 在 `caro` 帳號下 |
|------|------|------------------|
| **Owner（擁有者）** | 檔案的建立者 | 就是 `caro` |
| **Group（群組）** | 附屬的群組 | `caro` 可能在 `caro`、`sudo`、`www-data` 等群組 |
| **Others（其他人）** | 不屬於上述兩者的人 | 系統上其他使用者 |

### Linux 預設的三種身份

1. **擁有者（Owner）**：建立檔案的人
2. **群組（Group）**：可以多人共享一個群組
3. **其他人（Others）**：系統上的其他使用者

---

## 🔑 root — Linux 的「超級管理員」

### 什麼是 root？

`root` 是 Linux 系統中權力最大的帳號，類似 Windows 的「Administrator」：

- 可以刪除任何檔案
- 可以修改系統設定
- 可以安裝/移除軟體
- 可以變更其他使用者的檔案權限

### root 的家目錄

- 普通使用者的家目錄：`/home/caro`
- root 的家目錄：`/root`（直接在根目錄下，不是 `/home/root`）

### 預設禁用 root（新手要注意！）

**Ubuntu 和許多發行版預設禁用 root 帳號**，必須使用 `sudo` 臨時取得權限。

> 這是故意的！禁用 root 可以防止「因為忘了自己在 root 模式而誤刪系統檔案」。

### 查看你的身份

```bash
# 查看你是誰
whoami

# 查看你的 UID 和所屬群組
id
```

輸出範例：
```
uid=1000(caro) gid=1000(caro) groups=1000(caro),4(adm),24(cdrom),27(sudo)...
```

解讀：
- `uid=1000` — 使用者 ID（1000 是第一個建立的使用者）
- `gid=1000` — 主要群組 ID
- `groups=...` — 所有附屬群組

---

## ⚡ sudo — 暫時取得系統管理員權限

### 什麼是 sudo？

`sudo` = **S**uper **U**ser **DO**（以超級使用者執行）

**類比 PowerShell**：相當於 `Start-Process -Verb RunAs` 或右鍵「以系統管理員執行」

### 基本語法

```bash
# 在指令前面加 sudo
sudo apt update            # 以系統管理員執行 apt update
sudo rm -rf /tmp/test      # 以系統管理員刪除（危險！）
sudo systemctl restart fwupd  # 以系統管理員重啟 fwupd 服務
```

### 第一次使用 sudo

第一次在 Ubuntu 使用 `sudo` 時，會看到這樣的訊息：

```
[sudo] password for caro:
```

輸入**你自己的密碼**（不是 root 的密碼，因為根本沒有 root 密碼）。

**設計邏輯**：只有知道這個帳號密碼的人，才能臨時提升權限。

### sudo 需要輸入密碼多久？

Ubuntu 預設「記住密碼 15 分鐘」，這段時間內再使用 sudo 不需要重複輸入密碼。

### 為什麼設計成這樣？

```
Windows 的風險：
「我只是想安裝一個軟體...」
→ 咦，我本來就是管理員...
→ 奇怪，剛才誤刪了 C:\Windows\System32 的東西！

Linux 的設計：
「我只是想安裝一個軟體...」
→ 需要 sudo（再想一次）
→ 確認後執行
→ 降低了「手滑」的風險
```

---

## 👥 使用者群組（Groups）

### 什麼是群組？

**群組**是一群使用者的集合。用來一次授權很多人存取某個資源。

### 常見系統群組

| 群組名 | 用途 | 誰在裡面 |
|--------|------|---------|
| `sudo` | 執行 sudo 指令的權限 | 管理員帳號 |
| `adm` | 讀取系統日誌 | 系統管理者 |
| `www-data` | Web 伺服器的帳號 | Apache/Nginx |
| `docker` | Docker 容器的存取 | 需要用 Docker 的人 |
| `plugdev` | 存取 USB 等外接裝置 | 桌面使用者 |
| `audio` | 存取音效裝置 | 需要聲音的人 |

### 查看自己的群組

```bash
groups
# 或
id
```

### 把使用者加入群組

```bash
# 把使用者加入群組（需要 root/sudo）
sudo usermod -aG 群組名 使用者名

# 範例：把 user1 加入 docker 群組
sudo usermod -aG docker user1

# 範例：把 user1 加入 sudo 群組（讓他變管理員）
sudo usermod -aG sudo user1
```

> **參數說明**：`usermod` = 修改使用者，`-aG` = **A**ppend **G**roup（附加群組，不移除現有群組）

### 建立新使用者

```bash
# 建立新使用者
sudo adduser newuser

# 設定密碼
sudo passwd newuser

# 刪除使用者
sudo deluser newuser
```

---

## 🔍 實例：理解一個操作的權限需求

### 情境：安裝 Firefox

```bash
# 普通使用者執行的結果
$ apt install firefox
Reading package lists... Done
E: Could not open lock file /var/lib/apt/lists/lock
E: Unable to lock directory: /var/lib/apt/lists/
Permission denied.

# 因為需要寫入 /var/lib/apt/（系統目錄），需要管理員權限

# 用 sudo 執行
$ sudo apt install firefox
[sudo] password for caro:
Reading package lists... Done
Building dependency tree... Done
firefox is already the newest version (1:1.0 snap)
```

### 情境：讀取系統日誌

```bash
# 普通使用者讀取系統日誌
$ cat /var/log/syslog
cat: /var/log/syslog: Permission denied

# 因為只有特定群組可以讀取

# 用 sudo 執行
$ sudo cat /var/log/syslog
Mar 28 10:00:01 hostname kernel: [12345.67890] some log message...
```

### 情境：修改別人的檔案

```bash
# 以使用者 alice 的身份建立一個檔案
$ su - alice
$ touch /tmp/shared.txt
$ exit

# 以使用者 caro 的身份嘗試修改
$ cat "hello" > /tmp/shared.txt
bash: /tmp/shared.txt: Permission denied

# 因為 caro 不是擁有者，也不是同群組
```

---

## 🔧 實際練習：建立一個多使用者場景

```bash
# 1. 以系統管理員身份建立兩個使用者
sudo adduser alice
sudo adduser bob

# 2. 建立一個共享資料夾
sudo mkdir /opt/sharedproject
sudo chown alice:projectteam /opt/sharedproject
sudo chmod 770 /opt/sharedproject

# 3. 建立群組並加入成員
sudo groupadd projectteam
sudo usermod -aG projectteam alice
sudo usermod -aG projectteam bob

# 4. 把共享資料夾的群組設為專案群組
sudo chgrp projectteam /opt/sharedproject

# 5. 測試：bob 現在可以寫入
su - bob
echo "hello from bob" > /opt/sharedproject/notes.txt
cat /opt/sharedproject/notes.txt
exit

# 6. 如果另一個不是群組的使用者嘗試寫入
su - eve
echo "hack" > /opt/sharedproject/notes.txt  # 會被拒絕！
exit
```

---

## 🆚 對比：Windows vs Linux 權限模型

| 面向 | Windows | Linux |
|------|---------|-------|
| 超級管理員 | Administrator | root |
| 管理員身份 | 預設開啟，容易被利用 | 預設禁用，需要 sudo 主動提升 |
| 權限設定 | 較粗粒度（完全控制/讀取/寫入） | 細粒度（讀/寫/執行分開控制） |
| 群組 | 有，但使用頻率低 | 常被用來共享資源 |
| 檔案擁有者 | 少見 | 常見（每個檔案都有擁有者） |
| 危險操作提示 | UAC 彈窗 | sudo 要求密碼 |

---

## 🔑 重點整理

1. **root** 是 Linux 的超級管理員，權力最大（等同 Windows Administrator）
2. **sudo** 讓普通使用者暫時提升為系統管理員執行特定指令
3. Ubuntu 預設**禁用 root 帳號**，用 sudo 代替
4. **使用者群組**讓一群人可以共享資源（檔案、裝置等）
5. **usermod -aG** 可以把使用者加入群組（`-aG` = 附加群組，不移除現有群組）
6. 只有管理員才能**修改系統層級的設定**

---

## ▶️ 下一步

現在你了解使用者與權限了！接下來 [第5章：套件管理](./05_Package_Manager.md)，學習如何用 apt/dnf 安裝軟體，包含 fwupd！

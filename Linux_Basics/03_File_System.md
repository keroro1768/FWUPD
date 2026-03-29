# 第3章：檔案系統 — Linux 的檔案放在哪裡？

> **本章目標**：理解 Linux 的目錄結構、絕對路徑 vs 相對路徑、檔案權限概念  
> **前置知識**：會用基本指令（第二章）

---

## 🏠 先想一下 Windows 的檔案系統

在 Windows，檔案放在「磁碟機」裡：

```
C:\                          ← 這是 C 槽（主要硬碟）
├── Program Files\           ← 安裝的程式
├── Program Files (x86)\    ← 32 位元程式
├── Windows\                ← Windows 系統檔案
├── Users\                  ← 使用者的檔案
│   └── Caro\               ← 你的個人資料
│       ├── Documents\
│       ├── Downloads\
│       └── Desktop\
└── ...
```

Windows 用「磁碟機代號」區分不同的儲存設備：
- `C:` 是主要硬碟
- `D:` 可能是光碟機或第二顆硬碟
- `E:` 可能是 USB 隨身碟

---

## 🌳 Linux 的目錄樹 — 一切都是「一個」目錄

Linux 的設計哲學完全不同：**只有一個根目錄**（`/`），所有東西都長在這棵樹下面。

> **想像**：Linux 把所有硬碟、USB、網路芳鄰全部「掛」成一個資料夾，而不是像 Windows 分成 C:、D:、E:。

```
Linux 目錄樹：
/                               ← 根目錄（一切從這裡開始）
├── bin/                        ← 基本指令（ls, cp 等）
├── boot/                       ← 開機相關檔案（核心、GRUB）
├── dev/                        ← 硬體裝置（被當成檔案處理）
├── etc/                        ← 系統設定檔
├── home/                       ← 使用者的個人檔案
│   └── caro/                  ← 你的家目錄
│       ├── Documents/
│       ├── Downloads/
│       └── Desktop/
├── lib/                        ← 程式庫（程式需要的共享函式庫）
├── media/                      ← 掛載的可移除媒體（USB、光碟）
├── mnt/                        ← 臨時掛載點
├── opt/                        ← 額外安裝的軟體（可選軟體）
├── proc/                       ← 系統資訊（程序、記憶體等）
├── root/                       ← root 使用者的家目錄
├── run/                        ← 系統運行時的臨時檔案
├── sbin/                       ← 系統管理指令
├── srv/                        ← 服務提供的資料
├── sys/                        ← 系統硬體資訊
├── tmp/                        ← 臨時檔案
├── usr/                        ← 使用者程式和資料（Unix System Resources）
└── var/                        ← 會變動的資料（日誌、資料庫）
```

---

## 📂 常見目錄詳解

### /home — 使用者的家

**類比 Windows**：`C:\Users\`（你放個人檔案的地方）

```
/home/          ← 用來存放所有一般使用者的個人檔案
├── caro/       ← 使用者「caro」的家目錄
├── alice/      ← 使用者「alice」的家目錄
└── bob/
```

**什麼放這裡？**
- 你的文件、照片、下載檔案
- 你的程式碼
- 你的設定檔（隱藏的 `.` 開頭檔案）

**特殊捷徑**：
- `~` 代表目前使用者的家目錄
- `~caro` 代表使用者 caro 的家目錄

### /usr — 使用者程式（Unix System Resources）

**類比 Windows**：`C:\Program Files\`（安裝的軟體）

```
/usr/           ← 安裝的軟體和相關檔案
├── bin/        ← 一般使用者可執行的程式（ls, cat, gcc）
├── sbin/       ← 系統管理程式（需要 root 權限）
├── lib/        ← 程式庫
├── share/      ← 共享資料（說明文件、圖示等）
└── local/      ← 本地編譯安裝的軟體
```

### /etc — 設定檔

**類比 Windows**：`C:\Windows\System32\config\`（系統設定）

> **重要**：`/etc` 是「系統設定」的大本營。幾乎所有軟體的設定檔都在這裡。

```
/etc/
├── apt/                    ← apt 套件管理器的設定
│   └── sources.list        ← 軟體來源列表
├── ssh/                    ← SSH 服務設定
│   └── sshd_config         ← SSH 伺服器設定
├── systemd/                ← systemd 設定
│   └── system/              ← 系統服務設定
├── fwupd/                   ← fwupd 設定（韌體更新工具）
│   └── daemon.conf
├── network/                 ← 網路設定（某些發行版）
└── passwd                   ← 使用者帳號資料
```

### /var — 會變動的資料

**類比 Windows**：`C:\Windows\Temp\` + 部分 AppData

> **「var」是「variable」的縮寫**，代表「會變動的檔案」。

```
/var/
├── log/                    ← 系統日誌（記錄發生的事）
│   ├── syslog               ← 系統一般日誌
│   ├── auth.log             ← 認證日誌（誰登入、誰用 sudo）
│   └── fwupd/               ← fwupd 相關日誌
├── cache/                  ← 快取資料
├── lib/                    ← 軟體的資料庫（APT、dpkg 等）
├── spool/                  ← 待處理的任務（列印佇列等）
└── tmp/                    ← 臨時檔案（程式可以存放）
```

### /tmp — 臨時檔案

**類比 Windows**：`C:\Windows\Temp\` 或 `%TEMP%`

> **特點**：這個資料夾的內容**會被清空**！通常開機後或定期刪除 `/tmp` 的內容。

```bash
# 任何人都能在 /tmp 建立檔案
touch /tmp/my_temp_file.txt
```

### /dev — 硬體裝置（當成檔案）

**類比 Windows**：控制台 → 裝置管理員看到的硬體

在 Linux，「一切皆檔案」，包括硬體裝置：

```
/dev/
├── sda                      ← 第一顆硬碟
│   ├── sda1                 ← 第一顆硬碟的第一個分割區
│   └── sda2                 ← 第一顆硬碟的第二個分割區
├── null                     ← 特殊的「黑洞」裝置（丟掉的資料不見）
├── zero                     ← 讀取永遠返回 0
├── random                   ← 隨機數產生器
└── tty                      ← 終端機
```

> **有趣的事**：在 Terminal 執行 `cat /dev/urandom | head -c 10`，你會看到亂碼！因為 `/dev/urandom` 是「亂數產生器」。

### /proc — 系統資訊（記憶體中的檔案）

**類比 Windows**：工作管理員（Task Manager）看到的資訊

```
/proc/
├── 1/                       ← 程序 ID 1（通常是 systemd）
│   ├── cmdline              ← 這個程序的啟動指令
│   ├── meminfo              ← 記憶體使用情況
│   └── status               ← 程序狀態
├── cpuinfo                  ← CPU 資訊
├── meminfo                  ← 記憶體資訊
└── version                  ← Linux核心版本
```

> **注意**：`/proc` 和 `/dev` 的檔案不是真正的檔案，是作業系統即時產生的「虛擬檔案」，用來讓你用指令讀取系統資訊。

---

## 🔍 路徑：絕對路徑 vs 相對路徑

### 絕對路徑 — 從根目錄開始的完整地址

**類比**：完整的門牌地址「台北市信義區...」

```bash
/home/caro/Documents/report.txt
/etc/fwupd/daemon.conf
/usr/bin/gcc
```

**特點**：
- 一定以 `/` 開頭
- 從根目錄 `/` 開始計算
- 無論你在哪裡，絕對路徑都是固定的

### 相對路徑 — 從目前位置開始

**類比**：說「樓上臥室」而不是「台北市...」

```bash
# 如果你在 /home/caro
Documents/report.txt        ← 等同 /home/caro/Documents/report.txt
../caro/Desktop/            ← 從上一層目錄開始算
```

**常用相對路徑符號**：

| 符號 | 意義 | 範例 |
|------|------|------|
| `.` | 目前目錄 | `./script.sh`（執行目前的 script.sh） |
| `..` | 上一層目錄 | `../file.txt`（上一層的 file.txt） |
| `~` | 家目錄 | `~/Downloads/` |

### 實例演練

```bash
# 假設你在 /home/caro/Documents

pwd                          # 顯示 /home/caro/Documents

ls                           # 列出目前目錄內容
ls ..                        # 列出上一層（/home/caro）內容
ls ../Desktop                # 列出上一層的 Desktop 目錄
ls ~/Downloads               # 列出家目錄的 Downloads（不論你在哪裡）
```

---

## 🔐 檔案權限 — Linux 的安全機制

### 先想想 Windows 的權限

在 Windows，如果你是管理員，理論上可以做任何事：
- 可以刪改 System32 的檔案（Windows 會警告但通常允許）
- 任何程式都可以修改系統設定

### Linux 的權限模型

Linux 的設計更嚴格：

1. **每個檔案/資料夾都有「主人」和「隸屬群組」**
2. **權限分三種**：讀取（Read）、寫入（Write）、執行（Execute）
3. **三個身份**分別設定：擁有者（Owner）、群組（Group）、其他人（Others）

### 查看權限

```bash
ls -l /home/caro
```

輸出：
```
drwxr-xr-x  3 caro caro 4096 Mar 28 10:00 Documents
-rw-r--r--  1 caro caro  220 Mar 27 09:30 .bashrc
-rwxr-xr-x  1 root root 36864 Mar 27 09:30 fwupd
```

讓我們解讀第一欄 `drwxr-xr-x`：

```
d  rwx  r-x  r-x
│   │    │    │
│   │    │    └── 其他人的權限（r-x = 可讀、不可寫、可執行）
│   │    └─────── 群組的權限（r-x = 可讀、不可寫、可執行）
│   └──────────── 擁有者的權限（rwx = 可讀、可寫、可執行）
└──────────────── 檔案類型（d = 目錄，- = 一般檔案）
```

**權限代號**：

| 代號 | 意義 | 數字 |
|------|------|------|
| `r` | Read（讀取） | 4 |
| `w` | Write（寫入/修改） | 2 |
| `x` | Execute（執行） | 1 |

> **rwx = 4+2+1 = 7**  
> **r-x = 4+0+1 = 5**  
> **--- = 0+0+0 = 0**

### chmod — 改變檔案權限

**類比**：「設定誰可以讀/寫/執行這個檔案」

```bash
# 語法：chmod 權限 檔案

chmod 755 script.sh          # 設定 rwxr-xr-x（擁有者可讀寫執行，其他人可讀執行）
chmod 644 file.txt           # 設定 rw-r--r--（擁有者可讀寫，其他人只能讀）
chmod 700 secret.txt         # 設定 rwx------（只有擁有者能用）
chmod +x program            # 給所有人執行權限（+x = 加執行）
chmod -w file.txt           # 移除寫入權限
```

**常用數字組合**：

| 數字 | 意義 | 用途 |
|------|------|------|
| `777` | rwxrwxrwx | 完全開放（不安全！） |
| `755` | rwxr-xr-x | 常用於資料夾、腳本 |
| `644` | rw-r--r-- | 常用於一般檔案 |
| `600` | rw------- | 只有自己能讀寫的秘密檔案 |
| `700` | rwx------ | 只有自己能用的程式 |

### chown — 改變檔案擁有者

**類比**：「把這個檔案「給」別人」

```bash
# 語法：chown 擁有者:群組 檔案

chown caro:caro file.txt     # 把 file.txt 的擁有者改成 caro
chown root:root script.sh   # 把 script.sh 改成 root 擁有
chown -R caro:caro folder/  # 遞迴改變整個資料夾（-R = recursive）
```

### 實際例子

```bash
# 建立一個腳本
touch myscript.sh

# 查看它的權限（預設 644 = rw-r--r--）
ls -l myscript.sh
# -rw-r--r-- 1 caro caro 0 Mar 28 10:00 myscript.sh

# 讓它可以被執行
chmod +x myscript.sh

# 再次查看
ls -l myscript.sh
# -rwxr-xr-x 1 caro caro 0 Mar 28 10:00 myscript.sh
# ↑ 多了 x
```

---

## 📁 磁碟掛載 — Linux 如何「看到」額外儲存裝置

### Windows 的方式

在 Windows，插 USB 隨身碟後，會自動變成「E:」磁碟機。

### Linux 的方式：掛載（Mount）

在 Linux，你要手動把一個裝置「掛」到目錄樹的某個位置：

```bash
# 查看目前掛載的裝置
df -h

# 手動掛載 USB（通常 /dev/sdb1 是 USB）
sudo mount /dev/sdb1 /mnt/usb

# 卸載（移除 USB 前要執行）
sudo umount /mnt/usb
```

> **為什麼要麻煩掛載？** 因為 Linux 的設計哲學是「一切皆檔案」，所以把 USB 變成一個資料夾，存取方式完全一致。

**自動掛載的常見位置**：
- `/media/使用者名稱/隨身碟名稱` — Ubuntu 等桌面 Linux 自動掛載在這
- `/mnt/` — 手工掛載點

---

## 🔑 重點整理

1. **Linux 只有一個根目錄 `/`**，所有東西都在這棵樹下
2. **`/home`** 是使用者的個人檔案（等同 `C:\Users\`）
3. **`/etc`** 是系統設定檔（等同控制台 + Registry）
4. **`/var`** 是會變動的資料（日誌、資料庫）
5. **絕對路徑**從 `/` 開始，相對路徑從目前目錄開始
6. **`chmod`** 改變權限，`chown` 改變擁有者
7. **讀寫執行**三權限，數字代表 4/2/1

---

## ▶️ 下一步

權限概念很重要！接下來 [第4章：使用者與權限](./04_User_Permission.md) 會深入講解 root、sudo 和使用者群組。

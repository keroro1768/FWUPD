# 第2章：命令列基礎 — 開始和 Linux 說話

> **本章目標**：學會使用終端機（Terminal），掌握基本 Linux 指令  
> **前置知識**：了解什麼是 Linux（第一章）

---

## 🖥️ 什麼是 Terminal？

### Windows 的類比

在 Windows 世界，你有兩種方式操作電腦：

1. **圖形介面（GUI）**：用滑鼠點擊資料夾、雙擊開啟程式
2. **命令列（CLI）**：用文字指令操作電腦

```
Windows 命令列工具：
┌────────────────────────────────────────────────────┐
│                                                    │
│  CMD（命令提示字元）                                 │
│  - 傳統的黑色視窗                                   │
│  - 指令較簡單，功能有限                             │
│  - 快速、小巧                                       │
│                                                    │
└────────────────────────────────────────────────────┘

PowerShell
┌────────────────────────────────────────────────────┐
│                                                    │
│  - 藍色視窗                                         │
│  - 比 CMD 更強大                                   │
│  - 指令比較長，但功能更完整                          │
│  - Windows 10/11 內建                              │
│                                                    │
└────────────────────────────────────────────────────┘
```

### Linux 的 Terminal（終端機）

Linux 的命令列工具叫做 **Terminal**（終端機）或 **Console**（控制台）。

> **術語**：終端機、終端、Console、Shell — 這些詞常被混用，但意思差不多，都是「用文字指令操作電腦的程式」

### 開啟 Terminal

**在 Ubuntu（WSL）開啟方式：**
- 按 `Win + R`，輸入 `ubuntu` 或 `wsl`
- 或從開始選單找「Ubuntu」

**在真正的 Linux 桌面：**
- 按 `Ctrl + Alt + T`（通用快捷鍵）
- 或在應用程式選單找「Terminal」

---

## 💬 你的第一個 Linux 指令

打開 Terminal，你會看到這樣的畫面：

```
user@hostname:~$
```

讓我們解讀一下：

| 符號 | 意義 |
|------|------|
| `user` | 目前登入的使用者名稱 |
| `@` | 「在」的意思 |
| `hostname` | 電腦的名稱 |
| `:` | 分隔符號 |
| `~` | 目前所在的資料夾（`~` 是家目錄，之後會解釋） |
| `$` | 普通使用者提示符號（如果是 `#` 代表管理員） |

### 輸入你的第一個指令

試著輸入：

```bash
echo "Hello, Linux!"
```

按 Enter，你會看到輸出：

```
Hello, Linux!
```

> **什麼是 `echo`？**  
> `echo` 就像 CMD 的 `echo` 或 PowerShell 的 `Write-Output`，用來顯示文字。

---

## 📁 基本檔案指令

### ls — 列出檔案（等同 Windows `dir`）

**Windows CMD：**
```cmd
C:\> dir
```

**Linux：**
```bash
ls
```

**Windows PowerShell：**
```powershell
Get-ChildItem
```

### ls 的常用選項

```bash
ls              # 基本列出
ls -l           # 詳細列表（顯示權限、大小、日期）
ls -a           # 顯示所有檔案（包含隱藏檔）
ls -la          # 詳細列表 + 顯示隱藏檔
ls -lh          # 顯示人類可讀的大小（KB、MB）
```

> **為什麼 Linux 有「隱藏檔案」？**  
> 以 `.` 開頭的檔案就是隱藏檔。在 Linux 中，設定檔通常會隱藏起來（檔名前面加 `.`），讓目錄看起來更整潔。

### cd — 切換資料夾（等同 Windows `cd`）

```bash
cd /home           # 切換到 /home 目錄
cd ..              # 回到上一層目錄
cd ~               # 回到自己的家目錄（/home/你的使用者名稱）
cd                 # 不带參數，預設也是回家目錄
cd /usr/bin        # 切換到絕對路徑
```

> **提示**：`~` 是捷徑，代表「家目錄」，等同於 `/home/你的使用者名稱`

### pwd — 顯示現在位置（等同 Windows `cd` 不帶參數）

```bash
pwd
```

輸出：
```
/home/caro
```

### mkdir — 建立資料夾（等同 Windows `mkdir` 或 `md`）

```bash
mkdir myproject           # 建立一個叫 myproject 的資料夾
mkdir -p test/subfolder    # 建立多層目錄（如果父資料夾不存在也一起建立）
```

### rm — 刪除檔案（等同 Windows `del`）

```bash
rm myfile.txt             # 刪除檔案
rm -r myfolder            # 刪除資料夾和裡面的所有內容（-r = recursive）
rm -rf myfolder           # 強制刪除，不詢問（危險！看清楚再打）
```

> ⚠️ **警告**：`rm -rf` 會直接刪除，不會放到資源回收筒！這是少數 Linux 中「無法復原」的動作。

### cp — 複製檔案（等同 Windows `copy` 或 PowerShell `Copy-Item`）

```bash
cp source.txt dest.txt           # 複製檔案
cp file1.txt file2.txt backup/   # 複製多個檔案到 backup 資料夾
cp -r myfolder/ newfolder/       # 複製整個資料夾（需要 -r）
cp -i source.txt dest.txt        # 互動模式，覆蓋前會詢問（-i = interactive）
```

### mv — 移動或改名（等同 Windows `move`）

```bash
mv oldname.txt newname.txt       # 將檔案改名
mv file.txt /home/user/docs/     # 將檔案移動到其他目錄
mv myfolder/ ..                  # 將資料夾移動到上一層
```

> **小技巧**：`mv` 也是重新命名的指令！在 Linux 中沒有「改名」指令，就是用 `mv` 處理。

### cat — 顯示檔案內容（等同 Windows `type` 或 PowerShell `Get-Content`）

```bash
cat myfile.txt                   # 顯示檔案全部內容
cat file1.txt file2.txt          # 顯示多個檔案的內容
cat -n myfile.txt                # 顯示行號（-n = number）
```

---

## 🔧 其他實用的基本指令

### touch — 建立空檔案

```bash
touch newfile.txt        # 建立一個空的文字檔
touch app.log            # 建立一個空的日誌檔
```

### which — 找出指令的路徑

```bash
which ls                 # 找出 ls 這個指令實際在哪裡
```

輸出：
```
/usr/bin/ls
```

### man — 查看指令說明（等同 Windows `/?`）

```bash
man ls                   # 查看 ls 的完整說明文件
man cat                  # 查看 cat 的說明
```

> 按 `q` 離開說明畫面，按空白鍵往下翻頁

### history — 查看歷史指令

```bash
history                  # 列出最近用過的指令
```

### clear — 清空畫面

```bash
clear                    # 等同 CMD 的 `cls`
```

---

## 📦 套件管理器 — Linux 的軟體商店

### 什麼是套件管理器？

在 Windows，你安裝軟體通常是：
1. 去官網下載 `.exe` 或 `.msi` 安裝程式
2. 執行安裝程式，一路按「下一步」

在 Linux，你通常是：
1. 輸入一行指令，軟體自動下載並安裝
2. 輸入一行指令，軟體自動更新

這個「一行指令安裝軟體」的工具，就是 **套件管理器（Package Manager）**。

> **類比**：套件管理器就像 Windows 的「控制台 > 程式和功能」，但更強大。你不需要手動下載，指令會自動幫你找到、安裝、更新、移除軟體。

### 常見的套件管理器

| 套件管理器 | 使用的發行版 | 指令格式 |
|-----------|------------|---------|
| **apt** | Ubuntu, Debian | `apt install 軟體名` |
| **dnf** | Fedora, RHEL | `dnf install 軟體名` |
| **yum** | 舊版 Fedora/RHEL | `yum install 軟體名` |
| **pacman** | Arch Linux | `pacman -S 軟體名` |

### apt 指令示範

```bash
# 更新套件列表（類似檢查更新）
sudo apt update

# 安裝軟體
sudo apt install firefox

# 移除軟體
sudo apt remove firefox

# 升級已安裝的軟體
sudo apt upgrade
```

> **注意**：`sudo` 是什麼？這是「暫時取得系統管理員權限」的指令。稍後第四章會詳細解釋。現在你只要知道，系統級操作需要 `sudo`。

### dnf 指令示範

```bash
# 更新套件列表
sudo dnf check-update

# 安裝軟體
sudo dnf install firefox

# 移除軟體
sudo dnf remove firefox

# 升級所有套件
sudo dnf upgrade
```

---

## 📊 CMD / PowerShell / Linux 指令對照表

| 功能 | Windows CMD | Windows PowerShell | Linux Bash |
|------|-------------|-------------------|------------|
| 列出檔案 | `dir` | `Get-ChildItem` | `ls` |
| 切換資料夾 | `cd` | `cd` | `cd` |
| 顯示現在位置 | `cd` (無參數) | `Get-Location` | `pwd` |
| 建立資料夾 | `mkdir` | `New-Item -ItemType Directory` | `mkdir` |
| 刪除檔案 | `del` | `Remove-Item` | `rm` |
| 刪除資料夾 | `rmdir` | `Remove-Item` | `rm -r` |
| 複製檔案 | `copy` | `Copy-Item` | `cp` |
| 移動/改名 | `move` | `Move-Item` | `mv` |
| 顯示檔案內容 | `type` | `Get-Content` | `cat` |
| 建立空檔案 | `type nul > file` | `New-Item file` | `touch` |
| 複製資料夾 | `xcopy /s` | `Copy-Item -Recurse` | `cp -r` |
| 清除畫面 | `cls` | `Clear-Host` | `clear` |
| 說明 | `command /?` | `Get-Help command` | `man command` |
| 安裝軟體 | - | - | `sudo apt install` |

---

## 💡 實用技巧

### Tab 自動完成

輸入指令時，按 `Tab` 鍵可以自動完成：

```bash
# 輸入到一半
$ cd /usr/b

# 按 Tab，自動變成
$ cd /usr/bin/
```

### 上/下箭頭瀏覽歷史

- `↑`（上箭頭）：上一個指令
- `↓`（下箭頭）：下一個指令

### Ctrl + C 取消

按 `Ctrl + C` 可以取消當前輸入或正在跑的指令：

```bash
$ sudo apt install         # 打到一半後悔了
^C                          # 按 Ctrl + C 取消
```

### Ctrl + L 清空畫面

等同一 `clear`

---

## 🔑 重點整理

1. **Terminal** 是 Linux 的命令列工具，用文字指令操作電腦
2. **`ls`、`cd`、`pwd`、`mkdir`、`rm`、`cp`、`mv`、`cat`** 是最基本的指令
3. **套件管理器**（apt/dnf）讓你一行指令就能安裝或移除軟體
4. **`sudo`** 是暫時取得系統管理員權限的前綴詞
5. **`man`** 指令可以查看任何指令的說明文件

---

## �手動操作練習

在 Terminal 中試試以下操作：

```bash
# 1. 顯示現在位置
pwd

# 2. 列出目前目錄的所有檔案
ls -la

# 3. 建立一個練習資料夾
mkdir practice

# 4. 進入練習資料夾
cd practice

# 5. 建立一個空檔案
touch hello.txt

# 6. 顯示剛才建立的檔案
ls

# 7. 回到上一層
cd ..

# 8. 刪除練習資料夾
rm -rf practice
```

---

## ▶️ 下一步

現在你會用命令列了！接下來學習 [第3章：檔案系統](./03_File_System.md)，了解 Linux 的檔案放在哪裡，以及「目錄樹」的概念。

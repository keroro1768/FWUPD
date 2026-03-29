# Linux 基礎知識庫 — 學習地圖

> 專為 Windows 工程師打造的 Linux 入門指南  
> 目標：讓你理解 Linux 基礎知識，足以應對 FWUPD 開發任務

---

## 📍 這份知識庫的目標讀者

**你是一位 Windows 工程師**，日常開發環境是：
- Windows 10/11
- Visual Studio、VS Code
- CMD 或 PowerShell
- Git Bash 或 WSL（可能有一點接觸）

**現在你需要接觸 Linux**，可能是：
- 為 FWUPD 專案撰寫 Linux 原生程式碼
- 在 Linux 環境測試韌體更新功能
- 部署或維護 Linux 伺服器

**這份知識庫的使命：讓你從零到能操作 Linux。**

---

## 🗺️ 學習路線圖

建議依序閱讀，每章大約 10-15 分鐘：

```
第1章 → 第2章 → 第3章 → 第4章 → 第5章 → 第6章 → 第7章 → 第8章 → 第9章
 │         │        │        │        │        │        │        │
 └─► 第1步：理解 Linux 是什麼（和 Windows 的差異）
           │
           └─► 第2步：學會在命令列操作（等同 CMD/PowerShell）
                     │
                     └─► 第3步：搞懂檔案系統結構（等同 Windows 的 C:\）
                               │
                               └─► 第4步：理解使用者與權限（安全機制）
                                         │
                                         └─► 第5步：會安裝軟體（等同控制台/安裝程式）
                                                   │
                                                   └─► 第6步：理解背景服務（等同 Windows 服務）
                                                             │
                                                             └─► 第7步：知道電腦怎麼開機（除錯用）
                                                                       │
                                                                       └─► 第8步：網路指令（除錯用）
                                                                                 │
                                                                                 └─► 第9步：開發工具（寫碼用）
```

---

## 📖 章節一覽

| 章節 | 主題 | 核心問題 | 預計時間 |
|------|------|----------|----------|
| [01](./01_Windows_vs_Linux.md) | Windows vs Linux | Linux 是什麼？和 Windows 有什麼不同？ | 15 min |
| [02](./02_Command_Line.md) | 命令列基礎 | 怎麼在 Linux 打字下指令？ | 15 min |
| [03](./03_File_System.md) | 檔案系統 | Linux 的檔案放在哪？路徑怎麼看？ | 15 min |
| [04](./04_User_Permission.md) | 使用者與權限 | 為什麼不是管理員也能做事？ | 10 min |
| [05](./05_Package_Manager.md) | 套件管理 | 怎麼安裝軟體？fwupd 怎麼裝？ | 15 min |
| [06](./06_Service_Daemon.md) | 服務與 Daemon | 什麼是背景程式？fwupd daemon 是什麼？ | 15 min |
| [07](./07_Startup_Process.md) | 開機流程 | Linux 開機時發生了什麼？ | 15 min |
| [08](./08_Network_Command.md) | 網路基礎 | 怎麼測網路、連線到其他機器？ | 15 min |
| [09](./09_Development_Tools.md) | 開發工具 | 怎麼編譯程式、怎麼用 Git？ | 20 min |

---

## 🔧 實驗環境建議

### 方案一：WSL（推薦，Windows 原生）
```powershell
# 在 PowerShell 安裝 WSL
wsl --install
```
- 優點：不需要VM，直接在 Windows 執行 Linux
- 適用：日常練習、輕量使用

### 方案二：虛擬機（完整體驗）
- **VirtualBox**（免費）
- **VMware Workstation Player**（免費）
- 下載 Ubuntu Desktop ISO 安裝

### 方案三：雲端伺服器（最接近伺服器環境）
- **AWS EC2**（有免費方案）
- **Google Cloud**（有免費方案）
- **Azure**（有免費方案）

### 方案四：雙重開機（高手選項）
- 在實體機安裝 Linux，和 Windows dual boot
- 效能最好，但調整較麻煩

---

## 📚 參考資源

### 官方文件
- [ArchWiki](https://wiki.archlinux.org/) — Linux 百科全書，內容最詳盡
- [Ubuntu Tutorial](https://ubuntu.com/tutorials/) — Ubuntu 官方教學
- [Debian Documentation](https://www.debian.org/doc/) — Debian 官方文件
- [Fedora Documentation](https://docs.fedoraproject.org/) — Fedora 官方文件

### FWUPD 相關
- [fwupd 官方網站](https://fwupd.org/) — 韌體更新工具官方站
- [fwupd GitHub](https://github.com/fwupd/fwupd) — 原始碼
- [fwupd LVFS](https://fwupd.org/lvfs) — 韌體資料庫

### 工具文件
- [systemd 官方文件](https://www.freedesktop.org/wiki/Software/systemd/)
- [GNOME Wiki](https://wiki.gnome.org/) — Linux 桌面環境
- [GNU Coreutils](https://www.gnu.org/software/coreutils/) — 基本指令文件

---

## ⚡ 快速查詢表

當你忘記指令時，回來查這裡：

| 動作 | Windows CMD | Windows PowerShell | Linux Bash |
|------|-------------|---------------------|------------|
| 列出檔案 | `dir` | `Get-ChildItem` | `ls` |
| 切換資料夾 | `cd` | `cd` | `cd` |
| 顯示現在位置 | `cd` (無參數) | `Get-Location` | `pwd` |
| 建立資料夾 | `mkdir` | `New-Item -ItemType Directory` | `mkdir` |
| 刪除檔案 | `del` | `Remove-Item` | `rm` |
| 複製檔案 | `copy` | `Copy-Item` | `cp` |
| 移動/改名 | `move` | `Move-Item` | `mv` |
| 查看內容 | `type` | `Get-Content` | `cat` |
| 安裝軟體 | - | - | `apt install` |
| 系統服務 | `services.msc` | `Get-Service` | `systemctl` |
| 網路設定 | `ipconfig` | `Get-NetIPAddress` | `ip` |

---

## 🆘 遇到問題？

### 常見問題

**Q: 我打錯字了怎麼辦？**
- 在 Linux 命令列，按 `Ctrl + C` 可以取消當前輸入（等同 CMD 的 `Ctrl + C`）

**Q: 怎麼複製貼上？**
- `Ctrl + Shift + C` 複製，`Ctrl + Shift + V` 貼上（在終端機中）
- 或者用滑鼠選取文字，按右鍵貼上

**Q: 指令跑不出來？**
- 先檢查拼字是否正確
- 嘗試按 `Tab` 鍵自動完成
- 加上 `--help` 參數看說明，例如：`ls --help`

**Q: 怎麼離開終端機？**
- 輸入 `exit` 即可離開

### 下一步

準備好了嗎？從 [第1章：Windows vs Linux](./01_Windows_vs_Linux.md) 開始吧！ 🚀

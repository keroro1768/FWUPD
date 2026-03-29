# 第9章：開發工具 — 從編譯到版本控制

> **本章目標**：學會使用 GCC、Make、CMake、Git 等開發工具  
> **前置知識**：會用命令列操作檔案（第二、三章）

---

## 🔨 先想一下 Windows 怎麼編譯程式

在 Windows，如果用 Visual Studio：

```
1. 雙擊 .sln 或 .vcxproj 檔案
2. Visual Studio 幫你管理所有專案設定
3. 按下「建置」按鈕
4. IDE 自動呼叫編譯器、連結器
5. 產生 .exe 檔案
```

在 Linux，沒有 Visual Studio 這樣的「一鍵完成」工具。你需要學會：
- **編譯器**：把程式碼翻譯成機器碼
- **建置系統**：管理「哪個檔案先編譯、哪個後編譯」
- **版本控制**：追蹤程式碼的修改歷史

---

## ⚙️ GCC — C/C++ 編譯器

### 什麼是 GCC？

**GCC** = **G**NU **C**omplier **C**ollection

這是 Linux 下最流行的 C/C++ 編譯器。幾乎所有 Linux 系統都預設安裝。

> **歷史**：原本只是 C 編譯器（GNU C Compiler），後來支援更多語言（C++, Objective-C, Fortran, Ada 等），所以改名為「編譯器集合」。

### 基本用法

```bash
# 編譯 C 程式
gcc hello.c -o hello

# 執行編譯後的程式
./hello

# 編譯 C++ 程式
g++ hello.cpp -o hello
```

### 實際範例

**步驟 1**：建立一個 C 程式

```bash
nano hello.c
```

輸入：
```c
#include <stdio.h>

int main() {
    printf("Hello, Linux!\n");
    return 0;
}
```

按 `Ctrl + O` 儲存，`Ctrl + X` 離開。

**步驟 2**：編譯

```bash
gcc hello.c -o hello

# 如果沒有錯誤，不會顯示任何東西
# 如果有錯誤，會顯示錯誤訊息
```

**步驟 3**：執行

```bash
./hello

# 輸出：
# Hello, Linux!
```

### 常用編譯選項

| 選項 | 意義 | 範例 |
|------|------|------|
| `-o` | 指定輸出檔名 | `gcc -o myapp main.c` |
| `-Wall` | 顯示所有警告 | `gcc -Wall main.c` |
| `-g` | 加入除錯資訊 | `gcc -g main.c` |
| `-O2` | 最佳化等級 | `gcc -O2 main.c` |
| `-l` | 連結程式庫 | `gcc -o prog prog.c -lm` |
| `-I` | 加入 include 路徑 | `gcc -I./include main.c` |
| `-c` | 只編譯不連結（產生 .o 檔） | `gcc -c main.c` |

### 多檔案編譯

```bash
# 假設有 three files: main.c, foo.c, foo.h

# 編譯每個 .c 檔
gcc -c main.c
gcc -c foo.c

# 連結所有 .o 檔
gcc -o myapp main.o foo.o
```

---

## 📜 Make — 自動化建置工具

### 什麼是 Make？

**Make** 是一個「自動化建置工具」。它讀取 `Makefile`，知道「哪些檔案依賴哪些」，只重新編譯需要更新的檔案。

> **類比**：Make 就像一個「聰明的助理」。你告訴他「A 依賴 B 和 C」，下次只改 B 的話，他就只重新編譯 A 和 B，不動 C。

### 為什麼需要 Make？

想像你有 100 個 .c 檔的專案：
- 改了一個檔案，就要全部重新編譯嗎？
- 手動一個個編譯會累死人
- Make 自動幫你追蹤依賴關係

### Makefile 基本語法

```makefile
# 這是一個 Makefile

# 目標: 依賴
# [Tab] 要執行的指令

# 編譯整個專案
all: myapp

# 連結 myapp
myapp: main.o foo.o bar.o
    gcc -o myapp main.o foo.o bar.o

# 編譯 main.o
main.o: main.c foo.h
    gcc -c main.c

# 編譯 foo.o
foo.o: foo.c foo.h
    gcc -c foo.c

# 編譯 bar.o
bar.o: bar.c bar.h
    gcc -c bar.c

# 清理
clean:
    rm -f *.o myapp
```

### Make 常用指令

```bash
make              # 執行 Makefile 中的第一個目標
make all          # 編譯整個專案
make clean        # 刪除編譯產物
make myapp        # 編譯特定目標
make -n           # 預演模式（只顯示要做什麼，不實際執行）
make -j4          # 用 4 個執行緒平行編譯（加速）
```

### 簡化的 Makefile

實際上，`*.c` 依賴 `*.h` 的話，可以寫得很精簡：

```makefile
CC = gcc
CFLAGS = -Wall -g
TARGET = myapp
OBJ = main.o foo.o

all: $(TARGET)

$(TARGET): $(OBJ)
    $(CC) -o $(TARGET) $(OBJ)

%.o: %.c
    $(CC) $(CFLAGS) -c $<

clean:
    rm -f $(OBJ) $(TARGET)

.PHONY: all clean
```

---

## 🔧 CMake — 跨平台建置系統

### 什麼是 CMake？

**CMake** = **C**ross-platform **Make**

Make 的問題是：**不同的系統有不一樣的 Make 語法**（Windows 用 NMake，Linux 用 Make）。

CMake 的解決方案：**寫一份 `CMakeLists.txt`，CMake 自動產生你系統的 Makefile**。

> **類比**：CMake 像一個「翻譯機」，你告訴它「我要建置一個叫 myapp 的程式，用 GCC」，它幫你產生 Linux Makefile 或 Windows Visual Studio 專案。

### CMake 基本流程

```
1. 寫 CMakeLists.txt（設定檔）
2. cmake .                 ← 執行 CMake，產生 Makefile
3. make                    ← 用 make 編譯
```

### CMakeLists.txt 範例

```cmake
# CMake 最低版本需求
cmake_minimum_required(VERSION 3.10)

# 專案名稱
project(MyApp VERSION 1.0.0)

# 加入 C++ 標準
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 尋找一個套件
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)

# 告訴 CMake 原始碼在哪裡
add_executable(myapp
    main.cpp
    foo.cpp
    bar.cpp
)

# 加入 include 目錄
target_include_directories(myapp PRIVATE ${GLIB2_INCLUDE_DIRS})

# 加入程式庫
target_link_libraries(myapp ${GLIB2_LIBRARIES})

# 安裝目標
install(TARGETS myapp DESTINATION bin)
```

### CMake 常用指令

```bash
# 在目前目錄執行 CMake（產生 Makefile）
cmake .

# 在 build/ 目錄執行 CMake（推薦，保持原始碼乾淨）
mkdir build
cd build
cmake ..
make

# 指定產生的 Generator
cmake .. -G "Unix Makefiles"    # Linux
cmake .. -G "Visual Studio 17"  # Windows Visual Studio

# 清除並重新 CMake
rm -rf build && mkdir build && cd build && cmake ..

# 顯示詳細輸出
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
```

### CMake 目錄結構（推薦）

```
project/
├── CMakeLists.txt           # 頂層 CMakeLists
├── src/
│   ├── CMakeLists.txt       # src 的 CMakeLists
│   ├── main.cpp
│   ├── foo.cpp
│   └── foo.h
├── include/
│   └── foo.h                # 標頭檔
├── tests/
│   └── test_foo.cpp
├── build/                   # 編譯產物放這裡（不要放原始碼目錄）
└── README.md
```

---

## 📚 Git — 版本控制

### 什麼是版本控制？

**版本控制**就像遊戲的「存檔」功能，讓你能：

- 追蹤程式碼的修改歷史
- 多人同時編輯同一份程式碼
- 隨時回到之前的版本
- 建立分支（branch）開發新功能，不影響主線

### 基本概念

| 概念 | 說明 | 類比 |
|------|------|------|
| **Repository（Repo）** | 專案存放的地方（含所有歷史） | 遊戲存檔資料夾 |
| **Commit** | 一次「存檔」，記錄一組改動 | 遊戲存檔點 |
| **Branch** | 分支，平行開發不同功能 | 平行世界的你 |
| **Merge** | 合併分支 | 平行世界合併 |
| **Clone** | 複製 Repo 到本機 | 複製資料夾 |
| **Push** | 把本機改動上傳到遠端 | 上傳雲端 |
| **Pull** | 從遠端下載最新改動 | 從雲端下載 |

### Git 工作流程

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  Working Directory     Staging Area (Index)    Repository  │
│  （工作目錄）              （暫存區）                （版本庫）   │
│                                                             │
│     main.c                   ↑                              │
│        ↓                     │                              │
│     git add main.c ----→  main.c                           │
│        ↓                                                   │
│     git commit ------→  [一個 commit 形成]                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Git 基本指令

```bash
# 初始化一個新 Repo（在資料夾內）
git init

# 複製一個 Repo 到本機
git clone https://github.com/example/repo.git

# 查看目前狀態
git status

# 查看未暫存的修改
git diff

# 加入檔案到暫存區
git add file.txt
git add .                 # 加入所有修改

# 提交（建立一個 commit）
git commit -m "Add new feature"

# 查看歷史
git log
git log --oneline         # 精簡版

# 查看遠端設定
git remote -v

# 推送到遠端
git push origin main

# 拉取最新改動
git pull origin main
```

### Branch（分支）

```bash
# 查看所有分支
git branch

# 建立新分支
git branch new-feature

# 切換到新分支
git checkout new-feature
# 或（新語法）
git switch new-feature

# 建立並切換（新語法）
git switch -c new-feature

# 合併分支（先切到要合併過去的分支）
git checkout main
git merge new-feature

# 刪除分支
git branch -d new-feature
```

### .gitignore — 忽略不需要追蹤的檔案

在 Repo 根目錄建立 `.gitignore` 檔案：

```
# 編譯產物
*.o
*.exe
build/

# 隱藏檔案
.DS_Store
Thumbs.db

# 敏感資訊
*.pem
*.key
.env

# IDE
.vscode/
.idea/
```

---

## 🔗 fwupd 開發環境建置（實際案例）

fwupd 是一個用 C 寫的 Linux 韌體更新工具。讓我們實際演練如何建置開發環境：

### 安裝依賴（Ubuntu）

```bash
# 更新套件
sudo apt update

# 安裝編譯工具鍊
sudo apt install build-essential gcc g++ pkg-config

# 安裝 fwupd 開發依賴
sudo apt install libglib2.0-dev libxml2-dev libjson-glib-dev \
    libcurl4-openssl-dev libfwupd-dev meson \
    gnome-common libgusb-dev help2man

# 安裝測試工具
sudo apt install umockdev gjs
```

### 下載原始碼

```bash
# 用 Git 克隆 fwupd
git clone https://github.com/fwupd/fwupd.git
cd fwupd

# 查看分支
git branch -a

# 切換到 stable 版
git checkout 1.9.0
```

### 用 Meson 建置（fwupd 用 Meson，不是 CMake）

```bash
# 建立 build 目錄
meson setup build

# 編譯
meson compile -C build

# 安裝
sudo meson install -C build

# 如果要清除並重新開始
rm -rf build && meson setup build
```

### 查看 fwupd daemon 狀態

```bash
# 重啟 fwupd 服務（需要重新安裝後）
sudo systemctl restart fwupd

# 查看狀態
systemctl status fwupd

# 查看 fwupd 版本
fwupdmgr --version

# 查看可用裝置
fwupdmgr get-devices
```

---

## 🔑 重點整理

1. **GCC** = C/C++ 編譯器，把 .c/.cpp 變成執行檔
2. **Make** = 自動化建置工具，讀 Makefile 編譯專案
3. **CMake** = 跨平台的建置系統產生器，產生 Makefile 或 VS 專案
4. **Git** = 版本控制，追蹤程式碼歷史、多人協作
5. **Clone → Add → Commit → Push → Pull** 是基本的 Git 工作流程
6. fwupd 用 **Meson** 建置系統（不是 CMake 或 Make）

---

## 📚 延伸學習資源

- [GNU GCC Manual](https://gcc.gnu.org/onlinedocs/)
- [GNU Make Manual](https://www.gnu.org/software/make/manual/)
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)
- [Git 官方文件](https://git-scm.com/book/zh-tw/v2)
- [fwupd 開發文件](https://github.com/fwupd/fwupd/tree/master/docs)

---

## 🎉 恭喜完成！

你已經完成 Linux 基礎知識庫的全部 9 章！

**你現在應該能**：
- [x] 理解 Linux 和 Windows 的差異
- [x] 在 Terminal 操作檔案
- [x] 理解 Linux 的目錄結構
- [x] 理解使用者與權限模型
- [x] 用套件管理器安裝軟體
- [x] 理解 systemd 和服務管理
- [x] 知道 Linux 開機時發生了什麼
- [x] 用網路指令除錯
- [x] 用 GCC/Make/Git 開發程式

**下一步**：
- 在你的 Linux 環境（WSL、VM、雲端）實際操作練習
- 開始閱讀 [fwupd 開發文件](https://github.com/fwupd/fwupd)
- 加入 Linux 社群，提問題、學更多！

祝學習順利！ 🚀

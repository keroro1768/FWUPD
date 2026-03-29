# FWUPD 系統架構

## 架構總覽

FWUPD 的架構可分為四層，从上到下分别是：

```
┌─────────────────────────────────────────────────────────────┐
│                    客戶端層（Client Layer）                   │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐  │
│  │  fwupdmgr    │  │ GNOME        │  │  Other Tooling    │  │
│  │  (CLI Tool)  │  │ Software     │  │  (API Consumers)  │  │
│  └──────┬───────┘  └──────┬───────┘  └────────┬─────────┘  │
│         │                 │                    │             │
│         └────────────┬────┴────────────────────┘             │
│                      │ D-Bus (org.freedesktop.fwupd)        │
├──────────────────────┼──────────────────────────────────────┤
│                      │                                      │
│              ┌───────▼────────┐                             │
│              │  fwupd daemon  │  ← 系統服務（systemd 管理）   │
│              │  (Main Process)│                             │
│              └───────┬────────┘                             │
│                      │                                      │
│         ┌────────────┼────────────┐                         │
│         │            │            │                         │
│  ┌──────▼─────┐ ┌─────▼────┐ ┌────▼─────────┐                │
│  │  Plugin    │ │  Plugin  │ │  Plugin      │                │
│  │  Manager   │ │  (UEFI)  │ │  (USB/...)   │                │
│  └──────┬─────┘ └────┬─────┘ └──────┬───────┘                │
│         │            │             │                        │
│         └────────────┼─────────────┘                        │
│                      │ Plugin Dispatch                       │
├──────────────────────┼──────────────────────────────────────┤
│                      │                                      │
│  ┌───────────────────▼───────────────────────────────────┐  │
│  │              Plugin Instances (每個硬體一個)            │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │
│  │  │ UEFI     │  │ USB      │  │ SPI      │  ...       │  │
│  │  │ Plugin   │  │ Plugin   │  │ Plugin   │            │  │
│  │  └──────────┘  └──────────┘  └──────────┘            │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│                      硬體層（Hardware Layer）                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │ UEFI/    │  │ USB      │  │ SPI      │  │ EMMC/    │    │
│  │ BIOS     │  │ Device   │  │ Flash    │  │ NOR Flash│    │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘    │
├──────────────────────────────────────────────────────────────┤
│                   LVFS（外部服務）                            │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ https://fwupd.org/lvfs                                   │ │
│  │ 提供：韌體元資料下載、韌體 CAB 檔案、簽章驗證             │ │
│  └────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

---

## 各層元件說明

### 1. 客戶端層（Client Layer）

#### fwupdmgr（CLI 工具）
FWUPD 的命令行前端，提供與 daemon 互動的所有功能。

常用子命令：
```bash
fwupdmgr get-devices       # 列舉所有已知且支援更新的裝置
fwupdmgr get-updates       # 檢查 LVFS 上所有可用更新
fwupdmgr update            # 安裝待處理的更新
fwupdmgr install <FILE>    # 從本地 CAB 檔案安裝
fwupdmgr verify            # 驗證裝置韌體簽章
fwupdmgr refresh           # 強制刷新 LVFS 元資料快取
fwupdmgr dump              # 傾印所有已知裝置的詳細資訊
fwupdmgr clear-results     # 清除歷史更新結果
fwupdmgr get-history       # 查看過去的更新記錄
```

#### GNOME Software / 其他前端
GNOME Software（Fedora、Ubuntu 預設軟體中心）透過 D-Bus 整合 FWUPD，讓使用者在軟體中心直接看到韌體更新通知。

---

### 2. D-Bus 介面層

FWUPD daemon 暴露以下 D-Bus 介面：
- **Bus 名稱**：`org.freedesktop.fwupd`
- **Object 路徑**：`/`
- **主要介面**：`org.freedesktop.fwupd`

客戶端透過以下方式溝通：
```python
# Python 範例（使用 pydbus）
from pydbus import SessionBus
bus = SessionBus()
fwupd = bus.get('org.freedesktop.fwupd', '/')

# 取得所有裝置
devices = fwupd.GetDevices()
```

---

### 3. fwupd Daemon（核心）

#### 架構模組

```
fwupd Daemon
├── Main Loop (GLib Event Loop, GMainLoop)
├── D-Bus Service (GDBusConnection)
├── Plugin Manager
│   ├── Plugin Loader (動態載入 .so 插件)
│   ├── Plugin Registry
│   └── Plugin Chain (插件執行順序)
├── Device Manager
│   ├── Device Enumeration
│   ├── Device State Machine
│   └── Device Results Cache
├── Security Attributes (HSI 支援)
├── History Database (SQLite: /var/lib/fwupd/pending.db)
├── Local Metadata Store (/var/lib/fwupd/metadata/)
└── Config Store (/etc/fwupd/)
```

#### 關鍵設計特性

**1. 單例 daemon**
系統中只有一個 fwupd daemon 實例運行，透過 systemd socket activation 機制啟動。

**2. 插件隔離**
每個插件運行在同一程序空間，但透過 `FuPlugin` 結構管理生命週期和錯誤。

**3. 非同步作業**
所有 I/O 作業（LVFS 查詢、韌體讀寫）皆為非同步，防止 daemon 阻塞。

**4. 交易機制**
更新作業以「交易」為單位，支援原子性的提交或回滾。

---

### 4. 插件系統（Plugin System）

插件是 FWUPD 實質功能的核心，每種硬體類型對應一個插件。

#### 插件結構

```c
// 典型插件結構（fu-plugin-example.c）
#include "fu-plugin.h"
#include "fu-chunk.h"

struct _FuExamplePlugin {
    FuPlugin parent;
    // 私有資料
};

static void
fu_example_plugin_init (FuExamplePlugin *self)
{
    fu_plugin_set_build_hash (FU_PLUGIN (self), BUILD_HASH);
    fu_plugin_add_flag (FU_PLUGIN (self), FWUPD_PLUGIN_FLAG_NONE);
}

static gboolean
fu_example_plugin_write_firmware (FuPlugin *plugin,
                                   FuDevice *device,
                                   GBytes *blob_fw,
                                   FwupdInstallFlags flags,
                                   GError **error)
{
    // 實際寫入韌體的邏輯
    // 這是每個插件的核心實作
}

static void
fu_example_plugin_device_registered (FuPlugin *plugin, FuDevice *device)
{
    // 當新裝置插入或被偵測到時觸發
    // 通常用於設定裝置特定屬性
}

fu_plugin_builtin_add (FU_PLUGIN (self), TRUE);  // 內建插件
```

#### 插件生命週期鉤子

| 鉤子 | 時機 | 用途 |
|------|------|------|
| `constructed` | 插件物件建立後 | 初始化、註冊硬體ID |
| `startup` | daemon 啟動時 | 搶佔式初始化（如 ESRT） |
| `shutdown` | daemon 關閉時 | 清理資源 |
| `device_registered` | 新裝置登錄時 | 附加額外元資料/規則 |
| `device_probe` | 探測裝置時 | 讀取識別資訊 |
| `device_open` | 裝置開啟更新階段前 | 開啟硬體存取 |
| `write_firmware` | 寫入韌體時 | 執行實際燒錄 |
| `attach` | 進入更新模式前 | 將裝置切換到更新模式 |
| `detach` | 離開更新模式後 | 將裝置恢復正常模式 |
| `verify` | 驗證韌體時 | 讀取當前版本 |

---

## 資料流向

### 典型更新流程

```
1. fwupdmgr get-updates
   │
   ▼
2. Daemon 查詢 LVFS 元資料 (HTTPS/REST)
   │
   ▼
3. LVFS 返回可用更新列表（含 CAB URL、版本資訊）
   │
   ▼
4. Daemon 比對本地版本 → 顯示可更新項目
   │
   ▼
5. fwupdmgr update <device-id>
   │
   ▼
6. Daemon 下載 CAB 檔案（驗證簽章）
   │
   ▼
7. 根據裝置類型調用對應 Plugin
   │
   ▼
8. Plugin.write_firmware() → 燒錄到硬體
   │
   ▼
9. 結果寫入 History DB → 回傳狀態
```

---

## 檔案系統結構

```
/etc/fwupd/
├──.conf               # 主設定檔
├──daemon.conf         # Daemon 行為設定
├── plugin.conf        # 插件設定（如 UEFI 金鑰路徑）
└──/remotes.d/         # LVFS 和其他遠端設定
    ├── lvfs.conf      # LVFS 遠端
    └── vendor.conf    # 供應商私有遠端（企業部署用）

/var/lib/fwupd/
├── metadata/          # LVFS 下載的元資料（XML）
├── pending.db         # SQLite: 待處理/歷史更新記錄
├── reports/           # 匿名使用統計（可選）
└── auth/
    └── *.policy        # D-Bus 授權策略

/usr/lib/fwupd/
├── plugins/           # 插件 .so 檔案
│   ├── ufnvme.so
│   ├── uefi_capsule.so
│   ├── synaptics_mst.so
│   └── ...
└── metainfo/          # 預設 metainfo XML

/usr/share/fwupd/
├── device.dtd
└── quirks.d/          # 硬體快速識別資料（quirks）
    └── *.quirk
```

---

## 安全性模型

### 簽章驗證流程

```
CAB File (含韌體 + metainfo.xml + 簽章)
        │
        ▼
   ┌─────────────┐
   │  SHA256     │ ← 計算 CAB 內容摘要
   │  Checksum   │
   └──────┬──────┘
          │
          ▼
   ┌─────────────────┐
   │  LVFS / Local   │ ← 使用供應商私鑰簽章
   │  Signing Key    │
   └──────┬──────────┘
          │
          ▼
   ┌─────────────────┐
   │  HSI (Industry) │ ← 硬體安全介面
   │  Verification   │
   └──────┬──────────┘
          │
          ▼
   Device accepts firmware (or rejects)
```

### 信任鏈
1. **LVFS 端**：供應商上傳韌體 → LVFS 驗證供應商身份 → 簽章
2. **客戶端**：使用 LVFS 公開金鑰驗證 → 使用硬體的金鑰驗證（如 UEFI DB）
3. **UEFI Level**：ESRT + UEFI Update Capsule 標準

---

## 元資料格式

LVFS 提供的元資料為 XML 格式（`compendium.xml.gz`），結構如下：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<compendium version="1.0">
  <release version="1.2.3" urgency="high">
    <location>https://fwupd.org/downloads/...</location>
    <checksum type="sha1">abc123...</checksum>
    <description>
      <p>Bug fixes and security updates</p>
    </description>
  </release>
</compendium>
```

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

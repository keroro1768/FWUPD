# FWUPD HID I2C 產品韌體更新流程 — 總覽

## 1. 系統架構

```
┌─────────────────────────────────────────────────────────────┐
│                      使用者端 (Client)                       │
│  ┌──────────────┐   ┌──────────────┐   ┌───────────────┐  │
│  │ GNOME Software│   │  fwupdmgr    │   │  其他 GUI/Tools │  │
│  └──────┬───────┘   └──────┬───────┘   └───────┬───────┘  │
│         └───────────────────┼───────────────────┘          │
│                             ▼                               │
│                    ┌──────────────────┐                     │
│                    │    fwupd daemon  │                     │
│                    │  (fwupd.service)  │                     │
│                    └────────┬─────────┘                     │
│                             │                                │
│         ┌───────────────────┼───────────────────┐            │
│         ▼                   ▼                   ▼            │
│  ┌────────────┐     ┌────────────┐     ┌──────────────┐   │
│  │ USB Plugin │     │ HID Plugin  │     │ I2C/SPI/etc. │   │
│  └────────────┘     └────────────┘     └──────────────┘   │
└─────────────────────────────────────────────────────────────┘
                             │
                             ▼
              ┌──────────────────────────────┐
              │   Linux Kernel (hid-i2c,     │
              │    i2c-dev, udev)             │
              └──────────────────────────────┘
                             │
                             ▼
              ┌──────────────────────────────┐
              │   HID I2C 裝置 (自家產品)     │
              │   - I2C Master (主控端)       │
              │   - HID Protocol (I2C transport) │
              └──────────────────────────────┘

另外一路：
                             │
                             ▼
              ┌──────────────────────────────┐
              │   Linux Vendor Firmware       │
              │   Service (LVFS)             │
              │   fwupd.org/lvfs             │
              │                              │
              │  ┌─ private  (僅自己可見)     │
              │  ├─ embargo  (同 vendor group)│
              │  ├─ testing  (公開測試用戶)   │
              │  └─ stable   (正式版)         │
              └──────────────────────────────┘
```

## 2. 完整流程：開發 → 測試 → 部署 → 正式發布

```
階段 1: Plugin 開發 (本機)
────────────────────────────────────────────────────
  1. 分析自家 HID I2C 裝置的韌體更新協定
     ├── 定義 Protocol ID (e.g. com.company.i2chid)
     ├── 分析讀取/寫入韌體的 HID Report 格式
     └── 定義 detach (切換 bootloader) / attach (回 runtime) 流程
  
  2. 建立 fwupd plugin 目錄結構
     ├── fu-xxx-plugin.h / fu-xxx-plugin.c
     ├── fu-xxx-device.h / fu-xxx-device.c
     ├── fu-xxx-firmware.h / fu-xxx-firmware.c (視需要)
     ├── meson.build
     └── README.md
  
  3. 實作 FuPlugin 核心函數
     ├── coldplug()       → 枚舉並註冊 I2C HID 裝置
     ├── probe()          → 讀取 VID/PID/GUID
     ├── setup()          → 讀取韌體版本等資訊
     ├── detach()         → 切換至 bootloader 模式
     ├── attach()         → 切回 runtime 模式
     ├── write_firmware() → 將韌體寫入裝置
     └── reload()         → 重新讀取版本確認更新成功
  
  4. 建立 .quirk 檔案（用於快速匹配硬體）
  
  5. 編譯並本地測試 (meson + ninja)
     ├── fwupdmgr get-devices        → 確認裝置被偵測
     └── fwupdmgr install xxx.cab    → 測試韌體更新

階段 2: LVFS 帳號申請與設定
────────────────────────────────────────────────────
  1. 在 GitLab 開 ticket (lvfs-website repo)
     ├── 公司完整法律名稱
     ├── 官方網址
     ├── 公司 logo (高解析度)
     ├── 電子郵件 domain (用於認證)
     ├── 使用的更新協定 (custom HID I2C)
     ├── 韌體上傳許可文件 (法律文件)
     ├── Vendor ID (USB/HID VID)
     ├── AppStream reverse-DNS 前綴 (e.g. com.company)
     └── 安全事件回報 URL (PSIRT)
  
  2. 收到 LVFS 管理團隊審核回覆 (可能需幾天)
  
  3. LVFS 管理團隊建立 vendor group，並給予 trusted 狀態

階段 3: 本地離線測試
────────────────────────────────────────────────────
  A. 離線 .cab 安裝 (不需要網路)
     $ fwupdmgr install firmware.cab
  
  B. 建立私有 remote (同一網域內部部署)
     ├── 方式一: 使用 sync-pulp.py 同步 LVFS
     ├── 方式二: 建立 local remote (file://)
     └── 方式三: 把 .cab 檔放進 /usr/share/fwupd/remotes.d/
  
  C. 使用 fwupdtool 進行完整端對端測試

階段 4: LVFS 線上測試
────────────────────────────────────────────────────
  1. 上傳 firmware.cab + metainfo.xml 至 LVFS (private remote)
  
  2. 移到 vendor-embargo remote
     → 從 LVFS 下載 vendor-embargo.conf
     → fwupdmgr refresh 取得新 metadata
     → 在實際硬體上測試
  
  3. 移到 testing remote
     → 公開測試用戶可見
     → 收集使用者回報
  
  4. 移到 stable remote
     → 正式發布給所有用戶

階段 5: 正式發布與持續維護
────────────────────────────────────────────────────
  1. 在 LVFS stable 確認發布成功
     $ fwupdmgr refresh && fwupdmgr update
  
  2. 監控使用者回報 (report-history)
  
  3. 後續韌體版本: 重複上傳 → embargo → testing → stable 流程
```

## 3. 核心元件對應關係

| 自家產品的元件 | fwupd 對應元件 | 說明 |
|---|---|---|
| HID I2C 硬體 | FuUdevDevice (或 FuI2cDevice) | 透過 /dev/i2c-X 溝通 |
| HID Report Protocol | FuHidDevice | 處理 HID 報告格式 |
| 客製化更新協定 | FuPlugin (新建立) | 實作 write_firmware() |
| 韌體格式 (.bin) | FuFirmware (subclass) | 解析自家二進位格式 |
| 韌體版本號 | metainfo.xml <release version=""> | LVFS 需要的 metadata |

## 4. HID I2C 在 Linux 中的位置

HID I2C 是一個複合層：

```
使用者空間應用程式
        │
        ▼
   [hidraw driver]  ← 標準 HID 裝置節點 (/dev/hidrawX)
        │
        ▼
   [hid-i2c-core driver]  ← Linux kernel HID I2C transport layer
        │
        ▼
   [I2C bus driver]  ← 主機上的 I2C adapter driver
        │
        ▼
   [HID I2C 裝置 (自家產品)]
```

fwupd 的 `hidpp` plugin (Logitech) 和 `elantp` plugin (Elan TouchPad) 
都同時支援 HID 和原始 I2C 模式，可做為重要參考。

## 5. 與 ChromeOS EC Plugin 的對照

ChromeOS EC plugin (`cros-ec`) 的設計重點：
- Protocol ID: `com.google.usb.crosec`
- 支援 USB 和未來的 I2C 更新方式
- 使用 `REPLUG_MATCH_GUID` 處理 runtime/bootloader 模式切換
- 韌體格式: Google fmap file format

自家產品的 HID I2C plugin 可類似設計：
- 自訂 Protocol ID: `com.company.i2chid`
- 同時支援 HID (hidraw) 和原始 I2C (i2c-dev) 模式
- 参考 `elantp` plugin 的 I2C fallback recovery 機制

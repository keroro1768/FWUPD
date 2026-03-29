# 本地離線測試

## 1. 測試環境建置

### 1.1 基本 fwupd 安裝

```bash
# Ubuntu/Debian
sudo apt install fwupd fwupd-tools

# 檢查版本
fwupdmgr --version

# 確認 fwupd service 正在運行
systemctl status fwupd
```

### 1.2 建置開發版 fwupd（用於 Plugin 開發）

```bash
# 編譯 fwupd (包含本地修改的 plugin)
cd fwupd
meson setup build -Dplugin_tests=true
ninja -C build

# 以開發模式運行 fwupd daemon (不需要安裝)
sudo ./build/libexec/fwupd -v --plugins 'company-i2c-hid'
```

## 2. 離線 .cab 安裝測試

### 2.1 建立測試用的 .cab 檔案

在本地測試時，先建立一個測試用的 .cab archive：

```
# 目錄結構
test-firmware/
├── firmware.metainfo.xml
└── firmware.bin
```

**firmware.metainfo.xml**：
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- Copyright 2024 Your Company Name -->
<component type="firmware">
  <id>com.company.i2chid.firmware</id>
  <name>Company I2C HID Device</name>
  <summary>Firmware for Company I2C HID Device</summary>
  <description>
    <p>Test firmware update for Company I2C HID Device</p>
  </description>
  <provides>
    <firmware type="flashed">12345678-1234-1234-1234-123456789abc</firmware>
  </provides>
  <releases>
    <release urgency="high" version="1.0.1" date="2024-01-01">
      <checksum filename="firmware.bin" target="content"/>
      <description>
        <p>Initial test release</p>
      </description>
    </release>
  </releases>
  <custom>
    <value key="LVFS::UpdateProtocol">com.company.i2chid</value>
    <value key="LVFS::VersionFormat">triplet</value>
  </custom>
</component>
```

### 2.2 建立 .cab 檔案

```bash
# Linux: 使用 gcab
sudo apt install gcc  # for gcab
gcab -c -v test-firmware.cab firmware.metainfo.xml firmware.bin

# Windows: 使用 makecab
# 建立 config.txt:
.OPTION EXPLICIT
.set Cabinet=on
.set Compress=on
.set MaxDiskSize=0
.set DiskDirectoryTemplate=.
.set DestinationDir=DriverPackage
firmware.metainfo.xml
firmware.bin

# 執行:
makecab /F config.txt
# 輸出為 1.cab，命名為 test.cab
rename 1.cab test-firmware.cab
```

### 2.3 安裝離線 .cab

```bash
# 安裝離線韌體 (不需要網路)
sudo fwupdmgr install test-firmware.cab

# 或使用 --allow-reinstall --force
sudo fwupdmgr install --allow-reinstall --force test-firmware.cab

# 查看詳細輸出
sudo fwupdmgr install -v test-firmware.cab

# 查看安裝歷史
fwupdmgr get-history
```

## 3. 使用 Local Remote（離線部署）

若要在組織內大量部署，但不連網際網路：

### 3.1 方式一：Vendor Directory（最簡單）

將 .cab 檔案放到固定目錄，fwupd 直接讀取：

```bash
# 1. 建立目錄
sudo mkdir -p /usr/share/fwupd/remotes.d/vendor-directory

# 2. 複製 .cab 檔案
sudo cp *.cab /usr/share/fwupd/remotes.d/vendor-directory/

# 3. 啟用 vendor-directory remote
sudo fwupdmgr enable-remote vendor-directory

# 4. 確認啟用
cat /etc/fwupd/remotes.d/vendor-directory.conf
```

建立 `vendor-directory.conf`：
```ini
[fwupd Remote]
Enabled=true
Type=local
Directory=/usr/share/fwupd/remotes.d/vendor-directory
```

### 3.2 方式二：使用 sync-pulp.py 同步（有網路時）

若公司網路可以連到 LVFS，但終端機器無法上網：

```bash
# 1. 在有網路的機器上執行同步（sync-pulp.py 完整下載 URL）
python3 sync-pulp.py https://fwupd.org/downloads /mnt/lvfs-mirror \
    --username=your@company.com \
    --token=YOUR_TOKEN_HERE \
    --filter-tag=company-2024q1

# sync-pulp.py 腳本位置：
# https://gitlab.com/fwupd/lvfs-website/-/raw/master/contrib/sync-pulp.py

# 2. 將同步後的目錄放到 NFS/Samba 分享
# 3. 終端機器掛載並設定 remote

# /etc/fwupd/remotes.d/company-mirror.conf:
[fwupd Remote]
Enabled=true
Title=Company Firmware Mirror
Keyring=none
MetadataURI=file:///mnt/company-firmware/fwupd/remotes.d/firmware.xml.gz
FirmwareBaseURI=file:///mnt/company-firmware/fwupd/remotes.d

# 4. 停用公共 LVFS (可選)
sudo fwupdmgr disable-remote lvfs

# 5. 刷新並更新
sudo fwupdmgr refresh
sudo fwupdmgr update
```

### 3.3 方式三：完全私有 LVFS 部署

自行架設 LVFS 實例（適用於大型組織）：

```bash
# LVFS 是 Python Flask 應用
# 需要 PostgreSQL + Redis + Celery
# 詳細架設方式見: https://github.com/fwupd/lvfs

# 好處：
# - 完全控制韌體發布
# - 可以控制部署節奏（每天 1000 台上限）
# - 完整的失敗回報統計
```

## 4. fwupdtool 進階測試

`fwupdtool` 是 fwupd 的命令列工具，提供更多低階操作：

```bash
# 取得所有裝置（含詳細資訊）
sudo fwupdtool get-devices --verbose

# 顯示特定裝置的詳細資訊
sudo fwupdtool get-details "Company I2C HID Device"

# 取得所有 GUIDs
sudo fwupdtool hwids

# 匯出硬體 ID 到檔案
sudo fwupdtool hwids > hwids.txt

# 安裝並產生報告（用於離線分析）
sudo fwupdmgr install test-firmware.cab
sudo fwupdmgr report-history --sign

# 匯出報告
sudo fwupdmgr report-export --sign > test-report.fwupdreport

# 清除所有本地快取
sudo rm -rf /var/lib/fwupd/manifests/*
sudo fwupdmgr refresh
```

## 5. Plugin 開發除錯

### 5.1 以 Debug 模式運行 Plugin

```bash
# 設定環境變數讓特定 plugin 輸出詳細資訊
FWUPD_COMPANY_I2C_HID_VERBOSE=1 sudo systemctl restart fwupd

# 或直接用命令列
sudo fwupd --plugin-verbose=company-i2c-hid -v

# 查看 journal 輸出
journalctl -u fwupd -f
```

### 5.2 在 Plugin 中加入 Debug 輸出

```c
#include <fwupdplugin.h>

/* 在關鍵步驟加入 g_debug() */
static gboolean
fu_company_i2c_hid_device_write_firmware(...)
{
  g_debug("Starting firmware write, size=%zu", sz);
  g_debug("Sending enter-bootloader command...");

  if (FWUPD_COMPANY_I2C_HID_VERBOSE()) {
    g_debug("Writing block %u/%u", i, total);
  }
}
```

### 5.3 常見除錯情境

| 問題 | 除錯方式 |
|---|---|
| 裝置偵測不到 | 檢查 udev rules、`ls /dev/hidraw*`、`ls /dev/i2c-*` |
| Plugin 沒被載入 | `fwupdmgr get-plugins`、檢查 `meson.build` |
| 寫入失敗 | 加入 g_debug、使用 `strace -e write,read` |
| 版本比對失敗 | `fwupdmgr get-devices -v` 確認版本格式 |
| 權限問題 | 確認使用者群組是 plugdev、檢查 udev rules |

## 6. 端對端測試檢查清單

在正式送 LVFS 之前，確認以下測試都通過：

```
□ 1. 裝置偵測
   □ fwupdmgr get-devices 顯示正確的 VID/PID
   □ GUID 正確匹配

□ 2. 韌體版本讀取
   □ fu_device_set_version() 顯示正確版本格式
   □ 版本格式符合 metainfo.xml 的 LVFS::VersionFormat

□ 3. 韌體更新流程
   □ fwupdmgr install xxx.cab 成功完成
   □ 更新後版本號正確增加
   □ 更新失敗時不會磚化裝置（I2C recovery 可用）

□ 4. 離線安裝
   □ .cab 檔可以離線安裝
   □ 本地 remote 可正常運作

□ 5. 防降級測試
   □ 嘗試安裝舊版韌體被拒絕
   □ --allow-downgrade 強制降級成功

□ 6. 多設備測試
   □ 多個同型號設備同時插上可正確區分
   □ GUID 衝突檢查
```

## 7. 使用模擬硬體測試

若硬體尚未完成，可以先用 mock 測試：

```c
/* 在 coldplug 中加入 mock 支援 */
static gboolean
fu_company_i2c_hid_plugin_coldplug(FuPlugin *plugin, ...)
{
  /* 環境變數控制是否使用 mock */
  if (g_getenv("COMPANY_I2C_HID_MOCK") != NULL) {
    /* 建立 mock 裝置 */
    g_autoptr(FuDevice) dev = fu_device_new(NULL);
    fu_device_set_id(dev, "MockI2CHid-1:0");
    fu_device_set_version(dev, "1.0.0");
    fu_device_add_instance_id(dev, "MOCK\\VEN_ABCD&PID_1234");
    fu_device_add_flag(dev, FWUPD_DEVICE_FLAG_UPDATABLE);
    fu_plugin_add_device(plugin, dev);
    return TRUE;
  }
  /* 正常邏輯 */
}
```

```bash
# 使用 mock 模式測試
COMPANY_I2C_HID_MOCK=1 sudo systemctl restart fwupd
fwupdmgr get-devices
```

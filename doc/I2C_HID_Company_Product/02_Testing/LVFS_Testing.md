# LVFS 線上測試

## 1. LVFS 測試流程架構

```
┌──────────────────────────────────────────────────────────────────┐
│                        LVFS Server (fwupd.org)                    │
│                                                                   │
│   ┌──────────┐                                                    │
│   │ Private  │ ← 只有上傳者自己可見（帳號創立初期）                  │
│   └────┬─────┘                                                    │
│        ▼                                                          │
│   ┌──────────┐                                                    │
│   │ Embargo  │ ← 同 vendor group 可見，用於内部 QA 測試            │
│   └────┬─────┘                                                    │
│        ▼                                                          │
│   ┌──────────┐                                                    │
│   │ Testing  │ ← 公開給已啟用 testing 的用户（數千人）               │
│   └────┬─────┘                                                    │
│        ▼                                                          │
│   ┌──────────┐                                                    │
│   │ Stable   │ ← 正式發布，數千萬用戶可見                          │
│   └──────────┘                                                    │
└──────────────────────────────────────────────────────────────────┘
```

## 2. 上傳前的準備

### 2.1 建立 Cabinet Archive

完整的 .cab 檔案必須包含：
1. `firmware.metainfo.xml` — 韌體 metadata
2. `firmware.bin`（或自訂名稱）— 實際韌體 binary

```
firmware.cab/
├── firmware.metainfo.xml
└── firmware.bin
```

### 2.2 完整的 metainfo.xml 範例

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- Copyright 2024 Your Company Name -->
<component type="firmware">
  <!-- AppStream ID (必須唯一) -->
  <id>com.company.i2chid-device.firmware</id>

  <!-- 裝置名稱 -->
  <name>Company I2C HID Device</name>
  <name_variant_suffix lang="zh-TW">公司 I2C HID 裝置</name_variant_suffix>

  <!-- 摘要（一句話） -->
  <summary>Firmware update for Company I2C HID Device</summary>

  <!-- 詳細描述 -->
  <description>
    <p>Updating the firmware on your Company I2C HID Device improves
    performance and adds new features.</p>
    <p>This update includes bug fixes and stability improvements.</p>
  </description>

  <!-- 提供哪些 GUID (必須與 plugin 產生的 GUID 匹配) -->
  <provides>
    <firmware type="flashed">12345678-1234-1234-1234-123456789abc</firmware>
    <firmware type="flashed">87654321-4321-4321-4321-cba987654321</firmware>
  </provides>

  <!-- 官方網站 -->
  <url type="homepage">https://www.company.com/support</url>

  <!--  Licensing -->
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>Proprietary</project_license>

  <!--  韌體發布資訊 -->
  <releases>
    <release
      urgency="high"
      version="1.2.5"
      date="2024-03-01"
      install_duration="30">
      <checksum filename="company-firmware-v1.2.5.bin" target="content"/>
      <description>
        <p>This stable release fixes the following issues:</p>
        <ul>
          <li>Fixed connection stability issue</li>
          <li>Improved power consumption</li>
          <li>Added support for new hardware revision</li>
        </ul>
      </description>
      <issues>
        <issue type="cve">CVE-2024-12345</issue>
      </issues>
    </release>
  </releases>

  <!--  更新類別 -->
  <categories>
    <category>X-EmbeddedController</category>
  </categories>

  <!--  自定義 metadata -->
  <custom>
    <!-- 與 Plugin 的 protocol ID 一致 -->
    <value key="LVFS::UpdateProtocol">com.company.i2chid</value>
    <!-- 版本格式：triplet, quad, intel-me 等 -->
    <value key="LVFS::VersionFormat">triplet</value>
    <!--  設備完整性：signed 或 unsigned -->
    <value key="LVFS::DeviceIntegrity">signed</value>
  </custom>

  <!--  安裝前置需求 -->
  <requires>
    <!-- 要求最低 fwupd 版本 -->
    <id compare="ge" version="1.5.0">org.freedesktop.fwupd</id>
    <!-- 若需要特定 bootloader 版本 -->
    <firmware compare="ge" version="0.3.0">bootloader</firmware>
  </requires>

  <!--  顯示螢幕截圖（可選） -->
  <screenshots>
    <screenshot type="default">
      <caption>Update instructions</caption>
      <image>https://example.com/screenshots/update.png</image>
    </screenshot>
  </screenshots>
</component>
```

### 2.3 命名建議

```
# 建議的 .cab 檔案命名
company-i2chid-device-v1.2.5.cab

# 建議的韌體 binary 命名
company-i2chid-device-v1.2.5.bin
```

## 3. 手動上傳到 LVFS

### 3.1 透過 LVFS 網頁上傳

1. 登入 https://fwupd.org/lvfs/
2. 點擊 "Upload" 分頁
3. 選擇 .cab 檔案上傳
4. 填入 release notes（可選）
5. 點擊 "Upload"

### 3.2 透過指令碼自動上傳

```bash
# 產生使用者 token (在 LVFS 網頁的 Profile 設定中)
# 使用 curl 上傳

curl -X POST -F file=@company-i2chid-device-v1.2.5.cab \
     https://fwupd.org/lvfs/upload/token \
     --user "your@company.com:USER_TOKEN_HERE"
```

## 4. Embargo Remote 測試（最重要）

### 4.1 什麼是 embargo remote

Embargo remote 是 vendor group 內部可見的測試 remote。
上傳到 LVFS 的新韌體，預設就在 embargo 狀態。

### 4.2 下載 vendor-embargo.conf

1. 登入 LVFS
2. 前往 https://fwupd.org/lvfs/metadata/
3. 找到 "Embargo Remote" 區塊
4. 下載 `vendor-embargo.conf`

### 4.3 設定本地環境

```bash
# 1. 將下載的 conf 放到正確位置
sudo cp vendor-embargo.conf /etc/fwupd/remotes.d/

# 2. 編輯 conf，填入 LVFS 帳號 email
# 找到 Username= 行，填入 email
Username=your@company.com

# 3. 產生 LVFS user token
# 前往 https://fwupd.org/lvfs/profile
# 點擊 "Generate new token"
# 將 token 填入 conf 的 Password= 行

# 4. 刷新 metadata
sudo fwupdmgr refresh

# 5. 確認 embargo remote 已啟用
fwupdmgr remotes

# 預期輸出:
# company-embargo  [Enabled]  ← 這是 vendor-embargo remote 的簡稱
```

### 4.4 在實際硬體上進行端對端測試

```bash
# 1. 確認裝置被偵測
sudo fwupdmgr get-devices

# 2. 檢查是否有可用更新
sudo fwupdmgr check-update

# 3. 執行更新
sudo fwupdmgr update

# 4. 若有問題，查看詳細輸出
sudo fwupdmgr update -v

# 5. 查看更新歷史
fwupdmgr get-history

# 6. 提交已測試的回報（用於 LVFS 的 QA 追蹤）
sudo fwupdmgr report-history --sign
```

### 4.5 預期失敗情境的測試

```bash
# 測試 1: 韌體校驗失敗
# 使用錯誤的韌體 binary (錯誤的 VID/PID)
# 預期: fwupd 拒絕安裝並顯示錯誤

# 測試 2: 版本降級被阻擋
sudo fwupdmgr install --allow-reinstall --force old-version.cab
# 預期: 若設定了 version_lowest，應該被阻止

# 測試 3: 韌體更新中斷
# 在 write_firmware 中途中斷電源
# 預期: 裝置應可通過 I2C recovery 恢復
```

## 5. 移到 Testing Remote

### 5.1 條件

在移到 testing 之前，必須：
- ✅ 至少有一個成功的 embargo remote 端對端測試
- ✅ metainfo.xml 的測試全部通過（見下節）
- ✅ firmware 沒有 blocklist 問題（如 "DO NOT SHIP" 字樣）

### 5.2 在 LVFS 網頁操作

1. 登入 LVFS
2. 進入 firmware 頁面
3. 點擊 "Actions" → "Move to testing"
4. 確認移動

### 5.3 使用者層面

移到 testing 後，已啟用 testing 的公共用戶（`fwupdmgr cfg test-enabled`）
會看到這個更新。數量通常是數千人。

```bash
# 啟用 testing remote（確認已啟用）
fwupdmgr remotes
# 若未啟用：
fwupdmgr enable-remote vendor-testing

# 刷新 metadata
fwupdmgr refresh

# 之後就會收到 testing 版本的更新
fwupdmgr check-update
```

**注意：** `TestChecksEnabled` 的作用是在更新前驗證測試結果，
並非用於啟用接收 testing remote 的更新。

## 6. LVFS 測試（Test Cases）

上傳時 LVFS 會自動執行以下測試：

### 6.1 測試失敗類型

| 測試類型 | 失敗原因 | 解決方式 |
|---|---|---|
| Blocklist | binary 含 "DO NOT SHIP" | 移除 binary 中的 blocklist 字串 |
| UEFI Capsule | 缺少正確的 capsule header | 使用正確的 capsule 工具包裝 |
| DFU Footer | DFU 格式但缺少 UFD footer | 使用 dfu-tool 新增 footer |
| Microcode | 微碼版本降級 | 上傳正確版本 |
| PE Check | EFI shard 簽章過期 | 更新簽章 |

### 6.2 常見失敗：Blocklist

```
# 會觸發失敗的字串（不能出現在 binary 中）：
- "DO NOT SHIP"
- "To Be Defined By O.E.M"
- "TBD"
- 空白或預留空間的字元

# 解決方式：
strings firmware.bin | grep -E "DO NOT|SHIP|TBD"
# 若有輸出，修改韌體移除這些字串
```

### 6.3 版本格式警告

若 `LVFS::VersionFormat` 未設定，LVFS 會顯示警告。
確保在 metainfo.xml 中正確設定：

```xml
<custom>
  <value key="LVFS::VersionFormat">triplet</value>
</custom>
```

常見格式：
- `triplet`：如 "1.2.3"（最常用）
- `quad`：如 "1.2.3.4"（如 Intel ME）
- `intel-me`：Intel ME 專用格式
- `bcd`：如 "0x1234"

## 7. 使用 Signed Reports 進行可信測試

### 7.1 為何要 Signed Reports

普通的 `fwupdmgr report-history` 回報可以被偽造。
Signed report 使用本機憑證對回報內容進行簽章，
讓 LVFS 可以驗證這是真正在指定硬體上測試過的結果。

### 7.2 設定 Signed Reports

```bash
# 1. 上傳測試機器的憑證
# 前往 https://fwupd.org/lvfs/profile
# 點擊 "Upload Certificate"
# 選擇 ~/.local/share/fwupd/client.pem

# 2. 執行測試並簽章回報
fwupdmgr refresh
fwupdmgr update
# ... 重啟 ...
fwupdmgr report-history --sign

# 3. 在 LVFS 網頁驗證
# 前往 firmware 頁面的 "Reports" 標籤
# 應顯示 "Signed by: 你的 machine certificate"
```

### 7.3 使用 Token 的 Signed Reports

若 CI/CD 環境不方便上傳憑證：

```bash
# 設定 LVFS remote 的 username 和 password
fwupdmgr modify-remote company-embargo Username your@company.com
fwupdmgr modify-remote company-embargo Password YOUR_TOKEN

# 之後的 report-history 自動視為 signed
fwupdmgr report-history
```

## 8. 移到 Stable Remote

### 8.1 最後檢查清單

```
□ 硬體端對端測試完成
□ 所有 LVFS 測試通過（或已 waiver）
□ Release notes 正確填寫
□ 版本格式正確
□ Security issue (CVE) 已正確標註（如有）
□ Legal document 已上傳（允許 LVFS 重新發布韌體）
```

### 8.2 移到 Stable

1. 在 LVFS 網頁，點擊 "Actions" → "Move to stable"
2. 確認移動

### 8.3 發布後的等待

```
⚠ 重要：移動到 stable 後
- metadata 需要幾小時重新產生
- 用戶端最多需要 24 小時才會收到更新通知
- 大多數廠商在發布後第一天會看到大量下載
```

## 9. ChromeOS 特定的 LVFS 測試

若產品是 ChromeOS 生態系的一部分，見：
https://lvfs.readthedocs.io/en/latest/chromeos.html

重點：
- 需要在上傳前在 ChromeOS 硬體上完成測試
- CPFE (ChromeOS Partner Firmware Engine) 用於上傳到 Google 伺服器
- LVFS 上仍需上傳可重新發布的 .cab（用於 regression testing）

## 10. 測試失敗的除錯

### 10.1 查看 LVFS 測試結果

在 LVFS 網頁 firmware 頁面的 "Tests" 標籤下可以看到所有測試結果。

### 10.2 常見問題

```
Q: 上傳成功但始終沒有出現在 embargo remote
A: 等待 5-10 分鐘讓 LVFS 處理，確認 vendor-embargo.conf 設定正確

Q: fwupdmgr refresh 看不到更新
A: 檢查 GUID 是否匹配，確認 Instance ID 與 metainfo.xml 中的 GUID 一致

Q: 版本格式警告無法消除
A: 在 metainfo.xml 的 <custom> 中加入正確的 LVFS::VersionFormat

Q: 測試全部通過但仍無法移到 stable
A: 可能 vendor account 尚未 unlock (需要 legal document)，聯繫 LVFS 管理者
```

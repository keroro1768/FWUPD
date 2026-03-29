# 正式發布流程

## 1. 發布前的最後檢查

### 1.1 硬體與韌體檢查

```
□ Plugin 已在 fwupd 原始碼中 (或準備好發布)
□ Plugin 可正確偵測所有目標硬體型號
□ GUID 匹配正確，無衝突
□ 韌體版本號遞增正確
□ 防降級機制運作正常（version_lowest 已設定）
□ I2C recovery 機制已測試過
□ 韌體 binary 已確認不含 blocklist 字串 ("DO NOT SHIP", "TBD")
```

### 1.2 metainfo.xml 檢查

```
□ <id> 使用正確的 reverse-DNS 格式，結尾是 .firmware
□ <provides> 中的 GUID 與 Plugin coldplug 產生的 GUID 完全一致
□ <release version=""> 格式與 LVFS::VersionFormat 一致
□ <release urgency=""> 已正確設定 (low/medium/high/critical)
□ <description> 包含有意義的 release notes
□ 若有 CVE/Lenovo security advisory，已在 <issues> 中標註
□ <custom> 中包含正確的 LVFS::UpdateProtocol
```

### 1.3 法律文件

```
□ 上傳許可文件已提交並被 LVFS 管理者接受
□ Vendor account 已是 "trusted" 狀態（可移到 testing/stable）
□ 若有 export control 需求，已設定 LVFS::BannedCountryCodes
```

## 2. 版本命名與管理策略

### 2.1 版本號策略

建議使用 Semantic Versioning (semver)：
```
主版本.次版本.修訂版本
1.0.0 → 1.0.1 → 1.1.0 → 2.0.0
```

| 變更類型 | 版本號範例 | 說明 |
|---|---|---|
| Bug fix | 1.0.0 → 1.0.1 | 向後相容的 bug 修復 |
| 新功能 | 1.0.1 → 1.1.0 | 向後相容的新功能 |
| 破壞性變更 | 1.1.0 → 2.0.0 | 不向後相容的重大變更 |

### 2.2 防止降級

在 plugin 的 coldplug 中正確設定 `version_lowest`：

```c
static gboolean
fu_company_i2c_hid_plugin_coldplug(FuPlugin *plugin, ...)
{
  // ...
  fu_device_set_version(dev, "1.2.5");
  fu_device_set_version_lowest(dev, "1.0.0");  /* 防止降到 1.0.0 以下（使用 set_ 前綴）*/
  // ...
}
```

或在 quirk 檔案中設定：
```
[company-i2c-hid]
VersionLowest=1.0.0
```

## 3. 正式發布步驟

### 3.1 完整發布流程

```
Day 0: 準備階段
├── 完成所有本地測試 (Local_Testing.md)
├── 完成 embargo remote 端對端測試
└── 確認 LVFS 測試全部通過

Day 1: 上傳
├── 上傳 .cab 到 LVFS (private)
├── 檢查 LVFS 測試結果
└── 移到 embargo remote

Day 2-3: 內部 QA
├── 團隊在 embargo remote 進行完整測試
├── 收集 signed reports
└── 修復發現的問題（如有）

Day 4: 移到 Testing
├── 移到 testing remote
├── 觀察公共測試用戶回報
└── 確認無重大問題

Day 7+: 移到 Stable
├── 確認 testing 回報良好
├── 移到 stable remote
└── 開始監控失敗率和回報
```

### 3.2 Emergency 緊急發布

若發現重大安全漏洞需要緊急修復：

1. **跳過 testing**：直接移到 stable（確認安全後果承擔）
2. **提高 urgency**：`urgency="critical"` 在 metainfo.xml
3. **通知用戶**：聯繫 LVFS 管理者或透過 fwupd mailing list
4. **快速通道**：聯繫 fwupd 維護者加速 metadata 更新

## 4. 發布後監控

### 4.1 追蹤失敗率

```bash
# 查看失敗歷史
fwupdmgr get-history

# 在 LVFS 網頁的 firmware 頁面
# "Reports" 標籤顯示所有使用者的回報
# 包含失敗原因分析
```

### 4.2 使用者回報分析

LVFS 收集的回報包含：
- 設備 checksum（驗證韌體是否成功燒錄）
- fwupd 版本
- OS 版本
- 更新前/後的版本
- 失敗原因（如有）

### 4.3 失敗率警戒線

| 失敗率 | 行動 |
|---|---|
| < 1% | 正常範圍 |
| 1-5% | 密切監控，檢查失敗原因 |
| > 5% | 考慮從 stable 撤回，移到 testing 重新評估 |

## 5. 持續維護

### 5.1 後續韌體更新流程

每次發布新韌體都重複同樣的流程：
```
1. Plugin 更新（如有必要）
2. 準備新版本的 .cab (新 version number)
3. 上傳 → embargo → testing → stable
4. 監控失敗率
```

### 5.2 撤離 (Rollback) 流程

若發現嚴重問題需要撤離：

1. 在 LVFS 網頁，進入 firmware 頁面
2. 點擊 "Actions" → "Delete"
3. 確認刪除（從所有 remote 移除）

```bash
# 用戶端需要重新整理 metadata
fwupdmgr refresh
# 該韌體將不再出現
```

### 5.3 ODM/OEM 持續維護

若你是 ODM 替 OEM 生產：
- 每次新的硬體型號需要 ODM 提供新的 VID/PID 和 firmware
- ODM 負責上傳到 LVFS，OEM 審核和發布
- 使用 LVFS affiliate 機制管理權限

## 6. Plugin 維護

### 6.1 Plugin 上游維護

若 Plugin 最終 merge 到 fwupd 上游：

1. **追蹤 fwupd 發布**：
   - 每次 fwupd 釋出都會 regression test 所有 plugin
   - 提供硬體樣本給維護者（可加速 debug）

2. **Plugin 版本與 fwupd 版本**：
   - Plugin 版本通常跟隨 fwupd 版本號
   - 新增功能可能需要 fwupd 版本要求

3. **參與 community**：
   - fwupd mailing list: https://lists.freedesktop.org/mailman/listinfo/fwupd-devel
   - GitHub Issues: https://github.com/fwupd/fwupd/issues

### 6.2 Plugin 单独維護

若 Plugin 不在 fwupd 上游，需要自己維護：

```
# 自己的 plugin repo
plugins/company-i2c-hid/
├── meson.build
├── fu-company-i2c-hid-plugin.c
├── fu-company-i2c-hid-device.c
├── fu-company-i2c-hid.quirk
└── README.md

# 需要手動複製到編譯環境
# 或使用 fwupd 的 plugin 路徑
# /usr/lib/x86_64-linux-gnu/fwupd/plugins/
```

## 7. Windows Update 整合（可選）

LVFS 支援同時發布到 Linux LVFS 和 Microsoft Update。

### 7.1 設定 Windows Update

在 LVFS 網頁 firmware 頁面的 "Actions" 中：
- "Publish to Microsoft Update"

這需要額外的 Microsoft Partner Center 帳號。

### 7.2 注意事項

```
□ 同一個 .cab 檔案同時適用 Linux 和 Windows
□ PKCS#7 簽章用於 Windows Update
□ GPG 簽章用於 LVFS (Linux)
□ 兩種簽章同時存在於 .cab 內
□ 簽章互不覆蓋
```

## 8. 發布檢查清單（完整版）

```
┌─────────────────────────────────────────────────────────┐
│ Pre-Release (在移到 stable 之前)                         │
├─────────────────────────────────────────────────────────┤
│ □ Plugin 程式碼 review 完成                             │
│ □ Plugin 在本地成功編譯                                   │
│ □ 韌體 binary 通過所有 LVFS 測試                         │
│ □ metainfo.xml 所有欄位正確                              │
│ □ embargo remote 端對端測試完成                          │
│ □ 使用者可以成功更新                                      │
│ □ 更新失敗時 I2C recovery 正常                          │
│ □ CVE/LSA 已正確標註                                     │
│ □ Release notes 已填寫                                   │
│ □ Legal document 已提交                                  │
│ □ Vendor account 是 trusted 狀態                         │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Release Day                                              │
├─────────────────────────────────────────────────────────┤
│ □ 移到 stable                                            │
│ □ 確認 metadata 已更新 (5-15 分鐘)                       │
│ □ 通知相關利害關係人                                      │
│ □ 開始監控失敗率                                          │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Post-Release (移到 stable 後 1-2 週)                     │
├─────────────────────────────────────────────────────────┤
│ □ 失敗率 < 1%                                            │
│ □ 無重大使用者問題                                        │
│ □ 開始規劃下一版本                                        │
└─────────────────────────────────────────────────────────┘
```

## 9. 快速參考：常用指令

```bash
# 1. 確認 device 被偵測
sudo fwupdmgr get-devices

# 2. 查看特定設備詳細資訊
sudo fwupdmgr get-details "Company I2C HID"

# 3. 檢查可用更新
sudo fwupdmgr check-update

# 4. 安裝更新
sudo fwupdmgr update

# 5. 安裝離線 .cab
sudo fwupdmgr install firmware.cab

# 6. 查看歷史
fwupdmgr get-history

# 7. 刷新 metadata
sudo fwupdmgr refresh

# 8. 查看已啟用的 remotes
fwupdmgr remotes

# 9. 啟用 testing
fwupdmgr modify-config TestChecksEnabled true

# 10. 提交測試報告
sudo fwupdmgr report-history --sign
```

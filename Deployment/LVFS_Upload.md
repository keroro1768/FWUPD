# LVFS 上架流程

## 概述

LVFS（Linux Vendor Firmware Service）是 FWUPD 生態的韌體分發中心。將韌體上架到 LVFS 後，全球使用 fwupd 的 Linux 使用者都能自動發現並安裝更新。

上架流程分為五個階段：

```
申請帳號 → 新增產品 → 上傳 CAB → 填寫資訊 → 等待審核
```

---

## 階段一：申請 LVFS 帳號

### 1. 訪問 LVFS 網站

前往 https://fwupd.org/ ，點擊 **Sign Up** 或 **For Vendors**。

### 2. 填寫申請表

需要提供：
- **Company Name**（公司名稱）
- **Website URL**（公司網站）
- **Contact Email**（聯繫信箱，建議使用公司網域）
- **Product Category**（產品類別）
- **Expected Upload Volume**（預期上傳量）

### 3. 等待審批

LVFS 管理員（通常為 Richard Hughes）會審核申請：
- 開源社群公司：通常數小時～數天
- 首次申請：可能需要驗證公司真實性
- **不通過的常見原因**：
  - 使用個人 Email（非公司網域）
  - 產品與韌體無關
  - 缺乏基本的網路存在

### 4. 收到 API Key

審批通過後，你會收到：
- LVFS 登入帳號密碼（用於 Web UI）
- **API Key**（用於 CLI 上傳和自動化）

---

## 階段二：在 LVFS Web UI 中新增產品

### 1. 登入 LVFS

訪問 https://fwupd.org/lvfs/login 並使用收到的帳號密碼登入。

### 2. 新增供應商設定（首次）

如果這是該公司的首款上架產品，需要先建立供應商資訊：
- **Vendor Name**：顯示名稱
- **Vendor ID**：系統自動產生
- **Description**：供應商簡介
- **Logo**：公司標誌（用於 LVFS 頁面顯示）

### 3. 新增產品

在 **Products** 頁面點擊 **Add Product**，填寫：

| 欄位 | 說明 | 範例 |
|------|------|------|
| **Name** | 產品名稱 | `M487 USB Bootloader` |
| **ID** | 產品 ID（URL slug）| `m487-bootloader` |
| **Brand** | 品牌名稱 | `Nuvoton` |
| **Downloads** | 起始為 0 | — |
| **Category** | 產品類別 | `Device Drivers` |

### 4. 設定產品可見性

| 選項 | 說明 |
|------|------|
| **Public** | 所有人可見（正式發布）|
| **Testing** | 僅測試者能看到 |
| **Private** | 僅有連結的人能看到（需 URL）|

建議首次上傳時使用 **Private/Testing** 確認無誤後再設為 Public。

---

## 階段三：上傳 CAB 檔案

### 選項 A：透過 Web UI 上傳

最直觀的方式：
1. 進入產品頁面
2. 點擊 **New Upload**
3. 選擇 `.cab` 檔案
4. 填寫版本資訊
5. 提交

### 選項 B：透過 lvfs-cli 上傳（推薦）

```bash
# 安裝 lvfs-cli
sudo apt install lvfs        # Debian/Ubuntu
sudo dnf install lvfs        # Fedora

# 首次使用需要設定 API Key
lvfs-cli configure --api-key <YOUR_API_KEY>

# 上傳 CAB
lvfs-cli upload \
  --api-key <YOUR_API_KEY> \
  --product-id <PRODUCT_ID> \
  firmware.cab

# 完整範例
lvfs-cli upload \
  --api-key "lvfs_abc123xyz..." \
  --product-id "m487-bootloader" \
  m487_v1.2.0.cab
```

### 選項 C：透過 REST API 上傳

```bash
curl -X POST \
  "https://fwupd.org/api/1/upload" \
  -H "Authorization: Bearer <API_KEY>" \
  -F "file=@firmware.cab" \
  -F "name=M487 v1.2.0" \
  -F "version=1.2.0" \
  -F "urgency=high"
```

---

## 階段四：填寫版本資訊

上傳成功後，在 LVFS 網頁上完善以下資訊：

### 版本資訊

| 欄位 | 說明 | 範例 |
|------|------|------|
| **Version** | 韌體版本號 | `1.2.0` |
| **Release Date** | 預設為今天 | 可自訂 |
| **Urgency** | 緊急程度 | `High` |
| **Remote** | 目標遠端 | `LVFS` |
| **Embargo Date** | 公開日期（可選）| `2024-04-01` |

### Description / Release Notes

描述本次更新的內容：

```markdown
## Changes in v1.2.0

- Fixed USB enumeration issue on cold boot
- Improved power consumption in deep sleep mode
- Security: CVE-2024-XXXX resolved
- Updated peripheral library to v2.1.0
```

### Checksum

系統會自動計算 SHA1/SHA256，無需手動填寫。

### Flags（標誌）

| 標誌 | 說明 |
|------|------|
| `is-coldbat` | 冷啟動相關更新 |
| `trusted-pubkey` | 已驗證公鑰發布 |
| `no-attachment` | 只下載不安裝（需額外確認）|

---

## 階段五：審核與發布

### 審核流程

```
┌─────────────────────────────────────────┐
│  Upload Complete                         │
│       │                                  │
│       ▼                                  │
│  ┌─────────────────┐                    │
│  │  Auto Validation │ ← 格式/簽章驗證   │
│  └────────┬────────┘                    │
│           │                              │
│     Pass? │ Fail                         │
│      ┌────┴────┐                        │
│      ▼         ▼                        │
│   Continue  Reject                      │
│      │                                  │
│      ▼                                  │
│  ┌─────────────────┐                    │
│  │  Manual Review   │ ← 管理員審核      │
│  └────────┬────────┘                    │
│           │                              │
│     Approve? │ Reject                    │
│       ┌────┴────┐                        │
│       ▼         ▼                        │
│   Published  Feedback                   │
└─────────────────────────────────────────┘
```

### 常見審核不通過原因

1. **metainfo.xml 格式錯誤**：
   - 缺少必要欄位（如 `<provides>`）
   - ID 格式不符合規範
   - HTML 描述使用了不允許的標籤

2. **GUID 不匹配**：
   - `<provides>` 中的 GUID 與硬體不符
   - 硬體 Plugin 未正確註冊該 GUID

3. **簽章無效**：
   - 未使用 LVFS 接受的簽章格式
   - 私鑰過期或無效

4. **版本號問題**：
   - 新版本不優於舊版本（版本號未增加）
   - 降級但未設定允許降級標誌

5. **供應商未驗證**：
   - 首次上架需人工審核公司背景

### 發布後確認

```bash
# 等待約 5-10 分鐘元資料同步
fwupdmgr refresh

# 確認可以看到新版本
fwupdmgr get-updates

# 應顯示：
# Uup display name: M487 USB Bootloader
# Update version: 1.2.0
```

---

## Embargo（保密髮布）

對於需要保密的韌體，可以設定 **Embargo Date**：

```xml
<!-- 在 release 標籤中設定 -->
<release version="1.3.0" date="2024-04-01" urgency="critical">
  <embargo date="2024-04-15"/>
  ...
</release>
```

設定後：
- 韌體會立即通過審核
- 但在 `embargo date` 之前，使用者無法看到更新
- 只有知道直接連結（`https://fwupd.org/downloads/...`）的人可以提前下載
- 適合搶在公開揭露前先修補安全漏洞

---

## 維護與更新

### 新版本上架

每次韌體更新都是一次新的 CAB 上傳。重複**階段三到階段五**。

### 下架舊版本

在 LVFS 頁面中：
1. 進入產品頁面
2. 找到要下架的版本
3. 點擊 **Hide** 或 **Delete**

> ⚠️ 已經下載並安裝的使用者不受影響，只有新使用者看不到該版本。

### 關鍵字和搜尋優化

在 metainfo.xml 中添加 `<keywords>` 有助於 LVFS 搜尋：

```xml
<keywords>
  <keyword>nuvoton</keyword>
  <keyword>m487</keyword>
  <keyword>microcontroller</keyword>
  <keyword>usb</keyword>
</keywords>
```

---

## 自動化腳本範例

```python
#!/usr/bin/env python3
"""LVFS automated firmware upload script."""

import requests
import sys
from pathlib import Path

LVFS_API = "https://fwupd.org/api/1"
API_KEY = "lvfs_your_api_key_here"
PRODUCT_ID = "m487-bootloader"


def upload_firmware(cab_path: str, version: str, urgency: str = "medium"):
    """Upload firmware CAB to LVFS."""
    
    url = f"{LVFS_API}/upload"
    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Accept": "application/json"
    }
    
    with open(cab_path, "rb") as f:
        files = {"file": f}
        data = {
            "product_id": PRODUCT_ID,
            "version": version,
            "urgency": urgency,
        }
        
        response = requests.post(url, headers=headers, files=files, data=data)
    
    if response.status_code == 200:
        print(f"✅ Successfully uploaded {cab_path} as v{version}")
        return response.json()
    else:
        print(f"❌ Upload failed: {response.status_code}")
        print(response.text)
        sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <cab_file> <version> [urgency]")
        sys.exit(1)
    
    cab = sys.argv[1]
    ver = sys.argv[2]
    urg = sys.argv[3] if len(sys.argv) > 3 else "medium"
    
    upload_firmware(cab, ver, urg)
```

---

## 參考資源

- [LVFS 官方上傳指南](https://fwupd.org/lvfs/docs/applications)
- [LVFS API 文件](https://github.com/fwupd/lvfs.github.io/wiki/API-Docs)
- [lvfs-cli 使用說明](https://github.com/hughsie/lvfs/tree/main/src)

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

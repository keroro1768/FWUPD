# LVFS 上傳流程

## 1. LVFS 帳號申請

### 1.1 申請方式

LVFS 不提供公開的自我註冊。
必須透過 GitLab 開 ticket 來申請帳號：

**申請 URL**：https://gitlab.com/fwupd/lvfs-website/-/issues/new

**申請時需要提供的資訊**：

| 欄位 | 說明 | 範例 |
|---|---|---|
| Vendor Full Legal Name | 公司完整法律名稱 | Your Company Inc. |
| Public Homepage | 官方網站 | https://www.company.com |
| Vendor Logo | 高解析度 logo (PNG/SVG) | company-logo.png |
| Email Domain | 用於認證的 email domain | @company.com |
| Update Protocol | 使用的更新協定 | com.company.i2chid |
| Vendor ID | USB/HID VID | USB:0xABCD |
| AppStream ID Prefix | reverse-DNS 前綴 | com.company |
| PSIRT URL | 安全事件回報 URL | https://company.com/security |
| Legal Permission | 上傳許可文件 | 簽署後的 PDF |

### 1.2 帳號類型

```
Vendor Manager (Vendor Admin)
├── 可以新增/管理使用者
├── 可以將 firmware 移到 testing/stable
└── 主要聯絡窗口

QA User
├── 可以看到 vendor group 內所有 firmware
├── 可以將 firmware 移到 testing/stable
└── 通常是測試團隊

Upload Only User (預設)
└── 只能上傳 firmware到自己上傳的項目
```

### 1.3 信任狀態 (Trusted Status)

新建立的 vendor 帳號是 "untrusted" 狀態，
**無法**將 firmware 移到 testing 或 stable。

需要提供法律文件（表明允許 LVFS 重新發布韌體）才能解鎖。
詳見 https://lvfs.readthedocs.io/en/latest/apply.html

## 2. 準備上傳檔案

### 2.1 Cabinet Archive 結構

```
vendor-product-v1.2.3.cab/
├── vendor-product.metainfo.xml   ← metadata (必須)
└── vendor-product-v1.2.3.bin     ← 實際韌體 binary (必須)
```

### 2.2 重要 metadata 欄位說明

**AppStream ID (`<id>`)**：
- 必須唯一識別這個韌體流
- 格式：`{reverse-dns}.{device-model}.firmware`
- 範例：`com.company.i2chid-sensor.firmware`
- ⚠️ 絕對不能包含 `/` 或 `\`

**GUID (`<provides>`)**：
- 必須與 Plugin 實際產生的 GUID 完全匹配
- 建議使用小寫 UUID
- 多個 GUID 表示同一個裝置的不同匹配方式

**Version (`<release version="">`)**：
- 版本號格式必須與 `LVFS::VersionFormat` 一致
- 例如 `triplet` 格式：`1.2.3`
- 例如 `quad` 格式：`1.2.3.4`

**Protocol (`LVFS::UpdateProtocol`)**：
- 與 Plugin 實作的 protocol ID 一致
- 若使用自定義 protocol，LVFS 會創建新的 protocol 條目

### 2.3 建立 Cabinet Archive（Linux）

```bash
# 安裝 gcab
sudo apt install gcab

# 建立 archive
gcab -c -v company-i2chid-v1.2.3.cab \
    company-i2chid.metainfo.xml \
    company-i2chid-v1.2.3.bin

# 確認內容
gcab -t company-i2chid-v1.2.3.cab
```

### 2.4 建立 Cabinet Archive（Windows）

```bat
REM 建立 config.txt
echo .OPTION EXPLICIT > config.txt
echo .set Cabinet=on >> config.txt
echo .set Compress=on >> config.txt
echo .set MaxDiskSize=0 >> config.txt
echo .set DiskDirectoryTemplate=. >> config.txt
echo .set DestinationDir=DriverPackage >> config.txt
echo company-i2chid.metainfo.xml >> config.txt
echo company-i2chid-v1.2.3.bin >> config.txt

REM 執行 makecab
makecab /F config.txt

REM 命名輸出檔案
rename 1.cab company-i2chid-v1.2.3.cab
```

**⚠️ 重要**：config.txt 必須有 `.OPTION EXPLICIT`，
否則 archive 會被限制在 1.38MB 以內。

## 3. 手動上傳到 LVFS

### 3.1 透過網頁上傳

1. 前往 https://fwupd.org/lvfs/
2. 點擊右上角 "Login" 登入
3. 點擊 "Upload" 分頁
4. 拖放 .cab 檔案或點擊選擇檔案
5. 確認上傳後的韌體資訊正確
6. 點擊 "Upload"

上傳成功後，韌體預設處於 **private** 狀態（僅自己可見）。

### 3.2 透過 curl 自動上傳

```bash
# 格式
curl -X POST -F file=@firmware.cab \
     https://fwupd.org/lvfs/upload/token \
     --user "email@company.com:USER_TOKEN"

# 範例
curl -X POST -F file=@company-i2chid-v1.2.3.cab \
     https://fwupd.org/lvfs/upload/token \
     --user "developer@company.com:abc123xyz"
```

**如何產生 User Token**：
1. 登入 LVFS
2. 前往 https://fwupd.org/lvfs/profile
3. 點擊 "Generate new token"
4. 複製產生的 token（只會顯示一次）

### 3.3 查詢已上傳的韌體

```bash
# 列出所有韌體
curl https://fwupd.org/lvfs/firmware/auth \
     --user "email@company.com:USER_TOKEN" | jq '.'

# 查詢特定韌體
curl "https://fwupd.org/lvfs/firmware/{firmware_id}/auth" \
     --user "email@company.com:USER_TOKEN" | jq '.'
```

## 4. 上傳後的狀態管理

### 4.1 四種 Remote 狀態

```
┌────────┐    LVFS 網頁操作     ┌────────┐
│        │ ─────────────────▶  │        │
│ Private │                    │ Embargo │
└────────┘                    └────────┘
                                  │
                                  │ LVFS 網頁操作
                                  ▼
┌────────┐                    ┌────────┐
│        │ ◀───────────────── │        │
│ Stable │    LVFS 網頁操作    │ Testing │
└────────┘                    └────────┘
```

| 狀態 | 誰可以看到 | 常見用途 |
|---|---|---|
| Private | 只有上傳者 | 開發中測試 |
| Embargo | 同 vendor group | 内部 QA |
| Testing | 啟用 testing 的公共用戶 | 公開測試 |
| Stable | 所有用戶 | 正式發布 |

### 4.2 在 LVFS 網頁移動狀態

1. 找到上傳的韌體（"Firmware" 分頁）
2. 點擊韌體名稱進入详情頁
3. 點擊 "Actions" 下拉選單
4. 選擇 "Move to Embargo" / "Move to Testing" / "Move to Stable"
5. 確認移動

### 4.3 設定 release 詳情

在 LVFS 網頁可以編輯：
- Release notes（release notes）
- Update message（使用者更新時顯示的文字）
- Screenshots（更新說明截圖）
- Tags（用於分組發布的標籤）

## 5. Vendor Group 設定

### 5.1 新增使用者

身為 Vendor Manager：
1. 前往 LVFS 網頁 "Settings" → "Users"
2. 點擊 "Add User"
3. 輸入新使用者的 email（必須符合 vendor domain）
4. 設定角色（QA / Upload Only）
5. 發送邀請

### 5.2 Affiliate 設定（ODM/OEM 情境）

若你是 ODM，要替多個 OEM 上傳韌體：

1. OEM 在 LVFS 申請帳號
2. OEM 的 Vendor Manager 在 Settings 中將你設為 affiliate
3. 你上傳時可以選擇韌體歸屬於哪個 vendor

## 6. 常見問題

### 6.1 錯誤訊息分析

```
"Upload rejected: file size exceeds limit"
→ firmware binary 太大，檢查 .cab 格式

"Invalid metainfo.xml: missing required field"
→ 檢查 metainfo.xml 是否有必填欄位（如 <id>, <provides>）

"GUID mismatch: no matching devices found"
→ Plugin 產生的 GUID 與 metainfo.xml 中的 <provides> 不匹配

"Version format not specified"
→ 在 <custom> 中加入 LVFS::VersionFormat
```

### 6.2 上傳後遲遲看不到

```bash
# 確認檔案已上傳
curl https://fwupd.org/lvfs/firmware/auth \
     --user "email:token" | jq '.[] | select(.name | contains("company"))'

# 確認 metadata 更新
# LVFS metadata 通常每 5 分鐘更新一次
# 若超過 15 分鐘仍未出現，聯繫 LVFS 管理者
```

## 7. 完整上傳腳本範例

```bash
#!/bin/bash
# upload-firmware.sh - 自動上傳韌體到 LVFS

EMAIL="developer@company.com"
TOKEN="YOUR_LVFS_TOKEN"
CAB_FILE="$1"

if [ -z "$CAB_FILE" ]; then
    echo "Usage: $0 firmware.cab"
    exit 1
fi

echo "Uploading $CAB_FILE to LVFS..."
curl -X POST -F file=@${CAB_FILE} \
     https://fwupd.org/lvfs/upload/token \
     --user "${EMAIL}:${TOKEN}" \
     -w "\nHTTP Status: %{http_code}\n"

echo "Done. Check LVFS website to move to embargo."
```

```bash
chmod +x upload-firmware.sh
./upload-firmware.sh company-i2chid-v1.2.3.cab
```

## 8. ODM 情境的上傳流程

```
1. OEM (例如: Real OEM Inc.) 在 LVFS 申請帳號
2. Real OEM Inc. 的 Vendor Manager 將 ODM 新增為 affiliate
3. ODM 工程師登入 LVFS
4. 上傳韌體時，選擇 "Upload for: Real OEM Inc." (而非自己的公司)
5. 韌體出現在 Real OEM Inc. 的 vendor group
6. Real OEM Inc. 的 QA 團隊進行測試
7. Real OEM Inc. 將韌體移到 stable
```

好處：ODM 不需要知道最終 OEM 的登入資訊，
OEM 對所有以自己品牌發布的韌體有完整掌控權。

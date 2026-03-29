# LVFS 服務說明

## 什麼是 LVFS？

**LVFS（Linux Vendor Firmware Service）** 是 FWUPD 生態系統的韌體分發基礎設施，由 **Richard Hughes** 創建並托管於 fwupd.org。LVFS 扮演的角色類似於一個「韌體應用商店」——各家硬體供應商將韌體上傳至 LVFS，FWUPD 用戶端自動發現並下載更新。

LVFS 的核心價值：
- **供應商中立**：任何供應商只要遵守規範即可使用
- **全球分發**：基於 HTTPS 的全球 CDN 加速
- **安全可信**：所有韌體皆經過簽章驗證
- **免費使用**：對供應商和終端使用者皆免費

---

## LVFS 的運作模式

### 整體架構

```
┌──────────────────┐         ┌──────────────────┐
│  Hardware Vendor │         │   LVFS Server    │
│                  │ upload  │                  │
│  - 準備 CAB 檔案 │ ──────► │  - 儲存韌體       │
│  - 填寫 metainfo │         │  - 簽章驗證       │
│  - 簽章韌體      │         │  - 分發元資料     │
└──────────────────┘         └────────┬─────────┘
                                      │ metadata XML
                                      │ CAB downloads
                                      ▼
┌──────────────────┐         ┌──────────────────┐
│  Linux End User  │         │   CDN Network    │
│                  │ fetch   │                  │
│  fwupdmgr check  │ ──────► │  - 全球加速      │
│  fwupdmgr update │ ◄────── │  - 負載均衡      │
└──────────────────┘  CAB   └──────────────────┘
```

### 兩種使用模式

#### 1. 公開上架（Public）
- 韌體出現在 `fwupdmgr get-updates` 結果中
- 任何人皆可下載
- 適合主流消費級產品
- 需要 LVFS 管理員審核

#### 2. 私人內測（Private/Embargoed）
- 韌體先設為「審核中」狀態
- 僅知道特定 URL 的使用者可下載
- 適合早期測試、限量產品
- 可設定公開發布時間（embargo date）

---

## 供應商上傳流程

### 步驟 1：註冊 LVFS 帳號

1. 前往 https://fwupd.org/
2. 註冊帳號（通常使用供應商域名 Email）
3. 聯繫 LVFS 管理員開通供應商權限

### 步驟 2：新增產品類別

登入後在 LVFS Web UI 中建立產品，填寫：
- **Product Name**：產品型號名稱
- **Product ID**：唯一識別碼
- **Brand Name**：品牌名稱
- **Downloads**：（上傳後自動產生）

### 步驟 3：上傳 CAB 檔案

透過 Web UI 或 `lvfs-cli` 工具上傳：

```bash
# 安裝 lvfs-tools（若支援 CLI 上傳）
sudo apt install lvfs

# 上傳（需先在網頁取得 API Key）
lvfs-cli upload --api-key <YOUR_API_KEY> firmware.cab
```

### 步驟 4：填寫韌體元資料

在 LVFS 網頁介面填寫：
- **Version**：韌體版本號
- **Release Date**：發布日期
- **Description**：變更說明（Release Notes）
- **Urgency**：緊急程度（low / medium / high / critical）
- **Checksum**：SHA1/SHA256（自動計算）

### 步驟 5：提交審核

- **自動審核**：基本格式驗證由系統完成
- **人工審核**：首次上傳需 LVFS 管理員確認
- **常見不通過原因**：
  - metainfo.xml 格式錯誤
  - 缺少硬體識別資訊（GUID）
  - 簽章無效

---

## LVFS 元資料結構

### Metadata 類型

LVFS 提供兩種元資料：

| 類型 | 檔案名 | 說明 |
|------|--------|------|
| **Release Metadata** | `firmware.xml.gz` | 單一韌體的完整資訊 |
| **Compendium** | `compendium.xml.gz` | 所有韌體索引（龐大，漸進淘汰） |

### Metadata XML 結構（firmware.xml）

```xml
<?xml version="1.0" encoding="UTF-8"?>
<firmware version="1.0.2"
          vendor="Nuvoton Technology Corp."
          release_type="production">
  
  <!-- 元件識別 -->
  <components>
    <component type="firmware">
      <id>com.nuvoton.m487-firmware</id>
      <name>Nuvoton M487 Bootloader</name>
      <summary>Bootloader firmware for M487 series</summary>
      
      <!-- 硬體識別：GUID 列表 -->
      <provides>
        <firmware type="flashed">e7eb8030-...</firmware>
        <firmware type="flashed">6f14c95a-...</firmware>
      </provides>
      
      <!-- 相依其他元件 -->
      <requires>
        <firmware type="flashed">4e8c1a1d-...</firmware>
      </requires>

      <!-- 軟體需求 -->
      <checkSUM type="sha1">a1b2c3d4...</checkSUM>
      
      <!-- 交付方式 -->
      <location href="https://fwupd.org/downloads/..."/>
      
      <!-- 更新說明 -->
      <description>
        <p>This release addresses CVE-2024-XXXX and improves update reliability.</p>
      </description>
    </component>
  </components>
</firmware>
```

---

## 簽章機制

### LVFS 的三層簽章

```
Layer 1: 供應商簽章
  └─ 使用供應商私鑰簽章 CAB 內容
  └─ 驗證方式：供應商在 LVFS 設定中上傳公鑰

Layer 2: LVFS 平台簽章
  └─ LVFS 使用自己的私鑰對整個 metadata 再次簽章
  └─ 客戶端在安裝 fwupd 時已內建 LVFS 公鑰

Layer 3: UEFI DB 簽章（僅 UEFI 韌體）
  └─ 韌體本身還需符合 UEFI 安全啟動規範
  └─ 需要 Microsoft 合作夥伴簽章或 UEFI CA
```

### 供應商私鑰管理

建議做法：
```bash
# 1. 生成 Ed25519 或 RSA-2048 私鑰（硬體安全）
# 2. 私鑰離線儲存，嚴格管控存取
# 3. 公鑰上傳至 LVFS 供應商設定

# 4. 每次韌體發布前使用私鑰簽章
export FUZZBOMB=1
./make-cenjoy-firmware.sh  # 編譯
gpg --detach-sign firmware.cab  # 簽章（或使用特定工具）
```

⚠️ **警告**：FWUPD 正在逐步淘汰 GPG 簽章，轉向基於 PKCS#7/X.509 的簽章框架。建議新專案使用 `appstream-util validate-relax` 進行本地驗證。

---

## LVFS 的 REST API

LVFS 提供 RESTful API 供自動化腳本使用：

### 端點範例

```bash
# 取得供應商 ID
GET https://fwupd.org/api/1/vendor/<vendor-name>

# 取得產品列表
GET https://fwupd.org/api/1/vendor/<vendor-id>/products

# 取得特定韌體資訊
GET https://fwupd.org/api/1/firmware/<firmware-id>

# 上傳新韌體（需認證）
POST https://fwupd.org/api/1/upload
Authorization: Bearer <API_TOKEN>
Content-Type: multipart/form-data
```

### 使用 Python 自動化上傳

```python
import requests

# 上傳韌體
with open('firmware.cab', 'rb') as f:
    response = requests.post(
        'https://fwupd.org/api/1/upload',
        headers={'Authorization': f'Bearer {API_KEY}'},
        files={'file': f},
        data={
            'name': 'M487 Bootloader v1.2.0',
            'version': '1.2.0',
            'urgency': 'high',
        }
    )
    print(response.json())
```

---

## LVFS 安全性機制

### 1. 隔離審核（Quarantine）
上傳的韌體預設進入「隔離區」，需管理員審核後才公開。

### 2. Embargo Date
可設定「 embargo date 」，在此日期前即使審核通過也不對外公佈。

### 3. CVE 關聯
韌體可關聯到 CVE 資料庫，自動顯示漏洞影響範圍。

### 4. 匿名使用統計
LVFS 收集可選的匿名使用數據，幫助供應商了解韌體部署狀況：

```xml
<!-- 在 metainfo.xml 中可關閉報告 -->
<custom>
  <value key="StatsReporting">never</value>
</custom>
```

### 5. SHA 校驗和
所有下載均提供 SHA1/SHA256 校驗，客戶端自動驗證。

---

## LVFS 與企業部署

### 企業使用 LVFS 的方式

```
方式 1：直接使用 LVFS（小型/無安全合規需求）
  └─ 優點：零維護成本
  └─ 缺點：韌體流向不可控、網路依賴

方式 2：LVFS 作為基礎 + 企業代理快取（推薦）
  └─ 企業內部架設 fwupd mirror
  └─ 只允許特定韌體版本通過

方式 3：完全離線部署（高安全合規環境）
  └─ 從 LVFS 下載所需韌體 CAB
  └─ 透過企業內部分發（MDM、LDAP 觸發）
  └─ 無網路依賴
```

### 企業內部 Mirror 設定

```bash
# 在 /etc/fwupd/remotes.d/ 建立企業 mirror 設定
[remote]
DisplayName=Nuvoton Internal Firmware
MetadataURI=https://firmware.company.internal/lvfs/metadata/
FirmwareBaseURI=https://firmware.company.internal/lvfs/
Enabled=true
ApprovalRequired=false
```

---

## LVFS 現況與限制

### 已上架的知名供應商
- Intel（幾乎所有平台）
- Dell（全面支援）
- Lenovo（ThinkPad/ThinkStation）
- Samsung
- Wacom
- Logitech（部分產品）

### LVFS 的限制
1. **審核延遲**：首次上架可能需數天等待人工審核
2. **僅支援 FWUPD**：非 Linux 環境無直接支援
3. **地理延遲**：非中國區域 CDN 對亞洲用戶可能較慢
4. **韌體大小限制**：單一 CAB 通常限制在 2GB 以內
5. **中國網路環境**：fwupd.org 在中國大陸可能無法直接訪問

---

## 替代方案

針對中國市場或無法使用 LVFS 的場景：

| 方案 | 說明 |
|------|------|
| **本地 Mirror** | 在企業內部架設 LVFS Mirror |
| **供應商網站下載** | 使用者手動下載 CAB → fwupdmgr install |
| **OTA 更新服務** | 自建 OTA 平台（如 Mender、SWUpdate） |
| **專屬更新工具** | 供應商自行開發 CLI/GUI 更新工具 |

---

## 常見問題

**Q：LVFS 上架需要收費嗎？**
A：對開源社群和硬體供應商完全免費。

**Q：可以只發布給特定客戶嗎？**
A：可以。使用 embargo date 或建立私人（private）韌體版本，透過 URL 分享給特定人。

**Q：韌體更新失敗會怎樣？**
A：FWUPD 支援復原機制。當韌體包含 dual-bank（或還原分區）時，失敗可自動回滾。但並非所有硬體都支援此功能。

**Q：中國大陸如何穩定使用 LVFS？**
A：建議架設境內 Mirror 或使用企業內部離線部署方案。

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

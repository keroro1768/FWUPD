# metainfo.xml 格式說明

## 概述

`metainfo.xml` 是 FWUPD/LVFS 使用的 AppStream 元資料格式，用於描述韌體包的完整資訊。它基於 [AppStream](https://www.freedesktop.org/wiki/Specifications/metainfo-spec/) 規范，專門針對韌體/嵌入式裝置場景擴展。

---

## 最小可用的 metainfo.xml

```xml
<?xml version="1.0" encoding="UTF-8"?>
<component type="firmware">
  <id>com.vendor.product-firmware</id>
  <name>Product Name</name>
  <summary>Short one-line summary</summary>
  
  <provides>
    <firmware type="flashed">xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx</firmware>
  </provides>
  
  <description>
    <p>Description of what this firmware update provides.</p>
  </description>
  
  <releases>
    <release version="1.0.0" date="2024-01-01"/>
  </releases>
</component>
```

---

## 完整結構說明

### 1. 根元素（Root Element）

```xml
<component type="firmware">
```

`type` 必須為 `firmware`，區分於一般軟體應用程式。

---

### 2. 識別資訊（Identification）

#### id
產品的唯一識別符，建議使用反向域名格式：

```xml
<id>com.nuvoton.m487-bootloader</id>
```

命名規則：
- 只能包含字母、數字、減號、點號
- 以字母開頭
- 長度 1-255 字元
- 結尾不能是減號或點號

#### name
產品的可讀名稱：

```xml
<name>Nuvoton M487 Series</name>
```

可添加多語言版本：
```xml
<name xml:lang="en_US">Nuvoton M487 Series</name>
<name xml:lang="zh_TW">新唐科技 M487 系列</name>
```

#### name 命名建議

| 好範例 | 不好範例 |
|--------|----------|
| `Nuvoton M487 Bootloader` | `M487`（太短不明確）|
| `Company XYZ Sensor Firmware` | `Firmware`（太通用）|
| `Synaptics USB-C PD Controller` | `USB-C Driver`（非韌體）|

#### summary
單行簡短描述（不可超過約80字元）：

```xml
<summary>Firmware for M487 USB bootloader mode</summary>
```

---

### 3. provides（提供的功能）

用於告诉 FWUPD 這個韌體適用於哪些硬體，是最關鍵的匹配欄位。

#### firmware type="flashed"

表示這個韌體需要「燒錄」到硬體中（flash 到 ROM/Flash）：

```xml
<provides>
  <firmware type="flashed">ae4e3d10-05c8-5b5e-ae8e-0416c0c487</firmware>
  <firmware type="flashed">6f14c95a-7eb8-1234-5678-123456789abc</firmware>
</provides>
```

每個 GUID 是硬體的唯一識別碼，通常由供應商生成。

**如何生成 GUID？**

```bash
# 方法 1：使用 Python
python3 -c "import uuid; print(uuid.uuid4())"
# 輸出：ae4e3d10-05c8-5b5e-ae8e-0416c0c487

# 方法 2：使用 fwupd-tool
fwupd-tool generate-hwid "Nuvoton M487" "USB" "VID_0416&PID_C487"

# 方法 3：使用 uuidgen（macOS / Linux）
uuidgen
```

#### 提供類型對照表

| type 值 | 說明 | 範例 |
|---------|------|------|
| `flashed` | 燒錄到 Flash/ROM（主要類型）| UEFI、MCU |
| `runtime` | 運行時下載的軟體/驅動 | FPGA 配置 |
| `emulated` | 模擬設備使用的韌體 | 測試用虛擬裝置 |

---

### 4. requires（依賴需求）

指定安裝此韌體前必須先存在的其他韌體或條件：

```xml
<requires>
  <!-- 需要另一個韌體先安裝 -->
  <firmware type="flashed">4e8c1a1d-0000-0000-0000-000000000000</firmware>
  
  <!-- 需要特定硬體能力 -->
  <hardware>usb</hardware>
  <hardware>spi</hardware>
</requires>
```

---

### 5. description（詳細描述）

支援 HTML 子集，建議只使用：
- `<p>`：段落
- `<ul>`、`<ol>`、`<li>`：列表
- `<code>`、`<pre>`：程式碼
- `<em>`、`<strong>`：強調

```xml
<description>
  <p>This release addresses the following issues:</p>
  <ul>
    <li>Fixed USB enumeration failure on cold boot</li>
    <li>Improved power consumption in sleep mode by 15%</li>
    <li>Security update: CVE-2024-XXXX</li>
  </ul>
  <p><strong>Important:</strong> This update requires the device 
  to be connected via USB and will take approximately 2 minutes.</p>
</description>
```

⚠️ **禁止使用**：`<img>`、`<a>`（外鏈）、`<script>`、`<style>`

---

### 6. releases（版本發布資訊）

#### release 標籤屬性

```xml
<releases>
  <release 
    version="1.2.3"           <!-- 必填：版本號 -->
    date="2024-03-15"         <!-- 必填：發布日期 -->
    urgency="high"            <!-- 可選：緊急程度 -->
    installationते sto "./path/to/firmware.bin" <!-- 可選：安裝時執行的腳本 -->
  >
```

#### urgency 可選值

| 值 | 說明 | 使用時機 |
|-----|------|----------|
| `low` | 低優先順序 | 功能改進、非緊急修復 |
| `medium` | 中等優先順序 | 一般錯誤修復 |
| `high` | 高優先順序 | 重要安全性修復 |
| `critical` | 極緊急 | 嚴重安全漏洞、需立即更新 |

#### 版本號格式

FWUPD 支援多種版本格式，透過 `<developer_url>` 中的 `<value key="VersionFormat">` 指定：

```xml
<releases>
  <!-- 三段式版本（最常見）-->
  <release version="1.2.3" date="2024-01-01">
  
  <!-- 四段式版本 -->
  <release version="1.2.3.4" date="2024-01-01">
  
  <!-- 單純數字版本 -->
  <release version="12345" date="2024-01-01">
</releases>
```

**版本格式聲明**（在 `<metadata>` 或 quirks 中）：

```xml
<value key="VersionFormat">triplet</value>  <!-- 1.2.3 -->
<value key="VersionFormat">quad</value>      <!-- 1.2.3.4 -->
<value key="VersionFormat">numeric</value>   <!-- 12345 -->
```

#### release 子元素

```xml
<release version="1.2.0" date="2024-03-15" urgency="high">
  <!-- 變更說明 -->
  <description>
    <p>Major security update addressing CVE-2024-1234.</p>
  </description>
  
  <!-- SHA256 校驗和 -->
  <checksum type="sha256" target="content">
    a1b2c3d4e5f6...
  </checksum>
  
  <!-- 下載位置（LVFS 會自動填充）-->
  <location href="https://fwupd.org/downloads/..."/>
  
  <!-- 此版本的最小 LVFS 客戶端版本 -->
  <minLvfsVersion>1.0.5</minLvfsVersion>
  
  <!-- 安裝大小（磁碟空間需求）-->
  <size unit="B">524288</size>
</release>
```

---

### 7. categories（分類）

用於 LVFS 網站上的分類顯示：

```xml
<categories>
  <category>X-DeviceDriver</category>
</categories>
```

可選值：
- `X-DeviceDriver`：設備驅動/韌體
- `X-HardwareDriver`：硬體相關

---

### 8. metadata（附加元資料）

用於 FWUPD 特定的擴展設定：

```xml
<metadata>
  <value key="LVNS">1</value>
  <value key="StatsReporting">auto</value>
</metadata>
```

常用鍵值：
| 鍵 | 說明 |
|----|------|
| `LVNS` | LVFS namespace 標誌（通常設為 1）|
| `StatsReporting` | 使用統計回報（`auto`、`never`）|
| `DownloadCache` | 是否快取下載（`true`）|

---

### 9. custom（自訂資料）

用於供應商特定的自訂欄位：

```xml
<custom>
  <value key="OemId">ABC123</value>
  <value key="OemModel">XYZ-100</value>
</custom>
```

---

## 完整範例

```xml
<?xml version="1.0" encoding="UTF-8"?>
<component type="firmware">
  <!-- 基本識別 -->
  <id>com.nuvoton.m487-firmware</id>
  <name xml:lang="en">Nuvoton M487 Series</name>
  <name xml:lang="zh_TW">新唐 M487 系列</name>
  <summary>Firmware for Nuvoton M487 ARM Cortex-M4 microcontrollers</summary>
  
  <!-- 供應商資訊 -->
  <metadata>
    <value key="LVNS">1</value>
  </metadata>
  
  <!-- 硬體識別 GUID（S） -->
  <provides>
    <!-- 這個 GUID 必須與硬體中的識別碼一致 -->
    <firmware type="flashed">ae4e3d10-05c8-5b5e-ae8e-0416c0c487</firmware>
    <!-- 可提供多個 GUID（向後兼容性）-->
    <firmware type="flashed">b1c2d3e4-1111-2222-3333-444455556666</firmware>
  </provides>
  
  <!-- 依賴需求 -->
  <requires>
    <!-- 如有 bootloader 依賴，在此宣告 -->
  </requires>
  
  <!-- 詳細描述 -->
  <description>
    <p>
      This firmware package provides the latest bootloader and system firmware
      for Nuvoton M487 series microcontrollers. The M487 is an ARM Cortex-M4
      based MCU with USB 2.0 OTG, Ethernet, and extensive peripheral integration.
    </p>
    <p>This release includes:</p>
    <ul>
      <li>Security fix for CVE-2024-XXXX</li>
      <li>Improved USB enumeration reliability</li>
      <li>Reduced power consumption in deep sleep mode</li>
      <li>Updated peripheral driver library to v2.1.0</li>
    </ul>
    <p>
      <strong>Note:</strong> Updating takes approximately 2 minutes. 
      The device will briefly disconnect during the process.
    </p>
  </description>
  
  <!-- 版本發布記錄 -->
  <releases>
    <release version="1.2.0" date="2024-03-01" urgency="high">
      <description>
        <p>Production release with critical security updates.</p>
      </description>
      <checksum type="sha256" target="content">
        aabbccdd0011223344556677889900aabbccdd0011223344556677889900aabbccdd
      </checksum>
    </release>
    
    <release version="1.1.0" date="2023-11-15" urgency="medium">
      <description>
        <p>Minor bug fixes and documentation updates.</p>
      </description>
      <checksum type="sha256" target="content">
        11223344556677889900aabbccdd0011223344556677889900aabbccdd001122
      </checksum>
    </release>
    
    <release version="1.0.0" date="2023-06-01" urgency="low">
      <description>
        <p>Initial public release.</p>
      </description>
      <checksum type="sha256" target="content">
        00112233445566778899aabbccdd0011223344556677889900aabbccdd00112233
      </checksum>
    </release>
  </releases>
  
  <!-- 分類 -->
  <categories>
    <category>X-DeviceDriver</category>
  </categories>
  
  <!-- 關鍵字（用於 LVFS 搜尋）-->
  <keywords>
    <keyword>nuvoton</keyword>
    <keyword>m487</keyword>
    <keyword>arm</keyword>
    <keyword>cortex-m4</keyword>
    <keyword>mcu</keyword>
    <keyword>firmware</keyword>
  </keywords>
</component>
```

---

## 驗證 metainfo.xml

```bash
# 基本驗證（推薦）
appstream-util validate-relax metainfo.xml

# 驗證並查看詳細錯誤
appstream-util validate metainfo.xml

# 使用 fwupd-tool
fwupd-tool validate metainfo.xml

# 測試解析
fwupd-tool dump metainfo.xml
```

---

## 常見錯誤

| 錯誤訊息 | 原因 | 解法 |
|----------|------|------|
| `id is not valid` | ID 格式不符合規範 | 確認只使用允許的字元，參考上文 ID 規則 |
| `name is missing` | 缺少 name 標籤 | 添加 `<name>` 標籤 |
| `no releases defined` | 沒有版本資訊 | 添加至少一個 `<release>` |
| `no checksum` | 缺少校驗和 | 在 release 中添加 `<checksum>` |
| `invalid version format` | 版本格式無法解析 | 確認 version 格式與 VersionFormat 一致 |

---

## 參考資源

- [AppStream metainfo Specification](https://www.freedesktop.org/wiki/Specifications/metainfo-spec/)
- [LVFS metainfo 指南](https://fwupd.org/lvfs/docs/metainfo)
- [FWUPD quirks database](https://github.com/fwupd/fwupd/blob/main/data/quirks/quirks.md)

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

# CAB 檔案製作流程

## 什麼是 CAB 檔案？

FWUPD 的韌體分發使用 `.cab`（Cabinet）格式封裝。這個格式源自 Microsoft 的安裝程式封裝，但在 FWUPD 生態中被重新用於韌體分發。

CAB 檔案包含：
1. **metainfo.xml**：韌體元資料（版本、描述、識別資訊）
2. **firmware.bin**（或其他格式的韌體檔案）：實際的二進制燒錄資料
3. **簽章檔案**：用於驗證 CAB 內容真實性的數位簽章

---

## 檔案格式結構

```
firmware.cab
├── metainfo.xml          ← AppStream 元資料
├── firmware.bin          ← 實際韌體檔案（可命名）
└── [可選] 簽章檔案        ← 依簽章類型而定
```

---

## 工具需求

### 建構工具鏈

```bash
# Fedora / RHEL
sudo dnf install \
  fwupd \
  appstream-glib \
  gpgme \
  libxmlb \
  python3-gobject

# Ubuntu / Debian
sudo apt install \
  fwupd \
  appstream-util \
  gpgme \
  libxmlb-dev \
  python3-gi \
  python3-gi-cairo \
  gir1.2-appstream-1.0
```

### 核心工具

| 工具 | 用途 |
|------|------|
| `appstream-util` | 驗證 metainfo.xml 格式 |
| `msitools` (optional) | 處理 Microsoft CAB 格式 |
| `fwupd-tool` | 本地簽章、封裝、解析 CAB |
| `gpg` | GPG 簽章（legacy）|
| `openssl` / `pkcs7-tool` | X.509 PKCS#7 簽章（現代方式）|

---

## 步驟一：準備 metainfo.xml

詳細格式說明請參閱 `Firmware_CAB/Metainfo_XML.md`。

以下是一個最小可用的 metainfo.xml：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<component type="firmware">
  <id>com.company.m487-firmware</id>
  <name>Company M487 Device Firmware</name>
  <name xml:lang="zh_TW">公司 M487 裝置韌體</name>
  <summary>Firmware for M487-based product</summary>
  
  <provides>
    <firmware type="flashed">ae4e3d10-05c8-5b5e-ae8e-0416c0c487</firmware>
  </provides>
  
  <description>
    <p>描述此版本的主要變更內容。</p>
    <p>建議包含：</p>
    <ul>
      <li>Bug 修復</li>
      <li>安全性更新</li>
      <li>新功能</li>
    </ul>
  </description>
  
  <releases>
    <release version="1.2.0" date="2024-03-01">
      <description>
        <p>Initial production release with security fixes.</p>
      </description>
      <checksum type="sha256" target="content">
        <!-- 稍後填入 -->
      </checksum>
    </release>
  </releases>
  
  <categories>
    <category>X-DeviceDriver</category>
  </categories>
</component>
```

---

## 步驟二：準備韌體檔案

確保韌體檔案格式正確：

```bash
# 確認檔案格式（ELF、BIN、HEX 等）
file firmware.bin

# 確認檔案大小
ls -lh firmware.bin

# 計算 SHA256
sha256sum firmware.bin
```

### 命名慣例

建議命名格式：`${ProductName}_v${Version}.bin`

例如：`m487_bootloader_v1_2_0.bin`

---

## 步驟三：填入校驗和

在 `metainfo.xml` 的 `<checksum>` 標籤中填入實際的 SHA256 值：

```xml
<checksum type="sha256" target="content">
  a1b2c3d4e5f6...  <!-- 替換為實際 SHA256 -->
</checksum>
```

或使用指令自動產生包含正確校驗和的 metainfo：

```bash
# appstream-util 可幫助驗證
appstream-util validate-relax --no-suggest-remote firmware.metainfo.xml
```

---

## 步驟四：使用 fwupd-tool 建立 CAB

### 方法一：使用 fwupd-tool（推薦）

`fwupd-tool` 是 fwupd 原始碼提供的工具，位於原始碼的 `tools/` 目錄：

```bash
# 從原始碼編譯（需要先編譯 fwupd）
cd fwupd
meson setup build
ninja -C build fwupd-tool

# 建立 CAB 檔案
./build/fwupd-tool make-cab \
  --architecture x86_64 \
  --vendor "Company Name" \
  firmware.cab \
  metainfo.xml \
  firmware.bin
```

### 方法二：手動建立 CAB（使用標準工具）

```bash
# 確認 firmware.bin 的 SHA256
FIRMWARE_SHA=$(sha256sum firmware.bin | cut -d' ' -f1)

# 使用 AppStream CLI 生成完整 CAB
appstreamcli \
  repo-add \
  --pack \
  firmware.cab \
  metainfo.xml \
  firmware.bin

# 或使用標準 zip/cab 工具（不推薦，缺少 metadata）
```

### 方法三：使用 Python + libfwupd (較少用)

```python
#!/usr/bin/env python3
import subprocess

result = subprocess.run([
    'appstreamcli',
    'repo-add',
    '--pack',
    'output.cab',
    'metainfo.xml',
    'firmware.bin'
], capture_output=True, text=True)
print(result.stdout)
```

---

## 步驟五：簽章 CAB 檔案

### 現代方式：PKCS#7 / X.509 簽章（fwupd 2.x 推薦）

```bash
# 1. 生成 Ed25519 或 RSA 私鑰（一次性）
openssl genpkey -algorithm Ed25519 -outform PEM -out vendor.key

# 2. 生成自簽憑證（公開金鑰用於分發）
openssl req -new -x509 -key vendor.key -out vendor.crt -days 3650 \
  -subj "/O=Company Name/CN=Firmware Signing"

# 3. 使用私鑰對 CAB 簽章
# fwupd-tool sign firmware.cab --key vendor.key --cert vendor.crt

# 4. 驗證簽章
openssl smime -verify -in firmware.cab.sig -inform DER -noverify
```

### 傳統方式：GPG 簽章（逐漸淘汰）

```bash
# GPG 簽章（舊版 fwupd）
gpg --armor --detach-sign firmware.cab

# 產生的 firmware.cab.asc 即為簽章檔案
ls firmware.cab*
# firmware.cab
# firmware.cab.asc
```

> ⚠️ **注意**：fwupd 2.x 已逐步淘汰 GPG，強烈建議新專案使用 PKCS#7/X.509 簽章。

---

## 步驟六：驗證 CAB 檔案

### 本地驗證

```bash
# 驗證 metainfo.xml 格式
appstream-util validate firmware.metainfo.xml

# 驗證整個 CAB
fwupd-tool verify firmware.cab

# 查看 CAB 內容
fwupd-tool dump firmware.cab
```

### 測試安裝

```bash
# 離線安裝（不連接 LVFS）
fwupdmgr install firmware.cab

# 安裝並允許重新安裝相同版本
fwupdmgr install --allow-reinstall firmware.cab

# 強制安裝（跳過版本檢查）
fwupdmgr install --force firmware.cab
```

---

## 完整範例腳本

```bash
#!/bin/bash
# build-firmware-cab.sh

set -e

PRODUCT_NAME="m487_device"
VERSION="1.2.0"
METAINFO="metainfo.xml"
FIRMWARE="firmware.bin"
OUTPUT="m487_v${VERSION}.cab"
CERT="vendor.crt"
KEY="vendor.key"

echo "=== Building FWUPD CAB for ${PRODUCT_NAME} v${VERSION} ==="

# 1. 驗證 metainfo
echo "[1/6] Validating metainfo..."
appstream-util validate-relax "${METAINFO}" || {
    echo "ERROR: metainfo validation failed"
    exit 1
}

# 2. 計算校驗和並填入 metainfo
echo "[2/6] Computing SHA256..."
SHA256=$(sha256sum "${FIRMWARE}" | cut -d' ' -f1)
echo "SHA256: ${SHA256}"

# 3. 建立臨時 metainfo（含校驗和）
cat > "${METAINFO%.xml}_with_checksum.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!-- 請在此手動填入或使用程式生成 -->
EOF

# 4. 封裝 CAB
echo "[3/6] Creating CAB..."
if command -v fwupd-tool &> /dev/null; then
    fwupd-tool make-cab \
        --architecture arm64 \
        --vendor "Company Name" \
        "${OUTPUT}" \
        "${METAINFO}" \
        "${FIRMWARE}"
elif command -v appstreamcli &> /dev/null; then
    appstreamcli repo-add --pack "${OUTPUT}" "${METAINFO}" "${FIRMWARE}"
else
    echo "ERROR: Neither fwupd-tool nor appstreamcli found"
    exit 1
fi

# 5. 簽章
echo "[4/6] Signing..."
if [ -f "${KEY}" ] && [ -f "${CERT}" ]; then
    # 使用 fwupd-tool 簽章（如有）
    fwupd-tool sign "${OUTPUT}" --key "${KEY}" --cert "${CERT}" || \
    openssl smime -sign -binary -in "${OUTPUT}" -out "${OUTPUT}.p7b" \
        -signer "${CERT}" -inkey "${KEY}" -outform DER -nodetach || \
    echo "WARNING: Signing skipped (tool not available)"
else
    echo "WARNING: No signing key found, skipping signature"
fi

# 6. 驗證
echo "[5/6] Verifying..."
if command -v fwupd-tool &> /dev/null; then
    fwupd-tool verify "${OUTPUT}" || echo "Verification not available"
fi

echo "[6/6] Done!"
echo "Output: ${OUTPUT}"
echo ""
echo "SHA256 of CAB:"
sha256sum "${OUTPUT}"
```

---

## 常見問題

### Q：CAB 檔案大小有限制嗎？

FWUPD 本身對 CAB 大小沒有硬性限制，但 LVFS 可能對單一韌體有 2GB 上限。實際使用中，建議韌體保持在 100MB 以下以確保下載體驗。

### Q：多個韌體檔案可以放進同一個 CAB 嗎？

可以，但建議分開封裝以簡化管理。每個 CAB 對應一個獨立的 `component`（即一個 metainfo.xml）。

### Q：metainfo.xml 一定要和韌體檔名不同嗎？

不一定。fwupd-tool 會根據 metainfo.xml 中的 `<location>` 或推斷來找到實際的韌體檔案。建議保持一致命名。

### Q：如何在沒有 GPG 的情況下測試簽章？

可以使用 `appstream-util` 驗證元資料格式，CAB 簽章可以在實際部署時再處理（離線環境）。

---

## 參考資料

- [fwupd CAB creation](https://github.com/fwupd/fwupd/blob/main/docs/CAB.md)
- [LVFS Firmware Upload](https://fwupd.org/lvfs/docs/applications)
- [AppStream specification](https://www.freedesktop.org/wiki/Specifications/metainfo-spec/)

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

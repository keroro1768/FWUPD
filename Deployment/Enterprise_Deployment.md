# 企業內部部署方案

## 為什麼需要企業內部部署？

LVFS 是一個公開服務，適合一般消費者場景。但在企業環境中，往往需要：

- **安全合規**：韌體不能走外部網路，必須內網分發
- **版本管控**：特定產品線使用特定版本，不允許自動更新到最新
- **審計追蹤**：記錄誰在何時更新了哪個裝置
- **離線環境**：工控、醫療、軍事等完全隔離網路的環境
- **中國大陸環境**：fwupd.org 在境內可能無法直接訪問

---

## 方案一：fwupd 企業 Mirror（推薦）

### 原理

在企業內部架設一個 LVFS 的完整鏡像，只允許特定版本的韌體通過。

### 架構圖

```
┌──────────────────────────┐        ┌──────────────────────────┐
│   LVFS (fwupd.org)       │        │  Enterprise Mirror       │
│                          │  sync  │  (fwupd.company.local)    │
│  - 完整韌體庫            │ ──────►│  - 精選韌體庫             │
│  - 全球用戶訪問          │        │  - 企業認證               │
└──────────────────────────┘        └────────────┬─────────────┘
                                                  │
                                                  │ HTTPS
                                                  ▼
┌──────────────────────────────────────────────────────────────────┐
│                     Enterprise Network                            │
│                                                                   │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐        │
│   │   Server    │    │   Laptop    │    │   Embedded  │        │
│   │  (fwupdmgr) │    │  (fwupdmgr) │    │   Device    │        │
│   └─────────────┘    └─────────────┘    └─────────────┘        │
│                                                                   │
└──────────────────────────────────────────────────────────────────┘
```

### 實作步驟

#### 1. 建立 Mirror 設定

```bash
# 在 /etc/fwupd/remotes.d/ 建立企業 mirror 設定
sudo tee /etc/fwupd/remotes.d/company-mirror.conf << 'EOF'
[remote]
DisplayName=Company Internal Firmware Repository
MetadataURI=https://firmware.company.internal/lvfs/metadata/
FirmwareBaseURI=https://firmware.company.internal/lvfs/
Enabled=true
ApprovalRequired=false
Username=
Password=
AllowNonStageFirmware=false
EOF
```

#### 2. 同步 LVFS 內容

```bash
# 使用 rsync 或自建腳本定期同步（僅同步通過審批的韌體）
#!/bin/bash
# sync-lvfs-mirror.sh

LVFS_MIRROR="rsync://mirror.example.com::lvfs"
LOCAL_DIR="/var/www/firmware.company.internal/lvfs"
ALLOWED_PRODUCTS="m487-bootloader|company-sensor|company-gateway"

rsync -avz --include="*/" \
  --exclude="*" \
  -e "ssh" \
  ${LVFS_MIRROR}/ \
  ${LOCAL_DIR}/

# 只保留白名單中的產品
for product in $(ls ${LOCAL_DIR}/firmware/); do
  if ! echo "$product" | grep -qE "$ALLOWED_PRODUCTS"; then
    rm -rf ${LOCAL_DIR}/firmware/$product
    echo "Removed non-whitelisted: $product"
  fi
done

echo "Mirror sync completed at $(date)"
```

#### 3. 定期更新元資料

```bash
# 建立 cron 任務，每天凌晨 3 點同步
sudo crontab -e
# 0 3 * * * /opt/firmware/sync-lvfs-mirror.sh
```

---

## 方案二：離線包部署（適合隔離網路）

### 原理

從 LVFS 下載需要的 CAB 檔案，通過 U盤、內網共享或其他方式手動分發。

### 流程

```
1. 在有網路的環境下，下載 CAB 檔案
        │
        ▼
2. 複製到離線媒體（USB、網路共享）
        │
        ▼
3. 在隔離環境中使用 fwupdmgr 安裝
        │
        ▼
4. 記錄更新結果（書面或電子）
```

### 離線下載腳本

```bash
#!/bin/bash
# download-firmware-offline.sh
# 在有網路的機器上執行

OUTPUT_DIR="/media/usb/firmware-packages"
LVFS_PRODUCTS=(
    "com.nuvoton.m487-firmware"
    "com.company.sensor-firmware"
)

mkdir -p "${OUTPUT_DIR}"

for product in "${LVFS_PRODUCTS[@]}"; do
    echo "Downloading $product..."
    
    # 使用 lvfs-cli 查詢並下載
    lvfs-cli download --product-id "$product" --version latest \
        --output "${OUTPUT_DIR}/"
    
done

# 同時下載最新的 metadata
fwupdmgr refresh --force
cp ~/.cache/fwupd/*.xml.gz "${OUTPUT_DIR}/metadata/" 2>/dev/null || true

echo "Done. Files ready in ${OUTPUT_DIR}"
```

### 離線部署腳本

```bash
#!/bin/bash
# deploy-firmware.sh
# 在隔離環境的目標機器上執行

FW_DIR="/media/usb/firmware-packages"
LOG="/var/log/firmware-update.log"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG"
}

log "=== Starting firmware deployment ==="

# 離線模式：不嘗試連接 LVFS
export FWUPD_OFFLINE=1

for cab in "${FW_DIR}"/*.cab; do
    if [ -f "$cab" ]; then
        log "Installing: $cab"
        fwupdmgr install --force "$cab" 2>&1 | tee -a "$LOG"
        
        if [ $? -eq 0 ]; then
            log "SUCCESS: $cab"
        else
            log "FAILED: $cab"
        fi
    fi
done

log "=== Deployment completed ==="
```

---

## 方案三：MDM/EMM 整合（大規模管理）

### 整合企業移動裝置管理

對於筆記型電腦和桌上型電腦，可以使用現有的 MDM 工具觸發 fwupd 更新。

#### Intune / Microsoft Endpoint Manager

```powershell
# Intune Proactive Remediation Script (Windows)
# 觸發 fwupd 韌體更新（Windows 10/11 已原生整合 LVFS）

$fwupdPath = "C:\ProgramData\chocolatey\bin\fwupdmgr.exe"

if (Test-Path $fwupdPath) {
    # 檢查並安裝更新
    & $fwupdPath refresh
    $updates = & $fwupdPath get-updates
    
    if ($updates -match "No updates") {
        Write-Host "No firmware updates available"
    } else {
        & $fwupdPath update --force
    }
}
```

#### SCCM/ConfigMgr

```powershell
# 以 SYSTEM 權限執行，適合大批量部署
$ErrorActionPreference = "Stop"

try {
    Write-Host "Checking for firmware updates..."
    $result = & fwupdmgr get-devices
    
    if ($result -match "UUP1") {
        Write-Host "Device found: UUP1"
        & fwupdmgr update --device UUP1 --force
    }
} catch {
    Write-Error "Update failed: $_"
    exit 1
}
```

---

## 方案四：LDAP/AD 觸發自動更新

### 透過使用者登入腳本觸發

```bash
#!/bin/bash
# update-firmware.sh
# 放在網路芳鄰共享（\\company-server\firmware）

LOG="/var/log/fwupd-corporate.log"

log() {
    echo "[$(date)] $1" >> "$LOG"
}

log "=== Starting corporate firmware check ==="

# 刷新元資料（使用本地 mirror）
fwupdmgr refresh --force https://firmware.company.internal/lvfs/metadata/

# 檢查更新
UPDATES=$(fwupdmgr get-updates 2>&1)

if echo "$UPDATES" | grep -q "No updates"; then
    log "No updates available"
    exit 0
fi

# 只對 IT 指定群組自動更新，普通用戶顯示通知
USER_GROUP=$(id -Gn | grep -c "it-engineers")

if [ "$USER_GROUP" -gt 0 ]; then
    log "Auto-updating (IT user detected)"
    fwupdmgr update --force 2>&1 | tee -a "$LOG"
else
    log "Non-IT user - skipping auto-update"
    # 顯示桌面通知
    notify-send "Firmware Update Available" \
        "New firmware updates are available. Contact IT to schedule installation."
fi
```

---

## 方案五：專屬韌體倉庫（完全自建）

### 從頭建立企業韌體服務

適用於對安全要求極高的環境，完全不依賴 LVFS。

```bash
# 簡單的企業 fwupd 代理伺服器架設

# 1. 架設 Web 伺服器
sudo apt install nginx

# 2. 配置目錄結構
sudo mkdir -p /var/www/firmware.company.internal/{lvfs/metadata,lvfs/firmware}

# 3. 建立 metainfo 索引
# /var/www/firmware.company.internal/lvfs/metadata/firmware.xml.gz
# （由管理員手動放置）

# 4. nginx 配置
sudo tee /etc/nginx/sites-available/firmware << 'EOF'
server {
    listen 443 ssl;
    server_name firmware.company.internal;
    
    ssl_certificate /etc/ssl/certs/company.crt;
    ssl_certificate_key /etc/ssl/private/company.key;
    
    root /var/www/firmware.company.internal;
    
    location /lvfs/metadata/ {
        autoindex off;
        add_header Cache-Control "public, max-age=3600";
    }
    
    location /lvfs/firmware/ {
        autoindex off;
        add_header Cache-Control "public, max-age=86400";
        auth_basic "Restricted";
        auth_basic_user_file /etc/nginx/.htpasswd;
    }
}
EOF
```

---

## 企業部署檢查清單

### 準備階段
- [ ] 確認網路架構（能否訪問 LVFS）
- [ ] 盤點需要更新的裝置清單
- [ ] 確認各裝置支援 FWUPD（查詢 Plugin 支援狀態）
- [ ] 準備韌體 CAB 檔案（從 LVFS 下載或自行建立）
- [ ] 決定部署方案（Mirror / 離線 / MDM）

### 安全階段
- [ ] 確認韌體簽章驗證正常運作
- [ ] 建立韌體更新的審批流程
- [ ] 定義版本策略（如：不自動更新到最新，固定版本）
- [ ] 建立回滾計畫（當更新失敗時）

### 執行階段
- [ ] 先在少量測試裝置上驗證
- [ ] 準備使用者溝通材料
- [ ] 排定維護窗口（更新可能需要重啟）
- [ ] 執行分階段部署

### 監控階段
- [ ] 建立更新日誌集中收集（syslog / ELK）
- [ ] 設定更新失敗的告警
- [ ] 定期審視更新合規率

---

## 故障排除

### 常見問題

| 問題 | 原因 | 解法 |
|------|------|------|
| `No metadata available` | 元資料未刷新 | `fwupdmgr refresh` |
| `Connection refused` | 企業 Mirror 未啟動 | 檢查 Nginx/fwupd 服务 |
| `Signature verification failed` | 簽章不被信任 | 確認企業 CA 憑證已安裝 |
| `Device not supported` | Plugin 未安裝 | 安裝對應的 fwupd 插件 |
| `Update blocked by policy` | D-Bus 權限不足 | `pkexec fwupdmgr update` |

### 企業環境專屬除錯

```bash
# 檢查使用的遠端來源
fwupdmgr remotes

# 測試企業 Mirror 連線
curl -I https://firmware.company.internal/lvfs/metadata/

# 查看詳細錯誤
fwupdmgr get-updates --verbose 2>&1 | grep -i error

# 清除並重新刷新元資料
fwupdmgr clear-cache
fwupdmgr refresh --force

# 查看 fwupd daemon 日誌
journalctl -u fwupd -n 50 --no-pager
```

---

## 法規與合規考量

### 醫療裝置（MDR/FDA）
- 韌體更新必須記錄並可審計
- 必須驗證更新不會影響安全性關鍵功能
- 建議維持「更新前必須批准」的流程

### 工業控制（IEC 62443）
- 離線驗證韌體完整性
- 限制網路存取，只能透過受控管道更新
- 所有更新操作必須有事件日誌

### 汽車（ISO/SAE 21434）
- 韌體更新涉及車載安全系統
- 通常需要 OBD-II 或特殊工具，不走 LVFS

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

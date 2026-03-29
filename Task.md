# T038 任務追蹤 — Linux FWUPD 韌體更新功能研究

> 最後更新：2026-03-28
> 負責人：Kururu（子任務執行者）/ Giroro（協調）
> 工作目錄：`D:\AiWorkSpace\KM\KM-collect\FWUPD\`

---

## 任務狀態總覽

| 項目 | 狀態 | 說明 |
|------|------|------|
| **Basics 目錄** | ✅ 完成 | 3 份文件已建立 |
| **Plugin_Development 目錄** | ✅ 完成 | 3 份文件已建立 |
| **Firmware_CAB 目錄** | ✅ 完成 | 2 份文件已建立 |
| **Deployment 目錄** | ✅ 完成 | 2 份文件已建立 |
| **Task.md 更新** | ✅ 完成 | 本文件 |

---

## 任務目標

本任務旨在建立完整的 FWUPD（Linux Vendor Firmware Update）知識庫，供公司內部研究與技術參考使用。

研究重點包含：
1. FWUPD 基本概念與系統架構
2. LVFS（Linux Vendor Firmware Service）運作機制
3. Plugin 開發（尤其是 Nuvoton M487 的實作可能性）
4. CAB 檔案封裝流程
5. LVFS 上架與企業內部部署

---

## 已完成項目

### ✅ Basics 目錄（3/3 完成）

- [x] `What_is_FWUPD.md` — FWUPD 基本介紹
  - 定義、歷史背景、核心特性
  - 支援的韌體類型
  - 常見使用情境（消費者/企業/開發者）

- [x] `Architecture.md` — 系統架構圖與元件說明
  - 四層架構總覽（客戶端 / D-Bus / Daemon / Plugin）
  - 各層元件詳細說明
  - 資料流向與安全性模型

- [x] `LVFS_Explained.md` — LVFS 服務說明
  - LVFS 運作模式（公開上架 vs 私人內測）
  - 供應商上傳流程（5 步驟）
  - 簽章機制（三層簽章）
  - REST API 說明

### ✅ Plugin_Development 目錄（3/3 完成）

- [x] `Plugin_Types.md` — 三種 Plugin 類型說明
  - Emulated Plugin（運行時直接存取）
  - Desktop-style Plugin（detach/attach 模式切換）
  - Boot-time Plugin（開機搶佔式）
  - 類型選擇決策樹

- [x] `Development_Guide.md` — Plugin 開發指南
  - 開發環境建置
  - Plugin 基本結構（meson.build、C 程式碼）
  - FuDevice 子類別化
  - 關鍵 Plugin 鉤子（constructed、device_probe、write_firmware）
  - Quirks 系統
  - 測試與除錯流程

- [x] `M487_Example.md` — 以 M487 為例的實作分析
  - M487 晶片概覽與啟動模式
  - Plugin 類型選擇（Desktop-style Plugin）
  - M487 USB Download Protocol 分析
  - Plugin 實作架構（detach/write_firmware/attach）
  - Quirks 配置
  - LVFS 整合說明

### ✅ Firmware_CAB 目錄（2/2 完成）

- [x] `CAB_Creation.md` — CAB 檔案製作流程
  - 工具需求與環境建置
  - 五步驟製作流程
  - 簽章方式（PKCS#7/X.509 vs GPG）
  - 驗證與測試方法
  - 完整 Bash 腳本範例

- [x] `Metainfo_XML.md` — metainfo.xml 格式說明
  - 完整結構說明（id、name、provides、releases 等）
  - GUID 生成方法
  - 版本號格式
  - 完整範例
  - 驗證與常見錯誤

### ✅ Deployment 目錄（2/2 完成）

- [x] `LVFS_Upload.md` — LVFS 上架流程
  - 帳號申請（5 階段）
  - Web UI / CLI / REST API 三種上傳方式
  - Embargo（保密髮布）機制
  - 審核不通過常見原因
  - Python 自動化腳本範例

- [x] `Enterprise_Deployment.md` — 企業內部部署方案
  - 五種部署方案：
    1. fwupd 企業 Mirror（推薦）
    2. 離線包部署（隔離網路）
    3. MDM/EMM 整合（Intune/SCCM）
    4. LDAP/AD 觸發自動更新
    5. 專屬韌體倉庫（完全自建）
  - 企業部署檢查清單（準備/安全/執行/監控）
  - 法規合規考量（醫療 MDR、工業 IEC 62443、汽車 ISO/SAE 21434）

---

## 知識庫總產出

| 目錄 | 文件數 | 總大小（Bytes）|
|------|--------|----------------|
| Basics | 3 | ~17,700 |
| Plugin_Development | 3 | ~30,300 |
| Firmware_CAB | 2 | ~16,000 |
| Deployment | 2 | ~15,000 |
| **合計** | **10** | **~79,000** |

---

## 技術建議摘要

### M487 支援 FWUPD 的可行性評估

| 項目 | 評估 | 備註 |
|------|------|------|
| **Plugin 類型** | Desktop-style Plugin | 可行 |
| **通訊介面** | USB HID/Bulk | M487 USB Download Mode 天然支援 |
| **難度評估** | 中等 | 需要實作 USB 下載協定 |
| **Plugin 開發時間** | 2-4 週 | 取決於 Nuvoton 是否提供 Protocol 文件 |
| **LVFS 上架** | 可行 | 需聯繫 LVFS 管理員開通帳號 |

### 關鍵風險

1. **通訊協定文件取得**：Nuvoton 是否願意提供 USB Download Protocol 文件
2. **VID/PID 客製化**：量產時需申請獨立 VID/PID
3. **簽章金鑰管理**：需安全的私鑰管理機制
4. **China 網路環境**：fwupd.org 在境內可能無法訪問，需企業 Mirror

---

## 待下一步行動（不屬於本次研究範圍）

以下項目超出 T038 研究範圍，但值得後續評估：

- [ ] 向 Nuvoton 申請 M487 USB Download Protocol 文件或評估逆向可行性
- [ ] 評估其他公司 IC（如有）的 FWUPD 支援優先順序
- [ ] 建立公司內部 FWUPD Plugin 開發環境
- [ ] 聯繫 LVFS 管理員（Richard Hughes）諮詢 Nuvoton 上架事宜
- [ ] 評估在中國大陸環境下架設 LVFS Mirror 的可行性
- [ ] 制定公司韌體簽章金鑰管理政策

---

## 參考資源

| 資源 | 連結 |
|------|------|
| FWUPD 官網 | https://fwupd.org/ |
| LVFS 首頁 | https://fwupd.org/lvfs |
| FWUPD GitHub | https://github.com/fwupd/fwupd |
| Plugin 教程 | https://fwupd.github.io/libfwupdplugin/tutorial.html |
| AppStream metainfo 規範 | https://www.freedesktop.org/wiki/Specifications/metainfo-spec/ |
| ChromeOS Peripheral Guide | https://developers.google.com/chromeos/peripherals/fwupd-guide-for-peripherals |
| ArchWiki FWUPD | https://wiki.archlinux.org/title/Fwupd |

---

*本文件為 T038 任務追蹤文件，每次研究更新後請同步更新本文件狀態。*

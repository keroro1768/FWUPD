# 任務清單 / Task List

> 最後更新：2026-03-30
> 政策：每 30 分鐘 Heartbeat 檢查進度

---

## 任務狀態說明

| 狀態 | 標籤 | 意義 |
|------|------|------|
| Pending | ⏳ | 等待中（待研究、待驗證、待硬體） |
| Ongoing | 🔄 | 執行中 |
| Finish | ✅ | 已完成 |
| On Hold | ⏸️ | 暫停（外部依賴） |
| Deferred | ⏰ | 延後（短期不執行） |

---

## 任務執行規則

1. **軟體優先：** 無硬體也能執行的任務優先處理
2. **持續推進：** 每 30 分鐘 heartbeat 檢查，發現可執行任務立即開始
3. **相依管理：** 若任務被 block，維持 Pending 並檢查下游任務
4. **Fallback 原則：** 若所有任務皆因不可抗因素無法推進，**立即轉向 Review 與功能優化**，不得停滯

---

## FWUPD 知識庫建置任務

### F001 — HID I2C 產品 FWUPD 整合研究

**狀態：** ✅ Finish
**起始：** 2026-03-30
**負責：** 🐸 Kururu（研究）→ 🦀 Dororo（驗證）
**目標：** 建立自家 HID I2C 產品如何整合 FWUPD 的完整知識庫

**進度：**
- [x] Kururu 研究完成（9個文件）
- [x] Dororo 驗證完成（已修正錯誤）
- [x] Commit 至 GitHub（`129d23d`）

**文件位置：** `doc/I2C_HID_Company_Product/`

**驗證結果：**
- ✅ 正確：5 個文件（Overview, HID_I2C_Protocol, Examples, LVFS_Upload, Links）
- ⚠️ 小修正：3 個文件（Plugin_Guide, Local_Testing, LVFS_Testing）
- ❌ 已修正：2 個錯誤（Plugin_Guide I2C addr, Production version_lowest）

---

### F002 — FWUPD Plugin 開發實作

**狀態：** 🔄 進行中
**起始：** 2026-03-30
**負責：** 🐱 Giroro
**目標：** 實作完整的 FWUPD Plugin（company-i2c-hid）

**產出位置：** `plugin/company-i2c-hid/`

**進度：**
- [x] Task.md 建立
- [ ] Plugin 原始碼實作
- [ ] Dororo 驗證
- [ ] Commit + Push

---

## Enhancement Backlog

 Enhancement 提案存放於 `tasks/enhancement/` 目錄。

---

## 執行狀態摘要

| 任務 | 狀態 | 優先 | 負責 |
|------|------|------|------|
| F001 | ✅ Finish | - | Kururu/Dororo |
| F002 | 🔄 進行中 | P0 | Giroro |

---

## Review 政策

所有任務完成後，必須執行 Dororo 驗證：
1. 事實查核 — 文件內容是否正確
2. 來源確認 — 是否提供官方/官方程式碼出處
3. 完整性評估 — 是否缺少關鍵資訊

---

## 🚀 執行順序

### 第一梯隊（今天可開始）

| 順序 | 任務 | 負責 | 說明 |
|------|------|------|------|
| 1 | F001 I2C HID 研究 | ✅ 已完成 | - |
| 2 | F001 Review | ✅ 已完成 | Dororo 已驗證 |

### 下一階段（待 Karoro 指派）

| 順序 | 任務 | 說明 |
|------|------|------|
| - | F002 Plugin 開發實作 | 根據 F001 研究結果，開發實際的 FWUPD Plugin |
| - | F003 LVFS 帳號申請 | 申請vendor帳號，准備上線 |

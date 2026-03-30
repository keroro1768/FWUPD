# Task.md — F002

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | F002 |
| 標題 | FWUPD Plugin 開發實作 |
| 類型 | 🔧 實作 |
| 負責 | 🐱 Giroro |
| 起始日期 | 2026-03-30 |
| 狀態 | 🔄 進行中 |

---

## 目標

根據 F001 研究成果，實作一個可運作的 FWUPD Plugin 範例，針對 HID I2C 裝置。

**目標產出：**
- Plugin 完整原始碼（可編譯）
- 以 elantp 為最佳參考範本
- 支援 I2C 模式和 HID 模式雙支援（如果適用）
- 包含完整的設備枚舉、更新邏輯、韌體上傳

---

## 背景

F001 研究發現：
- **最佳參考 Plugin：** elantp（Elan TouchPad），完整實作 HID + I2C 雙模式
- **Protocol ID：** `com.company.i2c-hid`（待定義）
- **HW-000 Plugin：** 可參考的硬體 ID 對應方式

---

## 實作範圍

### 包含
- Plugin 目錄結構（meson.build + 原始碼）
- FuPlugin 子類別實作
- FuDevice 子類別實作
- 設備枚舉（coldplug）
- 設備 ID 生成策略
- 韌體更新邏輯
- FuDevice 生命週期鉤子（attach/ detach/ prepare）
- Quirk 檔案設定
- README.md（說明如何編譯和測試）

### 不包含
- 實際硬體測試（硬體未就緒）
- LVFS 上傳（需要 vendor 帳號）
- 正式發布流程

---

## 預估時間

約 2-4 小時（取決於複雜度）

---

## 標籤

#fwupd #plugin #implementation #giruor

---

## 更新日誌

| 日期 | 更新內容 |
|------|----------|
| 2026-03-30 | 建立任務 |

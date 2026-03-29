# Task.md — F001

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | F001 |
| 標題 | HID I2C 產品 FWUPD 整合研究 |
| 類型 | 🔬 研究 |
| 負責 | 🐸 Kururu |
| 起始日期 | 2026-03-30 |
| 狀態 | ✅ Finish |

---

## 目標

建立自家公司使用 HID I2C Protocol 的 I2C 介面產品，如何從零開始整合 FWUPD 的完整知識庫，包括：
1. FWUPD Plugin 開發（從零開始）
2. LVFS 帳號申請與上傳流程
3. 本地測試與正式發布流程

---

## 背景

Caro 在 2026-03-30 的 Brainstorm 會議中提出：
> 「如何一步步製作並可以實際上線測試到正式發布自家公司的 I2C 裝置的 FWUPD」

Keroro 小隊接受此任務，交由 Kururu 負責研究。

---

## 研究範圍

```
doc/I2C_HID_Company_Product/
├── 00_Overview.md           # 總覽與流程圖
├── 01_Plugin_Development/   # Plugin 開發相關
│   ├── Plugin_Guide.md      # Plugin 開發指南
│   ├── HID_I2C_Protocol.md  # HID I2C 協定說明
│   └── Examples.md          # 現有範例研究
├── 02_Testing/              # 測試流程
│   ├── Local_Testing.md     # 本地離線測試
│   └── LVFS_Testing.md      # LVFS 線上測試
├── 03_Deployment/           # 部署發布
│   ├── LVFS_Upload.md       # LVFS 上傳流程
│   └── Production.md        # 正式發布
└── 04_Reference/            # 參考資料
    └── Links.md             # 重要連結
```

---

## 資源

- fwupd 官方網站：https://fwupd.org/
- LVFS 文件：https://lvfs.readthedocs.io/
- fwupd GitHub：https://github.com/fwupd/fwupd
- HID I2C 協定：Linux kernel hid-i2c driver
- ChromeOS EC Plugin 範例

---

## 預估時間

約 30 分鐘（網路研究）

---

## 標籤

#fwupd #research #i2c #hid

---

## 更新日誌

| 日期 | 更新內容 |
|------|----------|
| 2026-03-30 | 建立任務 |
| 2026-03-30 | Kururu 完成研究（9個文件）|
| 2026-03-30 | Dororo 完成驗證（已修正錯誤）|

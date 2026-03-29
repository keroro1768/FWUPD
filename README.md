# FWUPD — Linux 韌體更新

> 最後更新：2026-03-29

## 什麼是 FWUPD？

FWUPD（Linux Vendor Firmware Update）是 Linux 開源韌體更新框架。

## 組成架構

```
┌──────────────────┐
│  GNOME Software  │
│    fwupdmgr     │
└────────┬─────────┘
         │ D-Bus
         ▼
┌──────────────────┐
│    fwupd daemon  │
│   (Plugin)       │
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│    LVFS Server    │
│  lvfs.fwupd.org  │
└──────────────────┘
```

## Plugin 類型

| 類型 | 說明 | 難度 |
|------|------|------|
| Quirk-only | 只需設定檔 | ⭐ |
| Simple Plugin | 有 Helper 可用 | ⭐⭐ |
| Complex Plugin | 完全自訂 | ⭐⭐⭐⭐ |

## 詳見

- `Deployment/` — LVFS 上架流程
- `Firmware_CAB/` — CAB 檔案製作

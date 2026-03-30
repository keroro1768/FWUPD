# Brainstorm 會議：虛擬硬體測試方案

> **日期：** 2026-03-30  
> **參與：** Keroro 小隊（Caro 主持）  
> **目的：** 研究是否可能創造虛擬硬體來測試 FWUPD Plugin

---

## 一、現有測試框架分析

### 1.1 fwupd 官方測試架構

fwupd 官方提供完整的測試框架，主要有以下幾種：

| 測試類型 | 說明 | 是否需要實體硬體 |
|----------|------|------------------|
| **Self-Test** | 單元測試（`*_self-test.c`） | ❌ 不需要 |
| **Emulation Test** | 模擬設備測試（JSON + emulation-url） | ❌ 不需要 |
| **Integration Test** | 完整整合測試 | ⚠️ 可能需要 |
| **Manual Test** | 手動測試 | ✅ 需要 |

### 1.2 Elantp Plugin 的測試方式

從 elantp plugin 的 `meson.build` 可以看到：

```meson
# Self-test
e = executable('elantp-self-test', ...)
test('elantp-self-test', e, env: env)

# Device tests
device_tests += files(
  'tests/elan-i2c-tp.json',
)

# Emulation
install_data(['tests/elantp.builder.xml'], ...)
```

**關鍵發現：**
- `elan-i2c-tp.json` 包含 `emulation-url` 欄位
- fwupd 有專門的 **Emulation System** 支援模擬設備
- `elantp.builder.xml` 使用 **Builder XML** 定義測試韌體

---

## 二、虛擬測試方案選項

### 方案 A：fwupd 官方 Emulation System ✅ 最推薦

**原理：** fwupd 支援模擬設備測試，不需要真實硬體。

**運作方式：**
```
測試定義（JSON）
    ↓
fwupd daemon（ emulation mode）
    ↓
模擬的 FuDevice（虛擬）
    ↓
Plugin邏輯測試
```

**優點：**
- 官方支援，架構完整
- 不需要實體硬體
- 可測試 Plugin 邏輯（枚舉、更新流程）
- 可自動化 CI/CD

**缺點：**
- 無法測試真實 USB/I2C 通訊
- 無法測試時序問題
- 需要定義模擬設備的行為

**如何實作：**
1. 建立測試 JSON 檔案（參考 `elan-i2c-tp.json`）
2. 建立 Builder XML（定義虛擬韌體）
3. 使用 `fwupdmgr emulatedevice` 或類似指令

**所需檔案：**
```
plugin/company-i2c-hid/tests/
├── company-i2c-hid-self-test.c      # 單元測試
├── company-i2c-hid.json              # 設備測試定義
└── company-i2c-hid.builder.xml       # 模擬韌體定義
```

---

### 方案 B：Mock Framework（GLib/GMock）

**原理：** 使用 GLib 的測試框架（g_test）配合 Mock 物件。

**fwupd 內建方式：**
- `fu_device_add_flag(FU_DEVICE, FWUPD_DEVICE_FLAG_EMULATED)` 標記為模擬設備
- `fu_context_add_firmware_gtype()` 替換韌體類型

**優點：**
- 可完全控制設備行為
- 可測試各種錯誤條件
- 不需要特權

**缺點：**
- 需要撰寫 Mock 程式碼
- 複雜度高

---

### 方案 C：Linux USB/I2C 模擬工具

**Linux 內建支援：**
1. **USB Gadget Configfs** — 模擬 USB Device
   ```bash
   modprobe libcomposite
   # 建立虛擬 USB 裝置
   ```

2. **I2C 模擬** — 使用 `i2c-stub` 或自訂 kernel module
   ```bash
   # i2c-stub (Linux kernel)
   modprobe i2c-stub chip_addr=0xXX
   ```

3. **hidraw 模擬** — 可透過 `uinput` 建立虛擬 HID 裝置

**優點：**
- 可類比真實 USB/I2C 行為
- 接近實際使用情境

**缺點：**
- 需要 Linux 環境
- 設定複雜
- 需要 root 權限

---

### 方案 D：QEMU + USB Passthrough

**原理：** 在虛擬機器中執行 fwupd，搭配 USB Passthrough。

**QEMU USB 模擬：**
```bash
# 建立虛擬 USB 裝置
qemu -usbdevice vendorid=0xXXXX productid=0xXXXX
```

**優點：**
- 可完整模擬 USB 協定
- 可重現各種 USB 場景

**缺點：**
- 資源消耗大
- 速度慢
- 設定複雜

---

### 方案 E：離線 .cab 安裝測試

**原理：** 不透過 LVFS，直接使用 `fwupdmgr install` 安裝本地 .cab 檔。

**測試流程：**
```bash
# 產生測試韌體 .cab
fwupd-tool buildCab firmware.cab company-i2c-hid.quirk

# 離線安裝
fwupdmgr install firmware.cab
```

**優點：**
- 最簡單的測試方式
- 不需要網路
- 可驗證 Plugin 的 `.cab` 處理邏輯

**缺點：**
- 仍需要實體設備
- 無法測試枚舉流程

---

## 三、推薦方案：混合測試策略

根據我們目前的狀況（無實體硬體），建議採用**多層次測試策略**：

### 第一層：Plugin 邏輯測試（立即可行）✅

使用 **fwupd Self-Test Framework**：
- 建立 `*_self-test.c` 單元測試
- Mock 掉所有硬體相依性
- 測試 Plugin 內部邏輯

**所需檔案：**
```c
// fu-company-i2c-hid-self-test.c
void
fu_company_i2c_hid_plugin_test_write_firmware(void)
{
    // Mock FuDevice
    // 建立測試用 FuCompanyI2cHidDevice
    // 呼叫 write_firmware
    // 驗證結果
}
```

### 第二層：模擬設備測試（中期目標）🔄

建立 **JSON 測試定義**：
```json
// tests/company-i2c-hid.json
{
  "name": "Company I2C HID Device",
  "interactive": false,
  "steps": [
    {
      "url": "test-firmware.cab",
      "emulation-url": "test-firmware-emulation.zip",
      "components": [
        {
          "version": "1.0.0",
          "guids": ["company-i2c-hid-xxxx"]
        }
      ]
    }
  ]
}
```

### 第三層：離線整合測試（硬體就緒後）⏳

- 使用 `fwupdmgr install` 離線安裝
- 驗證完整更新流程

---

## 四、實作路徑

### 立即可行（今天）

1. **建立 Self-Test 框架**
   - 在 `plugin/company-i2c-hid/tests/` 建立 `fu-self-test.c`
   - 測試 Plugin 初始化
   - 測試 Device 生命週期

2. **下載 elantp Self-Test 參考**
   - `https://github.com/fwupd/fwupd/blob/main/plugins/elantp/fu-self-test.c`

### 本週內

3. **建立模擬設備定義**
   - 建立 `tests/company-i2c-hid.json`
   - 建立 `tests/company-i2c-hid.builder.xml`

4. **建立 CI/CD 測試腳本**
   - GitHub Actions 自動執行測試

### 長期目標

5. **Linux USB/I2C 模擬整合**
   - 研究 USB Gadget Configfs
   - 建立可重現的測試環境

---

## 五、風險與限制

| 風險 | 可能性 | 影響 | 應對 |
|------|--------|------|------|
| 無法類比 USB 列舉 | 高 | 中 | 使用方案 A/B 的 Mock |
| 測試覆蓋率不足 | 中 | 中 | 增加更多測試案例 |
| CI 環境設定複雜 | 中 | 低 | 參考 fwupd 官方 CI |
| 時序問題無法測試 | 高 | 低 | 標記為已知限制 |

---

## 六、結論

**可行性評估：** ✅ **可行**

雖然無法完全替代實體硬體測試，但**多層次測試策略**可以：
1. 驗證 Plugin 邏輯正確性
2. 自動化回歸測試
3. 在 CI/CD 中快速驗證變更

**建議優先順序：**
1. ✅ **立即：** 建立 Self-Test 框架
2. 🔄 **本週：** 建立 JSON 模擬設備定義
3. ⏳ **長期：** Linux USB/I2C 模擬整合

---

## 七、關鍵資源

- [fwupd Plugin Tutorial](https://fwupd.github.io/libfwupdplugin/tutorial.html)
- [elantp self-test.c](https://github.com/fwupd/fwupd/blob/main/plugins/elantp/fu-self-test.c)
- [elan-i2c-tp.json](https://github.com/fwupd/fwupd/blob/main/plugins/elantp/tests/elan-i2c-tp.json)
- [fwupd Testing](https://github.com/fwupd/fwupd/blob/main/docs/CONTRIBUTING.md#testing)

---

*會議結論：虛擬硬體測試是可行的，建議採用多層次測試策略，優先建立 Self-Test 框架和模擬設備定義。此研究結果供後續實作參考。*

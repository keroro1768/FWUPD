# FWUPD Plugin 開發指南

## 開發環境建置

### 前置工具

```bash
# Fedora / RHEL / CentOS
sudo dnf install \
  fwupd-devel \
  meson \
  ninja-build \
  gcc \
  gpgme-devel \
  libxmlb-devel \
  glib2-devel \
  sqlite-devel \
  libcurl-devel \
  systemd-devel \
  polkit-devel \
  gobject-introspection-devel \
  python3-gobject

# Debian / Ubuntu
sudo apt install \
  fwupd-dev \
  meson \
  ninja-build \
  gcc \
  libgpgme-dev \
  libxmlb-dev \
  libglib2.0-dev \
  libsqlite3-dev \
  libcurl4-gnutls-dev \
  libsystemd-dev \
  libpolkit-gobject-1-dev \
  gir1.2-gudev-1.0 \
  python3-gi
```

### 獲取原始碼並建立構建環境

```bash
# 克隆 fwupd 原始碼
git clone https://github.com/fwupd/fwupd.git
cd fwupd

# 建立 build 目錄
meson setup build --prefix=/usr
cd build

# 編譯（可觀察插件編譯過程）
ninja

# 快速測試編譯（不完整安裝）
ninja build/fwupd
```

---

## Plugin 基本結構

### 最小 Plugin 範例

每一個 FWUPD Plugin 是一個 C 原始檔，通常位於 `fwupd/src/plugins/<plugin-name>/fu-plugin-<name>.c`。

#### 目錄結構

```
fwupd/src/plugins/nuvoton_m487/
├── meson.build              # 建構腳本
├── fu-nuvoton-m487-plugin.h # 標頭檔（可選）
├── fu-nuvoton-m487-plugin.c # 主要插件程式碼
└── fu-nuvoton-m487-ids.h   # 硬體 ID 定義（可選）
```

#### meson.build 結構

```meson
# fuupd/src/plugins/nuvoton_m487/meson.build

plugin_define('nuvoton-m487', sources: [
  'fu-nuvoton-m487-plugin.c',
  'fu-nuvoton-m487-ids.h',
])

# 告訴建構系統這是一個插件
plugin_data = plugin_define('nuvoton-m487', sources: sources)
```

#### 最基本 Plugin 程式碼

```c
#include "fu-plugin.h"
#include "fu-chunk.h"

struct _FuNuvotonM487Plugin {
    FuPlugin parent;
    // 在此添加插件私有資料
};

static void
fu_nuvoton_m487_plugin_init (FuNuvotonM487Plugin *self)
{
    // 1. 設定插件建構 Hash（用於偵錯）
    fu_plugin_set_build_hash (FU_PLUGIN (self), BUILD_HASH);

    // 2. 添加插件標誌（可選）
    fu_plugin_add_flag (FU_PLUGIN (self), FWUPD_PLUGIN_FLAG_NONE);
    
    // 3. 添加硬體 ID（讓 fwupd 能識別這個插件服務哪些硬體）
    fu_plugin_add_device_gtype (FU_PLUGIN (self), FU_TYPE_NUVOTON_M487_DEVICE);
}

static gboolean
fu_nuvoton_m487_plugin_write_firmware (FuPlugin *plugin,
                                        FuDevice *device,
                                        GBytes *blob_fw,
                                        FwupdInstallFlags flags,
                                        GError **error)
{
    // 這是核心：實際燒錄韌體的邏輯
    FuContext *ctx = fu_plugin_get_context (plugin);
    g_autoptr(FuDevice) device_local = NULL;
    
    // 讀取韌體資料
    const guint8 *data = g_bytes_get_data (blob_fw, NULL);
    gsize len = g_bytes_get_size (blob_fw);
    
    // 分塊寫入（建議每次不超過 4KB 或 64KB，視硬體而定）
    const guint chunk_size = 4096;
    for (gsize offset = 0; offset < len; offset += chunk_size) {
        gsize to_write = MIN (chunk_size, len - offset);
        
        if (!fu_device_write_chunk (device, offset, data + offset, to_write, error)) {
            g_prefix_error (error, "failed to write at offset 0x%zx: ", offset);
            return FALSE;
        }
        
        // 進度回饋
        fu_plugin_set_progress_full (plugin, offset + to_write, len);
    }
    
    return TRUE;
}

static gboolean
fu_nuvoton_m487_plugin_attach (FuPlugin *plugin, FuDevice *device, GError **error)
{
    // 將裝置從更新模式切換回正常模式
    // 具體實作視 M487 的更新协议而定
    return fu_device科技成果;
}

static gboolean
fu_nuvoton_m487_plugin_detach (FuPlugin *plugin, FuDevice *device, GError **error)
{
    // 將裝置切換到更新模式（Bootloader 模式）
    // 這通常涉及發送特定指令或控制信號
    return fu_device科技成果;
}

static void
fu_nuvoton_m487_plugin_register (FuPlugin *plugin)
{
    // 向 Plugin Manager 註冊這個插件
    fu_plugin_add_compile_flag (plugin, "-DFOO=1");
}

// 將上述函數鉤子到插件系統
fu_plugin_builtin_register (nuvoton-m487);
```

---

## FuDevice 子類別化

複雜的插件通常需要一個自訂的 `FuDevice` 子類別，用於儲存裝置特定狀態。

### 定義新設備類別

```c
// fu-nuvoton-m487-device.h

#ifndef FU_NUVOTON_M487_DEVICE_H
#define FU_NUVOTON_M487_DEVICE_H

#include "fu-nuvoton-m487-plugin.h"

#define FU_TYPE_NUVOTON_M487_DEVICE (fu_nuvoton_m487_device_get_type ())
G_DECLARE_DERIVABLE_TYPE (FuNuvotonM487Device, fu_nuvoton_m487_device, 
                          FU, NUVOTON_M487_DEVICE, FuDevice)

struct _FuNuvotonM487DeviceClass {
    FuDeviceClass parent_class;
    // 可擴展的虛方法表
};

#endif
```

```c
// fu-nuvoton-m487-device.c

#include "fu-nuvoton-m487-device.h"

struct _FuNuvotonM487Device {
    FuDevice parent;
    guint32 flash_size;      // Flash 大小
    guint32 flash_addr;      // 韌體燒錄起始地址
};

static void
fu_nuvoton_m487_device_init (FuNuvotonM487Device *self)
{
    fu_device_set_name (FU_DEVICE (self), "Nuvoton M487");
    fu_device_set_summary (FU_DEVICE (self), "Nuvoton M487 Cortex-M4 based MCU");
    fu_device_set_firmware_size (FU_DEVICE (self), 512 * 1024); // 512KB
    fu_device_add_flag (FU_DEVICE (self), FWUPD_DEVICE_FLAG_UPDATABLE);
    fu_device_add_flag (FU_DEVICE (self), FWUPD_DEVICE_FLAG_SIGNED_PAYLOAD);
}

static void
fu_nuvoton_m487_device_constructed (GObject *object)
{
    FuNuvotonM487Device *self = FU_NUVOTON_M487_DEVICE (object);
    
    // 從 device 屬性中讀取 flash 大小
    // 這些資訊可能來自於硬體識別表（quirks）
    if (self->flash_size == 0)
        self->flash_size = 512 * 1024;
}

G_DEFINE_TYPE (FuNuvotonM487Device, fu_nuvoton_m487_device, FU_TYPE_DEVICE)
```

---

## 關鍵 Plugin 鉤子詳解

### constructed — 初始化

```c
static void
fu_nuvoton_m487_plugin_constructed (GObject *object)
{
    FuNuvotonM487Plugin *self = FU_NUVOTON_M487_PLUGIN (object);
    FuContext *ctx = fu_plugin_get_context (self);
    
    // 添加硬體 ID（用於 Quirks 匹配）
    fu_plugin_add_device_gtype (FU_PLUGIN (self), FU_TYPE_NUVOTON_M487_DEVICE);
    
    // 添加 UDev 匹配規則（可選，讓 udev 自動載入插件）
    fu_plugin_add_udev_subsystem (self, "usb-serial");
}
```

### device_probe — 識別硬體

```c
static gboolean
fu_nuvoton_m487_plugin_device_probe (FuPlugin *plugin, FuDevice *device, GError **error)
{
    // 這個鉤子在你需要確認某個 FuDevice 是否由這個插件處理時呼叫
    
    // 確認 VID/PID
    if (fu_device_get_vid (device) != 0x0416)  // Nuvoton USB VID
        return FALSE;
    
    guint16 pid = fu_device_get_pid (device);
    if (pid != 0xc487 && pid != 0xb487)  // M487 variants
        return FALSE;
    
    // 設定裝置特定屬性
    fu_device_set_name (device, "Nuvoton M487 USB Device");
    fu_device_set_firmware_size_max (device, 512 * 1024);
    
    return TRUE;
}
```

### write_firmware — 燒錄韌體（核心）

```c
static gboolean
fu_nuvoton_m487_plugin_write_firmware (FuPlugin *plugin,
                                         FuDevice *device,
                                         GBytes *blob_fw,
                                         FwupdInstallFlags flags,
                                         GError **error)
{
    // 燒錄流程通常分為幾個階段
    
    // 階段 1：驗證韌體大小
    if (g_bytes_get_size (blob_fw) > fu_device_get_firmware_size_max (device)) {
        g_set_error (error, FWUPD_ERROR, FWUPD_ERROR_INVALID_FILE,
                      "firmware too large: max %u bytes",
                      fu_device_get_firmware_size_max (device));
        return FALSE;
    }
    
    // 階段 2：進入更新模式
    if (!fu_nuvoton_m487_enter_bootloader (device, error))
        return FALSE;
    
    // 階段 3：擦除 Flash
    if (!fu_nuvoton_m487_erase_flash (device, error))
        return FALSE;
    
    // 階段 4：燒錄
    g_autoptr(GBytes) chunk_iter = NULL;
    guint32 offset = 0;
    while ((chunk_iter = fu_chunk_iter_new (blob_fw, offset, CHUNK_SIZE)) != NULL) {
        FuChunk *chunk = fu_chunk_iter_get (chunk_iter);
        
        if (!fu_nuvoton_m487_write_page (device,
                                          fu_chunk_get_address (chunk),
                                          fu_chunk_get_data (chunk),
                                          fu_chunk_get_data_sz (chunk),
                                          error))
            return FALSE;
        
        offset += fu_chunk_get_data_sz (chunk);
        fu_plugin_set_progress_full (plugin, offset, g_bytes_get_size (blob_fw));
    }
    
    // 階段 5：驗證（讀回並比對 SHA）
    if (!fu_nuvoton_m487_verify (device, blob_fw, error))
        return FALSE;
    
    // 階段 6：重啟到正常模式
    if (!fu_nuvoton_m487_reset (device, error))
        return FALSE;
    
    return TRUE;
}
```

---

## Quirks 系統（硬體識別）

Quirks 是一個文字設定檔（`.quirk`），用於提供無法自動探測的硬體資訊。

### Quirks 檔案範例

建立 `/usr/share/fwupd/quirks.d/nuvoton-m487.quirk`：

```ini
[Plugin]
Name=nuvoton-m487
Module=nuvoton_m487

[DeviceInstance]
Name=Nuvoton M487 USB Bootloader
PlatformId=USB:VID_0416&PID_C487
Plugin=nuvoton_m487
Flags=updatable|signed
FirmwareSize=524288
VersionFormat=triplet

[DeviceInstance]
Name=Nuvoton M487 UART Bootloader
PlatformId=PCI:00:00
Plugin=nuvoton_m487
Flags=updatable|signed
FirmwareSize=524288
```

### 在插件中讀取 Quirks

```c
static void
fu_nuvoton_m487_plugin_device_registered (FuPlugin *plugin, FuDevice *device)
{
    FuNuvotonM487Device *self = fu_nuvoton_m487_device_new ();
    
    // 從 quirks 讀取設定值
    guint32 flash_size = fu_plugin_device_get_flash_size (plugin, device);
    if (flash_size > 0)
        fu_device_set_firmware_size (FU_DEVICE (self), flash_size);
    
    // 設定預設版本格式
    fu_device_set_version_format (FU_DEVICE (self), FWUPD_VERSION_FORMAT_TRIPLET);
    
    // 替換通用的 device 為具體的子類別
    fu_plugin_device_replace (plugin, device, FU_DEVICE (self));
}
```

---

## 測試與除錯

### 單元測試

```bash
# 執行所有測試
meson test -C build

# 執行特定插件測試
meson test -C build plugin_nuvoton_m487

# 使用 valgrind 檢測記憶體問題
meson test -C build plugin_nuvoton_m487 --wrap='valgrind --leak-check=full'
```

### 除錯輸出

```bash
# 啟用最大除錯日誌
RUST_LOG=debug fwupd -v

# 查看 systemd 日誌
journalctl -u fwupd -f

# 從原始碼重新安裝插件（開發時）
sudo ninja install -C build
sudo systemctl restart fwupd
```

### 快速開發迭代

```bash
# 1. 修改插件程式碼
vim src/plugins/nuvoton_m487/fu-nuvoton-m487-plugin.c

# 2. 只編譯該插件
ninja -C build src/plugins/nuvoton_m487/libfu_nuvoton_m487_plugin.so

# 3. 複製到系統目錄
sudo cp src/plugins/nuvoton_m487/libfu_nuvoton_m487_plugin.so /usr/lib/fwupd-2.0/plugins/

# 4. 重啟 daemon
sudo systemctl restart fwupd

# 5. 測試
fwupdmgr get-devices
```

### 常見錯誤

| 錯誤 | 原因 | 解法 |
|------|------|------|
| `No devices found` | 插件未載入 | 檢查 `/usr/lib/fwupd-2.0/plugins/` 中 `.so` 是否存在 |
| `Permission denied` | Polkit 權限不足 | 加入 `fwupd` 群組或使用 `pkexec` |
| `Write failed: error 2` | 韌體燒錄通訊失敗 | 檢查 USB/序列連線、檢查硬體回復模式 |
| `Invalid signature` | 簽章驗證失敗 | 確認使用正確的金鑰簽章 CAB |

---

## Plugin 提交到上游

如果希望將插件貢獻給 fwupd 上游：

1. **建立 GitHub Issue**：先在 https://github.com/fwupd/fwupd 提出功能請求
2. **遵循 Coding Style**：參考 `.clang-format` 和現有插件
3. **新增自我測試**：`src/plugins/<name>/self-test/` 下添加測試案例
4. **新增 Quirks 檔案**：在 `data/quirks.d/` 添加 vendor ID
5. **文件更新**：更新 `docs/` 下相關文件
6. **Pull Request**：提交 PR 並通過 CI（包含 Coverity, scan-build 等靜態分析）

---

## 參考資源

- **Plugin 開發文件**：https://fwupd.github.io/hacking/
- **Plugin 結構**：https://github.com/fwupd/fwupd/tree/main/src/plugins
- **FuPlugin API**：原始碼中 `src/fu-plugin.h`
- **LVFS 上傳指南**：https://fwupd.org/lvfs/docs/applications

---

*本文件屬於 FWUPD 知識庫，用於內部研究與技術參考。*

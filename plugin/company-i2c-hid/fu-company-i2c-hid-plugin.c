/*
 * fu-company-i2c-hid-plugin.c
 *
 * FWUPD Plugin for Company HID I2C Devices
 *
 * This plugin implements firmware update support for HID I2C devices.
 * Key features:
 *   - HID mode for fast firmware updates via hidraw
 *   - I2C fallback recovery mode via i2c-dev
 *   - Bootloader/runtime mode switching
 *   - Multi-level GUID generation for flexible matching
 *
 * Reference: elantp plugin (tw.com.emc.elantp)
 * Copyright (C) 2026 Company
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include "fu-company-i2c-hid-plugin.h"
#include "fu-company-i2c-hid-device.h"

/* Plugin private data */
struct _FuCompanyI2cHidPlugin {
  FuPlugin parent_instance;
  FuCompanyI2cHidPluginPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(FuCompanyI2cHidPlugin,
                             fu_company_i2c_hid_plugin,
                             FU_TYPE_PLUGIN)

/* Forward declarations */
static void
fu_company_i2c_hid_plugin_init(FuCompanyI2cHidPlugin *self);

static void
fu_company_i2c_hid_plugin_constructed(GObject *obj);

static void
fu_company_i2c_hid_plugin_finalize(GObject *obj);

static void
fu_company_i2c_hid_plugin_class_init(FuCompanyI2cHidPluginClass *klass);

static gboolean
fu_company_i2c_hid_plugin_device_set_quirk_kv(FuPlugin *plugin,
                                              FuDevice *device,
                                              const gchar *key,
                                              const gchar *value,
                                              GCancellable *cancellable,
                                              GError **error);

static gboolean
fu_company_i2c_hid_plugin_coldplug(FuPlugin *plugin,
                                    FuProgress *progress,
                                    GError **error);

static gboolean
fu_company_i2c_hid_plugin_device_prepare(FuPlugin *plugin,
                                         FuDevice *device,
                                         FuProgress *progress,
                                         FwupdInstallFlags flags,
                                         GError **error);

static gboolean
fu_company_i2c_hid_plugin_device_attach(FuPlugin *plugin,
                                        FuDevice *device,
                                        FuProgress *progress,
                                        GError **error);

static gboolean
fu_company_i2c_hid_plugin_device_detach(FuPlugin *plugin,
                                         FuDevice *device,
                                         FuProgress *progress,
                                         GError **error);

static gboolean
fu_company_i2c_hid_plugin_write_firmware(FuPlugin *plugin,
                                          FuDevice *device,
                                          GBytes *blob_fw,
                                          FuProgress *progress,
                                          FwupdInstallFlags flags,
                                          GError **error);

static gboolean
fu_company_i2c_hid_plugin_read_firmware(FuPlugin *plugin,
                                         FuDevice *device,
                                         GBytes **blob_fw,
                                         FuProgress *progress,
                                         GError **error);

static gboolean
fu_company_i2c_hid_plugin_reload(FuPlugin *plugin,
                                  FuDevice *device,
                                  GError **error);

/*--------------------------------------------------------------------------*/
/* Plugin lifecycle                                                         */
/*--------------------------------------------------------------------------*/

static void
fu_company_i2c_hid_plugin_class_init(FuCompanyI2cHidPluginClass *klass)
{
  FuPluginClass *plugin_class = FU_PLUGIN_CLASS(klass);
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->constructed = fu_company_i2c_hid_plugin_constructed;
  object_class->finalize = fu_company_i2c_hid_plugin_finalize;

  plugin_class->coldplug = fu_company_i2c_hid_plugin_coldplug;
  plugin_class->device_set_quirk_kv = fu_company_i2c_hid_plugin_device_set_quirk_kv;
  plugin_class->device_prepare = fu_company_i2c_hid_plugin_device_prepare;
  plugin_class->device_attach = fu_company_i2c_hid_plugin_device_attach;
  plugin_class->device_detach = fu_company_i2c_hid_plugin_device_detach;
  plugin_class->write_firmware = fu_company_i2c_hid_plugin_write_firmware;
  plugin_class->read_firmware = fu_company_i2c_hid_plugin_read_firmware;
  plugin_class->reload = fu_company_i2c_hid_plugin_reload;

  /* Register the protocol ID */
  fu_plugin_register_protocol_id(plugin_class, FU_COMPANY_I2C_HID_PROTOCOL_ID);
}

static void
fu_company_i2c_hid_plugin_init(FuCompanyI2cHidPlugin *self)
{
  FuCompanyI2cHidPluginPrivate *priv = fu_company_i2c_hid_plugin_get_instance_private(self);
  self->priv = priv;

  /* Initialize default values */
  priv->i2c_mode_available = FALSE;
  priv->i2c_slave_addr = FU_COMPANY_I2C_HID_I2C_ADDR_DEFAULT;

  fu_plugin_add_device_gtype(plugin_class, FU_TYPE_COMPANY_I2C_HID_DEVICE);
}

static void
fu_company_i2c_hid_plugin_constructed(GObject *obj)
{
  FuCompanyI2cHidPlugin *self = FU_COMPANY_I2C_HID_PLUGIN(obj);
  FuPlugin *plugin = FU_PLUGIN(obj);

  /* Chain up */
  G_OBJECT_CLASS(fu_company_i2c_hid_plugin_parent_class)->constructed(obj);

  /* Signal that we support both HID and I2C modes */
  fu_plugin_set_name(plugin_class, "company-i2c-hid");
}

static void
fu_company_i2c_hid_plugin_finalize(GObject *obj)
{
  FuCompanyI2cHidPlugin *self = FU_COMPANY_I2C_HID_PLUGIN(obj);
  FuCompanyI2cHidPluginPrivate *priv = self->priv;

  g_free(priv);

  G_OBJECT_CLASS(fu_company_i2c_hid_plugin_parent_class)->finalize(obj);
}

/*--------------------------------------------------------------------------*/
/* Device enumeration (coldplug)                                           */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_plugin_coldplug:
 *
 * Enumerate all Company HID I2C devices on the system.
 * This is called during fwupd startup to discover compatible devices.
 *
 * The function uses udev to find hidraw and i2c-dev nodes, then creates
 * FuDevice objects for each discovered device.
 */
static gboolean
fu_company_i2c_hid_plugin_coldplug(FuPlugin *plugin,
                                    FuProgress *progress,
                                    GError **error)
{
  FuContext *ctx = fu_plugin_get_context(plugin);
  g_autoptr(FuCompanyI2cHidDevice) device = NULL;
  g_autoptr(FuUdevDevice) udev_device = NULL;
  g_autofree gchar *vendor_id = NULL;
  g_autofree gchar *device_path = NULL;
  guint16 ic_type = 0;
  guint16 module_id = 0;

  fu_progress_set_steps(progress, 3);

  /* Step 1: Try to find HID device via hidraw */
  fu_progress_set_status(progress, FWUPD_STATUS_LOADING);
  udev_device = FU_UDEV_DEVICE(fu_context_udev_device_new(ctx,
                                                          "hidraw",
                                                          "HID_NAME", "Company HID Device",
                                                          NULL));

  if (udev_device == NULL) {
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_NOT_FOUND,
                "No Company HID I2C device found");
    return FALSE;
  }

  fu_progress_step_done(progress);

  /* Step 2: Create our device wrapper */
  fu_progress_set_status(progress, FWUPD_STATUS_LOADING);
  device = fu_company_i2c_hid_device_new(FU_DEVICE(udev_device));

  /* Get device path for identification */
  device_path = g_strdup(fu_udev_device_get_device_file(udev_device));
  if (device_path == NULL) {
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_NOT_FOUND,
                "Cannot get device file path");
    return FALSE;
  }

  /* Get vendor ID from udev (format: HIDRAW:VID) */
  vendor_id = fu_udev_device_get_vendor_id(udev_device);
  if (vendor_id != NULL) {
    fu_device_set_vendor_id(FU_DEVICE(device), vendor_id);
  }

  /* Read IC type and module ID from sysfs or HID descriptor */
  ic_type = fu_udev_device_get_sysfs_attr_as_uint16(udev_device, "device/ic_type", 0);
  module_id = fu_udev_device_get_sysfs_attr_as_uint16(udev_device, "device/module_id", 0);

  fu_company_i2c_hid_device_set_ic_type(device, ic_type);
  fu_company_i2c_hid_device_set_module_id(device, module_id);

  fu_progress_step_done(progress);

  /* Step 3: Setup device properties and register */
  fu_progress_set_status(progress, FWUPD_STATUS_LOADING);

  /* Set device ID based on HID path */
  fu_device_set_id(FU_DEVICE(device),
                   g_strdup_printf("CompanyI2CHid-%s",
                                    fu_udev_device_get_sysfs_path(udev_device)));

  /* Add instance IDs for GUID matching (multi-level) */
  fu_device_add_instance_id(FU_DEVICE(device), vendor_id);

  /* Add IC-specific instance ID */
  if (ic_type > 0) {
    g_autofree gchar *ic_instance_id = g_strdup_printf("COMPANY_I2C\\ICTYPE_%02X", ic_type);
    fu_device_add_instance_id(FU_DEVICE(device), ic_instance_id);
  }

  /* Add driver-specific instance IDs (HID vs I2C) */
  if (fu_company_i2c_hid_device_has_hid_mode(device)) {
    g_autofree gchar *hid_instance_id = g_strdup_printf(
        "COMPANY_I2C\\ICTYPE_%02X&DRIVER_HID", ic_type);
    fu_device_add_instance_id(FU_DEVICE(device), hid_instance_id);
  }

  if (fu_company_i2c_hid_device_has_i2c_mode(device)) {
    g_autofree gchar *i2c_instance_id = g_strdup_printf(
        "COMPANY_I2C\\ICTYPE_%02X&DRIVER_I2C", ic_type);
    fu_device_add_instance_id_full(FU_DEVICE(device), i2c_instance_id,
                                    FU_DEVICE_INSTANCE_FLAG_QUIRK);
  }

  /* Set device flags */
  fu_device_add_flag(FU_DEVICE(device), FWUPD_DEVICE_FLAG_UPDATABLE);
  fu_device_add_flag(FU_DEVICE(device), FWUPD_DEVICE_FLAG_NEEDS_BOOTLOADER);
  fu_device_add_flag(FU_DEVICE(device), FWUPD_DEVICE_FLAG_WAIT_FOR_REPLUG);

  /* Set summary */
  fu_device_set_summary(FU_DEVICE(device), "Company I2C HID Device");

  /* Set protocol */
  fu_device_set_protocol(FU_DEVICE(device), FU_COMPANY_I2C_HID_PROTOCOL_ID);

  /* Register device */
  fu_plugin_add_device(plugin, FU_DEVICE(device));

  fu_progress_step_done(progress);

  g_debug("Company HID I2C device discovered: path=%s, ic_type=0x%02X, module_id=0x%02X",
          device_path, ic_type, module_id);

  return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Quirk handling                                                           */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_plugin_device_set_quirk_kv:
 *
 * Process quirk key-value pairs for device-specific configuration.
 * Quirks are loaded from the .quirk file and allow per-device settings
 * like I2C address, IC type, module ID, etc.
 */
static gboolean
fu_company_i2c_hid_plugin_device_set_quirk_kv(FuPlugin *plugin,
                                               FuDevice *device,
                                               const gchar *key,
                                               const gchar *value,
                                               GCancellable *cancellable,
                                               GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_I2C_ADDR) == 0) {
    /* I2C slave address */
    guint64 addr = g_ascii_strtoull(value, NULL, 16);
    fu_company_i2c_hid_device_set_i2c_slave_addr(self, (guint16)addr);
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_IC_TYPE) == 0) {
    /* IC type identifier */
    guint64 ic_type = g_ascii_strtoull(value, NULL, 16);
    fu_company_i2c_hid_device_set_ic_type(self, (guint16)ic_type);
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_MODULE_ID) == 0) {
    /* Module identifier */
    guint64 module_id = g_ascii_strtoull(value, NULL, 16);
    fu_company_i2c_hid_device_set_module_id(self, (guint16)module_id);
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_IAP_PASSWORD) == 0) {
    /* IAP (In-Application Programming) password */
    guint64 password = g_ascii_strtoull(value, NULL, 16);
    fu_company_i2c_hid_device_set_iap_password(self, (guint32)password);
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_PAGE_COUNT) == 0) {
    /* Flash page count */
    guint64 page_count = g_ascii_strtoull(value, NULL, 16);
    fu_company_i2c_hid_device_set_page_count(self, (guint16)page_count);
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_BLOCK_SIZE) == 0) {
    /* Flash block size */
    guint64 block_size = g_ascii_strtoull(value, NULL, 16);
    fu_company_i2c_hid_device_set_block_size(self, (guint16)block_size);
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_BOARD_NAME) == 0) {
    /* Board name for additional GUID matching */
    fu_company_i2c_hid_device_set_board_name(self, value);
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_DRIVER) == 0) {
    /* Driver type: HID or I2C */
    if (g_strcmp0(value, FU_COMPANY_I2C_HID_DRIVER_HID) == 0) {
      fu_company_i2c_hid_device_set_hid_mode_available(self, TRUE);
    } else if (g_strcmp0(value, FU_COMPANY_I2C_HID_DRIVER_I2C) == 0) {
      fu_company_i2c_hid_device_set_i2c_mode_available(self, TRUE);
    }
    return TRUE;
  }

  /* Unknown quirk key */
  g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_NOT_FOUND,
              "unknown quirk key '%s'", key);
  return FALSE;
}

/*--------------------------------------------------------------------------*/
/* Device prepare/attach/detach                                            */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_plugin_device_prepare:
 *
 * Prepare device for firmware update.
 * This is called before write_firmware to ensure the device is ready.
 */
static gboolean
fu_company_i2c_hid_plugin_device_prepare(FuPlugin *plugin,
                                         FuDevice *device,
                                         FuProgress *progress,
                                         FwupdInstallFlags flags,
                                         GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);

  fu_progress_set_steps(progress, 2);

  /* Step 1: Verify device is accessible */
  fu_progress_set_status(progress, FWUPD_STATUS_LOADING);
  if (!fu_device_has_flag(device, FWUPD_DEVICE_FLAG_IS_BOOTLOADER)) {
    g_debug("Device is in runtime mode, will switch to bootloader for update");
  }

  fu_progress_step_done(progress);

  /* Step 2: Check available modes */
  fu_progress_set_status(progress, FWUPD_STATUS_LOADING);
  if (!fu_company_i2c_hid_device_has_hid_mode(self) &&
      !fu_company_i2c_hid_device_has_i2c_mode(self)) {
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED,
                "Neither HID nor I2C mode is available");
    return FALSE;
  }

  fu_progress_step_done(progress);

  return TRUE;
}

/**
 * fu_company_i2c_hid_plugin_device_detach:
 *
 * Switch device from runtime mode to bootloader mode.
 * This is typically done by sending a special HID report or I2C command.
 */
static gboolean
fu_company_i2c_hid_plugin_device_detach(FuPlugin *plugin,
                                         FuDevice *device,
                                         FuProgress *progress,
                                         GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);

  fu_progress_set_steps(progress, 3);

  /* Step 1: Send detach command via HID */
  fu_progress_set_status(progress, FWUPD_STATUS_DEVICE_BUSY);
  if (fu_company_i2c_hid_device_has_hid_mode(self)) {
    g_debug("Sending detach command via HID");
    if (!fu_company_i2c_hid_device_detach_hid(self, error))
      return FALSE;
  }

  fu_progress_step_done(progress);

  /* Step 2: If HID fails, try I2C fallback */
  fu_progress_set_status(progress, FWUPD_STATUS_DEVICE_BUSY);
  if (fu_company_i2c_hid_device_has_i2c_mode(self)) {
    g_debug("Sending detach command via I2C");
    if (!fu_company_i2c_hid_device_detach_i2c(self, error))
      return FALSE;
  }

  fu_progress_step_done(progress);

  /* Step 3: Wait for device to reboot into bootloader */
  fu_progress_set_status(progress, FWUPD_STATUS_DEVICE_RESTART);
  g_usleep(500 * 1000); /* 500ms wait for bootloader */

  fu_progress_step_done(progress);

  return TRUE;
}

/**
 * fu_company_i2c_hid_plugin_device_attach:
 *
 * Switch device from bootloader mode back to runtime mode.
 * This is typically done by sending a reset command.
 */
static gboolean
fu_company_i2c_hid_plugin_device_attach(FuPlugin *plugin,
                                        FuDevice *device,
                                        FuProgress *progress,
                                        GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);

  fu_progress_set_steps(progress, 2);

  /* Step 1: Send attach (reset) command */
  fu_progress_set_status(progress, FWUPD_STATUS_DEVICE_BUSY);
  if (fu_company_i2c_hid_device_has_hid_mode(self)) {
    g_debug("Sending attach command via HID");
    if (!fu_company_i2c_hid_device_attach_hid(self, error))
      return FALSE;
  }

  fu_progress_step_done(progress);

  /* Step 2: Wait for device to return to runtime */
  fu_progress_set_status(progress, FWUPD_STATUS_DEVICE_RESTART);
  g_usleep(500 * 1000); /* 500ms wait for runtime */

  fu_progress_step_done(progress);

  return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Firmware read/write                                                     */
/*--------------------------------------------------------------------------*/

/**
 * fwupd_company_i2c_hid_plugin_write_firmware:
 *
 * Write firmware to the device.
 *
 * Strategy:
 * 1. First try HID mode (fast)
 * 2. If HID fails, fallback to I2C mode (slow but reliable)
 * 3. Verify the write was successful
 */
static gboolean
fu_company_i2c_hid_plugin_write_firmware(FuPlugin *plugin,
                                          FuDevice *device,
                                          GBytes *blob_fw,
                                          FuProgress *progress,
                                          FwupdInstallFlags flags,
                                          GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);
  gboolean ret = FALSE;
  g_autoptr(GError) error_hid = NULL;
  g_autoptr(GError) error_i2c = NULL;

  fu_progress_set_steps(progress, 4);

  /* Step 1: Try HID mode first (fast) */
  fu_progress_set_status(progress, FWUPD_STATUS_DEVICE_WRITE);
  if (fu_company_i2c_hid_device_has_hid_mode(self)) {
    g_debug("Attempting firmware write via HID");
    ret = fu_company_i2c_hid_device_write_firmware_hid(self, blob_fw, progress, &error_hid);
    if (ret) {
      g_debug("Firmware written successfully via HID");
    } else {
      g_warning("HID firmware write failed: %s", error_hid->message);
    }
  }

  fu_progress_step_done(progress);

  /* Step 2: If HID failed, try I2C fallback (slow but reliable) */
  if (!ret && fu_company_i2c_hid_device_has_i2c_mode(self)) {
    fu_progress_set_status(progress, FWUPD_STATUS_DEVICE_WRITE);
    g_debug("Attempting firmware write via I2C fallback");
    ret = fu_company_i2c_hid_device_write_firmware_i2c(self, blob_fw, progress, &error_i2c);
    if (ret) {
      g_debug("Firmware written successfully via I2C");
    } else {
      g_warning("I2C firmware write failed: %s", error_i2c->message);
    }
  }

  fu_progress_step_done(progress);

  /* Step 3: Verify firmware */
  fu_progress_set_status(progress, FWUPD_STATUS_VERIFYING);
  if (ret) {
    if (!fu_company_i2c_hid_device_verify_firmware(self, blob_fw, error)) {
      ret = FALSE;
    }
  }

  fu_progress_step_done(progress);

  /* Step 4: Set result status */
  fu_progress_set_status(progress, FWUPD_STATUS_IDLE);
  if (!ret) {
    if (error_hid && error_i2c) {
      g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_WRITE,
                  "Both HID and I2C firmware write failed: HID=%s, I2C=%s",
                  error_hid->message, error_i2c->message);
    } else if (error_hid) {
      g_propagate_error(error, g_steal_pointer(&error_hid));
    } else if (error_i2c) {
      g_propagate_error(error, g_steal_pointer(&error_i2c));
    }
    return FALSE;
  }

  fu_progress_step_done(progress);

  return TRUE;
}

/**
 * fu_company_i2c_hid_plugin_read_firmware:
 *
 * Read firmware from the device (optional, for backup).
 */
static gboolean
fu_company_i2c_hid_plugin_read_firmware(FuPlugin *plugin,
                                         FuDevice *device,
                                         GBytes **blob_fw,
                                         FuProgress *progress,
                                         GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);

  fu_progress_set_steps(progress, 2);

  /* Try HID mode first */
  fu_progress_set_status(progress, FWUPD_STATUS_READING);
  if (fu_company_i2c_hid_device_has_hid_mode(self)) {
    if (fu_company_i2c_hid_device_read_firmware_hid(self, blob_fw, progress, error)) {
      fu_progress_step_done(progress);
      return TRUE;
    }
  }

  fu_progress_step_done(progress);

  /* Fallback to I2C */
  fu_progress_set_status(progress, FWUPD_STATUS_READING);
  if (fu_company_i2c_hid_device_has_i2c_mode(self)) {
    if (fu_company_i2c_hid_device_read_firmware_i2c(self, blob_fw, progress, error)) {
      fu_progress_step_done(progress);
      return TRUE;
    }
  }

  fu_progress_step_done(progress);

  g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_READ,
              "Failed to read firmware via any interface");
  return FALSE;
}

/*--------------------------------------------------------------------------*/
/* Device reload (post-update)                                              */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_plugin_reload:
 *
 * Reload device after firmware update to read new version.
 */
static gboolean
fu_company_i2c_hid_plugin_reload(FuPlugin *plugin,
                                  FuDevice *device,
                                  GError **error)
{
  g_autofree gchar *version = NULL;

  /* Read new firmware version */
  version = fu_company_i2c_hid_device_get_version(FU_COMPANY_I2C_HID_DEVICE(device), error);
  if (version == NULL)
    return FALSE;

  fu_device_set_version(device, version);
  g_debug("Device reloaded with new version: %s", version);

  return TRUE;
}

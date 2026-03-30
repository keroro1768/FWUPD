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
};

G_DEFINE_TYPE(FuCompanyI2cHidPlugin, fu_company_i2c_hid_plugin, FU_TYPE_PLUGIN)

/*--------------------------------------------------------------------------*/
/* Plugin lifecycle                                                         */
/*--------------------------------------------------------------------------*/

static void
fu_company_i2c_hid_plugin_class_init(FuCompanyI2cHidPluginClass *klass)
{
  FuPluginClass *plugin_class = FU_PLUGIN_CLASS(klass);

  plugin_class->constructed = fu_company_i2c_hid_plugin_constructed;
}

static void
fu_company_i2c_hid_plugin_init(FuCompanyI2cHidPlugin *self)
{
}

static void
fu_company_i2c_hid_plugin_constructed(GObject *obj)
{
  FuPlugin *plugin = FU_PLUGIN(obj);
  FuContext *ctx = fu_plugin_get_context(plugin);

  /* Register quirk keys */
  fu_context_add_quirk_key(ctx, FU_COMPANY_I2C_HID_QUIRK_I2C_ADDR);
  fu_context_add_quirk_key(ctx, FU_COMPANY_I2C_HID_QUIRK_IC_TYPE);
  fu_context_add_quirk_key(ctx, FU_COMPANY_I2C_HID_QUIRK_MODULE_ID);
  fu_context_add_quirk_key(ctx, FU_COMPANY_I2C_HID_QUIRK_IAP_PASSWORD);
  fu_context_add_quirk_key(ctx, FU_COMPANY_I2C_HID_QUIRK_PAGE_COUNT);
  fu_context_add_quirk_key(ctx, FU_COMPANY_I2C_HID_QUIRK_BLOCK_SIZE);
  fu_context_add_quirk_key(ctx, FU_COMPANY_I2C_HID_QUIRK_BOARD_NAME);
  fu_context_add_quirk_key(ctx, FU_COMPANY_I2C_HID_QUIRK_DRIVER);

  /* Register udev subsystems for device enumeration (per elantp pattern) */
  fu_plugin_add_udev_subsystem(plugin, "i2c");
  fu_plugin_add_udev_subsystem(plugin, "i2c-dev");
  fu_plugin_add_udev_subsystem(plugin, "hidraw");

  /* Register firmware and device types */
  fu_plugin_add_firmware_gtype(plugin, FU_TYPE_COMPANY_I2C_HID_FIRMWARE);
  fu_plugin_add_device_gtype(plugin, FU_TYPE_COMPANY_I2C_HID_DEVICE);

  /* Register the protocol ID */
  fu_plugin_register_protocol_id(plugin, FU_COMPANY_I2C_HID_PROTOCOL_ID);

  /* Chain up to parent */
  G_OBJECT_CLASS(fu_company_i2c_hid_plugin_parent_class)->constructed(obj);
}

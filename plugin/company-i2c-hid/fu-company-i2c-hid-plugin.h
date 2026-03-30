/*
 * fu-company-i2c-hid-plugin.h
 *
 * FWUPD Plugin for Company HID I2C Devices
 *
 * This plugin supports firmware updates for HID I2C devices manufactured by
 * Company. It implements HID + I2C dual-mode support, where HID mode is used
 * for fast updates and I2C mode serves as a recovery fallback.
 *
 * Copyright (C) 2026 Company
 * SPDX-License-Identifier: LGPL-2.1+
 *
 * Reference: elantp plugin (tw.com.emc.elantp)
 */

#pragma once

#include <fwupdplugin.h>

/* Plugin definition */
#define FU_TYPE_COMPANY_I2C_HID_PLUGIN (fu_company_i2c_hid_plugin_get_type())
G_DECLARE_FINAL_TYPE(FuCompanyI2cHidPlugin,
                      fu_company_i2c_hid_plugin,
                      FU, COMPANY_I2C_HID_PLUGIN,
                      FuPlugin)

/* Protocol ID for this plugin */
#define FU_COMPANY_I2C_HID_PROTOCOL_ID "com.company.i2c-hid"

/* I2C slave address default (can be overridden via quirk) */
#define FU_COMPANY_I2C_HID_I2C_ADDR_DEFAULT 0x2A

/* HID Report ID for firmware update commands */
#define FU_COMPANY_I2C_HID_REPORT_ID_UPDATE 0x01

#endif /* FU_COMPANY_I2C_HID_PLUGIN_H */

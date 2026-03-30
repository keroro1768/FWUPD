/*
 * fu-company-i2c-hid-firmware.h
 *
 * Custom firmware format support for Company HID I2C Devices
 *
 * This implements firmware parsing for the Company's binary firmware format.
 * The format consists of:
 *   - Header (16 bytes): magic, version, size, checksum
 *   - Body: raw firmware data
 *   - Footer (optional): metadata
 *
 * Copyright (C) 2026 Company
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_COMPANY_I2C_HID_FIRMWARE (fu_company_i2c_hid_firmware_get_type())
G_DECLARE_FINAL_TYPE(FuCompanyI2cHidFirmware,
                      fu_company_i2c_hid_firmware,
                      FU, COMPANY_I2C_HID_FIRMWARE,
                      FuFirmware)

/* Firmware format constants */
#define FU_COMPANY_I2C_HID_FIRMWARE_MAGIC       0x434F4D50  /* "COMP" */
#define FU_COMPANY_I2C_HID_FIRMWARE_HEADER_SIZE 16
#define FU_COMPANY_I2C_HID_FIRMWARE_VERSION_OFFSET 4
#define FU_COMPANY_I2C_HID_FIRMWARE_SIZE_OFFSET 8
#define FU_COMPANY_I2C_HID_FIRMWARE_checksum_OFFSET 12

/* Helper function to create firmware object */
FuCompanyI2cHidFirmware *fu_company_i2c_hid_firmware_new(void);

#endif /* FU_COMPANY_I2C_HID_FIRMWARE_H */

/*
 * fu-company-i2c-hid-device.h
 *
 * FWUPD Device support for Company HID I2C Devices
 *
 * Copyright (C) 2026 Company
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

/* Device-specific flags */
typedef enum {
  FU_COMPANY_I2C_HID_DEVICE_FLAG_NONE          = 0,
  FU_COMPANY_I2C_HID_DEVICE_FLAG_IS_BOOTLOADER = 1 << 0,  /* Device in bootloader mode */
  FU_COMPANY_I2C_HID_DEVICE_FLAG_HID_MODE      = 1 << 1,  /* Using HID interface */
  FU_COMPANY_I2C_HID_DEVICE_FLAG_I2C_MODE      = 1 << 2,  /* Using I2C interface */
} FuCompanyI2cHidDeviceFlags;

#define FU_TYPE_COMPANY_I2C_HID_DEVICE (fu_company_i2c_hid_device_get_type())
G_DECLARE_FINAL_TYPE(FuCompanyI2cHidDevice,
                      fu_company_i2c_hid_device,
                      FU, COMPANY_I2C_HID_DEVICE,
                      FuHidrawDevice)

/* Device private data */
struct _FuCompanyI2cHidDevice {
  FuHidrawDevice parent_instance;
  FuCompanyI2cHidDeviceFlags flags;
  guint16 ic_type;             /* IC type identifier */
  guint16 module_id;           /* Module identifier */
  guint16 i2c_slave_addr;      /* I2C slave address */
  guint32 iap_password;        /* In-Application Programming password */
  guint16 page_count;          /* Flash page count */
  guint16 block_size;          /* Flash block size */
  gchar *board_name;           /* Board name from quirk */
  guint16 iap_ctrl;            /* IAP control register value */
  guint8 pattern;              /* Pattern for IC type detection */
};

#define FU_COMPANY_I2C_HID_DEVICE_PRIVATE_FLAG_NONE 0

/* Quirks keys */
#define FU_COMPANY_I2C_HID_QUIRK_I2C_ADDR        "CompanyI2cHidI2cAddress"
#define FU_COMPANY_I2C_HID_QUIRK_IC_TYPE        "CompanyI2cHidIcType"
#define FU_COMPANY_I2C_HID_QUIRK_MODULE_ID      "CompanyI2cHidModuleId"
#define FU_COMPANY_I2C_HID_QUIRK_IAP_PASSWORD   "CompanyI2cHidIapPassword"
#define FU_COMPANY_I2C_HID_QUIRK_PAGE_COUNT      "CompanyI2cHidPageCount"
#define FU_COMPANY_I2C_HID_QUIRK_BLOCK_SIZE     "CompanyI2cHidBlockSize"
#define FU_COMPANY_I2C_HID_QUIRK_BOARD_NAME     "CompanyI2cHidBoardName"
#define FU_COMPANY_I2C_HID_QUIRK_DRIVER         "CompanyI2cHidDriver"

/* Driver types for quirk matching */
#define FU_COMPANY_I2C_HID_DRIVER_HID           "HID"
#define FU_COMPANY_I2C_HID_DRIVER_I2C           "I2C"

/* Protocol ID */
#define FU_COMPANY_I2C_HID_PROTOCOL_ID "com.company.i2c-hid"

/* I2C slave address default (can be overridden via quirk) */
#define FU_COMPANY_I2C_HID_I2C_ADDR_DEFAULT 0x2A

/* HID Report ID for firmware update commands */
#define FU_COMPANY_I2C_HID_REPORT_ID_UPDATE 0x01

/* Company-specific HID commands */
#define FU_COMPANY_I2C_HID_CMD_GET_VERSION      0x10
#define FU_COMPANY_I2C_HID_CMD_IAP              0x20
#define FU_COMPANY_I2C_HID_CMD_IAP_RESET        0x30
#define FU_COMPANY_I2C_HID_CMD_GET_HID_ID       0x40
#define FU_COMPANY_I2C_HID_CMD_IAP_PASSWORD     0x50

#endif /* FU_COMPANY_I2C_HID_DEVICE_H */

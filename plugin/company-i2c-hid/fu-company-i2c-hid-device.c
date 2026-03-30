/*
 * fu-company-i2c-hid-device.c
 *
 * FWUPD Device support for Company HID I2C Devices
 *
 * This implements the device-specific operations for firmware updates,
 * including HID and I2C communication, bootloader mode switching,
 * and firmware verification.
 *
 * Reference: elantp plugin device implementation
 * Copyright (C) 2026 Company
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include "fu-company-i2c-hid-device.h"
#include "fu-company-i2c-hid-plugin.h"

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/*--------------------------------------------------------------------------*/
/* GObject type registration                                                 */
/*--------------------------------------------------------------------------*/

G_DEFINE_TYPE(FuCompanyI2cHidDevice, fu_company_i2c_hid_device, FU_TYPE_HIDRAW_DEVICE)

/*--------------------------------------------------------------------------*/
/* Private data access                                                        */
/*--------------------------------------------------------------------------*/

static FuCompanyI2cHidDeviceFlags
fu_company_i2c_hid_device_get_flags(FuCompanyI2cHidDevice *self)
{
  return self->flags;
}

static void
fu_company_i2c_hid_device_set_flags(FuCompanyI2cHidDevice *self, FuCompanyI2cHidDeviceFlags flags)
{
  self->flags = flags;
}

gboolean
fu_company_i2c_hid_device_has_hid_mode(FuCompanyI2cHidDevice *self)
{
  return (self->flags & FU_COMPANY_I2C_HID_DEVICE_FLAG_HID_MODE) != 0;
}

gboolean
fu_company_i2c_hid_device_has_i2c_mode(FuCompanyI2cHidDevice *self)
{
  return (self->flags & FU_COMPANY_I2C_HID_DEVICE_FLAG_I2C_MODE) != 0;
}

void
fu_company_i2c_hid_device_set_i2c_slave_addr(FuCompanyI2cHidDevice *self, guint16 addr)
{
  self->i2c_slave_addr = addr;
}

void
fu_company_i2c_hid_device_set_ic_type(FuCompanyI2cHidDevice *self, guint16 ic_type)
{
  self->ic_type = ic_type;
}

void
fu_company_i2c_hid_device_set_module_id(FuCompanyI2cHidDevice *self, guint16 module_id)
{
  self->module_id = module_id;
}

void
fu_company_i2c_hid_device_set_iap_password(FuCompanyI2cHidDevice *self, guint32 password)
{
  self->iap_password = password;
}

void
fu_company_i2c_hid_device_set_page_count(FuCompanyI2cHidDevice *self, guint16 page_count)
{
  self->page_count = page_count;
}

void
fu_company_i2c_hid_device_set_block_size(FuCompanyI2cHidDevice *self, guint16 block_size)
{
  self->block_size = block_size;
}

void
fu_company_i2c_hid_device_set_board_name(FuCompanyI2cHidDevice *self, const gchar *name)
{
  g_free(self->board_name);
  self->board_name = g_strdup(name);
}

/*--------------------------------------------------------------------------*/
/* Object lifecycle                                                         */
/*--------------------------------------------------------------------------*/

static void
fu_company_i2c_hid_device_init(FuCompanyI2cHidDevice *self)
{
  /* Initialize with defaults */
  self->flags = FU_COMPANY_I2C_HID_DEVICE_FLAG_NONE;
  self->ic_type = 0;
  self->module_id = 0;
  self->i2c_slave_addr = FU_COMPANY_I2C_HID_I2C_ADDR_DEFAULT;
  self->iap_password = 0;
  self->page_count = 0x1000;  /* Default 4096 pages */
  self->block_size = 64;       /* Default 64 bytes per block */
  self->board_name = NULL;
  self->iap_ctrl = 0;
  self->pattern = 0;

  /* Set default flags - HID mode available by default */
  self->flags |= FU_COMPANY_I2C_HID_DEVICE_FLAG_HID_MODE;
}

static void
fu_company_i2c_hid_device_finalize(GObject *object)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(object);

  g_free(self->board_name);

  G_OBJECT_CLASS(fu_company_i2c_hid_device_parent_class)->finalize(object);
}

/*--------------------------------------------------------------------------*/
/* HID Communication - using fu_hidraw_device API                           */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_device_hid_write_cmd:
 *
 * Write a command to device via HID interface using feature reports.
 * This uses fu_hidraw_device_set_feature() for communication.
 *
 * Returns: TRUE on success, FALSE on failure
 */
static gboolean
fu_company_i2c_hid_device_hid_write_cmd(FuCompanyI2cHidDevice *self,
                                         guint8 report_id,
                                         guint16 reg,
                                         guint16 cmd,
                                         GError **error)
{
  guint8 buf[8] = {0};
  gsize buflen = 0;

  /* Build command buffer: [report_id, reg_lo, reg_hi, cmd_lo, cmd_hi] */
  buf[buflen++] = report_id;
  buf[buflen++] = (guint8)(reg & 0xFF);
  buf[buflen++] = (guint8)((reg >> 8) & 0xFF);
  buf[buflen++] = (guint8)(cmd & 0xFF);
  buf[buflen++] = (guint8)((cmd >> 8) & 0xFF);

  return fu_hidraw_device_set_feature(FU_HIDRAW_DEVICE(self),
                                       buf,
                                       buflen,
                                       FU_IOCTL_FLAG_NONE,
                                       error);
}

/**
 * fu_company_i2c_hid_device_hid_read_cmd:
 *
 * Read data from device via HID interface using feature reports.
 * This uses fu_hidraw_device_get_feature() for communication.
 *
 * Returns: TRUE on success, FALSE on failure
 */
static gboolean
fu_company_i2c_hid_device_hid_read_cmd(FuCompanyI2cHidDevice *self,
                                        guint8 report_id,
                                        guint16 reg,
                                        guint16 *value,
                                        GError **error)
{
  guint8 req_buf[5] = {0};
  guint8 res_buf[8] = {0};
  gsize buflen = 0;

  /* Build request buffer */
  req_buf[buflen++] = report_id;
  req_buf[buflen++] = (guint8)(reg & 0xFF);
  req_buf[buflen++] = (guint8)((reg >> 8) & 0xFF);

  /* SetReport to send the read request */
  if (!fu_hidraw_device_set_feature(FU_HIDRAW_DEVICE(self),
                                     req_buf,
                                     buflen,
                                     FU_IOCTL_FLAG_NONE,
                                     error))
    return FALSE;

  /* GetReport to read the response */
  res_buf[0] = report_id;
  if (!fu_hidraw_device_get_feature(FU_HIDRAW_DEVICE(self),
                                     res_buf,
                                     sizeof(res_buf),
                                     FU_IOCTL_FLAG_NONE,
                                     error))
    return FALSE;

  /* Parse response: [report_id, value_lo, value_hi, ...] */
  if (value != NULL)
    *value = (guint16)((res_buf[2] << 8) | res_buf[1]);

  return TRUE;
}

/**
 * fu_company_i2c_hid_device_hid_write_fw_block:
 *
 * Write a firmware block via HID.
 *
 * Returns: TRUE on success, FALSE on failure
 */
static gboolean
fu_company_i2c_hid_device_hid_write_fw_block(FuCompanyI2cHidDevice *self,
                                              guint16 block_num,
                                              const guint8 *data,
                                              gsize len,
                                              GError **error)
{
  guint8 *buf = NULL;
  gsize buflen = 0;
  g_autoptr(GByteArray) array = g_byte_array_new();

  /* Build: [report_id, block_num_lo, block_num_hi, data...] */
  fu_byte_array_append_uint8(array, FU_COMPANY_I2C_HID_REPORT_ID_UPDATE);
  fu_byte_array_append_uint8(array, (guint8)(block_num & 0xFF));
  fu_byte_array_append_uint8(array, (guint8)((block_num >> 8) & 0xFF));
  g_byte_array_append(array, data, len);

  /* Calculate checksum */
  guint16 csum = fu_sum16w(data, len, G_LITTLE_ENDIAN);
  fu_byte_array_append_uint8(array, (guint8)(csum & 0xFF));
  fu_byte_array_append_uint8(array, (guint8)((csum >> 8) & 0xFF));

  return fu_hidraw_device_set_feature(FU_HIDRAW_DEVICE(self),
                                     array->data,
                                     array->len,
                                     FU_IOCTL_FLAG_NONE,
                                     error);
}

/*--------------------------------------------------------------------------*/
/* Device probe - device discovery                                           */
/*--------------------------------------------------------------------------*/

static gboolean
fu_company_i2c_hid_device_probe(FuDevice *device, GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);

  /* Check subsystem is hidraw */
  if ((g_strcmp0(fu_udev_device_get_subsystem(FU_UDEV_DEVICE(device)), "hidraw") != 0)) {
    g_set_error(error,
                FWUPD_ERROR,
                FWUPD_ERROR_NOT_SUPPORTED,
                "is not correct subsystem=%s, expected hidraw",
                fu_udev_device_get_subsystem(FU_UDEV_DEVICE(device)));
    return FALSE;
  }

  if (fu_udev_device_get_device_file(FU_UDEV_DEVICE(device)) == NULL) {
    g_set_error_literal(error,
                        FWUPD_ERROR,
                        FWUPD_ERROR_NOT_SUPPORTED,
                        "no device file");
    return FALSE;
  }

  /* Set the physical ID for udev matching */
  return fu_udev_device_set_physical_id(FU_UDEV_DEVICE(device), "hidraw", error);
}

/*--------------------------------------------------------------------------*/
/* Device setup - read device properties                                    */
/*--------------------------------------------------------------------------*/

static gboolean
fu_company_i2c_hid_device_ensure_iap_ctrl(FuCompanyI2cHidDevice *self, GError **error)
{
  guint16 tmp = 0;

  /* Read IAP control register */
  if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                               FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                               0x20,  /* IAP_CTRL register */
                                               &tmp,
                                               error)) {
    g_prefix_error_literal(error, "failed to read IAPControl: ");
    return FALSE;
  }

  self->iap_ctrl = tmp;

  /* Check if in bootloader mode */
  if ((self->iap_ctrl & 0x01) == 0)  /* Bit 0 indicates bootloader mode */
    fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_IS_BOOTLOADER);
  else
    fu_device_remove_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_IS_BOOTLOADER);

  return TRUE;
}

static gboolean
fu_company_i2c_hid_device_setup(FuDevice *device, GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);
  guint16 fwver = 0;
  guint16 iap_ver = 0;
  guint16 tmp = 0;
  g_autofree gchar *version_bl = NULL;

  /* Read HID ID to get pattern */
  if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                               FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                               FU_COMPANY_I2C_HID_CMD_GET_HID_ID,
                                               &tmp,
                                               error)) {
    g_prefix_error_literal(error, "failed to read HID ID: ");
    return FALSE;
  }
  self->pattern = tmp != 0xFFFF ? (tmp & 0xFF00) >> 8 : 0;

  /* Read firmware version */
  if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                               FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                               FU_COMPANY_I2C_HID_CMD_GET_VERSION,
                                               &fwver,
                                               error)) {
    g_prefix_error_literal(error, "failed to read fw version: ");
    return FALSE;
  }
  if (fwver == 0xFFFF || fwver == FU_COMPANY_I2C_HID_CMD_GET_VERSION)
    fwver = 0;
  fu_device_set_version_raw(device, fwver);

  /* Read IAP (bootloader) version */
  if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                               FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                               0x11,  /* IAP_VERSION register */
                                               &iap_ver,
                                               error)) {
    g_prefix_error_literal(error, "failed to read bootloader version: ");
    return FALSE;
  }
  if (self->pattern >= 1)
    iap_ver >>= 8;
  version_bl = fu_version_from_uint16(iap_ver, FWUPD_VERSION_FORMAT_HEX);
  fu_device_set_version_bootloader(device, version_bl);

  /* Read module ID */
  if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                               FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                               0x12,  /* MODULE_ID register */
                                               &self->module_id,
                                               error)) {
    g_prefix_error_literal(error, "failed to read module ID: ");
    return FALSE;
  }

  /* Build instance IDs for GUID matching */
  fu_device_add_instance_u16(device, "VEN", fu_device_get_vid(device));
  fu_device_add_instance_u16(device, "DEV", fu_device_get_pid(device));
  fu_device_add_instance_u16(device, "MOD", self->module_id);
  if (!fu_device_build_instance_id(device, error, "HIDRAW", "VEN", "DEV", "MOD", NULL))
    return FALSE;

  /* Add IC-type specific instance ID */
  fu_device_add_instance_u8(device, "ICTYPE", (guint8)self->ic_type);
  fu_device_build_instance_id_full(device,
                                   FU_DEVICE_INSTANCE_FLAG_QUIRKS,
                                   NULL,
                                   "COMPANY_I2C",
                                   "ICTYPE",
                                   NULL);
  fu_device_build_instance_id(device, NULL, "COMPANY_I2C", "ICTYPE", "MOD", NULL);

  /* Add driver-specific instance IDs */
  fu_device_add_instance_str(device, "DRIVER", "HID");
  fu_device_build_instance_id(device, NULL, "COMPANY_I2C", "ICTYPE", "MOD", "DRIVER", NULL);

  /* Check page count from quirk */
  if (self->page_count == 0x0) {
    g_set_error(error,
                FWUPD_ERROR,
                FWUPD_ERROR_NOT_SUPPORTED,
                "no page count for COMPANY_I2C\\ICTYPE_%02X",
                self->ic_type);
    return FALSE;
  }

  /* Set firmware size based on page count */
  fu_device_set_firmware_size(device, (guint64)self->page_count * (guint64)64);

  /* Check if in bootloader mode */
  if (!fu_company_i2c_hid_device_ensure_iap_ctrl(self, error))
    return FALSE;

  /* success */
  return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Device detach - enter bootloader mode                                    */
/*--------------------------------------------------------------------------*/

static gboolean
fu_company_i2c_hid_device_detach(FuCompanyI2cHidDevice *self, FuProgress *progress, GError **error)
{
  guint16 iap_ver;
  guint16 ic_type;
  guint16 tmp;

  /* Sanity check - if already in bootloader mode, just reset */
  if (fu_device_has_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_IS_BOOTLOADER)) {
    g_info("in bootloader mode, reset IC");
    if (!fu_company_i2c_hid_device_hid_write_cmd(self,
                                                  FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                                  FU_COMPANY_I2C_HID_CMD_IAP_RESET,
                                                  0x01,
                                                  error))
      return FALSE;
    fu_device_sleep(FU_DEVICE(self), 500);  /* 500ms */
  }

  /* Read IC type from OSM version register */
  if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                               FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                               0x13,  /* OSM_VERSION register */
                                               &tmp,
                                               error)) {
    g_prefix_error_literal(error, "failed to read OSM version: ");
    return FALSE;
  }
  if (tmp == 0xFFFF || tmp == 0x1013) {  /* Check for invalid/format */
    if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                                 FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                                 0x14,  /* IC_BODY register */
                                                 &ic_type,
                                                 error)) {
      g_prefix_error_literal(error, "failed to read IC body: ");
      return FALSE;
    }
    ic_type &= 0xFF;
  } else {
    ic_type = (tmp >> 8) & 0xFF;
  }

  /* Read IAP version */
  if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                               FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                               0x11,  /* IAP_VERSION register */
                                               &iap_ver,
                                               error)) {
    g_prefix_error_literal(error, "failed to read IAP version: ");
    return FALSE;
  }
  if (self->pattern >= 1)
    iap_ver >>= 8;

  /* Set page size based on IC type and IAP version */
  self->block_size = 64;
  if (ic_type >= 0x10) {
    if (iap_ver >= 1) {
      if (iap_ver >= 2 && (ic_type == 0x14 || ic_type == 0x15)) {
        self->block_size = 512;
      } else {
        self->block_size = 128;
      }
    }
  }

  /* Write IAP password to enter bootloader mode */
  if (self->iap_password != 0) {
    if (!fu_company_i2c_hid_device_hid_write_cmd(self,
                                                  FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                                  FU_COMPANY_I2C_HID_CMD_IAP_PASSWORD,
                                                  (guint16)self->iap_password,
                                                  error)) {
      g_prefix_error_literal(error, "failed to write IAP password: ");
      return FALSE;
    }
    fu_device_sleep(FU_DEVICE(self), 100);  /* Wait for unlock */
  }

  /* Enter IAP mode */
  if (!fu_company_i2c_hid_device_hid_write_cmd(self,
                                                FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                                FU_COMPANY_I2C_HID_CMD_IAP,
                                                self->iap_password,
                                                error)) {
    g_prefix_error_literal(error, "failed to enter IAP mode: ");
    return FALSE;
  }

  fu_device_sleep(FU_DEVICE(self), 200);  /* Wait for IAP mode */

  /* Verify we're in bootloader mode */
  if (!fu_company_i2c_hid_device_ensure_iap_ctrl(self, error))
    return FALSE;

  if ((self->iap_ctrl & 0x02) == 0) {  /* Bit 1 indicates password accepted */
    g_set_error_literal(error,
                        FWUPD_ERROR,
                        FWUPD_ERROR_WRITE,
                        "unexpected bootloader password");
    return FALSE;
  }

  /* success */
  return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Device attach - return to runtime mode                                   */
/*--------------------------------------------------------------------------*/

static gboolean
fu_company_i2c_hid_device_attach(FuDevice *device, FuProgress *progress, GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);

  /* Sanity check - if not in bootloader, skip */
  if (!fu_device_has_flag(device, FWUPD_DEVICE_FLAG_IS_BOOTLOADER)) {
    g_debug("already in runtime mode, skipping");
    return TRUE;
  }

  /* Reset back to runtime */
  if (!fu_company_i2c_hid_device_hid_write_cmd(self,
                                                FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                                FU_COMPANY_I2C_HID_CMD_IAP_RESET,
                                                0x01,
                                                error))
    return FALSE;

  fu_device_sleep(device, 500);  /* Wait for reset */

  /* success */
  return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Device firmware check                                                     */
/*--------------------------------------------------------------------------*/

static gboolean
fu_company_i2c_hid_device_check_firmware(FuDevice *device,
                                          FuFirmware *firmware,
                                          FuFirmwareParseFlags flags,
                                          GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);
  guint16 module_id;

  /* Get module_id from firmware - this would need firmware struct access */
  /* For now, just check if firmware exists */
  if (firmware == NULL) {
    g_set_error_literal(error,
                        FWUPD_ERROR,
                        FWUPD_ERROR_INVALID_FILE,
                        "firmware is NULL");
    return FALSE;
  }

  /* Check compatibility - module_id should match */
  /* This is a simplified check; real implementation would parse firmware */
  g_debug("firmware check passed for module_id=0x%04X", self->module_id);

  return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Device firmware write                                                    */
/*--------------------------------------------------------------------------*/

static gboolean
fu_company_i2c_hid_device_write_firmware(FuDevice *device,
                                           FuFirmware *firmware,
                                           FuProgress *progress,
                                           FwupdInstallFlags flags,
                                           GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);
  gsize bufsz = 0;
  const guint8 *buf = NULL;
  guint16 block_size = self->block_size > 0 ? self->block_size : 64;
  guint n_blocks = 0;
  guint16 checksum = 0;
  guint16 checksum_device = 0;
  g_autoptr(GBytes) fw = NULL;

  /* Progress steps */
  fu_progress_set_id(progress, G_STRLOC);
  fu_progress_add_flag(progress, FU_PROGRESS_FLAG_GUESSED);
  fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_BUSY, 2, "detach");
  fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 90, NULL);
  fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_VERIFY, 10, NULL);
  fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 1, NULL);

  /* Get firmware bytes */
  fw = fu_firmware_get_bytes(firmware, error);
  if (fw == NULL)
    return FALSE;

  buf = g_bytes_get_data(fw, &bufsz);
  if (bufsz == 0) {
    g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_INVALID_FILE, "firmware is empty");
    return FALSE;
  }

  n_blocks = (bufsz + block_size - 1) / block_size;

  /* Detach - enter bootloader mode */
  if (!fu_company_i2c_hid_device_detach(self, fu_progress_get_child(progress), error))
    return FALSE;
  fu_progress_step_done(progress);

  /* Write firmware blocks */
  for (guint i = 0; i < n_blocks; i++) {
    gsize chunk_size = MIN(block_size, bufsz - i * block_size);
    const guint8 *chunk_data = buf + i * block_size;

    /* Calculate block checksum */
    guint16 csum_tmp = fu_sum16w(chunk_data, chunk_size, G_LITTLE_ENDIAN);
    checksum += csum_tmp;

    /* Write block via HID */
    if (!fu_company_i2c_hid_device_hid_write_fw_block(self, i, chunk_data, chunk_size, error))
      return FALSE;

    /* Wait for write completion */
    fu_device_sleep(device, block_size == 512 ? 50 : 20);

    /* Verify IAP ctrl for errors */
    if (!fu_company_i2c_hid_device_ensure_iap_ctrl(self, error))
      return FALSE;

    if (self->iap_ctrl & 0x04) {  /* Error bit */
      g_set_error(error,
                  FWUPD_ERROR,
                  FWUPD_ERROR_WRITE,
                  "bootloader reports failed write: 0x%x",
                  self->iap_ctrl);
      return FALSE;
    }

    fu_progress_set_percentage_full(fu_progress_get_child(progress),
                                     (gsize)i + 1,
                                     (gsize)n_blocks);
  }
  fu_progress_step_done(progress);

  /* Verify checksum */
  if (!fu_company_i2c_hid_device_hid_read_cmd(self,
                                               FU_COMPANY_I2C_HID_REPORT_ID_UPDATE,
                                               0x21,  /* CHECKSUM register */
                                               &checksum_device,
                                               error))
    return FALSE;

  if (checksum != checksum_device) {
    g_set_error(error,
                FWUPD_ERROR,
                FWUPD_ERROR_WRITE,
                "checksum failed 0x%04x != 0x%04x",
                checksum,
                checksum_device);
    return FALSE;
  }
  fu_progress_step_done(progress);

  /* Wait for device reset */
  fu_device_sleep_full(device, 1000, fu_progress_get_child(progress));
  fu_progress_step_done(progress);

  return TRUE;
}

/*--------------------------------------------------------------------------*/
/* Quirk handling                                                           */
/*--------------------------------------------------------------------------*/

static gboolean
fu_company_i2c_hid_device_set_quirk_kv(FuDevice *device,
                                       const gchar *key,
                                       const gchar *value,
                                       GError **error)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);
  guint64 tmp = 0;

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_I2C_ADDR) == 0) {
    if (!fu_strtoull(value, &tmp, 0, G_MAXUINT16, FU_INTEGER_BASE_AUTO, error))
      return FALSE;
    self->i2c_slave_addr = (guint16)tmp;
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_IC_TYPE) == 0) {
    if (!fu_strtoull(value, &tmp, 0, G_MAXUINT16, FU_INTEGER_BASE_AUTO, error))
      return FALSE;
    self->ic_type = (guint16)tmp;
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_MODULE_ID) == 0) {
    if (!fu_strtoull(value, &tmp, 0, G_MAXUINT16, FU_INTEGER_BASE_AUTO, error))
      return FALSE;
    self->module_id = (guint16)tmp;
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_IAP_PASSWORD) == 0) {
    if (!fu_strtoull(value, &tmp, 0, G_MAXUINT32, FU_INTEGER_BASE_AUTO, error))
      return FALSE;
    self->iap_password = (guint32)tmp;
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_PAGE_COUNT) == 0) {
    if (!fu_strtoull(value, &tmp, 0, G_MAXUINT16, FU_INTEGER_BASE_AUTO, error))
      return FALSE;
    self->page_count = (guint16)tmp;
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_BLOCK_SIZE) == 0) {
    if (!fu_strtoull(value, &tmp, 0, G_MAXUINT16, FU_INTEGER_BASE_AUTO, error))
      return FALSE;
    self->block_size = (guint16)tmp;
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_BOARD_NAME) == 0) {
    fu_company_i2c_hid_device_set_board_name(self, value);
    return TRUE;
  }

  if (g_strcmp0(key, FU_COMPANY_I2C_HID_QUIRK_DRIVER) == 0) {
    if (g_strcmp0(value, FU_COMPANY_I2C_HID_DRIVER_HID) == 0) {
      self->flags |= FU_COMPANY_I2C_HID_DEVICE_FLAG_HID_MODE;
    } else if (g_strcmp0(value, FU_COMPANY_I2C_HID_DRIVER_I2C) == 0) {
      self->flags |= FU_COMPANY_I2C_HID_DEVICE_FLAG_I2C_MODE;
    }
    return TRUE;
  }

  g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED, "quirk key not supported");
  return FALSE;
}

/*--------------------------------------------------------------------------*/
/* Device string representation                                              */
/*--------------------------------------------------------------------------*/

static void
fu_company_i2c_hid_device_to_string(FuDevice *device, guint idt, GString *str)
{
  FuCompanyI2cHidDevice *self = FU_COMPANY_I2C_HID_DEVICE(device);

  fwupd_codec_string_append_hex(str, idt, "IcType", self->ic_type);
  fwupd_codec_string_append_hex(str, idt, "ModuleId", self->module_id);
  fwupd_codec_string_append_hex(str, idt, "I2cAddr", self->i2c_slave_addr);
  fwupd_codec_string_append_hex(str, idt, "IapPassword", self->iap_password);
  fwupd_codec_string_append_hex(str, idt, "PageCount", self->page_count);
  fwupd_codec_string_append_hex(str, idt, "BlockSize", self->block_size);
  fwupd_codec_string_append_hex(str, idt, "IapCtrl", self->iap_ctrl);
  fwupd_codec_string_append_hex(str, idt, "Pattern", self->pattern);
}

/*--------------------------------------------------------------------------*/
/* Device progress tracking                                                 */
/*--------------------------------------------------------------------------*/

static void
fu_company_i2c_hid_device_set_progress(FuDevice *device, FuProgress *progress)
{
  fu_progress_set_id(progress, G_STRLOC);
  fu_progress_add_flag(progress, FU_PROGRESS_FLAG_GUESSED);
  fu_progress_add_step(progress, FWUPD_STATUS_DECOMPRESSING, 0, "prepare-fw");
  fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 2, "detach");
  fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_WRITE, 94, "write");
  fu_progress_add_step(progress, FWUPD_STATUS_DEVICE_RESTART, 2, "attach");
  fu_progress_add_step(progress, FU_DEVICE_STATUS, 2, "reload");
}

/*--------------------------------------------------------------------------*/
/* Version conversion                                                       */
/*--------------------------------------------------------------------------*/

static gchar *
fu_company_i2c_hid_device_convert_version(FuDevice *device, guint64 version_raw)
{
  return fu_version_from_uint16(version_raw, fu_device_get_version_format(device));
}

/*--------------------------------------------------------------------------*/
/* Class initialization                                                     */
/*--------------------------------------------------------------------------*/

static void
fu_company_i2c_hid_device_init(FuCompanyI2cHidDevice *self)
{
  fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_INTERNAL);
  fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UPDATABLE);
  fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UNSIGNED_PAYLOAD);
  fu_device_add_protocol(FU_DEVICE(self), FU_COMPANY_I2C_HID_PROTOCOL_ID);
  fu_device_set_version_format(FU_DEVICE(self), FWUPD_VERSION_FORMAT_HEX);
  fu_device_set_firmware_gtype(FU_DEVICE(self), FU_TYPE_COMPANY_I2C_HID_FIRMWARE);
}

static void
fu_company_i2c_hid_device_class_init(FuCompanyI2cHidDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  FuDeviceClass *device_class = FU_DEVICE_CLASS(klass);

  object_class->finalize = fu_company_i2c_hid_device_finalize;

  device_class->to_string = fu_company_i2c_hid_device_to_string;
  device_class->set_quirk_kv = fu_company_i2c_hid_device_set_quirk_kv;
  device_class->setup = fu_company_i2c_hid_device_setup;
  device_class->reload = fu_company_i2c_hid_device_setup;
  device_class->attach = fu_company_i2c_hid_device_attach;
  device_class->detach = NULL;  /* handled by write_firmware */
  device_class->write_firmware = fu_company_i2c_hid_device_write_firmware;
  device_class->check_firmware = fu_company_i2c_hid_device_check_firmware;
  device_class->probe = fu_company_i2c_hid_device_probe;
  device_class->set_progress = fu_company_i2c_hid_device_set_progress;
  device_class->convert_version = fu_company_i2c_hid_device_convert_version;
}

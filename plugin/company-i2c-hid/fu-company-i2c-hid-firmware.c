/*
 * fu-company-i2c-hid-firmware.c
 *
 * Custom firmware format support for Company HID I2C Devices
 *
 * This implements firmware parsing for the Company's binary firmware format.
 *
 * Firmware Format:
 *   - Header (16 bytes):
 *     - Magic number (4 bytes): 0x434F4D50 ("COMP")
 *     - Version (4 bytes): firmware version
 *     - Size (4 bytes): size of firmware data
 *     - Checksum (4 bytes): CRC32 checksum
 *   - Body: raw firmware data
 *   - Footer (optional): metadata
 *
 * Copyright (C) 2026 Company
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include "fu-company-i2c-hid-firmware.h"

#include <zlib.h>

/* Firmware private data */
struct _FuCompanyI2cHidFirmware {
  FuFirmware parent_instance;
  guint32 version;
  guint32 size;
  guint32 checksum;
  GBytes *image_data;
};

G_DEFINE_TYPE(FuCompanyI2cHidFirmware, fu_company_i2c_hid_firmware, FU_TYPE_FIRMWARE)

/* Forward declarations */
static gboolean
fu_company_i2c_hid_firmware_parse(FuFirmware *firmware,
                                   GBytes *blob,
                                   guint64 offset,
                                   FwupdInstallFlags flags,
                                   GError **error);

static GBytes *
fu_company_i2c_hid_firmware_write(FuFirmware *firmware,
                                   GError **error);

static gboolean
fu_company_i2c_hid_firmware_build(FuFirmware *firmware,
                                   XbNode *n,
                                   GError **error);

static void
fu_company_i2c_hid_firmware_add_image(FuCompanyI2cHidFirmware *self,
                                       GBytes *data,
                                       GError **error);

/*--------------------------------------------------------------------------*/
/* GObject type registration                                                */
/*--------------------------------------------------------------------------*/

static void
fu_company_i2c_hid_firmware_class_init(FuCompanyI2cHidFirmwareClass *klass)
{
  FuFirmwareClass *firmware_class = FU_FIRMWARE_CLASS(klass);

  firmware_class->parse = fu_company_i2c_hid_firmware_parse;
  firmware_class->write = fu_company_i2c_hid_firmware_write;
  firmware_class->build = fu_company_i2c_hid_firmware_build;
}

static void
fu_company_i2c_hid_firmware_init(FuCompanyI2cHidFirmware *self)
{
  self->version = 0;
  self->size = 0;
  self->checksum = 0;
  self->image_data = NULL;
}

/*--------------------------------------------------------------------------*/
/* Firmware object creation                                                 */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_firmware_new:
 *
 * Create a new Company I2C HID firmware object.
 *
 * Returns: (transfer full): A new FuCompanyI2cHidFirmware
 */
FuCompanyI2cHidFirmware *
fu_company_i2c_hid_firmware_new(void)
{
  return g_object_new(FU_TYPE_COMPANY_I2C_HID_FIRMWARE, NULL);
}

/*--------------------------------------------------------------------------*/
/* Firmware parsing                                                         */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_firmware_parse:
 *
 * Parse the Company's binary firmware format from a firmware blob.
 *
 * The format is:
 *   - Header (16 bytes)
 *   - Body (variable size, contains the actual firmware)
 *
 * If the firmware is already in raw format (no header), this function
 * treats the entire blob as firmware data.
 */
static gboolean
fu_company_i2c_hid_firmware_parse(FuFirmware *firmware,
                                   GBytes *blob,
                                   guint64 offset,
                                   FwupdInstallFlags flags,
                                   GError **error)
{
  FuCompanyI2cHidFirmware *self = FU_COMPANY_I2C_HID_FIRMWARE(firmware);
  gsize fw_size = 0;
  const guint8 *fw_data = g_bytes_get_data(blob, &fw_size);
  guint32 magic = 0;
  guint32 version = 0;
  guint32 size = 0;
  guint32 checksum = 0;

  /* Check minimum size */
  if (fw_size < FU_COMPANY_I2C_HID_FIRMWARE_HEADER_SIZE) {
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_INVALID_DATA,
                "Firmware too small (%zu bytes, minimum %u)",
                fw_size, FU_COMPANY_I2C_HID_FIRMWARE_HEADER_SIZE);
    return FALSE;
  }

  /* Read header fields (big-endian) */
  magic = fu_common_read_uint32(&fw_data[offset + 0], G_BIG_ENDIAN);
  version = fu_common_read_uint32(&fw_data[offset + FU_COMPANY_I2C_HID_FIRMWARE_VERSION_OFFSET], G_BIG_ENDIAN);
  size = fu_common_read_uint32(&fw_data[offset + FU_COMPANY_I2C_HID_FIRMWARE_SIZE_OFFSET], G_BIG_ENDIAN);
  checksum = fu_common_read_uint32(&fw_data[offset + FU_COMPANY_I2C_HID_FIRMWARE_CHECKSUM_OFFSET], G_BIG_ENDIAN);

  /* Verify magic number */
  if (magic != FU_COMPANY_I2C_HID_FIRMWARE_MAGIC) {
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_INVALID_FILE_FORMAT,
                "Invalid magic number: 0x%08X (expected 0x%08X)",
                magic, FU_COMPANY_I2C_HID_FIRMWARE_MAGIC);
    return FALSE;
  }

  /* Verify size */
  if (size != fw_size - FU_COMPANY_I2C_HID_FIRMWARE_HEADER_SIZE) {
    g_warning("Firmware size mismatch: header says %u, actual %zu",
              size, fw_size - FU_COMPANY_I2C_HID_FIRMWARE_HEADER_SIZE);
    size = fw_size - FU_COMPANY_I2C_HID_FIRMWARE_HEADER_SIZE;
  }

  /* Store parsed values */
  self->version = version;
  self->size = size;
  self->checksum = checksum;

  /* Create image from firmware body */
  if (size > 0) {
    g_autoptr(GBytes) image_data = g_bytes_new_from_bytes(blob,
                                                            offset + FU_COMPANY_I2C_HID_FIRMWARE_HEADER_SIZE,
                                                            size);
    fu_company_i2c_hid_firmware_add_image(self, image_data, error);
    if (*error != NULL)
      return FALSE;
  }

  /* Set default image ID */
  fu_firmware_set_id(firmware, "main");
  fu_firmware_set_offset(firmware, offset);

  g_debug("Parsed Company I2C HID firmware: version=0x%08X, size=%u, checksum=0x%08X",
          version, size, checksum);

  return TRUE;
}

/**
 * fu_company_i2c_hid_firmware_add_image:
 *
 * Add a firmware image to the firmware object.
 */
static void
fu_company_i2c_hid_firmware_add_image(FuCompanyI2cHidFirmware *self,
                                        GBytes *data,
                                        GError **error)
{
  FuFirmware *firmware = FU_FIRMWARE(self);
  g_autoptr(FuFirmwareImage) image = NULL;

  image = fu_firmware_image_new(data);
  fu_firmware_add_image(firmware, image);
  self->image_data = g_bytes_ref(data);
}

/*--------------------------------------------------------------------------*/
/* Firmware writing                                                         */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_firmware_write:
 *
 * Write the firmware back to a binary blob.
 * This serializes the internal firmware representation back to the
 * Company's firmware format with header.
 *
 * Returns: (transfer full): A GBytes containing the firmware binary
 */
static GBytes *
fu_company_i2c_hid_firmware_write(FuFirmware *firmware,
                                   GError **error)
{
  FuCompanyI2cHidFirmware *self = FU_COMPANY_I2C_HID_FIRMWARE(firmware);
  FuFirmwareImage *image = NULL;
  GBytes *image_data = NULL;
  gsize image_size = 0;
  guint8 *buf = NULL;
  gsize buf_size = 0;
  guint32 checksum = 0;
  guint8 *p = NULL;

  /* Get firmware image */
  image = fu_firmware_get_image_default(firmware);
  if (image == NULL) {
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_NO_FIRMWARE,
                "No firmware image available");
    return NULL;
  }

  image_data = fu_firmware_image_get_bytes(image);
  if (image_data == NULL) {
    g_set_error(error, FWUPD_ERROR, FWUPD_ERROR_INVALID_DATA,
                "No image data available");
    return NULL;
  }

  image_size = g_bytes_get_size(image_data);

  /* Allocate buffer: header + image data */
  buf_size = FU_COMPANY_I2C_HID_FIRMWARE_HEADER_SIZE + image_size;
  buf = g_malloc(buf_size);

  /* Write header */
  p = buf;
  fu_common_write_uint32(p, FU_COMPANY_I2C_HID_FIRMWARE_MAGIC, G_BIG_ENDIAN);
  p += 4;
  fu_common_write_uint32(p, self->version, G_BIG_ENDIAN);
  p += 4;
  fu_common_write_uint32(p, image_size, G_BIG_ENDIAN);
  p += 4;

  /* Calculate and write checksum (CRC32) */
  checksum = crc32(0L, NULL, 0);
  checksum = crc32(checksum,
                    g_bytes_get_data(image_data, NULL),
                    image_size);
  fu_common_write_uint32(p, checksum, G_BIG_ENDIAN);
  p += 4;

  /* Copy image data */
  memcpy(p, g_bytes_get_data(image_data, NULL), image_size);

  /* Store checksum in firmware object */
  self->checksum = checksum;
  self->size = image_size;

  return g_bytes_new_take(buf, buf_size);
}

/*--------------------------------------------------------------------------*/
/* Firmware building from XML                                               */
/*--------------------------------------------------------------------------*/

/**
 * fu_company_i2c_hid_firmware_build:
 *
 * Build firmware from an XML node (used by fwupdtool).
 * This allows the firmware format to be defined in firmware.builder.xml.
 */
static gboolean
fu_company_i2c_hid_firmware_build(FuFirmware *firmware,
                                   XbNode *n,
                                   GError **error)
{
  FuCompanyI2cHidFirmware *self = FU_COMPANY_I2C_HID_FIRMWARE(firmware);
  const gchar *version_str = NULL;

  /* Get version from XML */
  version_str = xb_node_query_text(n, "version", NULL);
  if (version_str != NULL) {
    self->version = fu_common_strtoull(version_str);
  }

  return TRUE;
}

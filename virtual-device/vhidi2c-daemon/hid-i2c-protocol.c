/* SPDX-License-Identifier: GPL-2.0-or-later */
/**
 * HID over I2C Protocol Implementation
 * Userspace library for virtual HID over I2C device using i2c-stub
 *
 * Copyright (C) 2026 Keroro Squad
 */

#include "hid-i2c-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

/* Debug logging macro */
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "[VHID-I2C DEBUG] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) do {} while (0)
#endif

/* Info logging macro */
#define INFO_PRINT(fmt, ...) \
    fprintf(stdout, "[VHID-I2C INFO] " fmt "\n", ##__VA_ARGS__)

/* Error logging macro */
#define ERROR_PRINT(fmt, ...) \
    fprintf(stderr, "[VHID-I2C ERROR] " fmt "\n", ##__VA_ARGS__)

/* Virtual device register storage (simulates device memory) */
static uint8_t g_registers[256] = {0};

/* HID Descriptor for our virtual device */
static const struct i2c_hid_desc vhid_descriptor = {
    .wHIDDescLength     = cpu_to_le16(sizeof(struct i2c_hid_desc)),  /* 30 */
    .bcdVersion         = cpu_to_le16(0x0101),                        /* HID 1.11 */
    .wReportDescLength  = cpu_to_le16(sizeof(hid_report_desc)),
    .wReportDescRegister= cpu_to_le16(REG_REPORT_DESC),
    .wInputRegister     = cpu_to_le16(REG_INPUT_REGISTER),
    .wMaxInputLength    = cpu_to_le16(64),
    .wOutputRegister    = cpu_to_le16(REG_OUTPUT_REGISTER),
    .wMaxOutputLength   = cpu_to_le16(64),
    .wCommandRegister   = cpu_to_le16(REG_COMMAND_REGISTER),
    .wDataRegister      = cpu_to_le16(REG_DATA_REGISTER),
    .wVendorID          = cpu_to_le16(0x1234),    /* Virtual vendor ID */
    .wProductID         = cpu_to_le16(0x5678),   /* Virtual product ID */
    .wVersionID         = cpu_to_le16(0x0001),    /* Version 0.0.1 */
    .reserved           = cpu_to_le32(0),
};

/* Utility: Convert CPU endian to little-endian
 * Extracts bytes from memory representation to ensure correct LE output
 * regardless of platform endianness. */
uint16_t cpu_to_le16(uint16_t val)
{
    uint8_t *p = (uint8_t *)&val;
    return ((uint16_t)p[0]) | ((uint16_t)p[1] << 8);
}

uint32_t cpu_to_le32(uint32_t val)
{
    uint8_t *p = (uint8_t *)&val;
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/**
 * Initialize the virtual HID I2C device structure
 */
int vhid_i2c_init(struct vhid_i2c_device *dev, int i2c_adapter, uint8_t slave_addr)
{
    if (!dev) {
        ERROR_PRINT("Invalid device pointer");
        return -EINVAL;
    }

    memset(dev, 0, sizeof(*dev));
    dev->fd = -1;
    dev->slave_addr = slave_addr;
    dev->power_state = HID_PWR_ON;
    dev->protocol = HID_PROTOCOL_REPORT;
    dev->idle_rate = 0;
    dev->report_id = 0;
    dev->initialized = false;

    INFO_PRINT("Device initialized: adapter=%d, addr=0x%02X", i2c_adapter, slave_addr);
    return 0;
}

/**
 * Open I2C bus and connect to the virtual device
 */
int vhid_i2c_open(struct vhid_i2c_device *dev)
{
    char i2c_dev_path[64];
    int ret;

    if (!dev) {
        return -EINVAL;
    }

    /* Open I2C bus device */
    snprintf(i2c_dev_path, sizeof(i2c_dev_path), "/dev/i2c-%d", VHID_I2C_ADAPTER);
    dev->fd = open(i2c_dev_path, O_RDWR);
    if (dev->fd < 0) {
        ERROR_PRINT("Failed to open %s: %s", i2c_dev_path, strerror(errno));
        ERROR_PRINT("Make sure i2c-dev kernel module is loaded and you have permission");
        return -ENODEV;
    }

    /* Set slave address */
    ret = ioctl(dev->fd, I2C_SLAVE, dev->slave_addr);
    if (ret < 0) {
        ERROR_PRINT("Failed to set slave address 0x%02X: %s", dev->slave_addr, strerror(errno));
        close(dev->fd);
        dev->fd = -1;
        return -EIO;
    }

    INFO_PRINT("Connected to I2C bus, slave address 0x%02X", dev->slave_addr);
    return 0;
}

/**
 * Close I2C bus connection
 */
void vhid_i2c_close(struct vhid_i2c_device *dev)
{
    if (dev && dev->fd >= 0) {
        close(dev->fd);
        dev->fd = -1;
        INFO_PRINT("I2C connection closed");
    }
}

/**
 * Write HID descriptor to the device via I2C
 * This simulates the device's HID descriptor that host reads on enumeration
 */
int vhid_i2c_write_hid_descriptor(struct vhid_i2c_device *dev)
{
    int ret;
    const uint8_t *desc_bytes = (const uint8_t *)&vhid_descriptor;
    int desc_len = sizeof(vhid_descriptor);

    if (!dev || dev->fd < 0) {
        return -EINVAL;
    }

    DEBUG_PRINT("Writing %d-byte HID descriptor to device", desc_len);

    /* In a real I2C HID device, the HID descriptor is read-only.
     * With i2c-stub, we "program" the descriptor into the stub's registers.
     * The host will read it back using i2c_master_recv().
     *
     * i2c-stub acts as an EEPROM, so we use i2c_smbus_write_i2c_block_data
     * or direct I2C writes to store our HID descriptor.
     */

    /* For i2c-stub: write HID descriptor starting at register 0x00 */
    ret = i2c_smbus_write_i2c_block_data(dev->fd, REG_HID_DESC,
                                         desc_len, desc_bytes);
    if (ret < 0) {
        ERROR_PRINT("Failed to write HID descriptor: %s", strerror(errno));
        return ret;
    }

    INFO_PRINT("HID Descriptor programmed:");
    INFO_PRINT("  - Length: %d bytes", desc_len);
    INFO_PRINT("  - Version: 1.11");
    INFO_PRINT("  - Report Desc Length: %zu bytes", sizeof(hid_report_desc));
    INFO_PRINT("  - Vendor ID: 0x%04X", 0x1234);
    INFO_PRINT("  - Product ID: 0x%04X", 0x5678);

    return 0;
}

/**
 * Write HID report descriptor to the device
 */
int vhid_i2c_write_report_descriptor(struct vhid_i2c_device *dev)
{
    int ret;

    if (!dev || dev->fd < 0) {
        return -EINVAL;
    }

    DEBUG_PRINT("Writing %zu-byte HID report descriptor", sizeof(hid_report_desc));

    /* Write report descriptor to the device */
    ret = i2c_smbus_write_i2c_block_data(dev->fd, REG_REPORT_DESC,
                                          sizeof(hid_report_desc), hid_report_desc);
    if (ret < 0) {
        ERROR_PRINT("Failed to write report descriptor: %s", strerror(errno));
        return ret;
    }

    INFO_PRINT("HID Report Descriptor programmed (%zu bytes)", sizeof(hid_report_desc));
    return 0;
}

/**
 * Read data from a register on the device
 */
int vhid_i2c_read_register(struct vhid_i2c_device *dev, uint8_t reg, uint8_t *buf, int len)
{
    int ret;

    if (!dev || dev->fd < 0 || !buf || len <= 0) {
        return -EINVAL;
    }

    /* Use SMBus read */
    ret = i2c_smbus_read_i2c_block_data(dev->fd, reg, len, buf);
    if (ret < 0) {
        DEBUG_PRINT("Read register 0x%02X failed: %s", reg, strerror(errno));
        return ret;
    }

    DEBUG_PRINT("Read %d bytes from register 0x%02X", ret, reg);
    return ret;
}

/**
 * Write data to a register on the device
 */
int vhid_i2c_write_register(struct vhid_i2c_device *dev, uint8_t reg, const uint8_t *data, int len)
{
    int ret;

    if (!dev || dev->fd < 0 || !data || len <= 0) {
        return -EINVAL;
    }

    ret = i2c_smbus_write_i2c_block_data(dev->fd, reg, len, data);
    if (ret < 0) {
        DEBUG_PRINT("Write register 0x%02X failed: %s", reg, strerror(errno));
        return ret;
    }

    DEBUG_PRINT("Wrote %d bytes to register 0x%02X", len, reg);
    return 0;
}

/**
 * Handle HID command from host
 * This is called when the host writes to the command register
 *
 * Command format (2 bytes):
 *   Byte 0: Report Type (0x01=Input, 0x02=Output, 0x03=Feature)
 *   Byte 1: Report ID (if using numbered reports, else 0)
 *   Byte 2+: Opcode (HID_CMD_*)
 *
 * For GET_REPORT: host writes [report_type][report_id][CMD_GET_REPORT]
 *                 then reads from data register
 *
 * For SET_REPORT: host writes [report_type][report_id][CMD_SET_REPORT]
 *                 then writes data to data register
 */
int vhid_i2c_handle_command(struct vhid_i2c_device *dev, uint8_t cmd,
                            uint8_t report_type, uint8_t report_id,
                            uint8_t *data, int data_len)
{
    if (!dev) {
        return -EINVAL;
    }

    DEBUG_PRINT("Handling command 0x%02X (type=0x%02X, id=0x%02X)",
                cmd, report_type, report_id);

    switch (cmd) {
    case HID_CMD_RESET:
        INFO_PRINT("Command: RESET");
        vhid_i2c_reset(dev);
        return 0;

    case HID_CMD_GET_REPORT:
        DEBUG_PRINT("Command: GET_REPORT (type=0x%02X, id=0x%02X)",
                    report_type, report_id);
        /* For GET_REPORT, we would normally return the report data.
         * For virtual device, return a minimal input report */
        if (data && data_len > 0) {
            /* Return a zeroed input report (keyboard with no keys pressed) */
            memset(data, 0, data_len < 8 ? data_len : 8);
            DEBUG_PRINT("Returning input report (all zeros)");
        }
        return 0;

    case HID_CMD_SET_REPORT:
        DEBUG_PRINT("Command: SET_REPORT (type=0x%02X, id=0x%02X, len=%d)",
                    report_type, report_id, data_len);
        /* Store the received report */
        if (data && data_len > 0) {
            memcpy(dev->output_buf, data, data_len < 64 ? data_len : 64);
            DEBUG_PRINT("Output report stored (%d bytes)", data_len);
        }
        return 0;

    case HID_CMD_GET_IDLE:
        DEBUG_PRINT("Command: GET_IDLE -> %d", dev->idle_rate);
        if (data && data_len > 0) {
            data[0] = dev->idle_rate;
        }
        return 0;

    case HID_CMD_SET_IDLE:
        DEBUG_PRINT("Command: SET_IDLE (rate=%d)", report_id);
        dev->idle_rate = report_id;
        return 0;

    case HID_CMD_GET_PROTOCOL:
        DEBUG_PRINT("Command: GET_PROTOCOL -> %d", dev->protocol);
        if (data && data_len > 0) {
            data[0] = dev->protocol;
        }
        return 0;

    case HID_CMD_SET_PROTOCOL:
        DEBUG_PRINT("Command: SET_PROTOCOL (protocol=%d)", report_id);
        dev->protocol = report_id;
        return 0;

    case HID_CMD_SET_POWER:
        DEBUG_PRINT("Command: SET_POWER (state=%d)", report_id);
        dev->power_state = report_id;
        return 0;

    default:
        ERROR_PRINT("Unknown command: 0x%02X", cmd);
        return -EINVAL;
    }
}

/**
 * Trigger an input report (simulate device-to-host data)
 * In a real device, this would be triggered by an IRQ.
 * Here we just store it for when the host reads.
 */
void vhid_i2c_trigger_input_report(struct vhid_i2c_device *dev, const uint8_t *report, int len)
{
    if (!dev || !report || len <= 0) {
        return;
    }

    int copy_len = len < 64 ? len : 64;
    memcpy(dev->input_buf, report, copy_len);

    DEBUG_PRINT("Input report queued (%d bytes)", copy_len);
    INFO_PRINT("*** Simulated interrupt: input report ready ***");
}

/**
 * Reset the device to initial state
 */
void vhid_i2c_reset(struct vhid_i2c_device *dev)
{
    if (!dev) {
        return;
    }

    dev->power_state = HID_PWR_ON;
    dev->protocol = HID_PROTOCOL_REPORT;
    dev->idle_rate = 0;
    dev->report_id = 0;
    memset(dev->input_buf, 0, sizeof(dev->input_buf));
    memset(dev->output_buf, 0, sizeof(dev->output_buf));

    INFO_PRINT("Device reset complete");
}

/**
 * Get device information string
 */
const char *vhid_i2c_get_device_info(void)
{
    return "Virtual HID over I2C Device v0.0.1\n"
           "  - Based on HID over I2C Specification 1.11\n"
           "  - Uses Linux i2c-stub\n"
           "  - Simulates a minimal keyboard HID device\n"
           "  - For FWUPD Plugin testing";
}

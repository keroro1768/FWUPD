/* SPDX-License-Identifier: GPL-2.0-or-later */
/**
 * HID over I2C Protocol Implementation
 * Header file defining structures, opcodes, and functions
 *
 * Copyright (C) 2026 Keroro Squad
 */

#ifndef HID_I2C_PROTOCOL_H
#define HID_I2C_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <linux/i2c-dev.h>

/* Define __le16, __le32 for userspace if not defined */
#ifndef __le16
#define __le16 uint16_t
#endif
#ifndef __le32
#define __le32 uint32_t
#endif

/* Compiler packed attribute for cross-platform */
#ifndef __packed
#define __packed __attribute__((packed))
#endif

/* I2C HID Device Configuration */
#define VHID_I2C_SLAVE_ADDR         0x2A    /**< Default I2C slave address */
#define VHID_I2C_ADAPTER            0       /**< I2C adapter number (i2c-0) */

/* HID over I2C Registers */
#define REG_HID_DESC                0x00    /**< HID Descriptor (read-only, 30 bytes) */
#define REG_REPORT_DESC             0x01    /**< Report Descriptor register */
#define REG_INPUT_REGISTER          0x02    /**< Input register for host to read */
#define REG_OUTPUT_REGISTER         0x03    /**< Output register for host to write */
#define REG_COMMAND_REGISTER        0x04    /**< Command register (write opcode) */
#define REG_DATA_REGISTER           0x05    /**< Data register (read/write data) */
#define REG_POWER                   0x06    /**< Power state register */

/* HID over I2C Command Opcodes */
#define HID_CMD_RESET               0x01    /**< Reset the device */
#define HID_CMD_GET_REPORT          0x02    /**< Get a report from device */
#define HID_CMD_SET_REPORT          0x03    /**< Set a report on device */
#define HID_CMD_GET_IDLE            0x04    /**< Get idle rate */
#define HID_CMD_SET_IDLE            0x05    /**< Set idle rate */
#define HID_CMD_GET_PROTOCOL        0x06    /**< Get protocol version */
#define HID_CMD_SET_PROTOCOL        0x07    /**< Set protocol version */
#define HID_CMD_SET_POWER           0x08    /**< Set power state */

/* Power States */
#define HID_PWR_ON                  0x00    /**< Normal operating state */
#define HID_PWR_SLEEP               0x01    /**< Sleep/low-power state */

/* HID Report Types (for GET/SET REPORT commands) */
#define HID_REPORT_TYPE_INPUT       0x01    /**< Input report */
#define HID_REPORT_TYPE_OUTPUT      0x02    /**< Output report */
#define HID_REPORT_TYPE_FEATURE     0x03    /**< Feature report */

/* Protocol version */
#define HID_PROTOCOL_REPORT         0x00    /**< Report protocol */
#define HID_PROTOCOL_BOOT           0x01    /**< Boot protocol */

/* HID Descriptor - 30 bytes, LE format */
struct i2c_hid_desc {
    __le16  wHIDDescLength;         /* Offset 0-1:  30 (0x1E) */
    __le16  bcdVersion;              /* Offset 2-3:  1.11 (0x0101) */
    __le16  wReportDescLength;      /* Offset 4-5:  Report descriptor length */
    __le16  wReportDescRegister;    /* Offset 6-7:  Report desc register address */
    __le16  wInputRegister;          /* Offset 8-9:  Input register address */
    __le16  wMaxInputLength;         /* Offset 10-11: Max input report length */
    __le16  wOutputRegister;         /* Offset 12-13: Output register address */
    __le16  wMaxOutputLength;        /* Offset 14-15: Max output report length */
    __le16  wCommandRegister;        /* Offset 16-17: Command register address */
    __le16  wDataRegister;           /* Offset 18-19: Data register address */
    __le16  wVendorID;              /* Offset 20-21: Vendor ID */
    __le16  wProductID;             /* Offset 22-23: Product ID */
    __le16  wVersionID;             /* Offset 24-25: Version ID */
    __le32  reserved;               /* Offset 26-29: Reserved (must be 0) */
} __packed;

/* HID Report Descriptor - minimal keyboard device */
/* This defines a very simple HID device that FWUPD can identify */
static const uint8_t hid_report_desc[] = {
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x06,        /* Usage (Keyboard) */
    0xA1, 0x01,        /* Collection (Application) */
    0x05, 0x07,        /*   Usage Page (Key Codes) */
    0x19, 0xE0,        /*   Usage Minimum (224) - Left Control */
    0x29, 0xE7,        /*   Usage Maximum (231) - Right GUI */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x01,        /*   Logical Maximum (1) */
    0x75, 0x01,        /*   Report Size (1) */
    0x95, 0x08,        /*   Report Count (8) */
    0x81, 0x02,        /*   Input (Data, Variable, Absolute) - Modifier byte */
    0x95, 0x01,        /*   Report Count (1) */
    0x75, 0x08,        /*   Report Size (8) */
    0x81, 0x01,        /*   Input (Constant) - Reserved byte */
    0x95, 0x05,        /*   Report Count (5) */
    0x75, 0x01,        /*   Report Size (1) */
    0x05, 0x08,        /*   Usage Page (LEDs) */
    0x19, 0x01,        /*   Usage Minimum (1) */
    0x29, 0x05,        /*   Usage Maximum (5) */
    0x91, 0x02,        /*   Output (Data, Variable, Absolute) */
    0x95, 0x01,        /*   Report Count (1) */
    0x75, 0x03,        /*   Report Size (3) */
    0x91, 0x01,        /*   Output (Constant) */
    0x95, 0x06,        /*   Report Count (6) */
    0x75, 0x08,        /*   Report Size (8) */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x65,        /*   Logical Maximum (101) */
    0x05, 0x07,        /*   Usage Page (Key Codes) */
    0x19, 0x00,        /*   Usage Minimum (0) */
    0x29, 0x65,        /*   Usage Maximum (101) */
    0x81, 0x00,        /*   Input (Data, Array) - Key array */
    0xC0,             /* End Collection */
};

/* Device state structure */
struct vhid_i2c_device {
    int                     fd;             /* I2C device file descriptor */
    uint8_t                 slave_addr;     /* I2C slave address */
    uint8_t                 power_state;   /* Current power state */
    uint8_t                 protocol;      /* Current protocol (report/boot) */
    uint8_t                 idle_rate;     /* Current idle rate */
    uint8_t                 report_id;     /* Current report ID */
    bool                    initialized;   /* Device initialized flag */
    uint8_t                 input_buf[64]; /* Input report buffer */
    uint8_t                 output_buf[64];/* Output report buffer */
};

/* Function prototypes */

/**
 * Initialize the virtual HID I2C device
 * @param dev Device structure to initialize
 * @param i2c_adapter I2C adapter number
 * @param slave_addr I2C slave address
 * @return 0 on success, negative errno on failure
 */
int vhid_i2c_init(struct vhid_i2c_device *dev, int i2c_adapter, uint8_t slave_addr);

/**
 * Open I2C bus and set slave address
 * @param dev Device structure
 * @return 0 on success, negative errno on failure
 */
int vhid_i2c_open(struct vhid_i2c_device *dev);

/**
 * Close I2C bus connection
 * @param dev Device structure
 */
void vhid_i2c_close(struct vhid_i2c_device *dev);

/**
 * Write HID descriptor to the device
 * This programs the "device" registers via i2c-stub
 * @param dev Device structure
 * @return 0 on success, negative errno on failure
 */
int vhid_i2c_write_hid_descriptor(struct vhid_i2c_device *dev);

/**
 * Write HID report descriptor to the device
 * @param dev Device structure
 * @return 0 on success, negative errno on failure
 */
int vhid_i2c_write_report_descriptor(struct vhid_i2c_device *dev);

/**
 * Handle HID command from host
 * @param dev Device structure
 * @param cmd Command opcode
 * @param report_type Report type (input/output/feature)
 * @param report_id Report ID
 * @param data Data buffer
 * @param data_len Data length
 * @return 0 on success, negative errno on failure
 */
int vhid_i2c_handle_command(struct vhid_i2c_device *dev, uint8_t cmd,
                            uint8_t report_type, uint8_t report_id,
                            uint8_t *data, int data_len);

/**
 * Read data from a register on the device
 * @param dev Device structure
 * @param reg Register address
 * @param buf Buffer to store data
 * @param len Number of bytes to read
 * @return Number of bytes read, negative errno on failure
 */
int vhid_i2c_read_register(struct vhid_i2c_device *dev, uint8_t reg, uint8_t *buf, int len);

/**
 * Write data to a register on the device
 * @param dev Device structure
 * @param reg Register address
 * @param data Data to write
 * @param len Number of bytes to write
 * @return 0 on success, negative errno on failure
 */
int vhid_i2c_write_register(struct vhid_i2c_device *dev, uint8_t reg, const uint8_t *data, int len);

/**
 * Simulate an interrupt from the device (send input report)
 * In a real device, this would trigger the host to read via IRQ
 * Here we just store the data for demonstration
 * @param dev Device structure
 * @param report Report data
 * @param len Report length
 */
void vhid_i2c_trigger_input_report(struct vhid_i2c_device *dev, const uint8_t *report, int len);

/**
 * Reset the device to initial state
 * @param dev Device structure
 */
void vhid_i2c_reset(struct vhid_i2c_device *dev);

/**
 * Get device info string
 * @return Static string with device information
 */
const char *vhid_i2c_get_device_info(void);

/* Utility functions */
uint16_t cpu_to_le16(uint16_t val);
uint32_t cpu_to_le32(uint32_t val);

#endif /* HID_I2C_PROTOCOL_H */

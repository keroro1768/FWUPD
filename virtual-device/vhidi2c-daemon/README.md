# Virtual HID over I2C Daemon (vhidi2c-daemon)

Userspace daemon that simulates a HID over I2C device using Linux `i2c-stub`.
This allows FWUPD Plugin to enumerate and communicate with a virtual I2C HID device for testing.

## Overview

This daemon implements the **HID over I2C** protocol (HID 1.11 specification) in userspace,
using the Linux `i2c-stub` kernel module to provide a virtual I2C bus.

```
┌─────────────────────────────────────────────────────────┐
│  i2c-stub (kernel module)                              │
│  - Simulates I2C EEPROM/chip                           │
│  - Provides virtual registers                           │
└─────────────────────┬───────────────────────────────────┘
                      │ I2C Bus (/dev/i2c-0)
                      │
┌─────────────────────▼───────────────────────────────────┐
│  vhidi2c-daemon (userspace)                             │
│  - Programs HID descriptor into i2c-stub                │
│  - Programs HID report descriptor                       │
│  - Responds to HID commands (GET/SET_REPORT, etc.)     │
│  - Maintains device state                               │
└─────────────────────────────────────────────────────────┘
```

## Architecture

### Files

```
vhidi2c-daemon/
├── vhidi2c-daemon.c     # Main daemon entry point
├── hid-i2c-protocol.c   # HID over I2C protocol implementation
├── hid-i2c-protocol.h   # Header with structures and function declarations
├── Makefile             # Build configuration
└── README.md            # This file
```

### HID Descriptor (30 bytes)

The virtual device provides a standard HID descriptor:

| Field | Value | Description |
|-------|-------|-------------|
| wHIDDescLength | 30 | Descriptor size |
| bcdVersion | 0x0101 | HID 1.11 |
| wReportDescLength | varies | Report descriptor size |
| wVendorID | 0x1234 | Virtual Vendor ID |
| wProductID | 0x5678 | Virtual Product ID |
| wVersionID | 0x0001 | Version 0.0.1 |

### Supported HID Commands

| Opcode | Command | Description |
|--------|---------|-------------|
| 0x01 | RESET | Reset device to initial state |
| 0x02 | GET_REPORT | Host requests a report |
| 0x03 | SET_REPORT | Host sends a report |
| 0x04 | GET_IDLE | Get idle rate |
| 0x05 | SET_IDLE | Set idle rate |
| 0x06 | GET_PROTOCOL | Get protocol version |
| 0x07 | SET_PROTOCOL | Set protocol version |
| 0x08 | SET_POWER | Set power state |

## Build

### Prerequisites

**Debian/Ubuntu:**
```bash
sudo apt-get install build-essential libi2c-dev i2c-tools
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc make i2c-tools-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel i2c-tools
```

### Compile

```bash
# Debug build (default)
make

# Release build
make release

# Clean
make clean
```

## Usage

### Step 1: Load Kernel Modules

```bash
# Load i2c-dev (provides /dev/i2c-* devices)
sudo modprobe i2c-dev

# Load i2c-stub with the desired slave address
sudo modprobe i2c-stub chip_addr=0x2A

# Verify i2c-stub is loaded
cat /sys/bus/i2c/devices/i2c-0/name
# Should show: i2c-stub ((driver unset))

# Or check with i2cdetect
sudo i2cdetect -l
```

### Step 2: Set Permissions (if not running as root)

```bash
# Add user to i2c group
sudo usermod -aG i2c $USER

# Or set device permissions
sudo chmod 666 /dev/i2c-0
```

### Step 3: Run the Daemon

```bash
# Basic usage (default: bus=0, addr=0x2A)
sudo ./vhidi2c-daemon

# Specify custom address and bus
sudo ./vhidi2c-daemon -a 0x2C -b 1

# With debug output (compile with -DDEBUG first)
sudo DEBUG=1 ./vhidi2c-daemon -d
```

### Step 4: Verify Operation

```bash
# Check that the device address is now "UU" (occupied by driver)
sudo i2cdetect -y 0
#      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
# 00:                         -- -- -- -- -- -- -- -- --
# 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 70: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

# Note: i2c-stub doesn't show as UU until a master accesses it
# The daemon programs registers via smbus calls

# Read HID descriptor from the virtual device
sudo i2cget -y 0 0x2A 0x00 w
# Should return the HID descriptor length (0x1E00 = 30)
```

## Testing with FWUPD

Once the daemon is running, you can test with FWUPD:

```bash
# Check if FWUPD can enumerate the device
sudo fwupdmgr get-devices

# Or use hid-info from hid-tools
sudo hid-info /dev/hidraw0
```

## Troubleshooting

### "Failed to open /dev/i2c-0: No such file or directory"

i2c-dev module not loaded:
```bash
sudo modprobe i2c-dev
ls -la /dev/i2c-*
```

### "Failed to set slave address"

i2c-stub not loaded or wrong address:
```bash
sudo modprobe i2c-stub chip_addr=0x2A
dmesg | grep i2c-stub
```

### "Permission denied" errors

```bash
sudo usermod -aG i2c $USER
# Log out and back in, then retry
```

### Daemon exits immediately

Check if another process is using the address:
```bash
sudo i2cdetect -y 0
# If address shows a number (not "--" or "UU"), another device exists
```

## Development

### Debug Build

```bash
make clean
make debug
./vhidi2c-daemon -d
```

### Adding Custom HID Reports

Edit `hid-i2c-protocol.c` and modify the `hid_report_desc[]` array
to define your custom HID report format.

### Protocol Extension

To add new commands, edit:
1. `hid-i2c-protocol.h`: Add new `HID_CMD_*` opcode
2. `hid-i2c-protocol.c`: Add case in `vhid_i2c_handle_command()`

## Limitations

- This is a **Phase 1** implementation using i2c-stub
- The device is **passive** - the host must initiate all communication
- Real interrupt-driven behavior is not fully simulated
- **Command handling limitation:** Since i2c-stub simulates a passive EEPROM, the daemon cannot receive HID commands (RESET/SET_REPORT/GET_REPORT) from the i2c-hid kernel driver. This design is **only suitable for testing FWUPD Plugin enumeration**. For full command handling, a **Phase 2 kernel module** is required.
- For full HID over I2C compliance, a **Phase 2 kernel module** is recommended

## References

- [HID over I2C Protocol Specification (Microsoft)](https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/hid-over-i2c-guide)
- [Linux i2c-hid driver](https://www.kernel.org/doc/Documentation/hid/i2c-hid.txt)
- [Linux i2c-stub documentation](https://www.kernel.org/doc/Documentation/i2c/i2c-stub)

## License

GPL-2.0-or-later

## Authors

Keroro Squad - Virtual Device Team

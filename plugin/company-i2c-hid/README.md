# FWUPD Plugin: company-i2c-hid

Company HID I2C Device Firmware Update Plugin for fwupd

## Overview

This plugin provides firmware update support for Company HID I2C devices through the fwupd (Firmware Update Daemon) framework.

## Features

- **HID Interface Support**: Fast firmware updates via the standard HID interface
- **I2C Recovery Fallback**: Reliable recovery via raw I2C when HID updates fail
- **Bootloader Mode Switching**: Automatic transition between runtime and bootloader modes
- **Multi-level GUID Matching**: Flexible device identification via instance IDs
- **Firmware Verification**: Checksum-based write verification

## Device Support

This plugin supports HID I2C devices with the following characteristics:

- HID-based communication (hidraw)
- Raw I2C access for recovery (i2c-dev)
- Custom firmware format with header containing version, size, and checksum

## Files

```
company-i2c-hid/
├── meson.build                      # Meson build configuration
├── fu-company-i2c-hid-plugin.h       # Plugin header
├── fu-company-i2c-hid-plugin.c       # Plugin implementation
├── fu-company-i2c-hid-device.h       # Device header
├── fu-company-i2c-hid-device.c       # Device implementation
├── fu-company-i2c-hid-firmware.h     # Firmware format header
├── fu-company-i2c-hid-firmware.c     # Firmware format implementation
├── fu-company-i2c-hid.quirk          # Device quirks
└── README.md                        # This file
```

## Protocol ID

`com.company.i2c-hid`

## GUID Patterns

The plugin generates GUIDs in the following patterns:

```
# Standard HID DeviceInstanceId
HIDRAW\VEN_COMPANY&PID_XXXX

# IC-specific matching
COMPANY_I2C\ICTYPE_XX

# Module-specific matching
COMPANY_I2C\ICTYPE_XX&MOD_XXXX

# Driver-specific (HID vs I2C)
COMPANY_I2C\ICTYPE_XX&DRIVER_HID
COMPANY_I2C\ICTYPE_XX&DRIVER_I2C
```

## Quirk Keys

| Key | Type | Description |
|-----|------|-------------|
| `I2cSlaveAddress` | Hex | I2C slave address (default: 0x2A) |
| `CompanyI2cHidIcType` | Hex | IC type identifier |
| `CompanyI2cHidModuleId` | Hex | Module identifier |
| `CompanyI2cHidIapPassword` | Hex | IAP (In-Application Programming) password |
| `CompanyI2cHidPageCount` | Hex | Flash page count |
| `CompanyI2cHidBlockSize` | Hex | Flash block size |
| `CompanyI2cHidBoardName` | String | Board name for identification |
| `QuirkCompanyI2cHidDriver` | String | Driver type: "HID" or "I2C" |

## Firmware Format

The plugin uses a custom binary firmware format:

```
+-------------------+
| Header (16 bytes) |
+-------------------+
| Magic   (4 bytes)| 0x434F4D50 ("COMP")
| Version (4 bytes) | Firmware version
| Size    (4 bytes) | Firmware data size
| Checksum(4 bytes) | CRC32 checksum
+-------------------+
| Firmware Data     |
+-------------------+
```

## Building

This plugin is built using Meson:

```bash
cd fwupd
meson setup build
ninja -C build
```

## Installation

After building, the plugin and quirks file are installed to:

- Plugin: `/usr/lib/fwupd/plugins/company-i2c-hid/`
- Quirks: `/usr/share/fwupd/quirks.d/`

## udev Rules

For non-root access to HID I2C devices, add udev rules:

```bash
# /etc/udev/rules.d/99-company-i2c-hid.rules

# HID interface access
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="company", MODE="0664", GROUP="plugdev", TAG+="uaccess"

# I2C interface access
SUBSYSTEM=="i2c-dev", ATTRS{idVendor}=="company", MODE="0664", GROUP="plugdev", TAG+="uaccess"
```

Reload udev rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Usage

### List devices

```bash
fwupdmgr get-devices
```

### Get device updates

```bash
fwupdmgr get-updates
```

### Update firmware

```bash
fwupdmgr install firmware.cab
```

### Update from LVFS (when published)

```bash
fwupdmgr refresh
fwupdmgr update
```

## Development

### Reference Plugins

- **elantp**: Elan TouchPad plugin - Best reference for HID + I2C dual-mode
- **hidpp**: Logitech HID++ plugin - HID communication patterns
- **cros-ec**: ChromeOS EC plugin - Custom protocol design

### Testing

Without actual hardware, the plugin can be tested using:

1. Mock devices via config files
2. Unit tests in the fwupd test suite
3. Manual testing with real hardware

### Debugging

Enable debug output:

```bash
FWUPD_LOGLEVEL=debug fwupdmgr get-devices
```

Or in fwupd.conf:

```ini
[Daemon]
LogLevel=debug
```

## Contributing

This plugin follows the fwupd contribution guidelines:

1. Open an issue to discuss the design
2. Follow the coding style (Linux kernel style)
3. Include tests
4. Sign off commits (DCO)
5. Submit pull request

## License

LGPL-2.1+

## References

- [fwupd Project](https://github.com/fwupd/fwupd)
- [fwupd Plugin Documentation](https://fwupd.github.io/libfwupdplugin/)
- [LVFS (Linux Vendor Firmware Service)](https://fwupd.org/lvfs)
- [fwupd Plugin Development Guide](./doc/I2C_HID_Company_Product/01_Plugin_Development/Plugin_Guide.md)

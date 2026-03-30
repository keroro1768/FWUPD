# meson.build — FWUPD Plugin: company-i2c-hid
#
# This plugin provides firmware update support for Company HID I2C devices.
# It follows the elantp plugin design pattern with HID + I2C dual-mode support.
#
# Protocol ID: com.company.i2c-hid

plugin_define('company-i2c-hid', sources: [
  'fu-company-i2c-hid-plugin.c',
  'fu-company-i2c-hid-device.c',
  'fu-company-i2c-hid-firmware.c',
], dependencies: [
  dependency('libfwupd', version: '>= 1.0.0'),
  dependency('libusb-1.0'),
  dependency('libudev'),
])

# Add quirk file to be installed
install_data('fu-company-i2c-hid.quirk',
  install_dir: 'lib/fwupd/plugins/company-i2c-hid/quirks',
  rename: 'company-i2c-hid.quirk')

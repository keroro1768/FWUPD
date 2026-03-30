#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# i2c-stub Setup Script
# Configures Linux i2c-stub kernel module for virtual HID device
#
# Usage:
#   sudo ./setup.sh [chip_addr]
#
# Default chip address: 0x2A
#
# Copyright (C) 2026 Keroro Squad

set -e

# Default I2C slave address
DEFAULT_CHIP_ADDR="0x2A"
CHIP_ADDR="${1:-$DEFAULT_CHIP_ADDR}"

# I2C bus to use
I2C_BUS="0"

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Check if module is loaded
check_module() {
    local module="$1"
    if lsmod | grep -q "^${module}"; then
        return 0
    else
        return 1
    fi
}

# Load kernel modules
load_modules() {
    log_info "Loading kernel modules..."

    # Load i2c-dev
    if ! check_module i2c_dev; then
        log_info "  Loading i2c-dev..."
        if modprobe i2c-dev; then
            log_info "  i2c-dev loaded successfully"
        else
            log_error "  Failed to load i2c-dev"
            exit 1
        fi
    else
        log_info "  i2c-dev already loaded"
    fi

    # Unload existing i2c-stub if any (with different address)
    if check_module i2c_stub; then
        log_warn "  i2c-stub already loaded, unloading..."
        modprobe -r i2c-stub 2>/dev/null || true
        sleep 1
    fi

    # Load i2c-stub with specified address
    log_info "  Loading i2c-stub with chip_addr=${CHIP_ADDR}..."
    if modprobe i2c-stub chip_addr="${CHIP_ADDR}"; then
        log_info "  i2c-stub loaded successfully at ${CHIP_ADDR}"
    else
        log_error "  Failed to load i2c-stub"
        log_error "  Check if i2c-stub is available in your kernel"
        exit 1
    fi
}

# Verify i2c devices
verify_devices() {
    log_info "Verifying I2C devices..."

    # Check /dev/i2c-* exists
    if [[ -e "/dev/i2c-${I2C_BUS}" ]]; then
        log_info "  /dev/i2c-${I2C_BUS} exists"
    else
        log_error "  /dev/i2c-${I2C_BUS} not found!"
        exit 1
    fi

    # Check i2c-stub in sysfs
    local stub_path="/sys/bus/i2c/devices/i2c-${I2C_BUS}/name"
    if [[ -e "$stub_path" ]]; then
        local stub_name
        stub_name=$(cat "$stub_path")
        log_info "  i2c-${I2C_BUS} name: $stub_name"
    else
        log_warn "  Cannot read i2c-stub name from sysfs"
    fi
}

# Set permissions
set_permissions() {
    log_info "Setting device permissions..."

    # Make device readable/writable by all (for testing)
    # In production, restrict to i2c group
    chmod 666 /dev/i2c-${I2C_BUS} 2>/dev/null || true
    log_info "  Permissions set on /dev/i2c-${I2C_BUS}"
}

# Test i2c access
test_access() {
    log_info "Testing I2C access..."

    # Check if i2cdetect is available
    if ! command -v i2cdetect &> /dev/null; then
        log_warn "  i2c-tools not installed, skipping i2cdetect test"
        return
    fi

    # List I2C buses
    log_info "  Available I2C buses:"
    i2cdetect -l 2>/dev/null || true

    # Try to detect our device
    log_info "  Scanning for device at ${CHIP_ADDR}..."
    # Note: i2cdetect may show "--" because i2c-stub doesn't claim the address
    # until a master actually accesses it. The daemon will handle this.
    i2cdetect -y ${I2C_BUS} 2>/dev/null || true
}

# Show usage instructions
show_instructions() {
    echo ""
    echo "========================================"
    echo "  i2c-stub Setup Complete!"
    echo "========================================"
    echo ""
    echo "Device: /dev/i2c-${I2C_BUS}"
    echo "Slave Address: ${CHIP_ADDR}"
    echo ""
    echo "Next steps:"
    echo "  1. Build the daemon:"
    echo "       cd ../vhidi2c-daemon"
    echo "       make"
    echo ""
    echo "  2. Run the daemon:"
    echo "       sudo ./vhidi2c-daemon -a ${CHIP_ADDR} -b ${I2C_BUS}"
    echo ""
    echo "  3. Test with i2c-tools:"
    echo "       sudo i2cget -y ${I2C_BUS} ${CHIP_ADDR} 0x00 w"
    echo ""
    echo "To unload i2c-stub:"
    echo "  sudo modprobe -r i2c-stub"
    echo ""
}

# Main
main() {
    echo ""
    echo "========================================"
    echo "  i2c-stub Setup Script"
    echo "========================================"
    echo ""

    check_root
    load_modules
    verify_devices
    set_permissions
    test_access
    show_instructions
}

main "$@"

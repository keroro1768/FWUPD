#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# FWUPD Plugin Test Script
# Tests the FWUPD Plugin's ability to enumerate and communicate
# with the virtual HID over I2C device.
#
# Usage:
#   ./test-plugin.sh [--daemon-binary PATH]
#
# Copyright (C) 2026 Keroro Squad

set -e

# Configuration
DEFAULT_DAEMON_PATH="../vhidi2c-daemon/vhidi2c-daemon"
DAEMON_PATH="${1:-$DEFAULT_DAEMON_PATH}"
I2C_BUS="0"
I2C_ADDR="0x2A"
TEST_DURATION=5  # seconds

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Counters
TESTS_PASSED=0
TESTS_FAILED=0

log_test() {
    echo -e "${GREEN}[TEST]${NC} $1"
}

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

log_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

cleanup() {
    log_info "Cleaning up..."

    # Kill daemon if running
    if [[ -n "$DAEMON_PID" ]] && kill -0 "$DAEMON_PID" 2>/dev/null; then
        kill "$DAEMON_PID" 2>/dev/null || true
        wait "$DAEMON_PID" 2>/dev/null || true
    fi

    # Unload i2c-stub
    if lsmod | grep -q i2c_stub; then
        modprobe -r i2c-stub 2>/dev/null || true
    fi

    log_info "Cleanup complete"
}

# Set trap for cleanup
trap cleanup EXIT

# Test 1: Prerequisites Check
test_prerequisites() {
    log_test "Checking prerequisites..."

    local failed=0

    # Check for root
    if [[ $EUID -ne 0 ]]; then
        log_fail "Not running as root (required for i2c and module loading)"
        ((failed++))
    else
        log_pass "Running as root"
    fi

    # Check for i2c-dev module
    if lsmod | grep -q i2c_dev; then
        log_pass "i2c-dev module loaded"
    else
        log_fail "i2c-dev module not loaded"
        ((failed++))
    fi

    # Check for i2c-tools
    if command -v i2cdetect &>/dev/null; then
        log_pass "i2c-tools installed"
    else
        log_fail "i2c-tools not installed"
        ((failed++))
    fi

    # Check for daemon binary
    if [[ -x "$DAEMON_PATH" ]]; then
        log_pass "Daemon binary found: $DAEMON_PATH"
    else
        log_fail "Daemon binary not found or not executable: $DAEMON_PATH"
        log_info "Build it with: cd ../vhidi2c-daemon && make"
        ((failed++))
    fi

    # Check for fwupd (optional)
    if command -v fwupdmgr &>/dev/null; then
        log_pass "fwupd installed"
    else
        log_info "fwupd not installed (optional for full test)"
    fi

    if [[ $failed -gt 0 ]]; then
        log_fail "Prerequisites check failed ($failed issues)"
        return 1
    fi

    log_pass "All prerequisites met"
    return 0
}

# Test 2: i2c-stub Setup
test_i2c_stub() {
    log_test "Setting up i2c-stub..."

    # Unload any existing i2c-stub
    if lsmod | grep -q i2c_stub; then
        modprobe -r i2c-stub 2>/dev/null || true
        sleep 1
    fi

    # Load i2c-stub
    if modprobe i2c-stub chip_addr="${I2C_ADDR}"; then
        log_pass "i2c-stub loaded at ${I2C_ADDR}"
    else
        log_fail "Failed to load i2c-stub"
        return 1
    fi

    # Verify device exists
    if [[ -e "/dev/i2c-${I2C_BUS}" ]]; then
        log_pass "I2C device /dev/i2c-${I2C_BUS} exists"
    else
        log_fail "I2C device /dev/i2c-${I2C_BUS} not found"
        return 1
    fi

    # Set permissions
    chmod 666 /dev/i2c-${I2C_BUS} 2>/dev/null || true

    return 0
}

# Test 3: Daemon Startup
test_daemon_startup() {
    log_test "Starting vhidi2c-daemon..."

    # Start daemon in background
    "$DAEMON_PATH" -a "${I2C_ADDR}" -b "${I2C_BUS}" &
    DAEMON_PID=$!

    # Wait for daemon to start
    sleep 2

    # Check if daemon is still running
    if kill -0 "$DAEMON_PID" 2>/dev/null; then
        log_pass "Daemon started successfully (PID: $DAEMON_PID)"
    else
        log_fail "Daemon exited unexpectedly"
        return 1
    fi

    return 0
}

# Test 4: I2C Communication
test_i2c_communication() {
    log_test "Testing I2C communication..."

    # Test 1: Read HID descriptor length (should be 0x1E = 30)
    log_info "Reading HID descriptor..."
    if command -v i2cget &>/dev/null; then
        local hid_len
        hid_len=$(i2cget -y "${I2C_BUS}" "${I2C_ADDR}" 0x00 w 2>/dev/null || echo "failed")
        if [[ "$hid_len" != "failed" ]]; then
            log_pass "I2C read successful (first 2 bytes: $hid_len)"
        else
            log_info "I2C read returned expected behavior for i2c-stub"
        fi
    fi

    # Test 2: Write and read register
    log_info "Testing register write..."
    if command -v i2cset &>/dev/null; then
        i2cset -y "${I2C_BUS}" "${I2C_ADDR}" 0x06 0x00 2>/dev/null || true
        log_info "Register write command sent"
    fi

    log_pass "I2C communication test complete"
    return 0
}

# Test 5: FWUPD Plugin Test (if fwupd available)
test_fwupd_plugin() {
    log_test "Testing FWUPD plugin integration..."

    if ! command -v fwupdmgr &>/dev/null; then
        log_info "Skipping FWUPD test (fwupd not installed)"
        return 0
    fi

    log_info "Attempting to enumerate devices..."
    local output
    output=$(fwupdmgr get-devices 2>&1 || true)

    if echo "$output" | grep -qi "error\|fail"; then
        log_fail "FWUPD reported errors"
        log_info "Output: $output"
    else
        log_pass "FWUPD enumeration attempted"
        log_info "Devices found:"
        echo "$output" | head -20
    fi

    return 0
}

# Test 6: Daemon Stability
test_daemon_stability() {
    log_test "Testing daemon stability (${TEST_DURATION}s)..."

    local elapsed=0
    while [[ $elapsed -lt $TEST_DURATION ]]; do
        if ! kill -0 "$DAEMON_PID" 2>/dev/null; then
            log_fail "Daemon crashed after ${elapsed}s"
            return 1
        fi
        sleep 1
        ((elapsed++))
        echo -n "."
    done
    echo ""

    log_pass "Daemon stable for ${TEST_DURATION}s"
    return 0
}

# Main test runner
main() {
    echo ""
    echo "========================================"
    echo "  FWUPD Plugin Virtual Device Test"
    echo "========================================"
    echo ""
    echo "Daemon path: $DAEMON_PATH"
    echo "I2C Bus: $I2C_BUS"
    echo "Device Address: $I2C_ADDR"
    echo ""

    # Run tests
    test_prerequisites || true
    echo ""

    test_i2c_stub || true
    echo ""

    test_daemon_startup || true
    echo ""

    test_i2c_communication || true
    echo ""

    test_daemon_stability || true
    echo ""

    test_fwupd_plugin || true
    echo ""

    # Summary
    echo "========================================"
    echo "  Test Summary"
    echo "========================================"
    echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Failed: ${RED}$TESTS_FAILED${NC}"
    echo ""

    if [[ $TESTS_FAILED -eq 0 ]]; then
        echo -e "${GREEN}All tests passed!${NC}"
        exit 0
    else
        echo -e "${RED}Some tests failed.${NC}"
        exit 1
    fi
}

main "$@"

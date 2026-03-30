/* SPDX-License-Identifier: GPL-2.0-or-later */
/**
 * Virtual HID over I2C Daemon (vhidi2c-daemon)
 *
 * Userspace daemon that simulates a HID over I2C device using i2c-stub.
 * This allows FWUPD Plugin to enumerate and communicate with a virtual
 * I2C HID device for testing purposes.
 *
 * Copyright (C) 2026 Keroro Squad
 *
 * Usage:
 *   ./vhidi2c-daemon [-a addr] [-b bus] [-d] [-h]
 *
 * Prerequisites:
 *   1. Load i2c-dev kernel module:  modprobe i2c-dev
 *   2. Load i2c-stub with address:  modprobe i2c-stub chip_addr=0x2A
 *   3. Run as root or have i2c permissions
 */

#include "hid-i2c-protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

/* Global running flag for signal handling */
static volatile bool g_running = true;

/**
 * Signal handler for graceful shutdown
 */
static void signal_handler(int sig)
{
    (void)sig;
    fprintf(stdout, "\n[Vhid-I2C] Received signal, shutting down...\n");
    g_running = false;
}

/**
 * Print usage information
 */
static void print_usage(const char *prog_name)
{
    printf("Usage: %s [options]\n", prog_name);
    printf("\nOptions:\n");
    printf("  -a <addr>   I2C slave address (default: 0x%02X)\n", VHID_I2C_SLAVE_ADDR);
    printf("  -b <bus>    I2C bus number (default: %d)\n", VHID_I2C_ADAPTER);
    printf("  -d          Enable debug output\n");
    printf("  -h          Show this help message\n");
    printf("\nExample:\n");
    printf("  %s -a 0x2A -b 0    # Connect to i2c-0 at address 0x2A\n", prog_name);
    printf("\nPrerequisites:\n");
    printf("  1. Load i2c-dev:  modprobe i2c-dev\n");
    printf("  2. Load i2c-stub: modprobe i2c-stub chip_addr=0x%02X\n", VHID_I2C_SLAVE_ADDR);
    printf("  3. Run as root or add user to 'i2c' group\n");
}

/**
 * Main entry point
 */
int main(int argc, char *argv[])
{
    struct vhid_i2c_device vhid_dev;
    int opt;
    int i2c_bus = VHID_I2C_ADAPTER;
    uint8_t slave_addr = VHID_I2C_SLAVE_ADDR;
    int ret;

    printf("\n");
    printf("========================================\n");
    printf("  Virtual HID over I2C Daemon (vhidi2c)\n");
    printf("========================================\n");
    printf("\n");

    /* Parse command line options */
    while ((opt = getopt(argc, argv, "a:b:dh")) != -1) {
        switch (opt) {
        case 'a':
            slave_addr = (uint8_t)strtol(optarg, NULL, 0);
            break;
        case 'b':
            i2c_bus = atoi(optarg);
            break;
        case 'd':
#ifdef DEBUG
            /* Already defined */
#else
            fprintf(stderr, "[Note] Compile with -DDEBUG for debug output\n");
#endif
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    fprintf(stdout, "%s", vhid_i2c_get_device_info());
    printf("\n");

    /* Install signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize device structure */
    ret = vhid_i2c_init(&vhid_dev, i2c_bus, slave_addr);
    if (ret < 0) {
        fprintf(stderr, "[Error] Failed to initialize device: %d\n", ret);
        return 1;
    }

    /* Open I2C bus */
    ret = vhid_i2c_open(&vhid_dev);
    if (ret < 0) {
        fprintf(stderr, "\n[Error] Failed to open I2C bus. Please check:\n");
        fprintf(stderr, "  1. i2c-dev kernel module loaded: modprobe i2c-dev\n");
        fprintf(stderr, "  2. i2c-stub loaded with address: modprobe i2c-stub chip_addr=0x%02X\n", slave_addr);
        fprintf(stderr, "  3. Permission to access /dev/i2c-%d\n", i2c_bus);
        fprintf(stderr, "  4. Try running as root or: sudo chmod 666 /dev/i2c-%d\n", i2c_bus);
        return 1;
    }

    /* Program HID descriptor into the device (i2c-stub) */
    ret = vhid_i2c_write_hid_descriptor(&vhid_dev);
    if (ret < 0) {
        fprintf(stderr, "[Error] Failed to write HID descriptor: %d\n", ret);
        vhid_i2c_close(&vhid_dev);
        return 1;
    }

    /* Program HID report descriptor */
    ret = vhid_i2c_write_report_descriptor(&vhid_dev);
    if (ret < 0) {
        fprintf(stderr, "[Error] Failed to write report descriptor: %d\n", ret);
        vhid_i2c_close(&vhid_dev);
        return 1;
    }

    vhid_dev.initialized = true;

    printf("\n");
    printf("========================================\n");
    printf("  Daemon is running\n");
    printf("========================================\n");
    printf("I2C Bus:     /dev/i2c-%d\n", i2c_bus);
    printf("Slave Addr:  0x%02X\n", slave_addr);
    printf("\n");
    printf("The virtual HID device is now ready.\n");
    printf("Host can enumerate and communicate with it.\n");
    printf("\n");
    printf("Press Ctrl+C to stop.\n");
    printf("\n");
    fflush(stdout);

    /* Main loop - keep device alive and process commands
     *
     * With i2c-stub, the "device" is passive - the host drives all communication.
     * Our daemon maintains the device registers in i2c-stub.
     *
     * In a real implementation with I2C slave hardware, we would also:
     * 1. Handle I2C slave callbacks for register access
     * 2. Manage interrupts to notify host of input data
     * 3. Handle power management states
     */
    while (g_running) {
        /* Sleep in small increments to respond to signals quickly */
        sleep(1);

        /* Periodic heartbeat to demonstrate daemon is alive
         * Every 10 seconds, trigger a simulated input report (for testing) */
        static int heartbeat_counter = 0;
        heartbeat_counter++;

        if (heartbeat_counter >= 10) {
            heartbeat_counter = 0;

            /* Trigger a periodic heartbeat report (optional, for testing) */
            uint8_t heartbeat_report[3] = {0x00, 0x00, 0x01};  /* LED state */
            vhid_i2c_trigger_input_report(&vhid_dev, heartbeat_report, sizeof(heartbeat_report));
        }
    }

    printf("\nShutting down...\n");
    vhid_i2c_close(&vhid_dev);
    printf("[Info] Daemon stopped\n");
    return 0;
}

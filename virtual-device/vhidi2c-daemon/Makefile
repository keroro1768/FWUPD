# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for Virtual HID over I2C Daemon (vhidi2c-daemon)
#
# Copyright (C) 2026 Keroro Squad
#
# Build:
#   make           - Build with debug symbols
#   make release   - Build optimized release version
#   make clean     - Clean build artifacts
#
# Install (as root):
#   make install   - Install binary to /usr/local/bin
#
# Usage:
#   ./vhidi2c-daemon [-a addr] [-b bus] [-d] [-h]

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic -std=c11
CFLAGS += -D_XOPEN_SOURCE=700
LDFLAGS := -li2c

# Debug build (default)
CFLAGS_DEBUG := $(CFLAGS) -g -O0 -DDEBUG
LDFLAGS_DEBUG := $(LDFLAGS)

# Release build
CFLAGS_RELEASE := $(CFLAGS) -O2 -DNDEBUG
LDFLAGS_RELEASE := $(LDFLAGS)

# Target
TARGET := vhidi2c-daemon
TARGET_DEBUG := $(TARGET)-debug
TARGET_RELEASE := $(TARGET)

# Source files
SRC := vhidi2c-daemon.c hid-i2c-protocol.c
OBJ := $(SRC:.c=.o)

# Directories
DESTDIR ?= /usr/local/bin
MAN_DIR ?= /usr/local/share/man/man1

.PHONY: all debug release clean install uninstall help

# Default: build debug version
all: debug

debug: $(TARGET_DEBUG)
	@echo "Debug build complete: $(TARGET_DEBUG)"
	@echo "Run with: ./$(TARGET_DEBUG) -d"

release: $(TARGET_RELEASE)
	@echo "Release build complete: $(TARGET_RELEASE)"
	@echo "Run with: ./$(TARGET_RELEASE)"

$(TARGET_DEBUG): $(SRC)
	$(CC) $(CFLAGS_DEBUG) -o $@ $^ $(LDFLAGS_DEBUG)

$(TARGET_RELEASE): $(SRC)
	$(CC) $(CFLAGS_RELEASE) -o $@ $^ $(LDFLAGS_RELEASE)

# Pattern rule for object files
%.o: %.c hid-i2c-protocol.h
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

clean:
	rm -f $(TARGET_DEBUG) $(TARGET_RELEASE) $(OBJ)
	@echo "Cleaned build artifacts"

install: release
	install -D -m 0755 $(TARGET_RELEASE) $(DESTDIR)/$(TARGET_RELEASE)
	@echo "Installed to $(DESTDIR)/$(TARGET_RELEASE)"
	@echo ""
	@echo "To run:"
	@echo "  1. Load i2c-dev:    sudo modprobe i2c-dev"
	@echo "  2. Load i2c-stub:  sudo modprobe i2c-stub chip_addr=0x2A"
	@echo "  3. Run daemon:      sudo $(DESTDIR)/$(TARGET_RELEASE)"
	@echo ""

uninstall:
	rm -f $(DESTDIR)/$(TARGET_RELEASE)
	@echo "Uninstalled"

help:
	@echo "Virtual HID over I2C Daemon - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build debug version (default)"
	@echo "  debug     - Build with debug symbols and -DDEBUG"
	@echo "  release   - Build optimized release version"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to $(DESTDIR)"
	@echo "  uninstall - Remove installed binary"
	@echo "  help      - Show this message"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - i2c-tools development files (libi2c-dev on Debian/Ubuntu)"
	@echo "  - Linux kernel headers"
	@echo ""
	@echo "Example:"
	@echo "  make && sudo make install"

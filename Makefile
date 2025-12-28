#!/usr/bin/env make

# LOTUS-AGI TEAM
# AGI - Automated Guest Isolation
# Chroot Environment Management System
#
# Build Configuration

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pipe -static
LDFLAGS := -static-libstdc++ -static-libgcc

TARGET := agi
SRCDIR := src
OBJDIR := obj
BINDIR := bin

SOURCES := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

PREFIX ?= /usr/local
INSTALL_BINDIR := $(PREFIX)/bin
INSTALL_DATADIR := $(PREFIX)/share/agi
INSTALL_CONFDIR := $(PREFIX)/etc/agi
INSTALL_DOCDIR := $(PREFIX)/share/doc/agi

# Directories
CONFIG_DIR := config
DOCS_DIR := docs
SCRIPTS_DIR := scripts

.PHONY: all clean install uninstall check run help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"
	@echo "Dependency check:"
	@ldd $(TARGET) 2>/dev/null | grep -E "=>" | head -5 || echo "Static linking successful"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)
	@echo "Cleanup complete"

install: $(TARGET)
	@echo "Installing AGI to system..."

	# Create directories
	install -d $(INSTALL_BINDIR)
	install -d $(INSTALL_DATADIR)
	install -d $(INSTALL_CONFDIR)
	install -d $(INSTALL_DOCDIR)

	# Install binary
	install -m 755 $(TARGET) $(INSTALL_BINDIR)/$(TARGET)

	# Install configuration template
	install -m 644 $(CONFIG_DIR)/agi_config.json $(INSTALL_CONFDIR)/agi_config.json.example

	# Install documentation
	install -m 644 $(DOCS_DIR)/USER_MANUAL.md $(INSTALL_DOCDIR)/
	install -m 644 $(DOCS_DIR)/ARCHITECTURE.md $(INSTALL_DOCDIR)/

	# Install wrapper script
	install -m 755 $(SCRIPTS_DIR)/agi-wrapper.sh $(INSTALL_BINDIR)/agi

	@echo "Installation complete!"
	@echo ""
	@echo "Usage: agi --help"
	@echo "Config file: $(INSTALL_CONFDIR)/agi_config.json"

uninstall:
	@echo "Uninstalling AGI..."
	rm -f $(INSTALL_BINDIR)/$(TARGET)
	rm -f $(INSTALL_BINDIR)/agi
	rm -rf $(INSTALL_DATADIR)
	rm -f $(INSTALL_CONFDIR)/agi_config.json.example
	rm -rf $(INSTALL_DOCDIR)
	@echo "Uninstallation complete!"

check: $(TARGET)
	@echo "Running basic checks..."
	@echo ""
	@echo "=== Binary Info ==="
	@file $(TARGET)
	@echo ""
	@echo "=== Dependency Check ==="
	@ldd $(TARGET) 2>/dev/null || echo "Static linking or no dependencies"
	@echo ""
	@echo "=== Static Analysis ==="
	@nm $(TARGET) 2>/dev/null | wc -l | xargs -I {} echo "Symbol count: {}"
	@echo ""
	@echo "=== Config Validation ==="
	@./$(TARGET) --validate-config $(CONFIG_DIR)/agi_config.json 2>&1 | head -20

run: $(TARGET)
	@echo "Running AGI (test mode)..."
	@./$(TARGET) --help

help:
	@echo "AGI - Automated Guest Isolation"
	@echo "=============================="
	@echo ""
	@echo "Build targets:"
	@echo "  make          - Build AGI binary"
	@echo "  make clean    - Clean build artifacts"
	@echo "  make install  - Install to system"
	@echo "  make uninstall- Uninstall from system"
	@echo "  make check    - Run integrity checks"
	@echo "  make run      - Run AGI help"
	@echo "  make help     - Show this help"
	@echo ""
	@echo "Usage:"
	@echo "  agi init          - Initialize config"
	@echo "  agi create <name> - Create chroot environment"
	@echo "  agi start <name>  - Start environment"
	@echo "  agi stop <name>   - Stop environment"
	@echo "  agi ssh <name>    - SSH connection"
	@echo "  agi status        - View status"
	@echo "  agi list          - List all environments"
	@echo "  agi remove <name> - Remove environment"
	@echo ""
	@echo "See docs/ directory for detailed documentation"

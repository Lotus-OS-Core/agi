#!/bin/bash
################################################################################
# AGI Installation Script
# Automated Guest Isolation - Interactive Installation Program
#
# Features:
#   - Install AGI to system
#   - Uninstall AGI from system
#   - Verify installation
#
# Usage:
#   ./installer.sh install     - Install AGI
#   ./installer.sh uninstall   - Uninstall AGI
#   ./installer.sh reinstall   - Reinstall AGI
#   ./installer.sh verify      - Verify installation
#
# Author: LOTUS-AGI Team ✓
# Version: 1.0.0
# Latest Update Date: 2025-12-28
################################################################################

# Color Definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# Configuration Variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY_NAME="agi"
BINARY_PATH="${SCRIPT_DIR}/${BINARY_NAME}"
INSTALL_PREFIX="/usr/local"
INSTALL_BINDIR="${INSTALL_PREFIX}/bin"
INSTALL_DATADIR="${INSTALL_PREFIX}/share/agi"
INSTALL_CONFDIR="${INSTALL_PREFIX}/etc/agi"
INSTALL_DOCDIR="${INSTALL_PREFIX}/share/doc/agi"

# Log File
LOG_FILE="/tmp/agi_install.log"

################################################################################
# Output Functions
################################################################################

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [INFO] $1" >> "${LOG_FILE}"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [WARN] $1" >> "${LOG_FILE}"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [ERROR] $1" >> "${LOG_FILE}"
}

log_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [STEP] $1" >> "${LOG_FILE}"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

################################################################################
# Check Functions
################################################################################

check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script requires root privileges to run"
        echo "Please use: sudo $0 $1"
        exit 1
    fi
}

check_dependencies() {
    log_step "Checking system dependencies..."
    
    local missing_deps=()
    
    # Check required commands
    for cmd in g++ make cmake; do
        if ! command -v $cmd &> /dev/null; then
            missing_deps+=("$cmd")
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_warn "Missing build tools: ${missing_deps[*]}"
        log_info "Please install them before running this script"
        echo "Debian/Ubuntu: apt-get install build-essential cmake"
        echo "RHEL/CentOS: yum groupinstall 'Development Tools'"
        return 1
    fi
    
    log_success "Dependency check passed"
    return 0
}

check_binary() {
    if [[ ! -f "${BINARY_PATH}" ]]; then
        log_error "AGI binary not found: ${BINARY_PATH}"
        log_info "Please compile first: make"
        return 1
    fi
    
    if [[ ! -x "${BINARY_PATH}" ]]; then
        log_error "Binary file is not executable"
        return 1
    fi
    
    log_success "Binary file check passed"
    return 0
}

check_installed() {
    if [[ -f "${INSTALL_BINDIR}/${BINARY_NAME}" ]]; then
        return 0
    fi
    return 1
}

################################################################################
# Installation Functions
################################################################################

do_install() {
    echo ""
    echo -e "${WHITE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${WHITE}║                    AGI Installer                            ║${NC}"
    echo -e "${WHITE}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    log_step "Starting AGI installation..."
    
    # Check dependencies
    check_dependencies || exit 1
    check_binary || exit 1
    
    # If already installed, prompt user
    if check_installed; then
        log_warn "AGI is already installed on the system"
        read -p "Do you want to reinstall? (y/n): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            log_info "Installation cancelled"
            exit 0
        fi
        do_uninstall
    fi
    
    # Create directories
    log_step "Creating installation directories..."
    mkdir -p "${INSTALL_BINDIR}"
    mkdir -p "${INSTALL_DATADIR}"
    mkdir -p "${INSTALL_CONFDIR}"
    mkdir -p "${INSTALL_DOCDIR}"
    mkdir -p "/var/log/agi"
    mkdir -p "/var/lib/agi/jails"
    
    # Install binary
    log_step "Installing binary file..."
    cp "${BINARY_PATH}" "${INSTALL_BINDIR}/${BINARY_NAME}"
    chmod 755 "${INSTALL_BINDIR}/${BINARY_NAME}"
    
    # Install configuration template
    log_step "Installing configuration template..."
    if [[ -f "${SCRIPT_DIR}/config/agi_config.json" ]]; then
        cp "${SCRIPT_DIR}/config/agi_config.json" "${INSTALL_CONFDIR}/agi_config.json.example"
        chmod 644 "${INSTALL_CONFDIR}/agi_config.json.example"
    fi
    
    # Install documentation
    log_step "Installing documentation..."
    if [[ -f "${SCRIPT_DIR}/docs/USER_MANUAL.md" ]]; then
        cp "${SCRIPT_DIR}/docs/USER_MANUAL.md" "${INSTALL_DOCDIR}/"
        chmod 644 "${INSTALL_DOCDIR}/USER_MANUAL.md"
    fi
    if [[ -f "${SCRIPT_DIR}/docs/ARCHITECTURE.md" ]]; then
        cp "${SCRIPT_DIR}/docs/ARCHITECTURE.md" "${INSTALL_DOCDIR}/"
        chmod 644 "${INSTALL_DOCDIR}/ARCHITECTURE.md"
    fi
    
    # Install wrapper script
    log_step "Installing command line tool..."
    if [[ -f "${SCRIPT_DIR}/scripts/agi-wrapper.sh" ]]; then
        cp "${SCRIPT_DIR}/scripts/agi-wrapper.sh" "${INSTALL_BINDIR}/agi"
        chmod 755 "${INSTALL_BINDIR}/agi"
    fi
    
    # Create symbolic link
    ln -sf "${INSTALL_BINDIR}/${BINARY_NAME}" "${INSTALL_BINDIR}/agi-bin" 2>/dev/null || true
    
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                    Installation Complete!                   ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    log_info "AGI has been successfully installed to the system"
    echo ""
    echo "Installation paths:"
    echo "  - Binary file: ${INSTALL_BINDIR}/${BINARY_NAME}"
    echo "  - Command tool: ${INSTALL_BINDIR}/agi"
    echo "  - Configuration: ${INSTALL_CONFDIR}/"
    echo "  - Data directory: ${INSTALL_DATADIR}/"
    echo "  - Documentation: ${INSTALL_DOCDIR}/"
    echo ""
    
    echo "Next steps:"
    echo "  1. Edit configuration file: ${INSTALL_CONFDIR}/agi_config.json.example"
    echo "  2. Copy as working config: cp ${INSTALL_CONFDIR}/agi_config.json.example ${INSTALL_CONFDIR}/agi_config.json"
    echo "  3. Initialize AGI: sudo agi init"
    echo "  4. View help: agi help"
    echo ""
    
    # Verify installation
    log_step "Verifying installation..."
    if "${INSTALL_BINDIR}/${BINARY_NAME}" --version &> /dev/null; then
        log_success "Installation verification passed"
        "${INSTALL_BINDIR}/${BINARY_NAME}" --version
    else
        log_warn "Installation verification failed, please check the binary file"
    fi
}

################################################################################
# Uninstallation Functions
################################################################################

do_uninstall() {
    echo ""
    echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║                    AGI Uninstaller                          ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    if ! check_installed; then
        log_warn "AGI is not installed on the system"
        exit 0
    fi
    
    log_warn "AGI is about to be uninstalled from the system"
    echo ""
    echo "This will delete:"
    echo "  - Binary file: ${INSTALL_BINDIR}/${BINARY_NAME}"
    echo "  - Command tool: ${INSTALL_BINDIR}/agi"
    echo "  - Configuration template: ${INSTALL_CONFDIR}/agi_config.json.example"
    echo "  - Documentation: ${INSTALL_DOCDIR}/"
    echo ""
    echo "Note: Runtime data and logs will not be deleted"
    echo ""
    
    read -p "Confirm uninstall? (enter 'uninstall' to confirm): " -r
    echo
    
    if [[ $REPLY != "uninstall" ]]; then
        log_info "Uninstallation cancelled"
        exit 0
    fi
    
    log_step "Starting uninstallation..."
    
    # Delete files
    rm -f "${INSTALL_BINDIR}/${BINARY_NAME}"
    rm -f "${INSTALL_BINDIR}/agi"
    rm -f "${INSTALL_BINDIR}/agi-bin"
    rm -f "${INSTALL_CONFDIR}/agi_config.json.example"
    rm -rf "${INSTALL_DATADIR}"
    rm -rf "${INSTALL_DOCDIR}"
    
    # Delete empty directories
    rmdir "${INSTALL_CONFDIR}" 2>/dev/null || true
    rmdir "${INSTALL_DATADIR}" 2>/dev/null || true
    
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                    Uninstallation Complete!                 ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    log_info "AGI has been uninstalled from the system"
    echo ""
    echo "Retained data:"
    echo "  - Runtime data: /var/lib/agi/"
    echo "  - Log files: /var/log/agi/"
    echo ""
    echo "To completely clean up, please manually delete the above directories"
}

################################################################################
# Verification Functions
################################################################################

do_verify() {
    echo ""
    echo -e "${WHITE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${WHITE}║                    AGI Verification                        ║${NC}"
    echo -e "${WHITE}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    local all_ok=true
    
    # Check binary file
    log_step "Checking binary file..."
    if [[ -x "${INSTALL_BINDIR}/${BINARY_NAME}" ]]; then
        log_success "Binary file exists: ${INSTALL_BINDIR}/${BINARY_NAME}"
        echo "  Version information:"
        "${INSTALL_BINDIR}/${BINARY_NAME}" --version 2>&1 | sed 's/^/    /'
    else
        log_error "Binary file does not exist or is not executable"
        all_ok=false
    fi
    
    # Check command tool
    log_step "Checking command tool..."
    if [[ -x "${INSTALL_BINDIR}/agi" ]]; then
        log_success "Command tool exists: ${INSTALL_BINDIR}/agi"
    else
        log_error "Command tool does not exist"
        all_ok=false
    fi
    
    # Check configuration file
    log_step "Checking configuration file..."
    if [[ -f "${INSTALL_CONFDIR}/agi_config.json.example" ]]; then
        log_success "Configuration template exists: ${INSTALL_CONFDIR}/agi_config.json.example"
    else
        log_warn "Configuration template does not exist"
    fi
    
    # Check documentation
    log_step "Checking documentation..."
    if [[ -f "${INSTALL_DOCDIR}/USER_MANUAL.md" ]]; then
        log_success "User manual exists: ${INSTALL_DOCDIR}/USER_MANUAL.md"
    else
        log_warn "User manual does not exist"
    fi
    
    # Check runtime directories
    log_step "Checking runtime directories..."
    if [[ -d "/var/lib/agi" ]]; then
        log_success "Data directory exists: /var/lib/agi"
    else
        log_warn "Data directory does not exist, will be created automatically when used"
    fi
    
    if [[ -d "/var/log/agi" ]]; then
        log_success "Log directory exists: /var/log/agi"
    else
        log_warn "Log directory does not exist, will be created automatically when used"
    fi
    
    echo ""
    if $all_ok; then
        echo -e "${GREEN}All checks passed!${NC}"
        return 0
    else
        echo -e "${YELLOW}Some checks did not pass, please see information above${NC}"
        return 1
    fi
}

################################################################################
# Reinstallation Functions
################################################################################

do_reinstall() {
    log_step "Reinstalling AGI..."
    do_uninstall
    echo ""
    sleep 1
    do_install
}

################################################################################
# Main Function
################################################################################

main() {
    # Initialize log file
    echo "" > "${LOG_FILE}"
    
    local command="${1:-install}"
    
    case "$command" in
        install)
            check_root "$command"
            do_install
            ;;
        uninstall|remove)
            check_root "$command"
            do_uninstall
            ;;
        reinstall)
            check_root "$command"
            do_reinstall
            ;;
        verify|check)
            do_verify
            ;;
        help|--help|-h)
            echo "AGI Installation Script"
            echo ""
            echo "Usage: $0 <command> [options]"
            echo ""
            echo "Commands:"
            echo "  install     - Install AGI to system"
            echo "  uninstall   - Uninstall AGI from system"
            echo "  reinstall   - Reinstall AGI"
            echo "  verify      - Verify installation status"
            echo "  help        - Display this help message"
            echo ""
            echo "Examples:"
            echo "  sudo $0 install"
            echo "  sudo $0 uninstall"
            echo "  $0 verify"
            ;;
        *)
            log_error "Unknown command: $command"
            echo "Use $0 help to view help"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"

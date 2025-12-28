#!/bin/bash
################################################################################
# AGI Command Line Wrapper Script
# Automated Guest Isolation - CLI Wrapper
#
# Features:
#   - Provide friendly command line interface
#   - Handle privileges and logging
#   - Integrate Bash auto-completion
#
# Usage:
#   agi <command> [arguments]
#
# Author: LOTUS-AGI Team ✓
# Version: 1.0.0
# Latest Update Date: 2025-12-28
################################################################################

# Color Configuration
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m'

# Configuration
BINARY_PATH="/usr/local/bin/agi"
WRAPPER_VERSION="1.0.0"

# Log Configuration
LOG_DIR="/var/log/agi"
LOG_FILE="${LOG_DIR}/wrapper.log"

################################################################################
# Output Functions
################################################################################

print_header() {
    echo -e "${WHITE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${WHITE}║  AGI - Automated Guest Isolation                           ║${NC}"
    echo -e "${WHITE}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

print_help() {
    print_header
    echo "Usage: agi <command> [arguments]"
    echo ""
    echo "Available commands:"
    echo ""
    echo -e "  ${CYAN}Basic Commands${NC}"
    echo "    help                Display this help message"
    echo "    version             Display version information"
    echo "    init                Initialize AGI configuration"
    echo ""
    echo -e "  ${CYAN}Environment Management${NC}"
    echo "    create <name>       Create new chroot environment"
    echo "    start <name>        Start specified environment"
    echo "    stop <name>         Stop specified environment"
    echo "    restart <name>      Restart specified environment"
    echo "    remove <name>       Delete specified environment"
    echo ""
    echo -e "  ${CYAN}Information View${NC}"
    echo "    status <name>       View status of specified environment"
    echo "    list                List all environments"
    echo "    validate            Validate configuration file"
    echo ""
    echo -e "  ${CYAN}Access Control${NC}"
    echo "    ssh <name>          SSH connect to specified environment"
    echo "    exec <name> <command> Execute command in environment"
    echo ""
    echo -e "  ${CYAN}System Management${NC}"
    echo "    backup <name>       Backup environment"
    echo "    restore <name>      Restore environment"
    echo "    logs <name>         View environment logs"
    echo ""
    echo "Examples:"
    echo "  agi init                    # Initialize configuration"
    echo "  agi create dev-env          # Create development environment"
    echo "  agi start dev-env           # Start environment"
    echo "  agi ssh dev-env             # SSH connection"
    echo "  agi stop dev-env            # Stop environment"
    echo "  agi list                    # List all environments"
    echo ""
    echo "For more information see: man agi"
    echo ""
}

print_version() {
    echo "AGI Wrapper Version: ${WRAPPER_VERSION}"
    echo "Binary Path: ${BINARY_PATH}"
    
    if [[ -x "${BINARY_PATH}" ]]; then
        echo ""
        echo "Binary version information:"
        "${BINARY_PATH}" --version 2>&1 | sed 's/^/  /'
    else
        echo -e "${RED}Error: AGI binary not found${NC}"
    fi
}

################################################################################
# Initialization Functions
################################################################################

init_wrapper() {
    # Ensure log directory exists
    if [[ ! -d "${LOG_DIR}" ]]; then
        mkdir -p "${LOG_DIR}" 2>/dev/null
    fi
    
    # Record startup log
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] agi $@" >> "${LOG_FILE}" 2>/dev/null
}

check_binary() {
    if [[ ! -x "${BINARY_PATH}" ]]; then
        echo -e "${RED}Error: AGI binary not found${NC}"
        echo ""
        echo "Please ensure AGI is properly installed:"
        echo "  1. Compile: make"
        echo "  2. Install: sudo ./scripts/installer.sh install"
        echo ""
        exit 1
    fi
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        echo -e "${RED}Error: AGI requires root privileges to run${NC}"
        echo ""
        echo "Please use: sudo agi $@"
        echo ""
        exit 1
    fi
}

################################################################################
# Command Handling Functions
################################################################################

cmd_init() {
    echo -e "${CYAN}Initializing AGI...${NC}"
    "${BINARY_PATH}" init "$@"
}

cmd_create() {
    local name="$1"
    if [[ -z "$name" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi create <name>"
        return 1
    fi
    echo -e "${CYAN}Creating environment: ${name}${NC}"
    "${BINARY_PATH}" create "$@"
}

cmd_start() {
    local name="$1"
    if [[ -z "$name" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi start <name>"
        return 1
    fi
    echo -e "${CYAN}Starting environment: ${name}${NC}"
    "${BINARY_PATH}" start "$@"
}

cmd_stop() {
    local name="$1"
    if [[ -z "$name" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi stop <name>"
        return 1
    fi
    echo -e "${CYAN}Stopping environment: ${name}${NC}"
    "${BINARY_PATH}" stop "$@"
}

cmd_restart() {
    local name="$1"
    if [[ -z "$name" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi restart <name>"
        return 1
    fi
    echo -e "${CYAN}Restarting environment: ${name}${NC}"
    "${BINARY_PATH}" restart "$@"
}

cmd_remove() {
    local name="$1"
    if [[ -z "$name" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi remove <name>"
        return 1
    fi
    echo -e "${YELLOW}Warning: Environment '${name}' will be deleted${NC}"
    "${BINARY_PATH}" remove "$@"
}

cmd_status() {
    if [[ -z "$1" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi status <name>"
        return 1
    fi
    "${BINARY_PATH}" status "$@"
}

cmd_list() {
    echo -e "${CYAN}Environment List:${NC}"
    "${BINARY_PATH}" list
}

cmd_validate() {
    echo -e "${CYAN}Validating configuration...${NC}"
    "${BINARY_PATH}" validate
}

cmd_ssh() {
    local name="$1"
    if [[ -z "$name" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi ssh <name>"
        return 1
    fi
    echo -e "${CYAN}Connecting to environment: ${name}${NC}"
    "${BINARY_PATH}" ssh "$@"
}

cmd_exec() {
    if [[ -z "$1" || -z "$2" ]]; then
        echo -e "${RED}Error: Insufficient parameters${NC}"
        echo "Usage: agi exec <name> <command>"
        return 1
    fi
    echo -e "${CYAN}Executing command...${NC}"
    "${BINARY_PATH}" exec "$@"
}

cmd_backup() {
    local name="$1"
    if [[ -z "$name" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi backup <name>"
        return 1
    fi
    echo -e "${CYAN}Backing up environment: ${name}${NC}"
    
    local backup_dir="/var/lib/agi/backups"
    local backup_file="${backup_dir}/${name}_$(date +%Y%m%d_%H%M%S).tar.gz"
    
    mkdir -p "${backup_dir}"
    
    if [[ -d "/var/lib/agi/jails/${name}" ]]; then
        tar -czf "${backup_file}" -C /var/lib/agi/jails "${name}"
        echo -e "${GREEN}Backup completed: ${backup_file}${NC}"
    else
        echo -e "${RED}Error: Environment does not exist${NC}"
    fi
}

cmd_restore() {
    local backup_file="$1"
    if [[ -z "$backup_file" ]]; then
        echo -e "${RED}Error: Please specify backup file${NC}"
        echo "Usage: agi restore <backup_file>"
        return 1
    fi
    
    if [[ ! -f "$backup_file" ]]; then
        echo -e "${RED}Error: Backup file does not exist${NC}"
        return 1
    fi
    
    echo -e "${CYAN}Restoring backup: ${backup_file}${NC}"
    
    local name=$(basename "$backup_file" | sed 's/_[0-9]*_[0-9]*\.tar\.gz$//')
    
    if [[ -d "/var/lib/agi/jails/${name}" ]]; then
        echo -e "${YELLOW}Warning: Environment '${name}' already exists${NC}"
        read -p "Overwrite? (y/n): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Restore cancelled"
            return 1
        fi
        rm -rf "/var/lib/agi/jails/${name}"
    fi
    
    tar -xzf "$backup_file" -C /var/lib/agi/jails
    echo -e "${GREEN}Restore completed${NC}"
}

cmd_logs() {
    local name="$1"
    if [[ -z "$name" ]]; then
        echo -e "${RED}Error: Please specify environment name${NC}"
        echo "Usage: agi logs <name>"
        return 1
    fi
    
    local log_file="/var/lib/agi/jails/${name}/var/log/jail.log"
    
    if [[ -f "$log_file" ]]; then
        tail -f "$log_file"
    else
        echo -e "${YELLOW}Log file not found: ${log_file}${NC}"
    fi
}

################################################################################
# Main Function
################################################################################

main() {
    # Initialize
    init_wrapper
    
    # Check binary
    check_binary
    
    # Handle help
    if [[ -z "$1" || "$1" == "help" || "$1" == "--help" || "$1" == "-h" ]]; then
        print_help
        exit 0
    fi
    
    # Handle version
    if [[ "$1" == "version" || "$1" == "--version" ]]; then
        print_version
        exit 0
    fi
    
    # Check root privileges
    check_root "$@"
    
    # Parse command
    local command="$1"
    shift
    
    case "$command" in
        init|create|start|stop|restart|remove|status|list|validate|ssh|exec)
            cmd_${command} "$@"
            ;;
        backup)
            cmd_backup "$@"
            ;;
        restore)
            cmd_restore "$@"
            ;;
        logs)
            cmd_logs "$@"
            ;;
        *)
            echo -e "${RED}Error: Unknown command '${command}'${NC}"
            echo ""
            echo "Use 'agi help' to view available commands"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"

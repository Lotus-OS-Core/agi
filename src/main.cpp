/**
 * @file main.cpp
 * @brief AGI Main Entry Point
 * @author AGI Team
 * @version 1.0.0
 * @date 2025-12-28
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

#include "utils.hpp"
#include "config.hpp"
#include "jail.hpp"
#include "logger.hpp"

namespace agi {

/**
 * @brief CLI Command Handler Class
 */
class CliHandler {
private:
    ConfigManager config_manager_;
    JailManagerPool jail_pool_;
    std::string program_name_;
    bool verbose_ = false;
    
public:
    CliHandler(int argc, char* argv[]) {
        program_name_ = argv[0];
        
        // Parse command line options
        parseOptions(argc, argv);
    }
    
    /**
     * @brief Display help information
     */
    void showHelp() {
        std::cout << R"(AGI - Automated Guest Isolation
================================

Usage: agi <command> [arguments]

Commands:
  init                Initialize AGI with default configuration
  create <name>       Create a new chroot environment
  start <name>        Start the specified environment
  stop <name>         Stop the specified environment
  restart <name>      Restart the specified environment
  status <name>       Show status of specified environment
  list                List all environments
  ssh <name>          SSH connect to specified environment
  exec <name> <cmd>   Execute command in environment
  remove <name>       Remove specified environment
  validate            Validate configuration file
  version             Show version information
  help                Show this help message

Options:
  -c, --config <file> Specify configuration file path
  -v, --verbose       Show verbose output
  -h, --help          Show this help message
  --version           Show version information

Examples:
  agi init                    # Initialize configuration
  agi create dev-env          # Create development environment
  agi start dev-env           # Start environment
  agi ssh dev-env             # SSH connect
  agi stop dev-env            # Stop environment
  agi list                    # List all environments
  agi remove dev-env          # Remove environment

For more information see: man agi or /usr/share/doc/agi/
)";
    }
    
    /**
     * @brief Display version information
     */
    void showVersion() {
        std::cout << "AGI Version: 1.0.0" << std::endl;
        std::cout << "Build Time: " << __DATE__ << " " << __TIME__ << std::endl;
        std::cout << "Compiler: " << __VERSION__ << std::endl;
    }
    
    /**
     * @brief Execute main program flow
     */
    int run() {
        // Check if running with root privileges
        if (geteuid() != 0) {
            std::cerr << "Error: AGI requires root privileges to run" << std::endl;
            std::cerr << "Please use: sudo agi <command>" << std::endl;
            return 1;
        }
        
        // Initialize logging
        initLogger("/var/log/agi/agi.log", 
                   verbose_ ? LogLevel::DEBUG : LogLevel::INFO);
        
        AGI_LOG_INFO("AGI started");
        
        // Load configuration
        if (!config_manager_.load()) {
            AGI_LOG_ERROR("Failed to load configuration: " + config_manager_.getError());
            if (verbose_) {
                std::cerr << "Configuration error: " << config_manager_.getError() << std::endl;
            }
            return 1;
        }
        
        // Validate configuration
        if (!config_manager_.validate()) {
            AGI_LOG_ERROR("Configuration validation failed");
            return 1;
        }
        
        // Initialize jail pool
        const auto& config = config_manager_.getConfig();
        for (const auto& env : config.environments) {
            jail_pool_.addEnvironment(env);
        }
        
        return 0;
    }
    
    /**
     * @brief Handle a single command
     */
    bool handleCommand(const std::string& command, 
                       const std::vector<std::string>& args) {
        bool result = true;
        
        if (command == "init") {
            result = cmdInit();
        } else if (command == "create") {
            result = cmdCreate(args);
        } else if (command == "start") {
            result = cmdStart(args);
        } else if (command == "stop") {
            result = cmdStop(args);
        } else if (command == "restart") {
            result = cmdRestart(args);
        } else if (command == "status") {
            result = cmdStatus(args);
        } else if (command == "list") {
            result = cmdList();
        } else if (command == "ssh") {
            result = cmdSsh(args);
        } else if (command == "exec") {
            result = cmdExec(args);
        } else if (command == "remove" || command == "delete") {
            result = cmdRemove(args);
        } else if (command == "validate") {
            result = cmdValidate();
        } else if (command == "version") {
            showVersion();
        } else if (command == "help" || command == "--help" || command == "-h") {
            showHelp();
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            std::cerr << "Use 'agi help' to see available commands" << std::endl;
            result = false;
        }
        
        return result;
    }
    
private:
    void parseOptions(int argc, char* argv[]) {
        static const option long_options[] = {
            {"config", required_argument, nullptr, 'c'},
            {"verbose", no_argument, nullptr, 'v'},
            {"help", no_argument, nullptr, 'h'},
            {"version", no_argument, nullptr, 1},
            {nullptr, 0, nullptr, 0}
        };
        
        int opt;
        while ((opt = getopt_long(argc, argv, "c:vh", long_options, nullptr)) != -1) {
            switch (opt) {
                case 'c':
                    config_manager_.load(optarg);
                    break;
                case 'v':
                    verbose_ = true;
                    break;
                case 'h':
                    showHelp();
                    exit(0);
                case 1:
                    showVersion();
                    exit(0);
                default:
                    showHelp();
                    exit(1);
            }
        }
    }
    
    bool cmdInit() {
        std::cout << "Initializing AGI configuration..." << std::endl;
        
        std::string config_path = "/etc/agi/agi_config.json";
        
        // Check if config already exists
        if (FileUtils::exists(config_path)) {
            std::cout << "Configuration file already exists: " << config_path << std::endl;
            std::cout << "Please delete existing config to reinitialize" << std::endl;
            return false;
        }
        
        // Create directory
        std::string dir = PathUtils::parent(config_path);
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
        
        // Generate default configuration
        ConfigManager default_config = ConfigManager::createDefault();
        std::string config_content = default_config.generateDefaultConfig();
        
        if (!FileUtils::write(config_path, config_content)) {
            std::cerr << "Error: Cannot write configuration file" << std::endl;
            return false;
        }
        
        // Set permissions
        chmod(config_path.c_str(), 0644);
        
        // Create log directory
        std::string log_dir = "/var/log/agi";
        if (!std::filesystem::exists(log_dir)) {
            std::filesystem::create_directories(log_dir);
        }
        
        // Create data directory
        std::string data_dir = "/var/lib/agi/jails";
        if (!std::filesystem::exists(data_dir)) {
            std::filesystem::create_directories(data_dir);
        }
        
        std::cout << "Initialization complete!" << std::endl;
        std::cout << "Configuration file: " << config_path << std::endl;
        std::cout << "Log directory: " << log_dir << std::endl;
        std::cout << std::endl;
        std::cout << "Please edit the configuration file to add your environment settings" << std::endl;
        
        return true;
    }
    
    bool cmdCreate(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: Please specify environment name" << std::endl;
            std::cerr << "Usage: agi create <name>" << std::endl;
            return false;
        }
        
        std::string name = args[0];
        auto* jail = jail_pool_.getJail(name);
        
        if (jail && jail->exists()) {
            std::cerr << "Error: Environment '" << name << "' already exists" << std::endl;
            return false;
        }
        
        const auto* env_config = config_manager_.findEnvironment(name);
        if (!env_config) {
            std::cerr << "Error: Environment '" << name << "' not found in configuration" << std::endl;
            std::cerr << "Please add environment definition to configuration file" << std::endl;
            return false;
        }
        
        jail = jail_pool_.getJail(name);
        if (!jail) {
            std::cerr << "Error: Cannot create jail manager" << std::endl;
            return false;
        }
        
        std::cout << "Creating environment: " << name << std::endl;
        
        if (!jail->create()) {
            std::cerr << "Error: Failed to create environment" << std::endl;
            return false;
        }
        
        std::cout << "Environment created successfully: " << jail->getPath() << std::endl;
        return true;
    }
    
    bool cmdStart(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: Please specify environment name" << std::endl;
            return false;
        }
        
        std::string name = args[0];
        auto* jail = jail_pool_.getJail(name);
        
        if (!jail) {
            std::cerr << "Error: Environment '" << name << "' does not exist" << std::endl;
            return false;
        }
        
        if (jail->getStatus() == JailStatus::RUNNING) {
            std::cout << "Environment is already running" << std::endl;
            return true;
        }
        
        std::cout << "Starting environment: " << name << std::endl;
        
        if (!jail->start()) {
            std::cerr << "Error: Failed to start environment" << std::endl;
            return false;
        }
        
        auto info = jail->getRuntimeInfo();
        std::cout << "Environment started" << std::endl;
        std::cout << "  SSH Port: " << info.ssh_port << std::endl;
        std::cout << "  Access Address: " << info.ip_address << std::endl;
        std::cout << std::endl;
        std::cout << "Connection command: agi ssh " << name << std::endl;
        
        return true;
    }
    
    bool cmdStop(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: Please specify environment name" << std::endl;
            return false;
        }
        
        std::string name = args[0];
        auto* jail = jail_pool_.getJail(name);
        
        if (!jail) {
            std::cerr << "Error: Environment '" << name << "' does not exist" << std::endl;
            return false;
        }
        
        if (jail->getStatus() == JailStatus::STOPPED) {
            std::cout << "Environment is already stopped" << std::endl;
            return true;
        }
        
        std::cout << "Stopping environment: " << name << std::endl;
        
        if (!jail->stop()) {
            std::cerr << "Error: Failed to stop environment" << std::endl;
            return false;
        }
        
        std::cout << "Environment stopped" << std::endl;
        return true;
    }
    
    bool cmdRestart(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: Please specify environment name" << std::endl;
            return false;
        }
        
        std::string name = args[0];
        
        std::cout << "Restarting environment: " << name << std::endl;
        
        if (!cmdStop(args)) {
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        return cmdStart(args);
    }
    
    bool cmdStatus(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: Please specify environment name" << std::endl;
            return false;
        }
        
        std::string name = args[0];
        auto* jail = jail_pool_.getJail(name);
        
        if (!jail) {
            std::cerr << "Error: Environment '" << name << "' does not exist" << std::endl;
            return false;
        }
        
        auto info = jail->getRuntimeInfo();
        
        std::cout << "Environment status: " << name << std::endl;
        std::cout << "  Status: " << statusToString(info.status) << std::endl;
        std::cout << "  Path: " << jail->getPath() << std::endl;
        
        if (info.status == JailStatus::RUNNING) {
            std::cout << "  SSH Port: " << info.ssh_port << std::endl;
            std::cout << "  Access Address: " << info.ip_address << std::endl;
            if (info.pid > 0) {
                std::cout << "  Process PID: " << info.pid << std::endl;
            }
        }
        
        if (!info.error_message.empty()) {
            std::cout << "  Error: " << info.error_message << std::endl;
        }
        
        return true;
    }
    
    bool cmdList() {
        auto jails = jail_pool_.listJails();
        
        if (jails.empty()) {
            std::cout << "No environments configured" << std::endl;
            return true;
        }
        
        std::cout << "Configured environments (" << jails.size() << "):" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        
        for (const auto& name : jails) {
            auto* jail = jail_pool_.getJail(name);
            if (jail) {
                auto info = jail->getRuntimeInfo();
                std::cout << name << std::endl;
                std::cout << "  Status: " << statusToString(info.status);
                if (info.status == JailStatus::RUNNING) {
                    std::cout << " (Port " << info.ssh_port << ")";
                }
                std::cout << std::endl;
            }
        }
        
        return true;
    }
    
    bool cmdSsh(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: Please specify environment name" << std::endl;
            return false;
        }
        
        std::string name = args[0];
        auto* jail = jail_pool_.getJail(name);
        
        if (!jail) {
            std::cerr << "Error: Environment '" << name << "' does not exist" << std::endl;
            return false;
        }
        
        if (jail->getStatus() != JailStatus::RUNNING) {
            std::cerr << "Error: Environment is not running" << std::endl;
            std::cerr << "Please start the environment first: agi start " << name << std::endl;
            return false;
        }
        
        auto info = jail->getRuntimeInfo();
        
        std::cout << "Connecting to " << name << "..." << std::endl;
        std::cout << "Use root user, password is randomly generated" << std::endl;
        std::cout << "Type 'exit' to quit" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        
        // Use ssh command to connect
        std::string cmd = "ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null ";
        cmd += "-p " + std::to_string(info.ssh_port) + " ";
        cmd += "root@" + info.ip_address;
        
        system(cmd.c_str());
        
        return true;
    }
    
    bool cmdExec(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            std::cerr << "Error: Insufficient arguments" << std::endl;
            std::cerr << "Usage: agi exec <name> <command>" << std::endl;
            return false;
        }
        
        std::string name = args[0];
        std::string command;
        for (size_t i = 1; i < args.size(); ++i) {
            if (i > 1) command += " ";
            command += args[i];
        }
        
        auto* jail = jail_pool_.getJail(name);
        
        if (!jail) {
            std::cerr << "Error: Environment '" << name << "' does not exist" << std::endl;
            return false;
        }
        
        std::cout << "Executing: " << command << std::endl;
        std::string output = jail->execute(command);
        std::cout << output << std::endl;
        
        return true;
    }
    
    bool cmdRemove(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cerr << "Error: Please specify environment name" << std::endl;
            return false;
        }
        
        std::string name = args[0];
        auto* jail = jail_pool_.getJail(name);
        
        if (!jail) {
            std::cerr << "Error: Environment '" << name << "' does not exist" << std::endl;
            return false;
        }
        
        std::cout << "Warning: This will permanently delete environment '" << name << "'" << std::endl;
        std::cout << "Confirm deletion? (Enter y to confirm): ";
        
        char confirm;
        std::cin >> confirm;
        
        if (confirm != 'y' && confirm != 'Y') {
            std::cout << "Cancelled" << std::endl;
            return true;
        }
        
        std::cout << "Deleting environment: " << name << std::endl;
        
        if (!jail->destroy()) {
            std::cerr << "Error: Failed to delete environment" << std::endl;
            return false;
        }
        
        jail_pool_.removeEnvironment(name);
        
        std::cout << "Environment deleted" << std::endl;
        return true;
    }
    
    bool cmdValidate() {
        if (!config_manager_.load()) {
            std::cerr << "Configuration error: " << config_manager_.getError() << std::endl;
            return false;
        }
        
        if (!config_manager_.validate()) {
            std::cerr << "Configuration validation failed" << std::endl;
            return false;
        }
        
        std::cout << "Configuration validation passed" << std::endl;
        
        const auto& config = config_manager_.getConfig();
        std::cout << std::endl;
        std::cout << "Configuration info:" << std::endl;
        std::cout << "  Base path: " << config.base_path << std::endl;
        std::cout << "  Log level: " << config.log_level << std::endl;
        std::cout << "  Environment count: " << config.environments.size() << std::endl;
        
        for (const auto& env : config.environments) {
            std::cout << std::endl;
            std::cout << "  Environment: " << env.name << std::endl;
            std::cout << "    Template: " << env.os_template << std::endl;
            std::cout << "    SSH port: " << env.ssh.port << std::endl;
            std::cout << "    User count: " << env.users.size() << std::endl;
        }
        
        return true;
    }
    
    static std::string statusToString(JailStatus status) {
        switch (status) {
            case JailStatus::STOPPED:   return "Stopped";
            case JailStatus::STARTING:  return "Starting";
            case JailStatus::RUNNING:   return "Running";
            case JailStatus::STOPPING:  return "Stopping";
            case JailStatus::ERROR:     return "Error";
            default:                    return "Unknown";
        }
    }
};

} // namespace agi

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    // Check command arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <command> [arguments]" << std::endl;
        std::cerr << "Use '" << argv[0] << " help' for help" << std::endl;
        return 1;
    }
    
    std::string command = argv[1];
    
    // Handle help and version
    if (command == "help" || command == "--help" || command == "-h") {
        agi::CliHandler(argc, argv).showHelp();
        return 0;
    }
    
    if (command == "version" || command == "--version") {
        agi::CliHandler(argc, argv).showVersion();
        return 0;
    }
    
    // Create CLI handler and run
    agi::CliHandler handler(argc, argv);
    int result = handler.run();
    
    if (result != 0) {
        return result;
    }
    
    // Collect command arguments
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    
    // Execute command
    if (!handler.handleCommand(command, args)) {
        return 1;
    }
    
    return 0;
}

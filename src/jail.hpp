/**
 * @file jail.hpp
 * @brief Chroot environment management class
 * @author AGI Team
 * @version 1.0.0
 * @date 2025-12-28
 */

#ifndef AGI_JAIL_HPP
#define AGI_JAIL_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "utils.hpp"
#include "config.hpp"

namespace agi {

/**
 * @brief Mount status enumeration
 */
enum class MountStatus {
    UNMOUNTED,
    MOUNTED,
    BUSY
};

/**
 * @brief Jail running status
 */
enum class JailStatus {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};

/**
 * @brief Mount information
 */
struct MountInfo {
    std::string source;
    std::string target;
    std::string type;
    std::string options;
};

/**
 * @brief Jail runtime information
 */
struct JailRuntimeInfo {
    std::string name;
    JailStatus status = JailStatus::STOPPED;
    int pid = -1;
    int ssh_port = -1;
    std::string ip_address;
    std::chrono::system_clock::time_point start_time;
    std::vector<MountInfo> mounts;
    std::string error_message;
};

/**
 * @brief Chroot environment manager
 */
class JailManager {
private:
    const EnvironmentConfig* config_ = nullptr;
    std::string jail_path_;
    std::string data_path_;
    std::string run_path_;
    
    std::atomic<JailStatus> status_{JailStatus::STOPPED};
    pid_t sshd_pid_ = -1;
    
    std::chrono::system_clock::time_point start_time_;
    std::vector<MountInfo> active_mounts_;
    mutable std::mutex mutex_;
    
    // Logger function
    std::function<void(const std::string&, const std::string&)> logger_;
    
public:
    explicit JailManager(const EnvironmentConfig& config) : config_(&config) {
        jail_path_ = "/var/lib/agi/jails/" + config.name;
        data_path_ = jail_path_ + "/data";
        run_path_ = jail_path_ + "/run";
    }
    
    /**
     * @brief Set logger callback
     */
    void setLogger(std::function<void(const std::string&, const std::string&)> logger) {
        logger_ = logger;
    }
    
    void log(const std::string& level, const std::string& message) {
        if (logger_) {
            logger_(level, message);
        }
    }
    
    /**
     * @brief Create jail environment
     * @return Whether successful
     */
    bool create() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            log("INFO", "Starting to create jail environment: " + config_->name);
            
            // Create directory structure
            createDirectoryStructure();
            
            // Copy base system files
            copyBaseSystem();
            
            // Create necessary device nodes
            createDeviceNodes();
            
            // Generate SSH configuration
            generateSshConfig();
            
            // Create initialization scripts
            createInitScripts();
            
            log("INFO", "Jail environment created successfully: " + jail_path_);
            return true;
            
        } catch (const std::exception& e) {
            log("ERROR", std::string("Failed to create jail: ") + e.what());
            return false;
        }
    }
    
    /**
     * @brief Destroy jail environment
     * @return Whether successful
     */
    bool destroy() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            // Stop first
            stop();
            
            log("INFO", "Destroying jail environment: " + config_->name);
            
            // Unmount all mount points
            unmountAll();
            
            // Delete directory
            if (std::filesystem::exists(jail_path_)) {
                std::filesystem::remove_all(jail_path_);
            }
            
            log("INFO", "Jail environment has been destroyed: " + config_->name);
            return true;
            
        } catch (const std::exception& e) {
            log("ERROR", std::string("Failed to destroy jail: ") + e.what());
            return false;
        }
    }
    
    /**
     * @brief Start jail environment
     * @return Whether successful
     */
    bool start() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (status_ == JailStatus::RUNNING) {
            log("WARN", "Jail is already running");
            return true;
        }
        
        try {
            log("INFO", "Starting jail environment: " + config_->name);
            status_ = JailStatus::STARTING;
            
            // Check if directory exists
            if (!std::filesystem::exists(jail_path_)) {
                throw JailError("Jail directory does not exist, please create it first");
            }
            
            // Set resource limits
            setResourceLimits();
            
            // Mount necessary filesystems
            mountFilesystems();
            
            // Start SSH service
            startSshd();
            
            // Run initialization script
            runInitScript();
            
            status_ = JailStatus::RUNNING;
            start_time_ = std::chrono::system_clock::now();
            
            log("INFO", "Jail environment has been started: " + config_->name + 
                " (SSH port: " + std::to_string(config_->ssh.port) + ")");
            return true;
            
        } catch (const std::exception& e) {
            status_ = JailStatus::ERROR;
            log("ERROR", std::string("Failed to start jail: ") + e.what());
            return false;
        }
    }
    
    /**
     * @brief Stop jail environment
     * @return Whether successful
     */
    bool stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (status_ == JailStatus::STOPPED) {
            return true;
        }
        
        try {
            log("INFO", "Stopping jail environment: " + config_->name);
            status_ = JailStatus::STOPPING;
            
            // Stop SSH service
            stopSshd();
            
            // Run cleanup script
            runCleanupScript();
            
            // Unmount filesystems
            unmountAll();
            
            status_ = JailStatus::STOPPED;
            
            log("INFO", "Jail environment has been stopped: " + config_->name);
            return true;
            
        } catch (const std::exception& e) {
            status_ = JailStatus::ERROR;
            log("ERROR", std::string("Failed to stop jail: ") + e.what());
            return false;
        }
    }
    
    /**
     * @brief Execute command in jail environment
     * @param command Command
     * @return Command output
     */
    std::string execute(const std::string& command) {
        if (status_ != JailStatus::RUNNING) {
            throw JailError("Jail is not running, cannot execute command");
        }
        
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (chroot(jail_path_.c_str()) != 0) {
                exit(1);
            }
            if (chdir("/") != 0) {
                exit(1);
            }
            
            execlp("/bin/sh", "sh", "-c", command.c_str(), nullptr);
            exit(1);
        } else if (pid > 0) {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    return "Command executed successfully";
                }
                return "Command execution failed (exit code: " + std::to_string(WEXITSTATUS(status)) + ")";
            }
            return "Command was interrupted by signal";
        }
        
        throw JailError("Cannot create child process");
    }
    
    /**
     * @brief Get runtime information
     */
    JailRuntimeInfo getRuntimeInfo() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        JailRuntimeInfo info;
        info.name = config_->name;
        info.status = status_;
        info.ssh_port = config_->ssh.port;
        info.ip_address = config_->ssh.listen_address;
        info.start_time = start_time_;
        info.mounts = active_mounts_;
        
        if (sshd_pid_ > 0) {
            info.pid = sshd_pid_;
        }
        
        return info;
    }
    
    /**
     * @brief Get status
     */
    JailStatus getStatus() const {
        return status_;
    }
    
    /**
     * @brief Get jail path
     */
    const std::string& getPath() const {
        return jail_path_;
    }
    
    /**
     * @brief Check if exists
     */
    bool exists() const {
        return std::filesystem::exists(jail_path_);
    }
    
    /**
     * @brief Validate configuration
     */
    bool validateConfig() const {
        if (config_->name.empty()) {
            throw JailError("Environment name cannot be empty");
        }
        
        if (config_->ssh.port < 1 || config_->ssh.port > 65535) {
            throw JailError("SSH port must be in range 1-65535");
        }
        
        // Check path safety
        if (config_->root_path.find("..") != std::string::npos) {
            throw JailError("Path contains invalid directory traversal");
        }
        
        return true;
    }
    
private:
    void createDirectoryStructure() {
        std::vector<std::string> dirs = {
            jail_path_,
            data_path_,
            run_path_,
            jail_path_ + "/bin",
            jail_path_ + "/sbin",
            jail_path_ + "/usr",
            jail_path_ + "/usr/bin",
            jail_path_ + "/usr/sbin",
            jail_path_ + "/lib",
            jail_path_ + "/lib64",
            jail_path_ + "/etc",
            jail_path_ + "/home",
            jail_path_ + "/root",
            jail_path_ + "/tmp",
            jail_path_ + "/var",
            jail_path_ + "/var/log",
            jail_path_ + "/var/run",
            jail_path_ + "/proc",
            jail_path_ + "/sys",
            jail_path_ + "/dev",
            jail_path_ + "/dev/pts",
            jail_path_ + "/usr/share/agi"
        };
        
        for (const auto& dir : dirs) {
            if (!std::filesystem::exists(dir)) {
                std::filesystem::create_directories(dir);
                log("DEBUG", "Creating directory: " + dir);
            }
        }
    }
    
    void copyBaseSystem() {
        // Copy essential system binaries
        std::vector<std::string> essential_bins = {
            "/bin/bash",
            "/bin/ls",
            "/bin/cat",
            "/bin/mkdir",
            "/bin/rm",
            "/bin/echo",
            "/bin/sleep",
            "/usr/bin/whoami"
        };
        
        for (const auto& bin : essential_bins) {
            copyBinary(bin);
        }
        
        // Copy essential shared libraries
        copyEssentialLibraries();
    }
    
    void copyBinary(const std::string& path) {
        std::string dest = jail_path_ + path;
        std::string parent = PathUtils::parent(dest);
        
        if (!std::filesystem::exists(parent)) {
            std::filesystem::create_directories(parent);
        }
        
        try {
            std::string content = FileUtils::read(path);
            FileUtils::write(dest, content);
            chmod(dest.c_str(), 0755);
            log("DEBUG", "Copying binary: " + path + " -> " + dest);
        } catch (const std::exception& e) {
            log("WARN", std::string("Cannot copy binary: ") + path + " - " + e.what());
        }
    }
    
    void copyEssentialLibraries() {
        // Copy bash dependencies
        std::vector<std::string> libs = {
            "/lib/x86_64-linux-gnu/libc.so.6",
            "/lib/x86_64-linux-gnu/libdl.so.2",
            "/lib/x86_64-linux-gnu/libtinfo.so.6",
            "/lib/x86_64-linux-gnu/libpthread.so.0",
            "/lib64/ld-linux-x86-64.so.2"
        };
        
        for (const auto& lib : libs) {
            if (std::filesystem::exists(lib)) {
                std::string dest = jail_path_ + lib;
                std::string parent = PathUtils::parent(dest);
                
                if (!std::filesystem::exists(parent)) {
                    std::filesystem::create_directories(parent);
                }
                
                try {
                    std::string content = FileUtils::read(lib);
                    FileUtils::write(dest, content);
                    log("DEBUG", "Copying library: " + lib);
                } catch (...) {
                    // Ignore errors
                }
            }
        }
    }
    
    void createDeviceNodes() {
        // Create device nodes
        createDeviceNode(jail_path_ + "/dev/null", 'c', 1, 3);
        createDeviceNode(jail_path_ + "/dev/zero", 'c', 1, 5);
        createDeviceNode(jail_path_ + "/dev/random", 'c', 1, 8);
        createDeviceNode(jail_path_ + "/dev/urandom", 'c', 1, 9);
        createDeviceNode(jail_path_ + "/dev/tty", 'c', 5, 0);
    }
    
    void createDeviceNode(const std::string& path, char type, int major, int minor) {
        if (std::filesystem::exists(path)) {
            return;
        }
        
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        if (type == 'c') {
            mknod(path.c_str(), S_IFCHR | mode, makedev(major, minor));
        } else {
            mknod(path.c_str(), S_IFBLK | mode, makedev(major, minor));
        }
    }
    
    void generateSshConfig() {
        std::string ssh_dir = jail_path_ + "/etc/ssh";
        if (!std::filesystem::exists(ssh_dir)) {
            std::filesystem::create_directories(ssh_dir);
        }
        
        std::string config = R"(Port )" + std::to_string(config_->ssh.port) + R"(
ListenAddress )" + config_->ssh.listen_address + R"(
HostKey /etc/ssh/ssh_host_rsa_key
HostKey /etc/ssh/ssh_host_ecdsa_key
HostKey /etc/ssh/ssh_host_ed25519_key
PermitRootLogin yes
PasswordAuthentication )" + (config_->ssh.password_auth ? "yes" : "no") + R"(
PubkeyAuthentication )" + (config_->ssh.pubkey_auth ? "yes" : "no") + R(
MaxAuthTries )" + std::to_string(config_->ssh.max_auth_tries) + R"(
ClientAliveInterval )" + std::to_string(config_->ssh.client_alive_interval) + R"(
ClientAliveCountMax 0
X11Forwarding no
AllowTcpForwarding yes
PermitEmptyPasswords no
PrintMotd no
AcceptEnv LANG LC_*)";
        
        FileUtils::write(ssh_dir + "/sshd_config", config);
        log("DEBUG", "Generating SSH configuration");
    }
    
    void createInitScripts() {
        // Create basic initialization script
        std::string init_script = R"#!/bin/bash
# Jail initialization script
echo "Jail started at $(date)" > /var/log/jail.log
mount -t proc proc /proc
mount -t sysfs sys /sys
mount -t devpts devpts /dev/pts
)";
        
        FileUtils::write(jail_path_ + "/usr/share/agi/init.sh", init_script);
        chmod((jail_path_ + "/usr/share/agi/init.sh").c_str(), 0755);
    }
    
    void setResourceLimits() {
        struct rlimit rl;
        
        rl.rlim_cur = config_->limits.max_cpu_time;
        rl.rlim_max = config_->limits.max_cpu_time;
        setrlimit(RLIMIT_CPU, &rl);
        
        rl.rlim_cur = config_->limits.max_memory * 1024;
        rl.rlim_max = config_->limits.max_memory * 1024;
        setrlimit(RLIMIT_AS, &rl);
        
        rl.rlim_cur = config_->limits.max_file_size * 1024;
        rl.rlim_max = config_->limits.max_file_size * 1024;
        setrlimit(RLIMIT_FSIZE, &rl);
        
        rl.rlim_cur = config_->limits.max_processes;
        rl.rlim_max = config_->limits.max_processes;
        setrlimit(RLIMIT_NPROC, &rl);
        
        rl.rlim_cur = config_->limits.max_open_files;
        rl.rlim_max = config_->limits.max_open_files;
        setrlimit(RLIMIT_NOFILE, &rl);
    }
    
    void mountFilesystems() {
        // Mount /proc
        mount("proc", (jail_path_ + "/proc").c_str(), "proc", 0, nullptr);
        addMountInfo("proc", jail_path_ + "/proc", "proc", "");
        
        // Mount /sys
        mount("sysfs", (jail_path_ + "/sys").c_str(), "sysfs", 0, nullptr);
        addMountInfo("sysfs", jail_path_ + "/sys", "sysfs", "");
        
        // Mount /dev/pts
        mount("devpts", (jail_path_ + "/dev/pts").c_str(), "devpts", 0, "gid=5,mode=620");
        addMountInfo("devpts", jail_path_ + "/dev/pts", "devpts", "gid=5,mode=620");
        
        // Mount /tmp
        std::string tmp_path = jail_path_ + "/tmp";
        mount("/tmp", tmp_path.c_str(), "none", MS_BIND, nullptr);
        addMountInfo("/tmp", tmp_path, "none", "bind");
        
        log("DEBUG", "Filesystem mounting completed");
    }
    
    void unmountAll() {
        // Unmount in reverse order
        for (auto it = active_mounts_.rbegin(); it != active_mounts_.rend(); ++it) {
            int retries = 3;
            while (retries > 0) {
                if (umount(it->target.c_str()) == 0) {
                    log("DEBUG", "Unmounting: " + it->target);
                    break;
                }
                if (errno == EBUSY) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    --retries;
                } else {
                    break;
                }
            }
        }
        active_mounts_.clear();
    }
    
    void addMountInfo(const std::string& source, const std::string& target,
                      const std::string& type, const std::string& options) {
        MountInfo info;
        info.source = source;
        info.target = target;
        info.type = type;
        info.options = options;
        active_mounts_.push_back(info);
    }
    
    void startSshd() {
        // Generate SSH host keys
        std::string ssh_dir = jail_path_ + "/etc/ssh";
        
        // Start SSH
        std::string sshd_path = jail_path_ + "/usr/sbin/sshd";
        if (!std::filesystem::exists(sshd_path)) {
            sshd_path = "/usr/sbin/sshd";
        }
        
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            chroot(jail_path_.c_str());
            chdir("/");
            
            setsid();
            
            execlp("sshd", "sshd", "-f", "/etc/ssh/sshd_config", "-E", "/var/log/sshd.log", nullptr);
            exit(1);
        } else if (pid > 0) {
            sshd_pid_ = pid;
            log("DEBUG", "SSH process started: PID " + std::to_string(pid));
        } else {
            throw JailError("Cannot start SSH");
        }
    }
    
    void stopSshd() {
        if (sshd_pid_ > 0) {
            kill(sshd_pid_, SIGTERM);
            
            int status;
            waitpid(sshd_pid_, &status, 0);
            
            sshd_pid_ = -1;
            log("DEBUG", "SSH process has been stopped");
        }
    }
    
    void runInitScript() {
        std::string init_script = jail_path_ + "/usr/share/agi/init.sh";
        if (std::filesystem::exists(init_script)) {
            pid_t pid = fork();
            if (pid == 0) {
                chroot(jail_path_.c_str());
                chdir("/");
                execlp("sh", "sh", init_script.c_str(), nullptr);
                exit(1);
            } else if (pid > 0) {
                waitpid(pid, nullptr, 0);
            }
        }
    }
    
    void runCleanupScript() {
        std::string cleanup_script = jail_path_ + "/usr/share/agi/cleanup.sh";
        if (std::filesystem::exists(cleanup_script)) {
            pid_t pid = fork();
            if (pid == 0) {
                chroot(jail_path_.c_str());
                chdir("/");
                execlp("sh", "sh", cleanup_script.c_str(), nullptr);
                exit(1);
            } else if (pid > 0) {
                waitpid(pid, nullptr, 0);
            }
        }
    }
};

/**
 * @brief Jail manager collection
 */
class JailManagerPool {
private:
    std::map<std::string, std::unique_ptr<JailManager>> jails_;
    std::function<void(const std::string&, const std::string&)> logger_;
    mutable std::mutex mutex_;
    
public:
    void setLogger(std::function<void(const std::string&, const std::string&)> logger) {
        logger_ = logger;
        for (auto& [name, jail] : jails_) {
            jail->setLogger(logger);
        }
    }
    
    bool addEnvironment(const EnvironmentConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (jails_.count(config.name)) {
            return false;
        }
        
        auto jail = std::make_unique<JailManager>(config);
        jail->setLogger(logger_);
        jails_[config.name] = std::move(jail);
        
        return true;
    }
    
    bool removeEnvironment(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = jails_.find(name);
        if (it == jails_.end()) {
            return false;
        }
        
        it->second->stop();
        jails_.erase(it);
        
        return true;
    }
    
    JailManager* getJail(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = jails_.find(name);
        if (it == jails_.end()) {
            return nullptr;
        }
        return it->second.get();
    }
    
    std::vector<std::string> listJails() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> result;
        for (const auto& [name, jail] : jails_) {
            result.push_back(name);
        }
        return result;
    }
    
    std::vector<JailRuntimeInfo> getAllRuntimeInfo() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<JailRuntimeInfo> result;
        for (const auto& [name, jail] : jails_) {
            result.push_back(jail->getRuntimeInfo());
        }
        return result;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return jails_.size();
    }
};

} // namespace agi

#endif // AGI_JAIL_HPP

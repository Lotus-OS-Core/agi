# AGI Architecture Document
LotusOS-Core ─► Lotus AGI

## Table of Contents

1. [System Overview](#system-overview)
2. [Architecture Design](#architecture-design)
3. [Core Components](#core-components)
4. [Data Flow](#data-flow)
5. [Security Mechanisms](#security-mechanisms)
6. [Extension Guide](#extension-guide)
7. [Development Guide](#development-guide)
8. [API Reference](#api-reference)

---

## System Overview

### Design Goals

AGI (Automated Guest Isolation) is a lightweight chroot environment management framework specifically designed for Linux systems. Its main design goals include:

- **Minimal Dependencies**: Uses only C++ standard library, no external dependencies
- **High Performance**: Native C++ implementation, low-overhead isolation environment management
- **Secure Isolation**: Multi-layer isolation through chroot, mount namespaces, and resource limits
- **Easy Extension**: Modular design, easy to add new features and templates
- **Standardized Interface**: Follows OpenAI-compatible API design patterns

### Technology Stack

- **Language**: C++17
- **Build System**: Make
- **Dependencies**: POSIX standard library (standard library only, no external dependencies)
- **Operating System**: Linux (kernel 3.10+)

### Project Structure

```
agi/
├── src/                    # C++ source code
│   ├── main.cpp           # Program entry point
│   ├── utils.hpp          # Utility functions
│   ├── config.hpp         # Configuration management
│   ├── jail.hpp           # Jail management
│   └── logger.hpp         # Logging system
├── scripts/               # Script files
│   ├── installer.sh       # Installation script
│   └── agi-wrapper.sh     # Command line wrapper
├── config/                # Configuration files
│   └── agi_config.json    # Default configuration
├── docs/                  # Documentation
│   ├── USER_MANUAL.md     # User manual
│   └── ARCHITECTURE.md    # Architecture document
└── Makefile               # Build configuration
```

---

## Architecture Design

### Overall Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         User Layer                          │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              agi-wrapper.sh (CLI Wrapper)           │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        Application Layer                    │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                   CliHandler                        │    │
│  │              (CLI argument parsing and dispatch)    │    │
│  └─────────────────────────────────────────────────────┘    │
│                              │                              │
│                              ▼                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │                 JailManagerPool                     │    │
│  │              (Multi-environment management pool)    │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                         Core Layer                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │ ConfigManager│  │ JailManager  │  │    Logger        │   │
│  │ (Config Parse│  │ (Environment │  │   (Logging       │   │
│  │    ing)      │  │  Management) │  │    System)       │   │
│  └──────────────┘  └──────────────┘  └──────────────────┘   │
│         │                │                   │              │
│         ▼                ▼                   ▼              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              Utility Library (utils.hpp)            │    │
│  │  PathUtils | StringUtils | FileUtils | TimeUtils    │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        System Layer                         │
│  ├── chroot()          - Root directory switching           │
│  ├── mount()           - Filesystem mounting                │
│  ├── setrlimit()       - Resource limit setting             │
│  ├── fork()/exec()     - Process creation and execution     │
│  └── Signal Handling   - Inter-process communication        │
└─────────────────────────────────────────────────────────────┘
```

### Component Relationships

```
┌──────────────────┐
│   ConfigManager  │◄──────────────────┐
│   (Config Mgmt)  │                   │
└────────┬─────────┘                   │
         │ Read configuration          │
         ▼                             │
┌──────────────────┐                   │
│   GlobalConfig   │                   │
│   EnvironmentCfg │                   │
└────────┬─────────┘                   │
         │                             │
         ▼                             │
┌──────────────────┐         ┌─────────┴─────────┐
│  JailManagerPool │         │   JSON Parser     │
│   (Mgmt Pool)    │         │   (Lightweight)   │
└────────┬─────────┘         └───────────────────┘
         │
         │ Create/Get
         ▼
┌──────────────────┐
│  JailManager     │
│  (Single Env Mgmt│
│  ├── create()    │◄── Initialize environment
│  ├── start()     │◄── Start SSH
│  ├── stop()      │◄── Stop service
│  ├── execute()   │◄── Execute command
│  └── destroy()   │◄── Clean up resources
└──────────────────┘
         │
         ├──► /proc, /sys mounting
         ├──► SSH service management
         ├──► Resource limit setting
         └──► chroot switching
```

---

## Core Components

### 1. ConfigManager (Configuration Management)

**Responsibility**: Parse and manage JSON configuration files

**Main Functions**:

```cpp
class ConfigManager {
    // Load configuration
    bool load(const std::string& path = "");
    
    // Save configuration
    bool save(const std::string& path = "");
    
    // Validate configuration
    bool validate();
    
    // Find environment
    const EnvironmentConfig* findEnvironment(const std::string& name);
    
    // Generate default configuration
    static ConfigManager createDefault();
};
```

**Data Structures**:

```cpp
struct GlobalConfig {
    std::string base_path;      // Environment root directory
    std::string log_path;       // Log directory
    std::string log_level;      // Log level
    bool daemonize;             // Whether to run in background
    std::vector<EnvironmentConfig> environments;
};

struct EnvironmentConfig {
    std::string name;           // Environment name
    std::string os_template;    // System template
    SshConfig ssh;              // SSH configuration
    ResourceLimits limits;      // Resource limits
    std::vector<MountPoint> mounts;  // Mount points
    std::vector<UserConfig> users;   // User configuration
};
```

### 2. JailManager (Environment Management)

**Responsibility**: Manage the lifecycle of a single chroot environment

**Main Functions**:

```cpp
class JailManager {
    // Lifecycle management
    bool create();              // Create environment
    bool start();               // Start environment
    bool stop();                // Stop environment
    bool destroy();             // Destroy environment
    
    // Command execution
    std::string execute(const std::string& command);
    
    // Status query
    JailStatus getStatus() const;
    JailRuntimeInfo getRuntimeInfo() const;
};
```

**State Machine**:

```
                    ┌─────────┐
                    │ STOPPED │
                    └────┬────┘
                         │ create()
                         ▼
                    ┌─────────┐
        stop() ───► │STARTING │──► start()
         error      └────┬────┘
               ◄─────────┘
                   │
                   ▼
              ┌─────────┐
              │ RUNNING │
              └────┬────┘
                   │ stop()
                   ▼
              ┌────────┐
              │STOPPING│──► stop()
              └────────┘
                   │
                   ▼
              ┌─────────┐
              │ STOPPED │
              └─────────┘
```

### 3. JSON Parser (Lightweight Parser)

**Responsibility**: JSON parsing without external libraries

**Implementation Features**:

- Recursive descent parsing
- Support for standard JSON types
- Exception handling mechanism

```cpp
class JsonParser {
    JsonValue parse();                    // Main parsing entry point
    static JsonValue parseFromString(const std::string& json);
};

class JsonValue {
    // Type checking
    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;
    
    // Value access
    bool asBool() const;
    double asNumber() const;
    std::string asString() const;
};
```

### 4. Logger (Logging System)

**Responsibility**: Unified log recording and management

```cpp
class Logger {
    bool initialize(const std::string& log_path, LogLevel level);
    void addSink(std::unique_ptr<LogSink> sink);
    
    void debug(const std::string& category, const std::string& message);
    void info(const std::string& category, const std::string& message);
    void warning(const std::string& category, const std::string& message);
    void error(const std::string& category, const std::string& message);
};
```

**Log Levels**:

```
CRITICAL > ERROR > WARNING > INFO > DEBUG
```

### 5. Utility Library (Utils)

Provides common auxiliary functions:

- **PathUtils**: Path normalization, security checks
- **StringUtils**: String processing, JSON escaping
- **FileUtils**: File reading/writing, directory operations
- **TimeUtils**: Time formatting
- **CommandUtils**: Command execution

---

## Data Flow

### Configuration Loading Flow

```
agi_config.json
      │
      ▼
┌──────────────┐
│ FileUtils    │──► Read file content
│   ::read()   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ JsonParser   │──► Parse JSON
│   ::parse()  │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ ConfigManager│──► Populate structures
│  ::load()    │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ GlobalConfig │◄─ Validate configuration
│EnvironmentCfg│   ::validate()
└──────────────┘
```

### Environment Creation Flow

```
agi create <name>
      │
      ▼
┌──────────────┐
│ CliHandler   │──► Parse command
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ JailManager  │──► Find configuration
│   ::create() │
└──────┬───────┘
       │
       ├──► createDirectoryStructure()
       │         │
       │         ▼
       │     ┌──────────────┐
       │     │ std::filesys │──► Create directory tree
       │     │   t::create_ │
       │     │  directories │
       │     └──────────────┘
       │
       ├──► copyBaseSystem()
       │         │
       │         ▼
       │     ┌──────────────┐
       │     │ copyBinary() │──► Copy system files
       │     │ copyLibrary()│──► Copy shared libraries
       │     └──────────────┘
       │
       ├──► createDeviceNodes()
       │         │
       │         ▼
       │     ┌──────────────┐
       │     │   mknod()    │──► Create device nodes
       │     └──────────────┘
       │
       └──► generateSshConfig()
                 │
                 ▼
             ┌──────────────┐
             │  Write sshd_ │──► SSH configuration
             │    config    │
             └──────────────┘
```

### Environment Startup Flow

```
agi start <name>
      │
      ▼
┌──────────────┐
│ CliHandler   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ JailManager  │──► Status check
│   ::start()  │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ setResource  │──► Set resource limits
│   Limits()   │         │
└──────┬───────┘         ▼
       │           ┌──────────────┐
       │           │ setrlimit()  │──► CPU, memory, file size
       │           └──────────────┘
       │
       ▼
┌──────────────┐
│ mountFilesys │──► Mount filesystems
│    tems()    │         │
└──────┬───────┘         ▼
       │           ┌──────────────┐
       │           │   mount()    │──► /proc, /sys, /dev/pts
       │           └──────────────┘
       │
       ▼
┌──────────────┐
│  startSshd() │──► Start SSH service
└──────┬───────┘         │
       │           ┌─────┴─────┐
       │           ▼           ▼
       │     ┌──────────┐ ┌──────────┐
       │     │  fork()  │ │ chroot() │──► Switch root directory
       │     └──────────┘ └────┬─────┘
       │                       │
       │                       ▼
       │                 ┌──────────┐
       │                 │ execlp() │──► Start sshd
       │                 └──────────┘
       │
       ▼
┌──────────────┐
│ runInitScrip │──► Execute initialization script
│      t()     │
└──────────────┘
       │
       ▼
   Change status to RUNNING
```

---

## Security Mechanisms

### Multi-Layer Isolation Model

```
┌─────────────────────────────────────────┐
│            Network Isolation Layer      │
│     (SSH port binding, IP whitelist)    │
├─────────────────────────────────────────┤
│            Resource Limit Layer         │
│     (CPU, memory, file size limits)     │
├─────────────────────────────────────────┤
│         Filesystem Isolation Layer      │
│         (chroot + mount namespaces)     │
├─────────────────────────────────────────┤
│            Capability Limit Layer       │
│         (Linux Capabilities)            │
└─────────────────────────────────────────┘
```

### Security Checkpoints

#### 1. Path Validation

```cpp
// Prevent directory traversal attacks
if (!PathUtils::isWithin(path, base_path)) {
    throw ConfigError("Path out of bounds");
}
```

#### 2. Port Range Check

```cpp
if (port < 1 || port > 65535) {
    throw ConfigError("Invalid port range");
}
```

#### 3. Sensitive Path Protection

```cpp
// Mount paths prohibited in configuration
static const std::vector<std::string> BLOCKED_PATHS = {
    "/etc/shadow",
    "/etc/passwd",
    "/var/lib",
    "/var/run"
};
```

### Resource Limits

```cpp
struct ResourceLimits {
    long max_cpu_time;       // CPU time limit (seconds)
    long max_memory;         // Memory limit (KB)
    long max_file_size;      // File size limit (KB)
    int max_processes;       // Process count limit
    int max_open_files;      // Open file count limit
};
```

---

## Extension Guide

### Adding New System Templates

1. Add template definition in configuration file:

```json
{
  "templates": {
    "my-template": {
      "packages": ["package1", "package2"],
      "mirror": "http://mirror.example.com"
    }
  }
}
```

2. Implement template installation logic in `jail.hpp`:

```cpp
void installTemplate(const std::string& template_name) {
    if (template_name == "my-template") {
        installMyTemplate();
    }
}
```

### Adding New Mount Types

1. Add new fields in `MountPoint` struct:

```cpp
struct MountPoint {
    std::string source;
    std::string target;
    std::string type;
    unsigned long flags;
    bool read_only;
    // New field
    std::string options;  // Mount options
};
```

2. Add processing logic in `mountFilesystems()`:

```cpp
void mountFilesystems() {
    // ... existing code
    
    for (const auto& mount : config_->mounts) {
        if (mount.type == "cifs") {
            mountCifs(mount);
        }
    }
}
```

### Adding New Resource Limits

1. Add fields in `ResourceLimits` struct:

```cpp
struct ResourceLimits {
    // ... existing fields
    long max_stack_size;     // Stack size limit
};
```

2. Implement in `setResourceLimits()`:

```cpp
void setResourceLimits() {
    // ... existing code
    
    rl.rlim_cur = config_->limits.max_stack_size * 1024;
    rl.rlim_max = config_->limits.max_stack_size * 1024;
    setrlimit(RLIMIT_STACK, &rl);
}
```

### Adding New Commands

1. Add handling function in `main.cpp`:

```cpp
bool cmdMyCommand(const std::vector<std::string>& args) {
    // Implement logic
    return true;
}
```

2. Add to command dispatch:

```cpp
if (command == "mycommand") {
    result = cmdMyCommand(args);
}
```

---

## Development Guide

### Development Environment Setup

```bash
# Install build tools
sudo apt-get install build-essential

# Clone project
cd agi

# Compile
make clean
make
```

### Debugging Tips

#### 1. Enable Verbose Output

```bash
./agi --verbose <command>
```

#### 2. Using GDB Debugging

```bash
gdb ./agi
(gdb) run <command>
(gdb) bt        # View call stack
(gdb) break <function_name>
(gdb) continue
```

#### 3. Memory Analysis

```bash
valgrind --leak-check=full ./agi <command>
```

### Code Standards

#### Naming Conventions

- Class names: PascalCase
- Function names: snake_case
- Constants: SCREAMING_SNAKE_CASE
- Member variables: prefix with underscore (_member)

#### Comment Standards

- Each public interface requires Doxygen comments
- Complex logic requires inline comments
- Keep comments synchronized with code updates

#### Example

```cpp
/**
 * @brief Create new jail environment
 * 
 * @param name Environment name
 * @return true Creation successful
 * @return false Creation failed
 */
bool create(const std::string& name) {
    // Check if name is valid
    if (name.empty()) {
        return false;
    }
    
    // Create directory structure
    createDirectoryStructure();
    
    return true;
}
```

### Testing

```bash
# Run configuration validation
./agi validate

# Run functional tests
./agi create test-env
./agi start test-env
./agi status test-env
./agi stop test-env
./agi remove test-env
```

### Contribution Guide

1. Fork the project repository
2. Create feature branch (`git checkout -b feature/xxx`)
3. Commit changes (`git commit -m "Add feature xxx"`)
4. Push to branch (`git push origin feature/xxx`)
5. Create Pull Request

---

## API Reference

### CliHandler

```cpp
class CliHandler {
    CliHandler(int argc, char* argv[]);
    int run();
    bool handleCommand(const std::string& command, 
                      const std::vector<std::string>& args);
};
```

### ConfigManager

```cpp
class ConfigManager {
    bool load(const std::string& path = "");
    bool save(const std::string& path = "");
    bool validate();
    const GlobalConfig& getConfig() const;
    const EnvironmentConfig* findEnvironment(const std::string& name);
    static ConfigManager createDefault();
};
```

### JailManager

```cpp
class JailManager {
    explicit JailManager(const EnvironmentConfig& config);
    
    bool create();
    bool start();
    bool stop();
    bool destroy();
    
    std::string execute(const std::string& command);
    JailStatus getStatus() const;
    JailRuntimeInfo getRuntimeInfo() const;
    const std::string& getPath() const;
    bool exists() const;
};
```

### JailManagerPool

```cpp
class JailManagerPool {
    void setLogger(std::function<void(const std::string&, const std::string&)> logger);
    bool addEnvironment(const EnvironmentConfig& config);
    bool removeEnvironment(const std::string& name);
    JailManager* getJail(const std::string& name);
    std::vector<std::string> listJails() const;
};
```

### Logger

```cpp
class Logger {
    bool initialize(const std::string& log_path = "", LogLevel level = LogLevel::INFO);
    void addSink(std::unique_ptr<LogSink> sink);
    void debug(const std::string& category, const std::string& message);
    void info(const std::string& category, const std::string& message);
    void warning(const std::string& category, const std::string& message);
    void error(const std::string& category, const std::string& message);
    void critical(const std::string& category, const std::string& message);
};
```

---

**Version**: 1.0.0
**Last Updated**: 2025-12-28
**Author**: LOTUS-AGI Team

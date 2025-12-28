# AGI - Automated Guest Isolation

A lightweight, secure chroot environment management tool built in pure C++. AGI enables developers and system administrators to create, manage, and isolate multiple sandboxed environments on Linux systems with minimal dependencies and maximum control.

## Overview

AGI (Automated Guest Isolation) addresses the common challenge of needing isolated environments for development, testing, and security-sensitive operations. Whether you are evaluating untrusted code, setting up CI/CD build pipelines, or creating teaching laboratories, AGI provides a robust and straightforward solution for filesystem isolation without the overhead of full virtualization.

Traditional approaches to environment isolation often require significant resources or complex configuration. Virtual machines provide strong isolation but consume substantial memory and CPU resources. Containers offer a lighter alternative but require kernel support and additional tooling. AGI takes a different approach by leveraging the proven chroot mechanism available in Linux systems, combined with modern C++ implementation practices to deliver a tool that is both efficient and easy to use.

The project was born from the need for a simple, no-dependency solution that could be deployed quickly in environments where installing container runtimes or virtualization platforms was not feasible. By relying solely on the C++ Standard Library and POSIX system calls, AGI compiles to a single executable that can be distributed and run anywhere with a compatible Linux kernel.

## Key Features

AGI distinguishes itself through several core capabilities that make it suitable for a wide range of isolation scenarios. The architecture prioritizes simplicity without sacrificing essential security features, making it accessible to users who need quick isolation solutions without extensive configuration overhead.

The lightweight isolation mechanism forms the foundation of AGI's design philosophy. Using chroot technology, AGI creates isolated filesystem environments that provide meaningful separation between the host system and running environments. Unlike full virtualization solutions that require booting entire operating systems, chroot-based isolation starts instantly and consumes minimal resources. A typical AGI environment uses only a few megabytes of disk space and can be created in seconds, compared to gigabytes and minutes required for virtual machines.

Multi-environment management capabilities allow administrators to run numerous isolated environments simultaneously. Each environment operates completely independently, with its own filesystem tree, configuration, and running services. The centralized management interface enables operators to start, stop, monitor, and clean up environments through a consistent command-line interface, reducing the cognitive load of managing multiple sandboxes.

Integrated SSH access provides convenient connectivity to isolated environments. Each AGI environment can run its own SSH server, configurable per-environment with independent ports, authentication methods, and network bindings. This feature proves particularly valuable when accessing environments from remote locations or when graphical or interactive terminal access is required.

Comprehensive resource limiting ensures that isolated environments cannot negatively impact the host system. Administrators can configure CPU time limits, memory caps, file size restrictions, process counts, and open file limits. These restrictions apply to all processes running within the environment, preventing runaway processes or malicious code from consuming host resources.

Secure mounting options enable controlled data sharing between host and environment. AGI supports read-only and read-write mounts of specific host directories, allowing precise control over what data the environment can access and modify. Sensitive paths can be protected from mounting, preventing accidental or intentional access to critical host files.

The pure C++ implementation guarantees portability and deployment simplicity. With no external library dependencies beyond the C++ Standard Library, AGI compiles to a static binary that runs on any Linux system meeting the minimum kernel requirements. This characteristic makes AGI suitable for embedded systems, container environments, and minimal Linux installations where package availability is limited.

## Architecture

AGI follows a layered architecture that separates concerns and enables maintainability. The design philosophy emphasizes clear interfaces between components, allowing users to understand and potentially extend the system without deep knowledge of internal implementation details.

The user interaction layer consists of the command-line wrapper script (`agi-wrapper.sh`) that provides a friendly interface for common operations. This script handles privilege elevation, logging, and user-friendly output formatting. Users interact primarily with this layer, which translates intuitive commands into operations on the core system.

The application layer contains the command dispatcher and environment pool manager. The command handler parses user input and routes requests to appropriate functions, while the environment pool maintains references to all active jail instances. This layer handles the lifecycle of jail objects and coordinates between configuration loading and runtime management.

The core layer implements the fundamental functionality of AGI. The configuration manager reads and validates JSON configuration files, providing typed access to all settings. The jail manager handles the creation, starting, stopping, and destruction of individual chroot environments, including filesystem setup, mount management, SSH service control, and resource limit application. The logging system provides consistent, configurable logging across all components.

The utility library provides supporting functions used throughout the system. Path utilities handle path normalization and security checks to prevent directory traversal attacks. String utilities perform text processing and JSON escaping. File utilities abstract filesystem operations with error handling. Time utilities format timestamps for logging. Command utilities execute external processes when required.

The system layer interfaces directly with Linux kernel features. Chroot system calls change the root filesystem for processes. Mount system calls establish filesystem views within the jail. Setrlimit system calls apply resource constraints. Fork and exec system calls create and run processes within the jail context. Signal handling ensures clean shutdown and proper cleanup of resources.

## Installation

AGI supports multiple installation methods to accommodate different deployment scenarios. The simplest approach involves using the provided installation script, while advanced users may prefer compiling from source or integrating with system package managers.

### System Requirements

Before installation, ensure your system meets the minimum requirements. AGI requires a Linux kernel version 3.10 or higher, which covers virtually all modern Linux distributions including CentOS 7+, Ubuntu 14.04+, and Debian 8+. The compiler must support C++17 features; GCC 7.0 or later and Clang 5.0 or later are fully supported. Make or CMake is required for building from source. Root privileges are necessary for installation because AGI requires access to system calls for chroot, mount, and resource limiting operations.

### Quick Install

The fastest way to get AGI running on your system is to use the automated installation script. First, clone or download the project repository to your local machine. Navigate to the project directory and execute the installation script with root privileges.

```bash
# clone the project
cd agi
sudo ./scripts/installer.sh install
```

The installation script performs several actions: creating necessary directories, copying the binary and supporting files, setting appropriate permissions, and generating a default configuration file. After installation completes, verify the setup by checking the version information.

```bash
agi --version
agi help
```

### Building from Source

For users who prefer to compile AGI themselves, the build process is straightforward. Ensure you have the necessary build tools installed, then compile the project using Make.

```bash
# Install build dependencies (Debian/Ubuntu)
sudo apt-get install build-essential

# Clone and build
# clone the project
cd agi
make clean
make
```

The compiled binary will appear in the project root directory. You can run it directly or install it system-wide using the installation script.

### Uninstalling

If you need to remove AGI from your system, the installation script also provides an uninstallation function. This operation removes all installed files while preserving runtime data and logs by default.

```bash
sudo ./scripts/installer.sh uninstall
```

## Usage Guide

AGI provides a comprehensive command-line interface for managing isolated environments. Understanding the basic workflow enables productive use within minutes.

### Initializing AGI

Before creating environments, initialize AGI's directory structure and default configuration. This step creates the necessary directories and generates a configuration file that you can customize for your needs.

```bash
sudo agi init
```

The initialization process creates configuration at `/etc/agi/agi_config.json`, data directories at `/var/lib/agi/jails/`, and log directories at `/var/log/agi/`. After initialization, you may edit the configuration file to define your environments.

### Creating Environments

Environments define the isolated sandboxes where operations occur. Create an environment using the configuration you've defined, or let AGI use default settings for a basic jail.

```bash
sudo agi create my-env
```

This command creates a new jail named "my-env" with the configuration specified in your config file. The creation process sets up the filesystem hierarchy, copies essential binaries and libraries, generates SSH configuration, and prepares the environment for operation.

### Starting and Stopping

Once created, environments can be started and stopped independently. Starting a jail activates the SSH server and makes the environment accessible.

```bash
sudo agi start my-env
sudo agi status my-env
```

When finished, stop the environment to release resources and halt all running processes within the jail.

```bash
sudo agi stop my-env
```

### Connecting to Environments

After starting an environment, connect via SSH using the agi command or a standard SSH client.

```bash
# Using agi wrapper
sudo agi ssh my-env

# Using standard SSH
ssh -p 2201 root@127.0.0.1
```

The default SSH configuration listens on localhost only, preventing external access. Modify the configuration to expose SSH on network interfaces if required.

### Executing Commands

For non-interactive command execution, use the exec command to run specific operations within an environment.

```bash
sudo agi exec my-env "ls -la /"
sudo agi exec my-env "cat /etc/os-release"
```

### Removing Environments

When an environment is no longer needed, remove it completely to free disk space. This action permanently deletes all data within the environment.

```bash
sudo agi remove my-env
```

## Configuration Reference

The configuration file controls all aspects of AGI's behavior. Understanding the available options enables fine-tuned control over environments.

### Global Configuration

The global section sets overall AGI behavior independent of specific environments.

```json
{
  "global": {
    "app_name": "agi",
    "base_path": "/var/lib/agi",
    "config_path": "/etc/agi",
    "log_path": "/var/log/agi",
    "log_level": "INFO",
    "daemonize": false
  }
}
```

The `base_path` specifies where AGI stores jail data, while `log_path` determines log file location. The `log_level` controls verbosity with values DEBUG, INFO, WARNING, ERROR, and CRITICAL.

### Environment Configuration

Each environment in the `environments` array defines a separate isolated jail.

```json
{
  "environments": [
    {
      "name": "dev-env",
      "description": "Development environment",
      "os_template": "debian",
      "ssh": {
        "port": 2201,
        "listen_address": "127.0.0.1",
        "password_auth": true,
        "pubkey_auth": true,
        "max_auth_tries": 3,
        "client_alive_interval": 300
      },
      "limits": {
        "max_cpu_time": 300,
        "max_memory": 524288,
        "max_file_size": 1048576,
        "max_processes": 64,
        "max_open_files": 256
      },
      "mounts": [
        {
          "source": "/home/dev/project",
          "target": "/project",
          "type": "none",
          "read_only": true
        }
      ],
      "enabled": true
    }
  ]
}
```

The `limits` section specifies resource constraints measured in seconds for CPU time and kilobytes for memory and file size. The `mounts` array defines host directories to expose within the jail, with the `read_only` flag controlling write access.

### Complete Configuration Example

This comprehensive example demonstrates typical AGI configuration for a multi-environment setup.

```json
{
  "global": {
    "app_name": "agi",
    "base_path": "/var/lib/agi",
    "config_path": "/etc/agi",
    "log_path": "/var/log/agi",
    "log_level": "INFO"
  },
  "environments": [
    {
      "name": "dev-env",
      "description": "Primary development environment",
      "os_template": "debian",
      "ssh": {
        "port": 2201,
        "listen_address": "127.0.0.1",
        "password_auth": true,
        "pubkey_auth": true
      },
      "limits": {
        "max_cpu_time": 600,
        "max_memory": 1048576
      },
      "mounts": [
        {
          "source": "/home/developer",
          "target": "/host-home",
          "read_only": true
        }
      ],
      "enabled": true
    },
    {
      "name": "ci-sandbox",
      "description": "CI/CD testing environment",
      "os_template": "debian",
      "ssh": {
        "port": 2202,
        "listen_address": "127.0.0.1"
      },
      "limits": {
        "max_cpu_time": 300,
        "max_memory": 524288,
        "max_file_size": 1048576
      },
      "enabled": true
    }
  ]
}
```

## Common Use Cases

AGI serves various isolation scenarios in development, testing, and operational contexts. Understanding these patterns helps users apply AGI effectively.

### Development Environment Isolation

When working with unfamiliar code or testing builds that modify the system, AGI provides a safe sandbox. Create an environment, start it, and perform all development activities within the jail. Any problematic operations affect only the jail, leaving the host system untouched. This approach proves valuable when experimenting with build systems, package managers, or configuration changes that might have unintended consequences.

```bash
# Create safe space for risky builds
sudo agi create build-sandbox
sudo agi start build-sandbox
sudo agi ssh build-sandbox
# Perform potentially destructive operations
exit
sudo agi stop build-sandbox
sudo agi remove build-sandbox
```

### CI/CD Pipeline Sandboxing

Continuous integration environments often build untrusted code from external contributors. AGI enables sandboxes for each build, preventing malicious code from affecting the build infrastructure. Configure resource limits to prevent builds from consuming excessive resources, and mount only necessary source code directories read-only.

### Educational Laboratories

Training environments benefit from AGI's isolation because students can practice system administration without risk. Instructors create pre-configured environments for exercises, and students can experiment freely. Mistakes inside the jail have no impact on the training machine, and instructors can easily reset environments by removing and recreating them.

### Security Research

When analyzing potentially malicious software, researchers require isolation to prevent damage to analysis infrastructure. AGI provides lightweight sandboxes that can be created, analyzed, and discarded rapidly. Configure strict resource limits and network isolation to contain threats while allowing observation.

### Multi-Tenant Development

Development teams sharing build servers benefit from environment isolation because each developer can have a private jail without interfering with others. Configure environments with appropriate permissions and mounts for each team member's needs.

## Security Considerations

While AGI provides meaningful isolation, understanding its limitations helps users apply it appropriately.

### Isolation Boundaries

AGI's chroot-based isolation provides filesystem separation but does not offer kernel-level isolation. Processes running with root privileges inside a jail retain significant capabilities and can potentially access kernel interfaces. For high-security requirements, consider container runtimes like Docker or full virtualization solutions.

### Root User Privileges

Root users inside a jail retain root privileges within that jail's context. This means malicious code running as root inside a jail can still perform privileged operations limited to the jail environment. Consider using non-root users when running untrusted code.

### Resource Limits

Configured resource limits apply to processes within the jail but depend on proper kernel enforcement. Test resource limits in your environment to ensure they behave as expected before relying on them for security purposes.

### Network Exposure

Default SSH configuration binds to localhost only, preventing external access. When exposing SSH on network interfaces, implement additional authentication controls and consider firewall rules to restrict access.

### Recommended Practices

Apply the principle of least privilege when configuring environments. Run non-root users whenever possible. Use read-only mounts for sensitive host directories. Set restrictive resource limits based on expected workload. Regularly update AGI and system packages. Monitor resource usage and logs for anomalies. Remove unused environments promptly to reduce attack surface.

## Contributing

Contributions are welcome and appreciated. Before starting significant work, open an issue to discuss proposed changes and ensure alignment with project goals.

To contribute, fork the repository, create a feature branch for your changes, make modifications following the coding standards documented in ARCHITECTURE.md, test thoroughly, and submit a pull request with a clear description of the changes.

## License

AGI is distributed under the BSL1 License, allowing free use, modification, and distribution with appropriate attribution.

## Contact

For questions, issues, or contributions, please use the GitHub repository issue tracker to report bugs and request features.

---

> LOTUS-AGI TEAM: [BLUE LOTUS](https://lotuschain.org/team)
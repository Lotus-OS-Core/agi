# AGI User Manual

## Table of Contents

1. [Introduction](#introduction)
2. [Installation Guide](#installation-guide)
3. [Quick Start](#quick-start)
4. [Configuration Guide](#configuration-guide)
5. [Command Reference](#command-reference)
6. [Advanced Features](#advanced-features)
7. [Troubleshooting](#troubleshooting)
8. [Security Considerations](#security-considerations)

---

## Introduction

### What is AGI

AGI (Automated Guest Isolation) is a lightweight chroot environment management tool based on C++. It allows users to create and manage multiple isolated environments on Linux systems, each with its own filesystem, network configuration, and resource limits. AGI is specifically designed for scenarios requiring secure isolated environments for development, testing, or sandbox operations.

### Key Features

- **Lightweight Isolation**: Uses chroot technology for filesystem isolation without full virtual machine overhead
- **Multi-Environment Management**: Supports managing multiple independent isolated environments simultaneously
- **SSH Access**: Each environment can be configured with independent SSH access ports
- **Resource Limits**: Configurable CPU, memory, file size, and other resource limits
- **Secure Mounting**: Supports read-only mounting of sensitive directories to prevent malicious operations within environments
- **Pure C++ Implementation**: Uses only standard libraries with no external dependencies, making deployment easy

### Use Cases

- Isolation of software development environments
- Execution environments for automated testing
- Secure sandboxes for third-party code
- Teaching and experimental environments
- Continuous integration build environments

---

## Installation Guide

### System Requirements

- Linux operating system (kernel 3.10+)
- GCC 7.0+ or Clang 5.0+
- Make tool
- Root privileges (for chroot and mount operations)

### Compile and Install

1. **Clone or download the source code**

```bash
# clone project
cd agi
```

2. **Compile the project**

```bash
make clean
make
```

3. **Install to system**

```bash
sudo ./scripts/installer.sh install
```

4. **Verify installation**

```bash
agi --version
agi help
```

### Uninstall

```bash
sudo ./scripts/installer.sh uninstall
```

### Running from Source

If you don't want to install to the system, you can directly use the compiled binary:

```bash
./agi help
```

---

## Quick Start

### Step 1: Initialize Configuration

First-time use requires initializing AGI configuration:

```bash
sudo agi init
```

This command will create:
- Configuration file: `/etc/agi/agi_config.json`
- Data directory: `/var/lib/agi/jails/`
- Log directory: `/var/log/agi/`

### Step 2: Edit Configuration File

Modify the configuration file according to your needs:

```bash
sudo nano /etc/agi/agi_config.json
```

For detailed configuration file explanations, see [Configuration Guide](#configuration-guide).

### Step 3: Create Environment

```bash
sudo agi create my-env
```

This command will create an isolated environment named `my-env`.

### Step 4: Start Environment

```bash
sudo agi start my-env
```

After starting, the environment will provide services through the configured SSH port.

### Step 5: Connect to Environment

```bash
sudo agi ssh my-env
```

Or use SSH to connect directly:

```bash
ssh -p 2201 root@127.0.0.1
```

### Step 6: Stop Environment

```bash
sudo agi stop my-env
```

---

## Configuration Guide

### Configuration File Structure

AGI uses JSON format configuration files, primarily containing the following sections:

```json
{
  "global": {
    // Global configuration
  },
  "environments": [
    // Environment definitions
  ],
  "templates": {
    // Template configuration
  },
  "security": {
    // Security configuration
  }
}
```

### Global Configuration

| Parameter | Description | Default Value |
|-----------|-------------|---------------|
| `app_name` | Application name | `agi` |
| `base_path` | Environment root directory | `/var/lib/agi` |
| `config_path` | Configuration directory | `/etc/agi` |
| `log_path` | Log directory | `/var/log/agi` |
| `log_level` | Log level | `INFO` |
| `daemonize` | Whether to run in background | `false` |

### Environment Configuration

Each environment definition contains the following parameters:

```json
{
  "name": "Environment name",
  "description": "Environment description",
  "os_template": "Operating system template",
  "ssh": {
    "port": SSH port number,
    "listen_address": Listen address,
    "password_auth": Whether to allow password authentication,
    "pubkey_auth": Whether to allow public key authentication
  },
  "limits": {
    "max_cpu_time": Maximum CPU time (seconds),
    "max_memory": Maximum memory (KB),
    "max_file_size": Maximum file size (KB)
  },
  "mounts": [
    {
      "source": "Source path",
      "target": "Target path",
      "type": "Filesystem type",
      "read_only": Whether read-only
    }
  ]
}
```

### SSH Configuration Example

```json
{
  "ssh": {
    "port": 2201,
    "listen_address": "127.0.0.1",
    "password_auth": true,
    "pubkey_auth": true,
    "max_auth_tries": 3,
    "client_alive_interval": 300,
    "banner_message": "Welcome to AGI Environment"
  }
}
```

### Resource Limits Example

```json
{
  "limits": {
    "max_cpu_time": 300,
    "max_memory": 524288,
    "max_file_size": 1048576,
    "max_processes": 64,
    "max_open_files": 256
  }
}
```

### Mount Configuration Example

```json
{
  "mounts": [
    {
      "source": "/proc",
      "target": "/proc",
      "type": "proc",
      "read_only": false
    },
    {
      "source": "/sys",
      "target": "/sys",
      "type": "sysfs",
      "read_only": true
    }
  ]
}
```

### Complete Configuration Example

```json
{
  "global": {
    "app_name": "agi",
    "base_path": "/var/lib/agi",
    "log_path": "/var/log/agi",
    "log_level": "INFO"
  },
  "environments": [
    {
      "name": "dev-env",
      "description": "Development environment",
      "os_template": "debian",
      "ssh": {
        "port": 2201,
        "listen_address": "127.0.0.1",
        "password_auth": true,
        "pubkey_auth": true
      },
      "limits": {
        "max_cpu_time": 300,
        "max_memory": 524288
      },
      "mounts": [
        {"source": "/proc", "target": "/proc", "type": "proc"},
        {"source": "/sys", "target": "/sys", "type": "sysfs"}
      ],
      "enabled": true
    }
  ]
}
```

---

## Command Reference

### Basic Commands

#### help

Display help information:

```bash
agi help
```

#### version

Display version information:

```bash
agi version
```

#### init

Initialize AGI configuration:

```bash
sudo agi init
```

### Environment Management Commands

#### create

Create a new isolated environment:

```bash
sudo agi create <environment_name>
```

Examples:

```bash
sudo agi create test-env
sudo agi create --template ubuntu dev-env
```

#### start

Start the specified environment:

```bash
sudo agi start <environment_name>
```

Examples:

```bash
sudo agi start test-env
```

#### stop

Stop the specified environment:

```bash
sudo agi stop <environment_name>
```

Examples:

```bash
sudo agi stop test-env
```

#### restart

Restart the specified environment:

```bash
sudo agi restart <environment_name>
```

#### remove

Delete the specified environment:

```bash
sudo agi remove <environment_name>
```

**Note**: This operation is irreversible and will permanently delete all data of the environment.

### Information View Commands

#### status

View the status of the specified environment:

```bash
agi status <environment_name>
```

Example output:

```
Environment status: test-env
  Status: Running
  Path: /var/lib/agi/jails/test-env
  SSH Port: 2202
  Access Address: 127.0.0.1
  Process PID: 12345
```

#### list

List all configured environments:

```bash
agi list
```

Example output:

```
Configured environments (2):
dev-env
  Status: Running (port 2201)

test-env
  Status: Stopped
```

#### validate

Validate configuration file:

```bash
agi validate
```

### Access Control Commands

#### ssh

SSH connect to the specified environment:

```bash
sudo agi ssh <environment_name>
```

#### exec

Execute commands in the environment:

```bash
sudo agi exec <environment_name> <command>
```

Examples:

```bash
sudo agi exec test-env "ls -la /"
sudo agi exec test-env "cat /etc/os-release"
```

### System Management Commands

#### backup

Backup environment:

```bash
sudo agi backup <environment_name>
```

Backup files are saved in the `/var/lib/agi/backups/` directory.

#### restore

Restore environment:

```bash
sudo agi restore <backup_file_path>
```

#### logs

View environment logs:

```bash
sudo agi logs <environment_name>
```

---

## Advanced Features

### Template System

AGI supports using operating system templates to quickly create environments. The `templates` section in the configuration file defines available templates:

```json
{
  "templates": {
    "debian": {
      "packages": ["bash", "coreutils", "procps"],
      "mirror": "http://deb.debian.org/debian"
    },
    "ubuntu": {
      "packages": ["bash", "coreutils", "procps"],
      "mirror": "http://archive.ubuntu.com/ubuntu"
    }
  }
}
```

### Resource Monitoring

AGI records resource usage for each environment. Log files are located at:

- Main log: `/var/log/agi/agi.log`
- Environment logs: `/var/lib/agi/jails/<environment_name>/var/log/`

### Batch Operations

Batch management can be achieved through scripts:

```bash
# Start all environments
for env in $(agi list | grep -v "Status:"); do
    agi start $env
done
```

### Integration with System Services

Configure AGI environment as a system service:

```bash
# Create systemd service file
sudo nano /etc/systemd/system/agi-dev-env.service
```

Service file content:

```ini
[Unit]
Description=AGI Development Environment
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/agi start dev-env
ExecStop=/usr/local/bin/agi stop dev-env
Restart=on-failure
User=root

[Install]
WantedBy=multi-user.target
```

Enable service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable agi-dev-env
sudo systemctl start agi-dev-env
```

---

## Troubleshooting

### Common Issues

#### Permission Errors

**Problem**: `Error: AGI requires root privileges to run`

**Solution**:

```bash
sudo agi <command>
```

#### Port Conflicts

**Problem**: `SSH port is already in use`

**Solution**: Modify the port number in the configuration file:

```json
{
  "ssh": {
    "port": 2205
  }
}
```

#### Environment Cannot Start

**Problem**: Environment fails to start

**Solution**:

1. Check logs: `/var/log/agi/agi.log`
2. Validate configuration: `agi validate`
3. Check if port is occupied: `netstat -tlnp | grep <port>`

#### Mount Failures

**Problem**: `mount: permission denied`

**Solution**:

1. Ensure running with root privileges
2. Check if mount point is correct
3. Confirm target directory is empty

#### SSH Connection Failures

**Problem**: Cannot connect to environment via SSH

**Solution**:

1. Confirm environment is running: `agi status <name>`
2. Check SSH configuration: Port number in `agi status <name>`
3. Verify firewall rules
4. Check if SSH service is running in the environment

### Viewing Logs

View AGI logs:

```bash
tail -f /var/log/agi/agi.log
```

View logs for specific environment:

```bash
tail -f /var/lib/agi/jails/<environment_name>/var/log/jail.log
```

### Debug Mode

Enable verbose output:

```bash
agi --verbose <command>
```

---

## Security Considerations

### Risk Warnings

Using chroot for isolation has the following limitations:

1. **Kernel-level Access**: Chroot does not provide kernel-level isolation; some system calls are still accessible within the environment
2. **Privilege Escalation Risk**: Root users still have elevated privileges within the chroot environment
3. **Resource Escape**: Improper configuration may allow resource limits to be bypassed

### Security Best Practices

1. **Principle of Least Privilege**
   - Do not run unnecessary services in the environment
   - Use read-only mounts for sensitive directories
   - Limit user permissions within the environment

2. **Network Isolation**
   - Default to listening only on loopback address (127.0.0.1)
   - For external access, use firewall to restrict source IP
   - Disable unnecessary network services

3. **Resource Limits**
   - Set reasonable resource limits as needed
   - Monitor resource usage
   - Stop environments that are no longer needed in a timely manner

4. **Access Control**
   - Use public key authentication instead of password authentication
   - Regularly change access credentials
   - Limit users who can access the environment

5. **Regular Maintenance**
   - Regularly update environment and system software
   - Regularly back up important data
   - Delete environments that are no longer needed in a timely manner

### Recommended Limit Configuration

```json
{
  "limits": {
    "max_cpu_time": 300,
    "max_memory": 524288,
    "max_file_size": 1048576,
    "max_processes": 64,
    "max_open_files": 256
  }
}
```

---

## Appendix

### Command Line Options

| Option | Description |
|--------|-------------|
| `-c, --config <file>` | Specify configuration file path |
| `-v, --verbose` | Display verbose output |
| `-h, --help` | Display help information |
| `--version` | Display version information |

### Default Port Allocation

| Environment | Default Port |
|-------------|--------------|
| dev-env | 2201 |
| test-env | 2202 |
| sandbox | 2203 |

### File Paths

| Path | Description |
|------|-------------|
| `/usr/local/bin/agi` | Main program |
| `/usr/local/bin/agi` | Command line tool |
| `/etc/agi/agi_config.json` | Main configuration file |
| `/var/lib/agi/jails/` | Environment root directory |
| `/var/log/agi/` | Log directory |

### Related Resources

- Issue reporting: ✓ issues
- Documentation updates: See docs/ directory in project repository

---

**Version**: 1.0.0
**Last Updated**: 2025-12-28
**Author**: LOTUS-AGI Team

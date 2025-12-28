## Understanding AGI's Chroot Isolation and Access Boundaries

### How Chroot Isolation Works in AGI

AGI uses **chroot** (change root) technology to create isolated filesystem environments. Here's how the access boundaries work:

#### Filesystem Isolation

When AGI creates and runs a jail environment:

1. **Inside the Chroot**: The process sees only the directory tree within the jail root. For example, if the jail is at `/var/lib/agi/jails/my-env`, the process sees:
   - `/` → Actually `/var/lib/agi/jails/my-env/`
   - `/etc` → Actually `/var/lib/agi/jails/my-env/etc`
   - `/home` → Actually `/var/lib/agi/jails/my-env/home`

2. **Outside the Chroot**: The host system's files remain completely invisible and inaccessible from within the jail, unless explicitly mounted.

#### What AGI Can Access Outside the Chroot

AGI itself (running on the host) **CAN** access outside resources because:

- **The Manager Process Runs on Host**: The main `agi` binary runs as root on the host system and has full access to the host filesystem
- **Controlled Mounting**: AGI can selectively mount host directories into the jail (like `/proc`, `/sys`, or specific data directories) for controlled access
- **Network Access**: The SSH service inside the jail can be configured to listen on host network interfaces

#### What Processes Inside the Jail Can Access

Processes running **inside** a jail environment:

| Resource | Access Level | Example |
|----------|--------------|---------|
| Host filesystem | **BLOCKED** by default | Cannot read `/etc/shadow` |
| Jail filesystem only | **FULL** | Can read/write within jail |
| Network | **CONFIGURABLE** | SSH port exposed or localhost-only |
| System calls | **LIMITED** | Cannot escape chroot easily |
| Host processes | **BLOCKED** | Cannot see or kill host processes |

### Practical Use Cases

#### 1. **Development Environment Isolation**

```
Scenario: Testing code that might modify the system

Host: Clean production system
├── agi manager (has full host access)
└── Jail: test-env
    ├── Untrusted code runs here
    ├── Can install packages freely
    ├── Changes stay contained
    └── If code does "rm -rf /", only destroys test-env, not host
```

**Example Workflow**:
```bash
# Developer wants to test a build script that modifies system
sudo agi create build-test
sudo agi start build-test
sudo agi ssh build-test
# Inside jail: ./build-script.sh -- might break things, but safe!
exit
sudo agi stop build-test
sudo agi remove build-test  # Clean up completely
```

#### 2. **CI/CD Build Sandboxing**

```
Scenario: Running untrusted third-party builds

Host: CI/CD infrastructure
├── agi manager
└── Jail: ci-sandbox-1 (build #1234)
    ├── Build executes here
    ├── Cannot access secrets on host
    ├── Cannot interfere with other builds
    └── Artifacts copied out, jail discarded
```

**Benefits**:
- Builds cannot corrupt host system
- Secrets remain secure (not mounted into jail)
- Each build gets fresh environment
- Resource limits prevent DoS attacks on host

#### 3. **Teaching and Training Environments**

```
Scenario: Computer lab training

Host: Teacher's workstation
├── agi manager
└── Jail: student-env-1 (for Student A)
    ├── Can practice Linux commands
    ├── Can make mistakes safely
    ├── "rm -rf /" only affects their jail
    └── Teacher can monitor and reset
```

**Example**:
```bash
# Teacher creates a "broken" environment for students to fix
sudo agi create lab1
sudo agi start lab1
sudo agi ssh lab1
# Student sees a messed-up system to practice recovery
```

#### 4. **Testing Untrusted Software**

```
Scenario: Running downloaded binaries from unknown sources

Host: Secure production system
├── agi manager
└── Jail: sandbox
    ├── Binary executes here
    ├── Cannot write to sensitive host directories
    ├── Limited CPU/memory prevents crypto mining
    ├── Network access can be disabled or monitored
    └── After testing: sudo agi remove sandbox
```

**Configuration for Maximum Security**:
```json
{
  "name": "sandbox",
  "limits": {
    "max_cpu_time": 60,
    "max_memory": 131072,
    "max_file_size": 10240
  },
  "ssh": {
    "listen_address": "127.0.0.1"
  }
}
```

#### 5. **Multi-Tenant Development Teams**

```
Scenario: Multiple developers sharing a build server

Host: Shared build server
├── agi manager
├── Jail: dev-alice
│   └── Alice's projects
├── Jail: dev-bob
│   └── Bob's projects
└── Jail: dev-charlie
    └── Charlie's projects

Each developer:
├── Has their own isolated environment
├── Cannot interfere with others' work
├── Can customize their jail as needed
└── Cannot access others' code
```

#### 6. **Plugin/Script Testing**

```
Scenario: Testing browser plugins or IDE extensions

Host: Developer machine
├── agi manager
└── Jail: plugin-test
    ├── Clean browser installation
    ├── Extension installed here
    ├── Can monitor behavior safely
    └── "Reset" by removing and recreating jail
```

### Access Control Examples

#### Scenario A: Jail with NO external access
```bash
# Configuration - no mounts, localhost-only SSH
{
  "name": "isolated-test",
  "mounts": [],  # No mounts = no host access
  "ssh": {
    "listen_address": "127.0.0.1"  # Localhost only
  }
}
```
**Result**: Jail is completely sealed off, can only communicate via SSH on localhost.

#### Scenario B: Jail with read-only host access
```bash
# Configuration - read-only mount of code directory
{
  "name": "build-env",
  "mounts": [
    {
      "source": "/home/dev/project",
      "target": "/project",
      "type": "none",
      "read_only": true
    }
  ]
}
```
**Result**: Jail can read code from host but cannot modify it.

#### Scenario C: Jail with data volume access
```bash
# Configuration - read-write access to specific data dir
{
  "name": "data-processing",
  "mounts": [
    {
      "source": "/data/input",
      "target": "/input",
      "read_only": true
    },
    {
      "source": "/data/output",
      "target": "/output",
      "read_only": false
    }
  ]
}
```
**Result**: Can read from `/input`, write results to `/output`, but cannot access other host directories.

### Summary

AGI's chroot isolation provides:

- **Host Protection**: Processes inside jail cannot access host filesystem
- **Resource Safety**: Configurable limits prevent resource exhaustion
- **Network Control**: SSH can be restricted to localhost or exposed
- **Audit Trail**: All operations logged for debugging
- **Clean Recovery**: Simply destroy and recreate jail to reset state

The key principle: **AGI the manager has full host access, but the jails it creates are isolated and controlled**.

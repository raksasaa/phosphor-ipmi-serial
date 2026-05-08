# phosphor-ipmi-serial: 使用说明与测试方案

本文档提供了 `phosphor-ipmi-serial` 守护进程的使用说明、配置选项以及一套完整的测试方案，用于验证其在重构后的稳健性、性能和正确性。

---

## 1. 程序使用说明

`phosphor-ipmi-serial` 是一个为 OpenBMC 设计的，通过串口（UART）实现 IPMI 协议的守护进程。它实现了 IPMI v2.0 规范中的串行/调制解调器接口，作为一个独立的 IPMI 通道运行。

### 构建程序

项目使用 Meson 和 Ninja 进行构建。

```bash
# 在项目根目录
meson setup build
ninja -C build
```
可执行文件将生成在 `build/` 目录下。

### 运行程序

```bash
# 示例：使用 /dev/ttyAMA1，波特率 115200，运行在 terminal 模式
./build/phosphor-ipmi-serial -d /dev/ttyAMA1 -b 115200 -m terminal -c serial0
```

### 命令行参数

| 选项 | 长选项 | 描述 | 默认值 |
| :--- | :--- | :--- | :--- |
| `-d` | `--device` | 串口设备路径 | `/dev/ttyAMA1` |
| `-b` | `--baud` | 波特率 (e.g., 9600, 115200) | `115200` |
| `-m` | `--mode` | 协议模式 (`terminal` 或 `basic`) | `terminal` |
| `-c` | `--channel` | IPMI 通道名称 | `serial0` |
| `-v` | `--verbose` | 启用详细日志输出 | (关闭) |
| | `--debug-frames` | 在日志中打印原始 IPMI 帧的十六进制转储 | (关闭) |
| | `--session-timeout`| 会话超时时间 (秒) | `60` |
| | `--max-retries`| 最大重试次数 | `3` |
| `-h` | `--help` | 显示帮助信息 | |

---

## 2. 功能与风险验证测试方案

本测试方案旨在验证前期代码审计中发现的核心风险点是否已通过重构得到修复。

### 测试环境

- **硬件**: 需要一个 OpenBMC 硬件平台，并通过串口连接到一台测试主机。
- **软件**: 
  - OpenBMC 系统中运行重构后的 `phosphor-ipmi-serial` 守护进程。
  - 测试主机上需要安装 `ipmitool` 用于发送 IPMI 命令。
  - 测试主机上需要安装 `busctl` 用于 D-Bus 交互测试。

### 测试用例

#### Case 1: I/O 多路复用与 D-Bus 集成测试

*   **目标**: 验证 `poll()` 能够正确、非阻塞地处理串口和 D-Bus 事件。
*   **1.1 - 基本串口通信**
    *   **步骤**: 在测试主机上，通过 `ipmitool` 发送一个基本命令。
      ```bash
      ipmitool -I serial-terminal -D /dev/ttyS1:115200 mc info
      ```
    *   **预期**: `ipmitool` 成功接收并显示 BMC 的设备信息。守护进程日志无错误。
*   **1.2 - D-Bus 稳定性**
    *   **步骤**: 在守护进程运行时，在 BMC 上使用 `busctl` 持续、高频地调用其他服务的 D-Bus 方法（例如查询传感器读数）。
      ```bash
      # 在 BMC 上执行
      while true; do busctl get-property xyz.openbmc_project.Sensors.CpuTemp /xyz/openbmc_project/sensors/temperature/cpu0 Value; sleep 0.1; done &
      ```
    *   **预期**: 守护进程不应崩溃或挂起，并且串口 IPMI 通信（如 `mc info`）保持正常。
*   **1.3 - CPU 占用率**
    *   **步骤**: 在串口空闲时，使用 `top` 或 `htop` 监控 `phosphor-ipmi-serial` 进程的 CPU 占用。
    *   **预期**: CPU 占用率应接近 0%。这验证了 `poll` 的超时机制取代了旧的“忙轮询”。

#### Case 2: 异步回调与序列号（`seqNum`）安全测试

*   **目标**: 验证在高并发请求下，IPMI 响应的序列号与请求精确对应。
*   **2.1 - 高并发请求测试**
    *   **步骤**: 编写一个脚本，在测试主机上通过串口快速、连续地发送多个不同的 IPMI 命令，而不等待前一个命令的响应。脚本需要记录每个发出命令的 `seqNum`。
    *   **预期**: 接收到的所有响应必须与它们各自的请求拥有完全匹配的 `seqNum`。如果出现任何 `seqNum` 错乱，则表明 `lambda` 捕获逻辑仍然存在问题。

#### Case 3: 异常安全与输入健壮性测试

*   **目标**: 验证守护进程在遇到非法输入或配置错误时能优雅退出。
*   **3.1 - 非法波特率**
    *   **步骤**: 启动守护进程时传入一个非法的波特率。
      ```bash
      ./phosphor-ipmi-serial -b "not-a-number"
      ```
    *   **预期**: 程序应在日志中打印 "Invalid baud rate" 相关的错误，并以非零状态码退出，而不是发生段错误。
*   **3.2 - 无效设备路径**
    *   **步骤**: 启动守护进程时传入一个不存在的设备路径。
      ```bash
      ./phosphor-ipmi-serial -d /dev/nonexistentdevice
      ```
    *   **预期**: 程序应打印 "Failed to open device" 或类似的错误，并优雅退出。
*   **3.3 - 权限不足**
    *   **步骤**: 以一个没有串口读写权限的用户身份运行守护进程。
    *   **预期**: 程序应打印 "Permission denied" 相关的错误，并优雅退出。

#### Case 4: 协议解析器鲁棒性测试

*   **目标**: 验证 `terminal` 和 `basic` 模式解析器对异常输入的处理能力。
*   **4.1 - Terminal 模式 - 格式错误**
    *   **步骤**: 通过 `echo` 或脚本向串口设备发送格式错误的 Terminal Mode 帧。
      ```bash
      # 混合非法字符
      echo "[04 01 GG 02]" > /dev/ttyS1
      # 帧不完整
      echo "[04 01" > /dev/ttyS1
      ```
    *   **预期**: 守护进程不应崩溃。它应该能忽略格式错误的帧，并正确解析后续的合法帧。
*   **4.2 - Basic 模式 - 转义序列**
    *   **步骤**: 构造一个 IPMI 命令，其数据载荷中包含 `0xA0`, `0xA5`, `0xAA` 等特殊字节。使用 `basic` 模式将此命令发送给守护进程。
    *   **预期**: 守护进程应能正确地“反转义”这些字节，并解析出原始的数据载荷，然后正确执行该命令。

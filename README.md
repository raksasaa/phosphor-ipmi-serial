# phosphor-ipmi-serial

Serial/UART IPMI daemon for OpenBMC. Implements IPMI Serial/Modem Interface
(IPMI v2.0 Specification Chapter 14) as a transport channel alongside the
existing LAN (phosphor-net-ipmid) and KCS (phosphor-ipmi-kcs) channels.

## Features

- Terminal Mode protocol (ASCII hex, human-readable)
- Basic Mode protocol (binary framing with escape sequences)
- Session management (no-auth initial, MD5/password planned)
- D-Bus integration with phosphor-ipmi-host (ipmid)
- Comprehensive debug/trace tooling
- Full unit test coverage

## Building

```bash
meson setup build
ninja -C build
```

## Usage

```bash
serialipmid -d /dev/ttyAMA1 -b 115200 -m terminal -c serial0
```

## Testing from Host

```bash
ipmitool -I serial-terminal -D /dev/ttyS1:115200 mc info
```

## Architecture

This project mirrors phosphor-net-ipmid architecture but replaces:
- UDP network transport → UART serial transport
- RMCP+/IPMI-over-LAN → IPMI Serial/Modem Interface (Terminal/Basic Mode)
- Ethernet socket → PL011 UART device (/dev/ttyAMA1)

## Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-d, --device` | UART device path | `/dev/ttyAMA1` |
| `-b, --baud` | Baud rate | `115200` |
| `-m, --mode` | Protocol mode (terminal/basic) | `terminal` |
| `-c, --channel` | IPMI channel name | `serial0` |
| `-v, --verbose` | Verbose logging | `false` |
| `--debug-frames` | Print hexdump of all frames | `false` |
| `--session-timeout` | Session timeout (seconds) | `60` |
| `--max-retries` | Maximum retries | `3` |
```

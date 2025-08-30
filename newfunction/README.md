# New Function Modules

This directory contains additional functionality modules for the xiaozhi-esp32 project.

## Modules

### 1. Alarm Function (闹钟功能)
- File: [alarm.h](alarm.h), [alarm.cc](alarm.cc)
- MCP Tool Integration: [alarm_mcp.cc](alarm_mcp.cc)
- Description: Implements alarm clock functionality with support for one-time and recurring alarms

### 2. WiFi Reconfiguration (WiFi重新配置)
- File: [wifi_reconfig.h](wifi_reconfig.h), [wifi_reconfig.cc](wifi_reconfig.cc)
- Description: Provides an MCP tool to reboot the device and enter WiFi configuration mode

## Integration

These modules are integrated into the main application through the MCP (Model Context Protocol) server. Each module provides one or more tools that can be called remotely to perform specific functions on the device.

### WiFi Reconfiguration Tool

The WiFi reconfiguration tool allows remote triggering of the device's WiFi configuration mode. When called, it will:

1. Clear the existing WiFi configuration
2. Set the device state to WiFi configuring mode
3. Reboot the device
4. Start the WiFi configuration access point
5. Allow users to connect and reconfigure WiFi settings

Tool name: `self.system.reconfigure_wifi`

Usage:
```json
{
  "method": "tools/call",
  "params": {
    "name": "self.system.reconfigure_wifi"
  }
}
```

**Note**: This tool should only be called after explicit user confirmation due to its disruptive nature (reboots the device).
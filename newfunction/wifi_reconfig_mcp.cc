#include "wifi_reconfig_mcp.h"
#include "wifi_reconfig.h"
#include <esp_log.h>

// 已将WiFi重新配置工具直接集成到mcp_server.cc中，此文件不再需要

static const char* TAG = "WifiReconfigMcp";

WifiReconfigMcpTool::WifiReconfigMcpTool() : McpTool(
    "self.system.reconfigure_wifi",
    "Reboot the device and enter WiFi configuration mode.\n"
    "**CAUTION** You must ask the user to confirm this action before calling this tool.",
    PropertyList(),
    [](const PropertyList& properties) -> ReturnValue {
        ESP_LOGI(TAG, "WiFi reconfiguration requested via MCP tool");
        
        // 获取WiFi重新配置实例
        auto& wifi_reconfig = WifiReconfig::GetInstance();
        
        // 执行WiFi配置重置和重启
        esp_err_t err = wifi_reconfig.ResetWifiConfigAndReboot();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reset WiFi config: %s", esp_err_to_name(err));
            return "Failed to reset WiFi configuration";
        }
        
        return "Device is now in reconfiguration mode. Please connect to the device's WiFi hotspot to configure network settings.";
    }
) {
}
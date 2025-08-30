#include "wifi_reconfig.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 声明外部函数而不是包含application.h
namespace {
    extern "C" void application_set_device_state(int state);
    extern "C" bool is_audio_speaking();
}

static const char* TAG = "WifiReconfig";

WifiReconfig::WifiReconfig() {
}

WifiReconfig::~WifiReconfig() {
}

esp_err_t WifiReconfig::Initialize() {
    ESP_LOGI(TAG, "Initializing WiFi reconfiguration module");
    return ESP_OK;
}

esp_err_t WifiReconfig::ResetWifiConfigAndReboot() {
    ESP_LOGI(TAG, "Resetting WiFi configuration and rebooting");
    
    // 清除WiFi配置
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    nvs_handle_t nvs_handle;
    err = nvs_open("nvs.net80211", NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        nvs_erase_all(nvs_handle);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "Erased WiFi configuration");
    } else {
        ESP_LOGE(TAG, "Failed to open NVS handle for WiFi config");
        return err;
    }
    
    // 设置标志位，重启后进入配网模式
    {
        nvs_handle_t settings_handle;
        err = nvs_open("wifi", NVS_READWRITE, &settings_handle);
        if (err == ESP_OK) {
            err = nvs_set_i32(settings_handle, "force_ap", 1);
            if (err == ESP_OK) {
                nvs_commit(settings_handle);
                ESP_LOGI(TAG, "Set force_ap flag to 1");
            } else {
                ESP_LOGE(TAG, "Failed to set force_ap flag");
            }
            nvs_close(settings_handle);
        } else {
            ESP_LOGE(TAG, "Failed to open NVS handle for settings");
            return err;
        }
    }
    
    // 等待语音播报完成后再改变设备状态
    const TickType_t delay_ms = 100;
    const TickType_t max_wait_time = 5000; // 最多等待5秒
    TickType_t wait_time = 0;
    
    while (is_audio_speaking() && wait_time < max_wait_time) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        wait_time += delay_ms;
    }
    
    if (wait_time >= max_wait_time) {
        ESP_LOGW(TAG, "Audio speaking timeout, forcing state change");
    } else {
        ESP_LOGI(TAG, "Audio speaking finished, changing state");
    }
    
    // 通知应用程序进入WiFi配置状态
    application_set_device_state(2); // kDeviceStateWifiConfiguring = 2
    
    // 等待状态变更完成后再重启
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    
    return ESP_OK;
}
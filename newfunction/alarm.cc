#include "alarm.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>
#include <cstring>
#include <cJSON.h>
#include <algorithm>
#include <ctime>

static const char* TAG = "Alarm";

AlarmManager::AlarmManager() : nvs_handle_(0) {
}

AlarmManager::~AlarmManager() {
    if (nvs_handle_ != 0) {
        nvs_close(nvs_handle_);
    }
}

esp_err_t AlarmManager::Initialize() {
    esp_err_t err = nvs_open(nvs_namespace_.c_str(), NVS_READWRITE, &nvs_handle_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }
    
    // 加载已保存的闹钟
    LoadAlarms();
    
    return ESP_OK;
}

esp_err_t AlarmManager::AddAlarm(const Alarm& alarm) {
    Alarm new_alarm = alarm;
    new_alarm.id = GenerateUniqueId();
    UpdateAlarmTriggerTime(new_alarm);
    
    alarms_.push_back(new_alarm);
    
    ESP_LOGI(TAG, "Added alarm ID %lu: %02d:%02d, repeat mode %d", 
             (unsigned long)new_alarm.id, new_alarm.hour, new_alarm.minute, new_alarm.repeat_mode);
    
    return SaveAlarms();
}

esp_err_t AlarmManager::DeleteAlarm(uint32_t alarm_id) {
    auto it = std::find_if(alarms_.begin(), alarms_.end(), 
                          [alarm_id](const Alarm& alarm) { return alarm.id == alarm_id; });
    
    if (it != alarms_.end()) {
        alarms_.erase(it);
        ESP_LOGI(TAG, "Deleted alarm ID %lu", (unsigned long)alarm_id);
        return SaveAlarms();
    }
    
    ESP_LOGW(TAG, "Alarm ID %lu not found for deletion", (unsigned long)alarm_id);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t AlarmManager::UpdateAlarm(const Alarm& alarm) {
    auto it = std::find_if(alarms_.begin(), alarms_.end(), 
                          [alarm](const Alarm& a) { return a.id == alarm.id; });
    
    if (it != alarms_.end()) {
        *it = alarm;
        UpdateAlarmTriggerTime(*it);
        ESP_LOGI(TAG, "Updated alarm ID %lu", (unsigned long)alarm.id);
        return SaveAlarms();
    }
    
    ESP_LOGW(TAG, "Alarm ID %lu not found for update", (unsigned long)alarm.id);
    return ESP_ERR_NOT_FOUND;
}

const Alarm* AlarmManager::GetAlarmById(uint32_t alarm_id) const {
    auto it = std::find_if(alarms_.begin(), alarms_.end(), 
                          [alarm_id](const Alarm& alarm) { return alarm.id == alarm_id; });
    
    if (it != alarms_.end()) {
        return &(*it);
    }
    
    return nullptr;
}

esp_err_t AlarmManager::EnableAlarm(uint32_t alarm_id, bool enable) {
    auto it = std::find_if(alarms_.begin(), alarms_.end(), 
                          [alarm_id](const Alarm& alarm) { return alarm.id == alarm_id; });
    
    if (it != alarms_.end()) {
        it->enabled = enable;
        if (enable) {
            UpdateAlarmTriggerTime(*it);
        } else {
            it->next_trigger_time = 0;
        }
        ESP_LOGI(TAG, "%s alarm ID %lu", enable ? "Enabled" : "Disabled", (unsigned long)alarm_id);
        return SaveAlarms();
    }
    
    ESP_LOGW(TAG, "Alarm ID %lu not found for enabling/disabling", (unsigned long)alarm_id);
    return ESP_ERR_NOT_FOUND;
}

bool AlarmManager::CheckAlarms() {
    time_t now;
    time(&now);
    
    bool alarm_triggered = false;
    
    for (auto& alarm : alarms_) {
        if (!alarm.enabled) {
            continue;
        }
        
        // 对于短期闹钟（触发时间在接下来的15秒内），我们使用更精确的判断
        if (alarm.next_trigger_time > 0 && now >= alarm.next_trigger_time) {
            ESP_LOGI(TAG, "Alarm triggered: ID %lu, %s", (unsigned long)alarm.id, alarm.label.c_str());
            alarm_triggered = true;
            
            // 更新下次触发时间
            UpdateAlarmTriggerTime(alarm);
        }
    }
    
    // 保存更新后的闹钟信息
    if (alarm_triggered) {
        SaveAlarms();
    }
    
    return alarm_triggered;
}

time_t AlarmManager::CalculateNextTriggerTime(const Alarm& alarm) const {
    if (!alarm.enabled) {
        return 0;
    }
    
    time_t now;
    time(&now);
    
    struct tm* timeinfo = localtime(&now);
    
    // 设置为目标时间
    timeinfo->tm_hour = alarm.hour;
    timeinfo->tm_min = alarm.minute;
    timeinfo->tm_sec = 0;
    
    time_t target_time = mktime(timeinfo);
    
    // 如果目标时间已经过去，则根据重复模式调整
    if (target_time <= now) {
        switch (alarm.repeat_mode) {
            case ALARM_ONCE:
                // 一次性闹钟，已过期就不再触发
                return 0;
                
            case ALARM_DAILY:
                // 每天重复，推迟到明天
                target_time += 24 * 60 * 60;
                break;
                
            case ALARM_WEEKDAYS:
                // 工作日重复
                if (timeinfo->tm_wday == 0 || timeinfo->tm_wday == 6) {
                    // 周末，跳到下周一
                    int days_to_add = (8 - timeinfo->tm_wday) % 7;
                    target_time += days_to_add * 24 * 60 * 60;
                } else {
                    // 工作日，推迟到明天
                    target_time += 24 * 60 * 60;
                }
                break;
                
            case ALARM_WEEKENDS:
                // 周末重复
                if (timeinfo->tm_wday >= 1 && timeinfo->tm_wday <= 5) {
                    // 工作日，跳到周六
                    int days_to_add = 6 - timeinfo->tm_wday;
                    target_time += days_to_add * 24 * 60 * 60;
                } else {
                    // 周末，推迟到明天
                    target_time += 24 * 60 * 60;
                }
                break;
                
            case ALARM_WEEKLY:
                // 每周重复，推迟到下周同一天
                target_time += 7 * 24 * 60 * 60;
                break;
                
            case ALARM_CUSTOM:
                // 自定义重复
                {
                    //int days_checked = 0;
                    int current_wday = timeinfo->tm_wday;
                    time_t next_time = 0;
                    
                    // 检查接下来7天
                    for (int i = 1; i <= 7; i++) {
                        int next_wday = (current_wday + i) % 7;
                        if (alarm.custom_days & (1 << next_wday)) {
                            next_time = target_time + i * 24 * 60 * 60;
                            break;
                        }
                    }
                    
                    if (next_time > 0) {
                        target_time = next_time;
                    } else {
                        // 没有找到下一个触发日，禁用闹钟
                        return 0;
                    }
                }
                break;
        }
    }
    
    return target_time;
}

uint32_t AlarmManager::GenerateUniqueId() {
    uint32_t id = 1;
    bool id_exists = false;
    
    do {
        id_exists = false;
        for (const auto& alarm : alarms_) {
            if (alarm.id == id) {
                id_exists = true;
                id++;
                break;
            }
        }
    } while (id_exists);
    
    return id;
}

void AlarmManager::UpdateAlarmTriggerTime(Alarm& alarm) {
    if (alarm.enabled) {
        alarm.next_trigger_time = CalculateNextTriggerTime(alarm);
    } else {
        alarm.next_trigger_time = 0;
    }
}

esp_err_t AlarmManager::SaveAlarms() {
    cJSON* root = cJSON_CreateObject();
    cJSON* alarms_array = cJSON_CreateArray();
    
    cJSON_AddItemToObject(root, "alarms", alarms_array);
    
    for (const auto& alarm : alarms_) {
        cJSON* alarm_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(alarm_json, "id", alarm.id);
        cJSON_AddStringToObject(alarm_json, "label", alarm.label.c_str());
        cJSON_AddNumberToObject(alarm_json, "hour", alarm.hour);
        cJSON_AddNumberToObject(alarm_json, "minute", alarm.minute);
        cJSON_AddNumberToObject(alarm_json, "repeat_mode", alarm.repeat_mode);
        cJSON_AddNumberToObject(alarm_json, "custom_days", alarm.custom_days);
        cJSON_AddBoolToObject(alarm_json, "enabled", alarm.enabled);
        cJSON_AddNumberToObject(alarm_json, "next_trigger_time", alarm.next_trigger_time);
        
        cJSON_AddItemToArray(alarms_array, alarm_json);
    }
    
    char* json_str = cJSON_PrintUnformatted(root);
    if (json_str == nullptr) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "Failed to print JSON");
        return ESP_FAIL;
    }
    
    esp_err_t err = nvs_set_str(nvs_handle_, "alarms_data", json_str);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save alarms to NVS: %s", esp_err_to_name(err));
    } else {
        nvs_commit(nvs_handle_);
        ESP_LOGI(TAG, "Saved %d alarms to NVS", alarms_.size());
    }
    
    cJSON_free(json_str);
    cJSON_Delete(root);
    
    return err;
}

esp_err_t AlarmManager::LoadAlarms() {
    // 先清空现有闹钟
    alarms_.clear();
    
    // 从NVS读取数据
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(nvs_handle_, "alarms_data", nullptr, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to get alarms data size: %s", esp_err_to_name(err));
        return err;
    }
    
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No alarms data found in NVS");
        return ESP_OK;
    }
    
    char* json_str = (char*)malloc(required_size);
    if (json_str == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate memory for alarms data");
        return ESP_ERR_NO_MEM;
    }
    
    err = nvs_get_str(nvs_handle_, "alarms_data", json_str, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read alarms data: %s", esp_err_to_name(err));
        free(json_str);
        return err;
    }
    
    // 解析JSON
    cJSON* root = cJSON_Parse(json_str);
    if (root == nullptr) {
        ESP_LOGE(TAG, "Failed to parse alarms JSON data");
        free(json_str);
        return ESP_FAIL;
    }
    
    cJSON* alarms_array = cJSON_GetObjectItem(root, "alarms");
    if (alarms_array == nullptr || !cJSON_IsArray(alarms_array)) {
        ESP_LOGE(TAG, "Invalid alarms data format");
        cJSON_Delete(root);
        free(json_str);
        return ESP_FAIL;
    }
    
    int alarm_count = cJSON_GetArraySize(alarms_array);
    for (int i = 0; i < alarm_count; i++) {
        cJSON* alarm_json = cJSON_GetArrayItem(alarms_array, i);
        if (alarm_json == nullptr || !cJSON_IsObject(alarm_json)) {
            continue;
        }
        
        Alarm alarm;
        cJSON* id_item = cJSON_GetObjectItem(alarm_json, "id");
        cJSON* label_item = cJSON_GetObjectItem(alarm_json, "label");
        cJSON* hour_item = cJSON_GetObjectItem(alarm_json, "hour");
        cJSON* minute_item = cJSON_GetObjectItem(alarm_json, "minute");
        cJSON* repeat_mode_item = cJSON_GetObjectItem(alarm_json, "repeat_mode");
        cJSON* custom_days_item = cJSON_GetObjectItem(alarm_json, "custom_days");
        cJSON* enabled_item = cJSON_GetObjectItem(alarm_json, "enabled");
        cJSON* next_trigger_time_item = cJSON_GetObjectItem(alarm_json, "next_trigger_time");
        
        if (id_item) alarm.id = id_item->valueint;
        if (label_item) alarm.label = label_item->valuestring ? label_item->valuestring : "";
        if (hour_item) alarm.hour = hour_item->valueint;
        if (minute_item) alarm.minute = minute_item->valueint;
        if (repeat_mode_item) alarm.repeat_mode = (AlarmRepeatMode)repeat_mode_item->valueint;
        if (custom_days_item) alarm.custom_days = custom_days_item->valueint;
        if (enabled_item) alarm.enabled = cJSON_IsTrue(enabled_item);
        if (next_trigger_time_item) alarm.next_trigger_time = next_trigger_time_item->valueint;
        
        alarms_.push_back(alarm);
    }
    
    ESP_LOGI(TAG, "Loaded %d alarms from NVS", alarms_.size());
    
    cJSON_Delete(root);
    free(json_str);
    
    return ESP_OK;
}

// 将闹钟信息转换为JSON字符串
std::string AlarmManager::GetAlarmsAsJson() const {
    cJSON* root = cJSON_CreateObject();
    cJSON* alarms_array = cJSON_CreateArray();
    
    cJSON_AddItemToObject(root, "alarms", alarms_array);
    
    for (const auto& alarm : alarms_) {
        cJSON* alarm_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(alarm_json, "id", alarm.id);
        cJSON_AddStringToObject(alarm_json, "label", alarm.label.c_str());
        cJSON_AddNumberToObject(alarm_json, "hour", alarm.hour);
        cJSON_AddNumberToObject(alarm_json, "minute", alarm.minute);
        cJSON_AddNumberToObject(alarm_json, "repeat_mode", alarm.repeat_mode);
        cJSON_AddNumberToObject(alarm_json, "custom_days", alarm.custom_days);
        cJSON_AddBoolToObject(alarm_json, "enabled", alarm.enabled);
        cJSON_AddNumberToObject(alarm_json, "next_trigger_time", alarm.next_trigger_time);
        
        cJSON_AddItemToArray(alarms_array, alarm_json);
    }
    
    char* json_str = cJSON_PrintUnformatted(root);
    std::string result(json_str ? json_str : "{}");
    
    if (json_str) {
        cJSON_free(json_str);
    }
    cJSON_Delete(root);
    
    return result;
}
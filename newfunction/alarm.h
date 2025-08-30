#ifndef _ALARM_H_
#define _ALARM_H_

#include <string>
#include <vector>
#include <ctime>
#include <esp_err.h>
#include <nvs.h>

// 闹钟重复模式
enum AlarmRepeatMode {
    ALARM_ONCE = 0,          // 一次性闹钟
    ALARM_DAILY = 1,         // 每天重复
    ALARM_WEEKDAYS = 2,      // 工作日重复
    ALARM_WEEKENDS = 3,      // 周末重复
    ALARM_WEEKLY = 4,        // 每周重复
    ALARM_CUSTOM = 5         // 自定义重复（按星期几）
};

// 闹钟结构体
struct Alarm {
    uint32_t id;                    // 闹钟ID
    std::string label;              // 闹钟标签
    int hour;                       // 小时 (0-23)
    int minute;                     // 分钟 (0-59)
    AlarmRepeatMode repeat_mode;    // 重复模式
    uint8_t custom_days;            // 自定义重复日（位标志：周日=bit0, 周一=bit1, ...）
    bool enabled;                   // 是否启用
    time_t next_trigger_time;       // 下次触发时间
    
    Alarm() : id(0), label(""), hour(0), minute(0), 
              repeat_mode(ALARM_ONCE), custom_days(0), 
              enabled(false), next_trigger_time(0) {}
};

class AlarmManager {
public:
    static AlarmManager& GetInstance() {
        static AlarmManager instance;
        return instance;
    }
    
    // 初始化闹钟管理器
    esp_err_t Initialize();
    
    // 添加闹钟
    esp_err_t AddAlarm(const Alarm& alarm);
    
    // 删除闹钟
    esp_err_t DeleteAlarm(uint32_t alarm_id);
    
    // 更新闹钟
    esp_err_t UpdateAlarm(const Alarm& alarm);
    
    // 获取所有闹钟
    const std::vector<Alarm>& GetAlarms() const { return alarms_; }
    
    // 根据ID获取闹钟
    const Alarm* GetAlarmById(uint32_t alarm_id) const;
    
    // 启用/禁用闹钟
    esp_err_t EnableAlarm(uint32_t alarm_id, bool enable);
    
    // 检查是否有闹钟触发
    bool CheckAlarms();
    
    // 计算下次触发时间
    time_t CalculateNextTriggerTime(const Alarm& alarm) const;
    
    // 保存闹钟到NVS
    esp_err_t SaveAlarms();
    
    // 从NVS加载闹钟
    esp_err_t LoadAlarms();
    
    // 获取闹钟信息的JSON表示
    std::string GetAlarmsAsJson() const;

private:
    AlarmManager();
    ~AlarmManager();
    
    // NVS相关
    nvs_handle_t nvs_handle_;
    std::string nvs_namespace_ = "alarms";
    
    // 闹钟列表
    std::vector<Alarm> alarms_;
    
    // 生成唯一ID
    uint32_t GenerateUniqueId();
    
    // 更新闹钟下次触发时间
    void UpdateAlarmTriggerTime(Alarm& alarm);
};

#endif // _ALARM_H_
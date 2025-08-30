#ifndef _WIFI_RECONFIG_H_
#define _WIFI_RECONFIG_H_

#include <esp_err.h>

// 添加供外部组件调用的C接口声明
#ifdef __cplusplus
extern "C" {
#endif

void application_set_device_state(int state);

#ifdef __cplusplus
}
#endif

class WifiReconfig {
public:
    static WifiReconfig& GetInstance() {
        static WifiReconfig instance;
        return instance;
    }
    
    // 初始化WiFi重新配置功能
    esp_err_t Initialize();
    
    // 清除WiFi配置并重启进入配网模式
    esp_err_t ResetWifiConfigAndReboot();

private:
    WifiReconfig();
    ~WifiReconfig();
};

#endif // _WIFI_RECONFIG_H_
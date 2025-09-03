#pragma once

#include <libs/gif/lv_gif.h>
#include "settings.h"
#include <esp_lvgl_port.h>

#include "display/lcd_display.h"

// Jerry开发板表情GIF声明 - 使用otto-emoji-gif-component组件中的6个表情
LV_IMAGE_DECLARE(staticstate);  // 静态状态/中性表情
LV_IMAGE_DECLARE(sad);          // 悲伤
LV_IMAGE_DECLARE(happy);        // 开心
LV_IMAGE_DECLARE(scare);        // 惊吓/惊讶
LV_IMAGE_DECLARE(buxue);        // 不学/困惑
LV_IMAGE_DECLARE(anger);        // 愤怒

// Jerry开发板新增的自定义GIF声明
LV_IMAGE_DECLARE(natural);      // 自然表情

// 新添加的GIF表情声明
LV_IMAGE_DECLARE(angry_25fps);
LV_IMAGE_DECLARE(disdain_25fps);
LV_IMAGE_DECLARE(scare_25fps);
LV_IMAGE_DECLARE(natural_new_25fps);
LV_IMAGE_DECLARE(sad_25fps);
LV_IMAGE_DECLARE(excitement_25fps);

/**
 * @brief Jerry ESP32-S3开发板GIF表情显示类
 * 继承SpiLcdDisplay，添加GIF表情支持
 */
class JerryEmojiDisplay : public SpiLcdDisplay {
public:
    /**
     * @brief 构造函数，参数与SpiLcdDisplay相同
     */
    JerryEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                      int width, int height, int offset_x, int offset_y, bool mirror_x,
                      bool mirror_y, bool swap_xy, DisplayFonts fonts);

    virtual ~JerryEmojiDisplay() = default;

    // 重写表情设置方法
    virtual void SetEmotion(const char* emotion) override;

    // 重写聊天消息设置方法
    virtual void SetChatMessage(const char* role, const char* content) override;

    // 重写图标设置方法
    virtual void SetIcon(const char* icon) override;
    
    // 重写主题设置方法
    virtual void SetTheme(const std::string& theme_name) override;

private:
    void SetupGifContainer();
    void UpdateGifForCurrentTheme(); // 添加新方法声明

    lv_obj_t* emotion_gif_;  ///< GIF表情组件
    const char* current_emotion_; ///< 当前显示的表情名称

    // 表情映射
    struct EmotionMap {
        const char* name;
        const lv_image_dsc_t* gif;
    };

    static const EmotionMap emotion_maps_[];
};
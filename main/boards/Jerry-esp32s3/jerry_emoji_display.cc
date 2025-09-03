#include "jerry_emoji_display.h"

#include <esp_log.h>
#include <font_awesome.h>

#include <algorithm>
#include <cstring>
#include <string>

#define TAG "JerryEmojiDisplay"

// Inversion版本的GIF声明
LV_IMAGE_DECLARE(angry_25fps_Inversion);
LV_IMAGE_DECLARE(disdain_25fps_Inversion);
LV_IMAGE_DECLARE(scare_25fps_Inversion);
LV_IMAGE_DECLARE(natural_new_25fps_Inversion);
LV_IMAGE_DECLARE(sad_25fps_Inversion);
LV_IMAGE_DECLARE(excitement_25fps_Inversion);

// 表情映射表 - 使用新的GIF资源
const JerryEmojiDisplay::EmotionMap JerryEmojiDisplay::emotion_maps_[] = {
    // 基础表情映射到新的GIF资源
    {"neutral", &natural_new_25fps},
    {"relaxed", &natural_new_25fps},
    {"sleepy", &natural_new_25fps},
    {"happy", &excitement_25fps},
    {"laughing", &excitement_25fps},
    {"funny", &excitement_25fps},
    {"loving", &excitement_25fps},
    {"confident", &excitement_25fps},
    {"winking", &excitement_25fps},
    {"cool", &excitement_25fps},
    {"delicious", &excitement_25fps},
    {"kissy", &excitement_25fps},
    {"silly", &excitement_25fps},
    {"sad", &sad_25fps},
    {"crying", &sad_25fps},
    {"angry", &angry_25fps},
    {"surprised", &scare_25fps},
    {"shocked", &scare_25fps},
    {"thinking", &natural_new_25fps},
    {"confused", &disdain_25fps},
    {"embarrassed", &disdain_25fps},
    {"natural", &natural_new_25fps},
    
    // 新添加的表情映射（移除与上面重复的条目）
    {"disdain", &disdain_25fps},
    {"scare", &scare_25fps},
    {"natural_new", &natural_new_25fps},
    {"excitement", &excitement_25fps},

    {nullptr, nullptr}  // 结束标记
};

JerryEmojiDisplay::JerryEmojiDisplay(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_handle_t panel, int width, int height,
                                     int offset_x, int offset_y, bool mirror_x, bool mirror_y,
                                     bool swap_xy, DisplayFonts fonts)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy,
                    fonts),
      emotion_gif_(nullptr),
      current_emotion_("neutral") {
    ESP_LOGI(TAG, "JerryEmojiDisplay constructor begin");
    // 注意：SpiLcdDisplay构造函数已经设置了主题和基本UI
    // 我们只需要确保Jerry特定的UI元素与主题匹配
    SetupGifContainer();
    
    // 确保所有UI绘制完成后再刷新显示
    if (lvgl_port_lock(0)) {
        lv_refr_now(lv_display_get_default());
        lvgl_port_unlock();
    }
    
    ESP_LOGI(TAG, "JerryEmojiDisplay constructor end");
}

void JerryEmojiDisplay::SetupGifContainer() {
    ESP_LOGI(TAG, "SetupGifContainer begin");
    DisplayLockGuard lock(this);
    
    // 创建GIF容器
    emotion_gif_ = lv_gif_create(lv_screen_active());
    if (emotion_gif_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create GIF container");
        return;
    }

    // 设置GIF位置和大小为70%，上边距30px（与连接成功状态保持一致）
    lv_obj_align(emotion_gif_, LV_ALIGN_TOP_MID, 0, 30);
    int gif_size = LV_HOR_RES * 0.7; // 调整为70%屏幕宽度
    lv_obj_set_size(emotion_gif_, gif_size, gif_size);
    
    // 根据当前主题选择初始GIF
    const lv_image_dsc_t* initial_gif = &natural_new_25fps;
    if (lv_color_eq(current_theme_.background, lv_color_white())) {
        initial_gif = &natural_new_25fps_Inversion;
    }
    
    // 设置默认GIF
    lv_gif_set_src(emotion_gif_, initial_gif);
    
    // 根据GIF大小调整缩放比例
    int gif_width = 240; // 默认GIF宽度
    int scale = (gif_size * 256) / gif_width;
    lv_image_set_scale(emotion_gif_, scale);

    // 移除GIF边框和背景设置，避免在暗色模式下出现白色边框
    lv_obj_set_style_bg_opa(emotion_gif_, LV_OPA_COVER, 0);  // 保持背景不透明
    // 设置GIF背景色与屏幕背景色一致
    lv_obj_set_style_bg_color(emotion_gif_, current_theme_.background, 0);

    // 创建聊天消息标签并设置在底部
    chat_message_label_ = lv_label_create(lv_screen_active()); // 在同一屏幕层级创建
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9);
    lv_obj_set_height(chat_message_label_, 42); // 设置固定高度以显示两行
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_WRAP); // 支持换行
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0); // 水平居中对齐
    
    // 设置聊天消息背景色根据当前主题变化
    lv_obj_set_style_bg_opa(chat_message_label_, LV_OPA_COVER, 0);
    if (lv_color_eq(current_theme_.background, lv_color_white())) {
        // 当前是亮色主题
        lv_obj_set_style_text_color(chat_message_label_, lv_color_black(), 0);
        lv_obj_set_style_bg_color(chat_message_label_, lv_color_white(), 0);
    } else {
        // 当前是暗色主题
        lv_obj_set_style_text_color(chat_message_label_, lv_color_white(), 0);
        lv_obj_set_style_bg_color(chat_message_label_, lv_color_black(), 0);
    }
    lv_obj_set_style_pad_ver(chat_message_label_, 0, 0);
    lv_obj_set_style_pad_hor(chat_message_label_, 0, 0); // 水平方向无内边距

    // 聊天消息定位在底部
    lv_obj_set_align(chat_message_label_, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(chat_message_label_, -20);  // 底部对齐，向上偏移10像素

    // 确保聊天消息标签在GIF之上显示
    lv_obj_move_foreground(chat_message_label_);
    
    // 刷新显示
    lv_refr_now(lv_display_get_default());
    
    // 添加延时以确保UI绘制完成
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "SetupGifContainer end");
}

void JerryEmojiDisplay::SetEmotion(const char* emotion) {
    ESP_LOGI(TAG, "SetEmotion begin: %s", emotion ? emotion : "null");
    if (!emotion || !emotion_gif_) {
        ESP_LOGI(TAG, "SetEmotion early return");
        return;
    }

    // 更新当前表情
    current_emotion_ = emotion;

    // 使用线程安全的方式更新UI元素
    if (lvgl_port_lock(0)) {
        // 设置GIF位置和尺寸
        // 设置GIF位置和大小为70%，上边距30px
        lv_obj_align(emotion_gif_, LV_ALIGN_TOP_MID, 0, 30);
        int gif_size = LV_HOR_RES * 0.7; // 调整为70%屏幕宽度
        lv_obj_set_size(emotion_gif_, gif_size, gif_size);

        // 查找匹配的表情
        const lv_image_dsc_t* gif = &natural_new_25fps;  // 默认表情改为&natural_new_25fps
        int gif_width = 240; // 默认GIF宽度
        
        for (int i = 0; emotion_maps_[i].name; i++) {
            if (strcmp(emotion, emotion_maps_[i].name) == 0) {
                gif = emotion_maps_[i].gif;
                break;
            }
        }

        // 根据当前主题决定使用哪个版本的GIF
        const lv_image_dsc_t* themed_gif = gif;
        if (lv_color_eq(current_theme_.background, lv_color_white())) {
            // 亮色主题使用Inversion版本
            if (gif == &angry_25fps) {
                themed_gif = &angry_25fps_Inversion;
            } else if (gif == &disdain_25fps) {
                themed_gif = &disdain_25fps_Inversion;
            } else if (gif == &scare_25fps) {
                themed_gif = &scare_25fps_Inversion;
            } else if (gif == &natural_new_25fps) {
                themed_gif = &natural_new_25fps_Inversion;
            } else if (gif == &sad_25fps) {
                themed_gif = &sad_25fps_Inversion;
            } else if (gif == &excitement_25fps) {
                themed_gif = &excitement_25fps_Inversion;
            }
        }

        lv_gif_set_src(emotion_gif_, themed_gif);
        
        // 根据不同GIF设置合适的缩放比例
        if (themed_gif == &natural) {
            gif_width = 236;
        } else if (themed_gif == &angry_25fps_Inversion || themed_gif == &disdain_25fps_Inversion || 
                   themed_gif == &scare_25fps_Inversion || themed_gif == &natural_new_25fps_Inversion ||
                   themed_gif == &sad_25fps_Inversion || themed_gif == &excitement_25fps_Inversion) {
            // Inversion版本的GIF宽度也是240
            gif_width = 240;
        }
        // 计算缩放比例，使GIF适应显示区域
        int scale = (gif_size * 256) / gif_width;
        lv_image_set_scale(emotion_gif_, scale);
        
        // 确保GIF容器在主题切换后没有边框，并正确设置背景色
        lv_obj_set_style_border_width(emotion_gif_, 0, 0);
        lv_obj_set_style_bg_color(emotion_gif_, current_theme_.background, 0);
        
        // 确保聊天消息标签始终保持在前景
        if (chat_message_label_) {
            lv_obj_move_foreground(chat_message_label_);
        }
        
        // 强制刷新显示
        lv_refr_now(lv_display_get_default());
        
        lvgl_port_unlock();
    }
    ESP_LOGI(TAG, "SetEmotion end");
}

void JerryEmojiDisplay::SetChatMessage(const char* role, const char* content) {
    ESP_LOGI(TAG, "SetChatMessage begin: %s", content ? content : "null");
    if (!chat_message_label_ || !content) {
        ESP_LOGI(TAG, "SetChatMessage early return");
        return;
    }

    // 使用线程安全的方式更新UI元素
    if (lvgl_port_lock(0)) {
        lv_label_set_text(chat_message_label_, content);
        
        // 根据当前主题设置文字颜色和背景色
        if (lv_color_eq(current_theme_.background, lv_color_white())) {
            // 亮色主题使用黑色文字和白色背景
            lv_obj_set_style_text_color(chat_message_label_, lv_color_black(), 0);
            lv_obj_set_style_bg_color(chat_message_label_, lv_color_white(), 0);
        } else {
            // 暗色主题使用白色文字和黑色背景
            lv_obj_set_style_text_color(chat_message_label_, lv_color_white(), 0);
            lv_obj_set_style_bg_color(chat_message_label_, lv_color_black(), 0);
        }
        
        // 确保聊天消息标签始终保持在前景
        lv_obj_move_foreground(chat_message_label_);
        
        // 强制刷新显示
        lv_refr_now(lv_display_get_default());
        
        lvgl_port_unlock();
    }
    ESP_LOGI(TAG, "SetChatMessage end");
}

void JerryEmojiDisplay::SetIcon(const char* icon) {
    ESP_LOGI(TAG, "SetIcon: %s", icon ? icon : "null");
    // Jerry开发板暂不支持图标显示
}

void JerryEmojiDisplay::SetTheme(const std::string& theme_name) {
    ESP_LOGI(TAG, "SetTheme begin: %s", theme_name.c_str());
    // 保存旧的主题背景色用于比较
    lv_color_t old_background = current_theme_.background;
    
    // 调用父类的SetTheme方法
    SpiLcdDisplay::SetTheme(theme_name);
    
    // 使用线程安全的方式更新UI元素
    if (lvgl_port_lock(0)) {
        // 特别处理聊天消息标签的颜色
        if (chat_message_label_) {
            if (theme_name == "light" || theme_name == "LIGHT") {
                // 亮色主题使用黑色文字和白色背景
                lv_obj_set_style_text_color(chat_message_label_, lv_color_black(), 0);
                lv_obj_set_style_bg_color(chat_message_label_, lv_color_white(), 0);
            } else {
                // 暗色主题使用白色文字和黑色背景
                lv_obj_set_style_text_color(chat_message_label_, lv_color_white(), 0);
                lv_obj_set_style_bg_color(chat_message_label_, lv_color_black(), 0);
            }
        }
        
        // 确保GIF容器在主题切换后没有边框，并正确设置背景色
        if (emotion_gif_) {
            lv_obj_set_style_border_width(emotion_gif_, 0, 0);
            // 设置GIF背景色与屏幕背景色一致
            lv_obj_set_style_bg_color(emotion_gif_, current_theme_.background, 0);
            
            // 只有在背景色发生变化时才更新GIF
            if (!lv_color_eq(old_background, current_theme_.background)) {
                // 更新当前显示的GIF动画以匹配新主题
                UpdateGifForCurrentTheme();
            }
        }
        
        // 确保聊天消息标签始终保持在前景
        if (chat_message_label_) {
            lv_obj_move_foreground(chat_message_label_);
        }
        
        // 强制刷新显示
        lv_refr_now(lv_display_get_default());
        
        lvgl_port_unlock();
    }
    ESP_LOGI(TAG, "SetTheme end");
}

/**
 * @brief 根据当前主题更新GIF动画
 * 在主题切换时调用此方法，确保显示正确的GIF版本（普通版或Inversion版）
 */
void JerryEmojiDisplay::UpdateGifForCurrentTheme() {
    ESP_LOGI(TAG, "UpdateGifForCurrentTheme begin");
    // 使用当前表情查找对应的GIF
    const lv_image_dsc_t* gif = &natural_new_25fps;  // 默认表情改为&natural_new_25fps
    
    for (int i = 0; emotion_maps_[i].name; i++) {
        if (strcmp(current_emotion_, emotion_maps_[i].name) == 0) {
            gif = emotion_maps_[i].gif;
            break;
        }
    }
    
    // 根据当前主题决定使用哪个版本的GIF
    const lv_image_dsc_t* themed_gif = gif;
    if (lv_color_eq(current_theme_.background, lv_color_white())) {
        // 亮色主题使用Inversion版本
        if (gif == &angry_25fps) {
            themed_gif = &angry_25fps_Inversion;
        } else if (gif == &disdain_25fps) {
            themed_gif = &disdain_25fps_Inversion;
        } else if (gif == &scare_25fps) {
            themed_gif = &scare_25fps_Inversion;
        } else if (gif == &natural_new_25fps) {
            themed_gif = &natural_new_25fps_Inversion;
        } else if (gif == &sad_25fps) {
            themed_gif = &sad_25fps_Inversion;
        } else if (gif == &excitement_25fps) {
            themed_gif = &excitement_25fps_Inversion;
        }
    }
    
    // 更新GIF显示
    lv_gif_set_src(emotion_gif_, themed_gif);
    
    // 确保聊天消息标签始终保持在前景
    if (chat_message_label_) {
        lv_obj_move_foreground(chat_message_label_);
    }
    
    // 强制刷新显示
    lv_refr_now(lv_display_get_default());
    ESP_LOGI(TAG, "UpdateGifForCurrentTheme end");
}

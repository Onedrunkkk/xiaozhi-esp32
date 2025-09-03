#include "jerry_emoji_display.h"

#include <esp_log.h>
#include <font_awesome.h>

#include <algorithm>
#include <cstring>
#include <string>

#define TAG "JerryEmojiDisplay"

// 表情映射表 - 将所有表情都映射到happy GIF
const JerryEmojiDisplay::EmotionMap JerryEmojiDisplay::emotion_maps_[] = {
    // 所有表情都使用happy GIF
    {"neutral", &happy},
    {"relaxed", &happy},
    {"sleepy", &happy},
    {"happy", &happy},
    {"laughing", &happy},
    {"funny", &happy},
    {"loving", &happy},
    {"confident", &happy},
    {"winking", &happy},
    {"cool", &happy},
    {"delicious", &happy},
    {"kissy", &happy},
    {"silly", &happy},
    {"sad", &happy},
    {"crying", &happy},
    {"angry", &happy},
    {"surprised", &happy},
    {"shocked", &happy},
    {"thinking", &happy},
    {"confused", &happy},
    {"embarrassed", &happy},
    {"natural", &happy},

    {nullptr, nullptr}  // 结束标记
};

JerryEmojiDisplay::JerryEmojiDisplay(esp_lcd_panel_io_handle_t panel_io,
                                     esp_lcd_panel_handle_t panel, int width, int height,
                                     int offset_x, int offset_y, bool mirror_x, bool mirror_y,
                                     bool swap_xy, DisplayFonts fonts)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy,
                    fonts),
      emotion_gif_(nullptr) {
    SetupGifContainer();
}

void JerryEmojiDisplay::SetupGifContainer() {
    DisplayLockGuard lock(this);

    if (emotion_label_) {
        lv_obj_del(emotion_label_);
    }
    if (chat_message_label_) {
        lv_obj_del(chat_message_label_);
    }
    if (content_) {
        lv_obj_del(content_);
    }

    content_ = lv_obj_create(container_);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(content_, LV_HOR_RES, LV_HOR_RES);
    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_center(content_);

    emotion_label_ = lv_label_create(content_);
    lv_label_set_text(emotion_label_, "");
    lv_obj_set_width(emotion_label_, 0);
    lv_obj_set_style_border_width(emotion_label_, 0, 0);
    lv_obj_add_flag(emotion_label_, LV_OBJ_FLAG_HIDDEN);

    emotion_gif_ = lv_gif_create(content_);
    // 设置GIF显示区域为屏幕的60%
    int gif_size = LV_HOR_RES * 0.6;
    lv_obj_set_size(emotion_gif_, gif_size, gif_size);
    lv_obj_set_style_border_width(emotion_gif_, 0, 0);
    lv_obj_set_style_bg_opa(emotion_gif_, LV_OPA_TRANSP, 0);
    lv_obj_center(emotion_gif_);
    lv_gif_set_src(emotion_gif_, &happy);
    // 设置GIF缩放以适应显示区域
    lv_image_set_scale(emotion_gif_, (gif_size * 256) / 238); // 256是LVGL缩放的基数

    chat_message_label_ = lv_label_create(content_);
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9);
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(chat_message_label_, lv_color_white(), 0);
    lv_obj_set_style_border_width(chat_message_label_, 0, 0);

    lv_obj_set_style_bg_opa(chat_message_label_, LV_OPA_70, 0);
    lv_obj_set_style_bg_color(chat_message_label_, lv_color_black(), 0);
    lv_obj_set_style_pad_ver(chat_message_label_, 5, 0);

    lv_obj_align(chat_message_label_, LV_ALIGN_BOTTOM_MID, 0, 0);

    LcdDisplay::SetTheme("dark");
}

void JerryEmojiDisplay::SetEmotion(const char* emotion) {
    if (!emotion || !emotion_gif_) {
        return;
    }

    DisplayLockGuard lock(this);

    // 查找匹配的表情
    const lv_image_dsc_t* gif = &happy;  // 默认表情改为happy
    int gif_width = 238; // 默认GIF宽度
    int display_size = LV_HOR_RES * 0.6; // 显示区域大小
    
    for (int i = 0; emotion_maps_[i].name; i++) {
        if (strcmp(emotion, emotion_maps_[i].name) == 0) {
            gif = emotion_maps_[i].gif;
            break;
        }
    }

    lv_gif_set_src(emotion_gif_, gif);
    
    // 根据不同GIF设置合适的缩放比例
    if (gif == &natural) {
        gif_width = 236;
    }
    // 计算缩放比例，使GIF适应显示区域
    int scale = (display_size * 256) / gif_width;
    lv_image_set_scale(emotion_gif_, scale);
}

void JerryEmojiDisplay::SetChatMessage(const char* role, const char* content) {
    if (!chat_message_label_ || !content) {
        return;
    }

    DisplayLockGuard lock(this);
    lv_label_set_text(chat_message_label_, content);
}

void JerryEmojiDisplay::SetIcon(const char* icon) {
    // Jerry开发板暂不支持图标显示
}
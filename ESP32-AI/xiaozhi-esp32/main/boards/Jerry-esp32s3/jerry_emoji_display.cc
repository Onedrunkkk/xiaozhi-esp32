void JerryEmojiDisplay::SetupGifContainer() {
    DisplayLockGuard lock(this);
    
    // 创建GIF容器
    emotion_gif_ = lv_gif_create(lv_screen_active());
    if (emotion_gif_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create GIF container");
        return;
    }
    
    // 设置GIF位置和大小为80%
    lv_obj_align(emotion_gif_, LV_ALIGN_TOP_MID, 0, 0);
    int gif_size = LV_HOR_RES * 0.8; // 调整为80%屏幕宽度
    lv_obj_set_size(emotion_gif_, gif_size, gif_size);
    lv_obj_set_y(emotion_gif_, -10);  // 向上移动10像素，更贴近状态栏
    
    // 设置默认GIF
    lv_gif_set_src(emotion_gif_, &natural_new_25fps);
    
    // 根据GIF大小调整缩放比例
    int gif_width = 240; // 默认GIF宽度
    int display_size = LV_HOR_RES * 0.8; // 显示区域大小为80%
    int scale = (display_size * 256) / gif_width;
    lv_image_set_scale(emotion_gif_, scale);


    // 创建聊天消息标签并设置在底部
    chat_message_label_ = lv_label_create(content_);
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9);
    lv_obj_set_height(chat_message_label_, 40); // 设置固定高度以显示两行
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_WRAP); // 支持换行
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);
    
    // 设置聊天消息背景色根据主题变化
    lv_obj_set_style_bg_opa(chat_message_label_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(chat_message_label_, lv_color_white(), 0); // 默认白色背景
    lv_obj_set_style_pad_ver(chat_message_label_, 5, 0);

    // 聊天消息定位在底部
    lv_obj_align(chat_message_label_, LV_ALIGN_BOTTOM_MID, 0, -10);  // 底部对齐，向上偏移10像素

    LcdDisplay::SetTheme("dark");  // 默认使用暗色主题
}

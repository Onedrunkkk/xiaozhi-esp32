# Jerry ESP32-S3 开发板支持

## 简介

Jerry ESP32-S3 是一款基于 ESP32-S3 芯片的开发板，集成了 LCD 显示屏、摄像头、麦克风、扬声器等丰富的外设，适用于各种 AI 和 IoT 应用。

## 硬件特性

- 主控芯片：ESP32-S3
- 显示屏：1.44 英寸 LCD，分辨率 128x128
- 摄像头：OV2640
- 音频：SPM1423HM4H-B 麦克风，NS4150 音频放大器
- 存储：microSD 卡槽
- 其他：用户按键、LED 指示灯

## 引脚分配

### 音频接口
- I2S 麦克风 (INMP441)：
  - WS 引脚：GPIO4
  - SCK 引脚：GPIO5
  - DIN 引脚：GPIO6

- I2S 功放 (MAX98367A)：
  - DOUT 引脚：GPIO7
  - BCLK 引脚：GPIO15
  - LRCK 引脚：GPIO16

### 显示接口
- SPI 显示屏 (ST7789)：
  - MOSI 引脚：GPIO47
  - SCLK 引脚：GPIO21
  - CS 引脚：GPIO41
  - DC 引脚：GPIO40
  - RST 引脚：GPIO45
  - BL 引脚：GPIO42

### 其他接口
- RGB 灯带 (WS2812)：GPIO48
- 用户按键：GPIO0

## 新增功能和改进

### GIF 表情显示系统增强

Jerry 开发板现在支持全新的 GIF 表情显示系统，具有以下增强功能：

1. **高帧率动画支持**：
   - 支持 25fps 的流畅 GIF 动画播放
   - 使用新的表情资源，提供更生动的视觉效果

2. **智能主题适配**：
   - 支持亮色和暗色主题切换
   - GIF 动画会根据当前主题自动切换普通版或 Inversion 版
   - 启动时会加载上次使用的主题设置

3. **优化的 UI 层级管理**：
   - 聊天消息文本始终显示在 GIF 表情上方
   - 支持聊天消息的水平居中对齐和自动换行
   - 改进了背景色处理，避免在暗色主题下出现白边

4. **启动流程优化**：
   - 消除了初始化过程中的屏幕闪烁问题
   - 优化了 LVGL 初始化顺序，确保 UI 一次性完整显示

### 问题解决过程

在开发过程中，我们遇到了并解决了以下关键问题：

1. **启动闪屏问题**：
   - **问题**：设备启动时屏幕会闪烁，用户体验不佳
   - **原因**：硬件初始化绘制白色背景与 LVGL 初始化绘制界面存在时间差
   - **解决方案**：调整初始化顺序，先完成 LVGL 初始化再开启显示

2. **主题切换时 GIF 未更新**：
   - **问题**：切换亮色/暗色主题时，GIF 表情未自动切换到对应版本
   - **原因**：缺少在主题切换时更新 GIF 的机制
   - **解决方案**：添加 `UpdateGifForCurrentTheme()` 方法，在主题切换时自动更新 GIF

3. **UI 元素层级问题**：
   - **问题**：聊天消息文本有时被 GIF 表情遮挡
   - **原因**：LVGL 对象层级管理不当
   - **解决方案**：使用 `lv_obj_move_foreground()` 确保消息标签始终在最前

4. **GIF 边框问题**：
   - **问题**：在暗色主题下 GIF 周围出现白色边框
   - **原因**：GIF 容器默认样式未正确设置
   - **解决方案**：移除边框设置并正确配置背景色

## 开发环境搭建

1. 安装 ESP-IDF v5.4.2
2. 克隆项目代码
3. 执行 `idf.py menuconfig` 配置开发板参数
4. 执行 `idf.py build` 编译项目
5. 执行 `idf.py flash` 烧录固件

## 自定义字体生成

使用 `lv_font_conv` 工具生成自定义字体：

```bash
npx lv_font_conv --font "D:/ESP32/Project/font/YSHaoShenTi.ttf" --size 16 --format lvgl --bpp 4 --no-compress -o "D:/ESP32/Project/font/font_YSHaoShenTi_16px_b4.c" --range 0x20-0x7F --range 0x4E00-0x9FFF --range 0x3000-0x303F --range 0xFF00-0xFFEF
```

## 问题解决记录

### 字体链接错误问题

**问题描述：**
在编译过程中遇到如下错误：
```
undefined reference to `font_YSHaoShenTi_16px_b4'
```

**问题分析：**
1. 首先发现是字体符号未正确链接到最终可执行文件中
2. 检查发现字体文件虽然存在，但未被正确添加到构建系统中
3. 通过分析其他开发板实现方式，发现项目中字体文件应该在主 [main/CMakeLists.txt](file:///d:\ESP32\Project\ESP32-AI\xiaozhi-esp32\main\CMakeLists.txt) 中统一管理，而不是在各开发板中单独添加

**解决方案：**
1. 在 [main/CMakeLists.txt](file:///d:\ESP32\Project\ESP32-AI\xiaozhi-esp32\main\CMakeLists.txt) 的 `SOURCES` 列表中添加字体文件路径：
   ```cmake
   "../newfunction/font/font_YSHaoShenTi_16px_b4.c"
   "../newfunction/font/font_YSHaoShenTi_18px_b4.c"
   ```
2. 移除 [Jerry-esp32s3/CMakeLists.txt](file:///d:\ESP32\Project\ESP32-AI\xiaozhi-esp32\main\boards\Jerry-esp32s3\CMakeLists.txt) 中重复的字体文件包含，避免冲突
3. 确保 [Jerry-esp32s3/jerry_esp32s3_board.cc](file:///d:\ESP32\Project\ESP32-AI\xiaozhi-esp32\main\boards\Jerry-esp32s3\jerry_esp32s3_board.cc) 中正确使用 `LV_FONT_DECLARE` 声明字体：
   ```cpp
   LV_FONT_DECLARE(font_YSHaoShenTi_16px_b4);
   ```

**验证结果：**
修改后重新编译项目，成功解决了字体链接错误问题，项目可以正常编译和运行。

### GIF表情资源适配LVGL新版本API问题

**问题描述：**
在编译过程中遇到如下错误：
```
error: 'LV_IMG_CF_RAW_CHROMA_KEYED' undeclared here (not in a function)
error: 'lv_image_header_t' has no member named 'always_zero'
```

**问题分析：**
1. 项目中使用的LVGL版本已更新，部分API发生了变化
2. 新添加的GIF资源文件仍在使用旧版本的API格式
3. 旧API中的字段和常量已被新版本替代

**解决方案：**
1. 将所有GIF资源文件中的图像描述结构体定义从旧格式：
   ```c
   .header.cf = LV_IMG_CF_RAW_CHROMA_KEYED,
   .header.always_zero = 0,
   .header.reserved = 0,
   ```
   
   更新为新格式：
   ```c
   .header.cf = LV_COLOR_FORMAT_RGB565,
   .header.magic = LV_IMAGE_HEADER_MAGIC,
   ```

2. 删除多余的 `#endif /*LV_ATTRIBUTE_MEM_ALIGN*/` 行

**验证结果：**
修改后重新编译项目，成功解决了GIF资源适配问题，所有表情动画可以正常显示。

## 功能特性

### 表情显示功能

Jerry开发板支持丰富的表情显示功能，通过GIF动画形式展现不同的情绪状态：

1. 支持的基础表情：
   - neutral（中性）-> natural_new_25fps.gif
   - relaxed（放松）-> natural_new_25fps.gif
   - sleepy（困倦）-> natural_new_25fps.gif
   - happy（开心）-> excitement_25fps.gif
   - laughing（笑）-> excitement_25fps.gif
   - funny（有趣）-> excitement_25fps.gif
   - loving（喜爱）-> excitement_25fps.gif
   - confident（自信）-> excitement_25fps.gif
   - winking（眨眼）-> excitement_25fps.gif
   - cool（酷）-> excitement_25fps.gif
   - delicious（美味）-> excitement_25fps.gif
   - kissy（亲吻）-> excitement_25fps.gif
   - silly（傻气）-> excitement_25fps.gif
   - sad（悲伤）-> sad_25fps.gif
   - crying（哭泣）-> sad_25fps.gif
   - angry（愤怒）-> angry_25fps.gif
   - surprised（惊讶）-> excitement_25fps.gif
   - shocked（震惊）-> scare_25fps.gif
   - thinking（思考）-> natural_new_25fps.gif
   - confused（困惑）-> disdain_25fps.gif
   - embarrassed（尴尬）-> disdain_25fps.gif
   - natural（自然）-> natural_new_25fps.gif

2. 特殊表情映射：
   - disdain -> disdain_25fps.gif
   - scare -> scare_25fps.gif
   - natural_new -> natural_new_25fps.gif
   - excitement -> excitement_25fps.gif

### 表情显示位置调整

为了优化视觉效果，GIF表情显示区域已调整至紧贴屏幕顶部：

1. 表情GIF默认显示在屏幕上方
2. 表情区域占据屏幕约70%的空间
3. 通过调整 `lv_obj_set_y(emotion_gif_, 0)` 确保表情紧贴顶部显示

### 聊天消息显示

屏幕底部支持聊天消息显示功能：

1. 消息显示区域位于屏幕底部
2. 支持多行文本显示
3. 具有半透明黑色背景以提高可读性

## 使用说明

1. 按照开发环境搭建步骤配置好环境
2. 根据需要修改配置参数
3. 编译并烧录固件
4. 观察 LCD 显示和 LED 灯效果

### 表情控制接口

通过调用 `SetEmotion(const char* emotion)` 方法可以切换显示的表情：
```cpp
display->SetEmotion("happy");     // 显示开心表情
display->SetEmotion("sad");       // 显示悲伤表情
display->SetEmotion("angry");     // 显示愤怒表情
```

### 聊天消息显示接口

通过调用 `SetChatMessage(const char* role, const char* content)` 方法可以显示聊天消息：
```cpp
display->SetChatMessage("user", "你好，小智！");  // 显示用户消息
display->SetChatMessage("assistant", "你好！有什么可以帮助你的吗？");  // 显示助手消息
```
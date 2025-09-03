# Jerry ESP32-S3 开发板

Jerry ESP32-S3是一款专为AI聊天机器人设计的开发板，具有以下特点：

## 硬件特性

- 主控芯片：ESP32-S3
- 集成2.4寸彩色LCD显示屏（240x320分辨率）
- 集成摄像头（用于人脸识别等应用）
- 集成麦克风和扬声器（语音交互）
- 集成WS2812 RGB LED（状态指示）
- 支持Wi-Fi和蓝牙连接

## 开发板配置

该开发板使用标准的ESP32-S3配置，具有以下特点：

- 使用SPI接口连接的ST7789 LCD显示屏
- 使用I2S接口连接的音频编解码器
- 使用GPIO连接的按钮和LED

## 字体配置

本开发板使用YSHaoShenTi字体作为主要文本显示字体，具有良好的中文显示效果。字体配置信息如下：

- 文本字体：YSHaoShenTi 18px (4位色深)
- 图标字体：Font Awesome 20px
- 表情字体：内置Emoji字体

如需修改字体，可以在`jerry_esp32s3_board.cc`文件中修改[DisplayFonts](file:///d:/ESP32/Project/ESP32-AI/xiaozhi-esp32/managed_components/78__xiaozhi-display/src/display.h#L12-L16)结构体中的[text_font](file:///d:/ESP32/Project/ESP32-AI/xiaozhi-esp32/managed_components/78__xiaozhi-display/src/display.h#L13-L13)字段，并在CMakeLists.txt中添加相应的字体文件路径。

### 字体转换指令

如果需要将TTF字体文件转换为LVGL可用的C数组格式，可以使用以下命令：

```bash
npx lv_font_conv --font "D:/ESP32/Project/font/YSHaoShenTi.ttf" --size 16 --format lvgl --bpp 4 --no-compress -o "D:/ESP32/Project/font/font_YSHaoShenTi_16px_b4.c" --range 0x20-0x7F --range 0x4E00-0x9FFF
```

参数说明：
- `--font`: 指定输入的TTF字体文件路径
- `--size`: 设置字体大小（例如16像素）
- `--format lvgl`: 指定输出格式为LVGL兼容格式
- `--bpp 4`: 设置颜色深度为4位（16级灰度）
- `--no-compress`: 禁用压缩以获得更好的显示效果
- `-o`: 指定输出文件路径
- `--range`: 指定字符范围，包括ASCII字符(0x20-0x7F)和中文常用汉字(0x4E00-0x9FFF)

## 引脚分配

### 音频部分
- I2S麦克风引脚：
  - WS: GPIO4
  - SCK: GPIO5
  - DIN: GPIO6

- I2S功放引脚：
  - DOUT: GPIO7
  - BCLK: GPIO15
  - LRCK: GPIO16

### 显示部分
- SPI显示屏引脚：
  - MOSI: GPIO11
  - SCLK: GPIO12
  - CS: GPIO10
  - DC: GPIO13
  - RST: GPIO14

### 其他外设
- RGB LED: GPIO48
- 按键: GPIO0
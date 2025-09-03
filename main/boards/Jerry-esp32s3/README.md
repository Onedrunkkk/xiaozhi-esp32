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

## GIF表情显示功能

Jerry开发板支持显示GIF格式的表情动画，用于增强人机交互体验。GIF表情功能要点如下：

### GIF资源管理
- GIF文件存储在`newfunction/gif/`目录下
- 每个GIF文件都转换为C数组格式，以便直接嵌入到固件中
- 支持的GIF表情包括：happy, natural, sad, scare, buxue, anger等

### GIF显示特性
- 使用LVGL的GIF解码器显示动画
- 自动适配屏幕尺寸，确保GIF在240x240的显示区域内正确缩放
- 支持动态切换不同表情，通过SetEmotion接口实现
- 所有表情统一映射到happy.gif以简化资源管理

### 显示区域配置
- GIF显示区域设置为屏幕的60%大小（约144x144像素）
- GIF自动居中显示在屏幕上
- 底部保留区域用于显示聊天消息

## 加载GIF动画的前置条件

要在Jerry开发板上成功加载和显示GIF动画，需要满足以下前置条件：

### 1. LVGL GIF支持库
- 确保项目中已集成`lvgl/src/extra/libs/gif/lv_gif.h`库文件
- 启用LVGL配置中的GIF支持选项（LV_USE_GIF）
- 需要链接`lvgl/src/extra/libs/gif/lv_gif.c`源文件

### 2. GIF文件格式要求
- GIF文件必须转换为LVGL兼容的C数组格式
- 使用的GIF应该是尺寸适中的动画（推荐宽度不超过240像素）
- GIF文件应优化以适应嵌入式设备的内存限制
- 每帧的尺寸应该保持一致

### 3. 内存要求
- 确保有足够的RAM来存储GIF数据和解码缓冲区
- ESP32-S3的8MB PSRAM足以支持中等大小的GIF动画
- 复杂的GIF动画可能需要额外的内存优化

### 4. 文件转换步骤
要将标准GIF文件转换为可在项目中使用的格式，需要执行以下步骤：

#### 步骤1：准备GIF文件
- 准备目标GIF动画文件，确保尺寸适中（推荐宽度不超过240像素）
- 优化GIF的颜色数量和帧数以减小文件大小

#### 步骤2：使用工具转换
可以使用以下方法将GIF转换为C数组：

方法一：使用在线转换工具
- 访问LVGL官方提供的在线图像转换工具
- 上传GIF文件并设置适当的参数（颜色格式、压缩等）
- 下载生成的C文件和头文件

方法二：使用命令行工具
```bash
# 使用lvgl官方工具进行转换
python lvgl/scripts/lv_img_conv.py --ofmt=C --cf=RGB565 --compress=NONE input.gif -o output.c
```

#### 步骤3：集成到项目中
- 将生成的C文件和头文件放入`newfunction/gif/`目录
- 在对应的CMakeLists.txt中添加新文件的引用
- 在需要使用GIF的源文件中包含相应的头文件
- 声明GIF资源并使用LVGL API进行显示

### 5. 显示配置
- 需要创建lv_gif对象来显示动画
- 使用`lv_gif_set_src()`函数设置GIF源数据
- 可以使用`lv_image_set_scale()`调整GIF显示尺寸
- 需要合理设置GIF显示区域大小以适应屏幕

### 6. 性能优化建议
- 对于复杂的GIF动画，建议预加载到PSRAM中
- 避免同时播放多个GIF动画以节省内存
- 对于简单的表情动画，可以考虑使用帧动画替代GIF

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
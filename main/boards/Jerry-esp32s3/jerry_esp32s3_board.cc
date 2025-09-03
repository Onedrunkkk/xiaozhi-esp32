#include "wifi_board.h"
#include "codecs/box_audio_codec.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "jerry_emoji_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "led/circular_strip.h"
#include "led/single_led.h"
#include <string>  // 确保包含string头文件

#include <esp_log.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_commands.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_lcd_panel_st7789.h>
#include <wifi_station.h>
#include <driver/i2c_master.h>

#define TAG "JerryEsp32s3Board"

LV_FONT_DECLARE(font_YSHaoShenTi_16px_b4);
LV_FONT_DECLARE(font_awesome_20_4);

class JerryEsp32s3Board : public WifiBoard {
private:
    Button boot_button_;
    Display* display_;
    Led* led_;
    i2c_master_bus_handle_t i2c_bus_handle_;

    void InitializeI2C() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = GPIO_NUM_8,
            .scl_io_num = GPIO_NUM_9,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_handle_));
    }

    void InitializeDisplay() {
        // 初始化SPI总线
        spi_bus_config_t buscfg = {
            .mosi_io_num = DISPLAY_SPI_MOSI_PIN,
            .miso_io_num = GPIO_NUM_NC,
            .sclk_io_num = DISPLAY_SPI_SCLK_PIN,
            .quadwp_io_num = GPIO_NUM_NC,
            .quadhd_io_num = GPIO_NUM_NC,
            .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2 + 8,
            .flags = SPICOMMON_BUSFLAG_MASTER,
            .intr_flags = 0
        };
        
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
        
        // 添加SPI设备配置
        spi_device_interface_config_t devcfg = {
            .command_bits = 8,
            .address_bits = 24,
            .mode = 0,
            .clock_speed_hz = 40 * 1000 * 1000,
            .spics_io_num = DISPLAY_SPI_CS_PIN,
            .flags = SPI_DEVICE_HALFDUPLEX,
            .queue_size = 10,
        };
        
        spi_device_handle_t spi_device;
        ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_device));
        
        // IO配置
        esp_lcd_panel_io_spi_config_t io_config = {
            .cs_gpio_num = DISPLAY_SPI_CS_PIN,
            .dc_gpio_num = DISPLAY_DC_PIN,
            .spi_mode = 0,
            .pclk_hz = 40 * 1000 * 1000,
            .trans_queue_depth = 10,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
        };
        
        esp_lcd_panel_io_handle_t io_handle;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io_handle));
        
        // Panel配置
        esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = DISPLAY_RST_PIN,
            .rgb_endian = LCD_RGB_ENDIAN_RGB,
            .bits_per_pixel = 16,
        };
        
        esp_lcd_panel_handle_t panel_handle;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        
        // 初始化完成后发送初始化命令
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
        
        // 设置显示参数
        esp_lcd_panel_invert_color(panel_handle, true);  // 添加颜色反色设置
        esp_lcd_panel_swap_xy(panel_handle, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel_handle, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        esp_lcd_panel_set_gap(panel_handle, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y);
        
        // 创建LCD显示对象
        DisplayFonts fonts = {
            .text_font = &font_YSHaoShenTi_16px_b4,
            .icon_font = &font_awesome_20_4,
            .emoji_font = font_emoji_32_init(),  // 使用32像素的emoji字体
        };
        
        display_ = new JerryEmojiDisplay(
            io_handle,
            panel_handle,
            DISPLAY_WIDTH,
            DISPLAY_HEIGHT,
            DISPLAY_OFFSET_X,
            DISPLAY_OFFSET_Y,
            DISPLAY_MIRROR_X,
            DISPLAY_MIRROR_Y,
            DISPLAY_SWAP_XY,
            fonts
        );
    }

    void InitializeLed() {
        // 初始化WS2812 RGB灯带
        if (BUILTIN_LED_GPIO != GPIO_NUM_NC) {
            if (BUILTIN_LED_COUNT > 1) {
                led_ = new CircularStrip(BUILTIN_LED_GPIO, BUILTIN_LED_COUNT);
            } else {
                led_ = new SingleLed(BUILTIN_LED_GPIO);
            }
        }
    }

public:
    JerryEsp32s3Board() : boot_button_(BOOT_BUTTON_GPIO), display_(nullptr), led_(nullptr) {
        ESP_LOGI(TAG, "Initializing Jerry ESP32-S3 Board");
        
        // 初始化I2C总线
        InitializeI2C();
        
        // 初始化显示屏
        InitializeDisplay();
        
        // 初始化LED
        InitializeLed();
    }

    virtual ~JerryEsp32s3Board() {
        if (display_) {
            ESP_LOGD(TAG, "Deleting display instance");
            delete display_;
            display_ = nullptr;
        }
        if (led_) {
            ESP_LOGD(TAG, "Deleting LED instance");
            delete led_;
            led_ = nullptr;
        }
    }

    virtual std::string GetBoardType() override {
        return "Jerry-esp32s3";
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS,
            AUDIO_I2S_MIC_GPIO_DIN
        );
#else
        static NoAudioCodecDuplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN
        );
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Led* GetLed() override {
        if (!led_) {
            ESP_LOGW(TAG, "LED hardware not initialized or not available");
        }
        return led_;
    }

    virtual std::string GetBoardJson() override {
        return "{\"board\":\"Jerry-esp32s3\"}";
    }
};

// 创建开发板实例的工厂函数
void* create_board() {
    static JerryEsp32s3Board board;
    return &board;
}
#include "wifi_board.h"
#include "adc_pdm_audio_codec.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "display/lcd_display.h"
#if ECE445_ENABLE_CAMERA
#include "esp32_camera.h"
#endif
#include "face_display.h"

#include <esp_log.h>
#include <esp_rom_sys.h>
#include <driver/spi_common.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>

#define TAG "ECE445_CUSTOM_S3"

namespace {
// ST7735 init sequence, compressed. The ESP-IDF ST7789 driver issues SWRESET,
// SLPOUT, COLMOD, MADCTL, and DISPON — enough for a real ST7789 but NOT enough
// for ST7735/ST7735S, which requires frame-rate, power and gamma register
// writes before the panel will actually drive pixels. Without this, the
// backlight is on but the screen stays black.
//
// Format: {cmd, len_of_params_including_delay_byte, params..., delay_ms_byte}
// delay_ms_byte is encoded as last param with high bit set (0x80|ms). 0 if no delay.
struct St7735InitCmd { uint8_t cmd; uint8_t len; uint8_t data[16]; };

constexpr St7735InitCmd kSt7735Init[] = {
    {0x11, 0,  {}                                                                             }, // SLPOUT
    // Frame rate control (in normal mode / idle mode / partial mode)
    {0xB1, 3,  {0x01, 0x2C, 0x2D}                                                             },
    {0xB2, 3,  {0x01, 0x2C, 0x2D}                                                             },
    {0xB3, 6,  {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D}                                           },
    {0xB4, 1,  {0x07}                                                                          }, // Display inversion control
    // Power control
    {0xC0, 3,  {0xA2, 0x02, 0x84}                                                             },
    {0xC1, 1,  {0xC5}                                                                          },
    {0xC2, 2,  {0x0A, 0x00}                                                                   },
    {0xC3, 2,  {0x8A, 0x2A}                                                                   },
    {0xC4, 2,  {0x8A, 0xEE}                                                                   },
    {0xC5, 1,  {0x0E}                                                                          }, // VMCTR1
    // Positive and negative gamma
    {0xE0, 16, {0x0F, 0x1A, 0x0F, 0x18, 0x2F, 0x28, 0x20, 0x22,
                0x1F, 0x1B, 0x23, 0x37, 0x00, 0x07, 0x02, 0x10}                                },
    {0xE1, 16, {0x0F, 0x1B, 0x0F, 0x17, 0x33, 0x2C, 0x29, 0x2E,
                0x30, 0x30, 0x39, 0x3F, 0x00, 0x07, 0x03, 0x10}                                },
    {0x3A, 1,  {0x05}                                                                          }, // COLMOD 16bpp
    {0x29, 0,  {}                                                                             }, // DISPON
};

void SendSt7735Init(esp_lcd_panel_io_handle_t io) {
    for (const auto& c : kSt7735Init) {
        esp_lcd_panel_io_tx_param(io, c.cmd, c.data, c.len);
    }
    // SLPOUT needs ~120ms before commands that access frame buffer are safe.
    esp_rom_delay_us(120 * 1000);
}
}  // namespace

class Ece445CustomS3Board : public WifiBoard {
private:
    Button boot_button_;
    LcdDisplay* display_ = nullptr;
    Camera* camera_ = nullptr;

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num     = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num     = GPIO_NUM_NC;
        buscfg.sclk_io_num     = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num   = GPIO_NUM_NC;
        buscfg.quadhd_io_num   = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel       = nullptr;

        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num       = DISPLAY_CS_PIN;     // GPIO_NUM_NC (-1) = CS tied to GND on board
        io_config.dc_gpio_num       = DISPLAY_DC_PIN;
        io_config.spi_mode          = DISPLAY_SPI_MODE;
        io_config.pclk_hz           = 10 * 1000 * 1000;   // Conservative for bring-up; raise to 20 MHz later.
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits      = 8;
        io_config.lcd_param_bits    = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;    // NC if tied high on board (uses SWRESET)
        panel_config.rgb_ele_order  = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        // ST7735 panels accept the ST7789 driver's command-register layout for
        // addressing, but need their own power/frame-rate/gamma init, which we
        // send by hand below after the ST7789 driver's SWRESET+SLPOUT pulse.
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        SendSt7735Init(panel_io);   // ST7735-specific power/gamma.
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        // ST7735 80x160 sits inside a 132x162 controller frame buffer starting
        // at column 24, row 0. We let the panel driver add this gap so every
        // draw_bitmap call lands in the visible window. LVGL offset is passed
        // as (0, 0) below to avoid double-offsetting the flush coordinates.
        esp_lcd_panel_set_gap(panel, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y);
        esp_lcd_panel_disp_on_off(panel, true);

        auto* face = new FaceLcdDisplay(panel_io, panel,
                                        DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                        /*offset_x=*/0, /*offset_y=*/0,
                                        DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
                                        DISPLAY_SWAP_XY);
        // Expose the face-specific MCP tool (self.face.set_playing_music)
        // so the LLM can flip the face to "music" while a song is playing.
        // The moving face is driven directly from robot_app::Submit().
        face->RegisterFaceMcpTools();
        display_ = face;
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

#if ECE445_ENABLE_CAMERA
    void InitializeCamera() {
        camera_config_t camera_config = {};
        camera_config.pin_pwdn     = CAMERA_PIN_PWDN;
        camera_config.pin_reset    = CAMERA_PIN_RESET;
        camera_config.pin_xclk     = CAMERA_PIN_XCLK;
        camera_config.pin_sccb_sda = CAMERA_PIN_SIOD;
        camera_config.pin_sccb_scl = CAMERA_PIN_SIOC;
        camera_config.pin_d7 = CAMERA_PIN_D7;
        camera_config.pin_d6 = CAMERA_PIN_D6;
        camera_config.pin_d5 = CAMERA_PIN_D5;
        camera_config.pin_d4 = CAMERA_PIN_D4;
        camera_config.pin_d3 = CAMERA_PIN_D3;
        camera_config.pin_d2 = CAMERA_PIN_D2;
        camera_config.pin_d1 = CAMERA_PIN_D1;
        camera_config.pin_d0 = CAMERA_PIN_D0;
        camera_config.pin_vsync = CAMERA_PIN_VSYNC;
        camera_config.pin_href  = CAMERA_PIN_HREF;
        camera_config.pin_pclk  = CAMERA_PIN_PCLK;

        camera_config.xclk_freq_hz = CAMERA_XCLK_FREQ_HZ;
        camera_config.ledc_timer   = LEDC_TIMER_1;    // LEDC_TIMER_0 is used by audio/backlight PWM elsewhere.
        camera_config.ledc_channel = LEDC_CHANNEL_2;

        // RGB565 @ QQVGA (160x120) lets Esp32Camera::Capture() push the
        // frame straight to the LCD via SetPreviewImage (the JPEG path in
        // esp32_camera.cc logs but cannot draw to LVGL). The 160x120 buffer
        // is 38 KB -- two of them live in PSRAM via fb_count=2. For the
        // server-side "explain" flow the capture is re-encoded to JPEG
        // on demand by image_to_jpeg_cb(), so we don't lose that path.
        camera_config.pixel_format = PIXFORMAT_RGB565;
        camera_config.frame_size   = FRAMESIZE_QQVGA;
        camera_config.jpeg_quality = 12;
        camera_config.fb_count     = 2;
        camera_config.fb_location  = CAMERA_FB_IN_PSRAM;
        camera_config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
        camera_config.sccb_i2c_port = -1;  // Let the driver create its own I2C master on SIOD/SIOC.

        camera_ = new Esp32Camera(camera_config);
    }
#endif

public:
    Ece445CustomS3Board() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeButtons();
#if ECE445_ENABLE_CAMERA
        // Camera: this module is ESP32-S3-WROOM-1-N16R8, so Octal PSRAM
        // permanently owns GPIO33..GPIO37 (MSPI data lanes + DQS). The
        // board was reworked so that XCLK/HREF/D6 moved off those pins
        // to GPIO9/10/11 -- see config.h for the full mapping.
        InitializeCamera();
        ESP_LOGI(TAG, "Initialized custom ESP32-S3 board with ST7735 80x160 LCD + OV5640");
#else
        ESP_LOGI(TAG, "Initialized custom ESP32-S3 board with ST7735 80x160 LCD (camera disabled)");
#endif
    }

    virtual AudioCodec* GetAudioCodec() override {
        static AdcPdmAudioCodec audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_ADC_MIC_CHANNEL,
            AUDIO_PDM_SPEAK_P_GPIO,
            AUDIO_PDM_SPEAK_N_GPIO,
            AUDIO_PA_CTL_GPIO
        );
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual Camera* GetCamera() override {
        return camera_;
    }
};

DECLARE_BOARD(Ece445CustomS3Board);

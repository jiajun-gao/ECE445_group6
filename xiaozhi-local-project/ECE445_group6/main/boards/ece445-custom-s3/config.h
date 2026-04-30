#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <esp_lcd_types.h>

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// ADC1_CHANNEL_3 on ESP32-S3 maps to GPIO4 (OPA_OUT)
#define AUDIO_ADC_MIC_CHANNEL       3
#define AUDIO_PDM_SPEAK_P_GPIO      GPIO_NUM_6
#define AUDIO_PDM_SPEAK_N_GPIO      GPIO_NUM_7
#define AUDIO_PA_CTL_GPIO           GPIO_NUM_5

#define BUILTIN_LED_GPIO            GPIO_NUM_NC
#define BOOT_BUTTON_GPIO            GPIO_NUM_0

// 0 = no camera hardware: skip OV5640 init; GetCamera() stays null so the
// server never lists MCP tool self.camera.take_photo. Set to 1 when the
// sensor is populated and wired per this board's camera pins in config.h.
#ifndef ECE445_ENABLE_CAMERA
#define ECE445_ENABLE_CAMERA 0
#endif

// -------- LCD (ST7735 80x160, SPI) --------
// Schematic wiring:
//   IO12 -> LED_RS  (Data/Command select, aka DC)
//   IO13 -> LED_SDA (SPI MOSI)
//   IO14 -> LED_SCL (SPI SCLK)
// CS is tied to GND on-module, RST is tied to 3V3 on-module, backlight to 3V3.
#define DISPLAY_DC_PIN         GPIO_NUM_12
#define DISPLAY_MOSI_PIN       GPIO_NUM_13
#define DISPLAY_CLK_PIN        GPIO_NUM_14
#define DISPLAY_CS_PIN         GPIO_NUM_NC
#define DISPLAY_RST_PIN        GPIO_NUM_NC
#define DISPLAY_BACKLIGHT_PIN  GPIO_NUM_NC

// 80x160 ST7735 IPS, mounted in LANDSCAPE orientation.
//   Native portrait:  80 wide x 160 tall, visible window at col 24, row 0.
//   Landscape (MV=1): 160 wide x 80 tall, visible window at col 0,  row 24.
// MADCTL MV (swap_xy) rotates the scan order 90 degrees; the visible window
// inside the 132x162 controller framebuffer moves with it, so the gap that
// was on the X axis in portrait now lives on the Y axis.
// If the picture ends up upside down or mirrored after flashing, flip
// DISPLAY_MIRROR_Y (or DISPLAY_MIRROR_X) below.
#define DISPLAY_WIDTH   160
#define DISPLAY_HEIGHT  80
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 24
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y true
#define DISPLAY_SWAP_XY  true
#define DISPLAY_INVERT_COLOR true
#define DISPLAY_RGB_ORDER    LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_SPI_MODE     0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

// -------- Camera (OV5640 on DVP 8-bit parallel) --------
// Wiring per ECE445 schematic. The OV5640 sensor drives its upper 8 data
// bits (Y2..Y9) to the ESP32-S3; esp32-camera's pin_d0..pin_d7 correspond
// to Y2 (LSB) .. Y9 (MSB). SCCB (I2C) shares no bus with any other device
// on this board, so the camera driver initializes its own I2C peripheral.
//
// NOTE: The original schematic routed XCLK, HREF and the D6/Y8 data lane
// to GPIO36, GPIO37 and GPIO35. On this module (ESP32-S3-WROOM-1-N16R8)
// those pins are permanently owned by the Octal PSRAM MSPI bus and cannot
// be repurposed. Three nets have been reworked to free GPIOs:
//
//   XCLK  : GPIO36 -> GPIO9    (ESP -> sensor, 20 MHz LEDC clock)
//   HREF  : GPIO37 -> GPIO10   (sensor -> ESP)
//   D6/Y8 : GPIO35 -> GPIO11   (sensor -> ESP, DVP data lane)
//
// All other camera pins are unchanged.
//
//   Sensor pin   ESP32-S3 GPIO
//   SIOC (SCL)   IO1
//   SIOD (SDA)   IO38
//   VSYNC        IO2
//   HREF         IO10   (reworked; was IO37)
//   PCLK         IO43
//   XCLK         IO9    (reworked; was IO36)
//   RESET        IO39   (PWDN is not wired; pulled low on the module)
//   D2 (Y2)      IO21   (LSB of 8-bit DVP data bus)
//   D3 (Y3)      IO40
//   D4 (Y4)      IO47
//   D5 (Y5)      IO41
//   D6 (Y6)      IO48
//   D7 (Y7)      IO42
//   D8 (Y8)      IO11   (reworked; was IO35)
//   D9 (Y9)      IO44   (MSB)
#define CAMERA_PIN_PWDN      -1
#define CAMERA_PIN_RESET     GPIO_NUM_39
#define CAMERA_PIN_XCLK      GPIO_NUM_9
#define CAMERA_PIN_SIOD      GPIO_NUM_38
#define CAMERA_PIN_SIOC      GPIO_NUM_1
#define CAMERA_PIN_VSYNC     GPIO_NUM_2
#define CAMERA_PIN_HREF      GPIO_NUM_10
#define CAMERA_PIN_PCLK      GPIO_NUM_43
// Data lanes, D0 = sensor Y2 (LSB) .. D7 = sensor Y9 (MSB).
#define CAMERA_PIN_D0        GPIO_NUM_21
#define CAMERA_PIN_D1        GPIO_NUM_40
#define CAMERA_PIN_D2        GPIO_NUM_47
#define CAMERA_PIN_D3        GPIO_NUM_41
#define CAMERA_PIN_D4        GPIO_NUM_48
#define CAMERA_PIN_D5        GPIO_NUM_42
#define CAMERA_PIN_D6        GPIO_NUM_11
#define CAMERA_PIN_D7        GPIO_NUM_44
// XCLK is routed over a greenwire from the ESP32-S3 (GPIO9) to the
// OV5640's XCLK pad (original schematic net was GPIO36, which is now
// owned by Octal PSRAM). A 20 MHz square wave does not survive the
// extra parasitic inductance/capacitance on that wire well, so start
// conservatively at 10 MHz -- OV5640's XCLK range is 6..27 MHz and
// the sensor's internal PLL multiplies this up for the pixel clock,
// so 10 MHz has no effect on the achievable frame rate at QQVGA.
// Bump back to 20000000 after verifying the signal on a scope.
#define CAMERA_XCLK_FREQ_HZ  10000000

#endif // _BOARD_CONFIG_H_

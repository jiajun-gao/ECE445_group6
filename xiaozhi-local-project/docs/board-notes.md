# Board Notes

## Observed From Photos

- Marking on back: `ATK-DNESP32S3 V1.3`
- Branding: `ALIENTEK / 正点原子`
- Board silkscreen on front: `ESP32S3`
- Visible peripherals:
  - speaker
  - microSD slot
  - USB
  - RGB LCD connector
  - headers

## Deployment Implication

This looks like a development/training board rather than a XiaoZhi-native finished product.

That means one of two paths is likely:

1. The upstream `xiaozhi-esp32` repo already includes a matching board definition.
2. We will need to add a custom board definition with the correct audio, button, display, and pin configuration.

We have now verified that option 1 is true for your board.

## Current Recommendation

Start with:

- conversation only
- English language
- local backend on your computer

Then add:

- voiceprint service for speaker identification

## Open Hardware Checks

Before flashing, confirm:

- microphone path available and working
- speaker/audio output path available and working
- flash size
- PSRAM presence

## Verified Upstream Match

The cloned `xiaozhi-esp32` repo already contains:

- board profile: `atk-dnesp32s3`
- board config directory: `main/boards/atk-dnesp32s3/`

## Verified Firmware Pin Map

From the upstream board config:

- audio codec: `ES8388`
- boot button: `GPIO0`
- builtin LED: `GPIO1`
- display size: `320x240`
- audio sample rate: `24000`

Audio pins:

- `MCLK GPIO3`
- `WS GPIO9`
- `BCLK GPIO46`
- `DIN GPIO14`
- `DOUT GPIO10`

Codec I2C pins:

- `SDA GPIO41`
- `SCL GPIO42`

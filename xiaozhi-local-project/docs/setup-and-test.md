# Setup And Test

## 1. Do You Need Arduino IDE

No.

This project is built for `ESP-IDF`, not Arduino IDE.
The upstream firmware documentation recommends the ESP-IDF plugin with SDK `5.4+`.

Use one of these:

- VS Code + ESP-IDF extension
- Cursor + ESP-IDF extension
- terminal with ESP-IDF installed

## 2. How To Connect The Board

Use a USB data cable from the board's USB port to your Mac.

After connecting, check for a serial device:

```bash
ls /dev/cu.* /dev/tty.*
```

A real board connection usually appears as something like:

- `/dev/cu.usbmodem...`
- `/dev/cu.wchusbserial...`
- `/dev/cu.SLAB_USBtoUART`

Right now your Mac only shows Bluetooth/debug ports, which means the board is not currently visible over USB.

## 3. If The Board Does Not Show Up

Check these in order:

1. Use a known USB data cable, not a charge-only cable.
2. Try the other USB connector on the board if it has more than one.
3. Press `RST` once after plugging in.
4. Try holding `BOOT`, then pressing `RST`, then releasing `BOOT`.
5. Try another USB port on your Mac or a powered hub.
6. Re-run:

```bash
ls /dev/cu.* /dev/tty.*
```

## 4. Firmware Build Path

Board profile:

- `atk-dnesp32s3`

Firmware repo:

- [xiaozhi-esp32](/Users/gaojiajun/Documents/New%20project/xiaozhi-local-project/firmware/xiaozhi-esp32)

Recommended build approach:

```bash
cd /Users/gaojiajun/Documents/New\ project/xiaozhi-local-project/firmware/xiaozhi-esp32
python scripts/release.py atk-dnesp32s3
```

Manual ESP-IDF flow:

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
idf.py -p /dev/cu.YOUR_PORT flash monitor
```

## 5. Backend Setup For Phase 1

Backend repo:

- [xiaozhi-esp32-server](/Users/gaojiajun/Documents/New%20project/xiaozhi-local-project/server/xiaozhi-esp32-server)

Your English-first override file is here:

- [data/.config.yaml](/Users/gaojiajun/Documents/New%20project/xiaozhi-local-project/server/xiaozhi-esp32-server/main/xiaozhi-server/data/.config.yaml)

Important:

1. Replace `YOUR_LAN_IP` with your Mac's Wi-Fi LAN IP.
2. Replace `REPLACE_WITH_YOUR_GLM_API_KEY` with a valid API key.
3. Download the `SenseVoiceSmall` ASR model into:

- `server/xiaozhi-esp32-server/main/xiaozhi-server/models/SenseVoiceSmall/model.pt`

## 6. How To Pick The Correct IP

The ESP32 must talk to an address reachable on the same network.

Good examples:

- `192.168.x.x`
- `10.x.x.x`
- `172.16-31.x.x`

Do not use a Tailscale-only `100.x.x.x` address unless your device can also reach that network path.

## 7. Backend Test Before Touching The Board

Start the backend first and confirm it prints a websocket address.

Typical expected output includes:

- `Websocket地址是 ws://YOUR_LAN_IP:8000/xiaozhi/v1/`
- `OTA接口是 http://YOUR_LAN_IP:8003/xiaozhi/ota/`

The repo also includes a browser test page:

- `server/xiaozhi-esp32-server/main/xiaozhi-server/test/test_page.html`

Open that in Chrome to test the server audio path before flashing the ESP32.

## 8. How To Test On The Board

Recommended order:

1. Start the backend on your Mac.
2. Confirm the websocket address is correct.
3. Build and flash firmware for `atk-dnesp32s3`.
4. Open serial monitor.
5. Put the board on the same Wi-Fi as your Mac.
6. Watch for Wi-Fi connection and websocket connection logs.
7. Speak and verify:

- microphone capture works
- TTS plays through the speaker
- English responses come back

## 9. Phase 2 Speaker Identification

Voiceprint repo:

- [voiceprint-api](/Users/gaojiajun/Documents/New%20project/xiaozhi-local-project/voiceprint/voiceprint-api)

Add this only after phase 1 works.

High-level steps:

1. Start MySQL.
2. Start `voiceprint-api`.
3. Set `voiceprint.url` in the backend config.
4. Register one WAV sample per user with the correct `speaker_id`.
5. Retest live conversation and confirm the recognized speaker is passed through.

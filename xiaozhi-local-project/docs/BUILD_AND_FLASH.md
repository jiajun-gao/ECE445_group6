# Build and flash — ECE445 Jarvis firmware (ESP32-S3)

Firmware lives in **`ECE445_group6/`** (ESP-IDF project). Target chip: **ESP32-S3**. Board profile: **ECE445 Custom S3**.

---

## 1. Prerequisites

- **ESP-IDF** **v5.4 or newer** (see project `README.md`).
- On macOS: install the Espressif **ESP-IDF** toolchain (installer or VS Code / Cursor **ESP-IDF extension**).
- USB cable to the module’s UART port (often CP2102 / CH340 — install the matching driver if needed).

Each new terminal session that builds firmware must load the IDF environment:

```bash
# Typical ESP-IDF install path on macOS (adjust to yours):
. $HOME/esp/esp-idf/export.sh
```

---

## 2. One-time: set chip target

From the firmware root:

```bash
cd /path/to/xiaozhi-local-project/ECE445_group6

# If you ever built for ESP32 before, clear the stale target:
unset IDF_TARGET

idf.py set-target esp32s3
```

If `set-target` fails complaining about the wrong chip, **`unset IDF_TARGET`** first, then retry.

---

## 3. Select your board in menuconfig

```bash
idf.py menuconfig
```

Navigate:

- **`Xiaozhi Assistant`** → **`Board Type`** → select **`ECE445 Custom S3`**.

Other useful options (same menu tree):

- **`Default Language`** — e.g. English for your demo.
- **`Default OTA URL`** — point at your own OTA server if you use OTA; otherwise leave default for class experiments.
- Wake word / assets options — follow upstream XiaoZhi docs if you change wake assets.

Save and exit.

---

## 4. Build

```bash
idf.py build
```

Fix any compile errors before flashing.

---

## 5. Flash and serial monitor

1. Put the ESP32-S3 in download mode if your board requires it (often: hold **BOOT**, tap **RST**, release **BOOT**).
2. Find the serial port:

   **macOS:** `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART` / similar.

3. Flash + monitor (replace the port):

```bash
idf.py -p /dev/cu.usbserial-XXXX build flash monitor
```

Exit the monitor with **`Ctrl+]`** (default ESP-IDF monitor).

---

## 6. Optional: flash only or monitor only

```bash
idf.py -p /dev/cu.usbserial-XXXX flash
idf.py -p /dev/cu.usbserial-XXXX monitor
```

---

## 7. Match the Mac server (voice pipeline)

After flashing, the device must reach your **`xiaozhi-esp32-server`** WebSocket URL.

Edit **`server/xiaozhi-esp32-server/main/xiaozhi-server/data/.config.yaml`**:

```yaml
server:
  websocket: ws://<YOUR_MAC_LAN_IP>:8000/xiaozhi/v1/
```

Use the same Wi‑Fi as the ESP32. Rebuild/restart the server after changing the IP.

Wi‑Fi credentials are usually set on the device the first time (AP / provisioning flow per XiaoZhi), or via `nvs`/config — follow your team’s setup.

---

## 8. Board-specific firmware notes (`ece445-custom-s3`)

- **Camera:** In `main/boards/ece445-custom-s3/config.h`, **`ECE445_ENABLE_CAMERA`** controls OV5640 init. Set to **`0`** if you have no camera — MCP will not expose `self.camera.take_photo`.
- **Power:** Weak batteries or thin wires can cause wake/ASR issues; see team notes on supply and decoupling.

---

## 9. Troubleshooting

| Symptom | What to try |
|--------|-------------|
| `set-target` wrong chip | `unset IDF_TARGET` then `idf.py set-target esp32s3` |
| Flash fails / no serial | Correct USB port, driver, BOOT/EN sequence, cable |
| Wrong board behavior | `menuconfig` → Board Type = **ECE445 Custom S3**, rebuild |
| Device talks to wrong server | Fix `websocket` in `.config.yaml`, restart server, reconnect device |

---

## Quick reference (copy-paste)

```bash
. $HOME/esp/esp-idf/export.sh   # adjust path
cd ~/Desktop/xiaozhi-local-project/ECE445_group6
unset IDF_TARGET
idf.py set-target esp32s3
idf.py menuconfig               # Board Type → ECE445 Custom S3
idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

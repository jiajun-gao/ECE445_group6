# Jarvis — Interactive Desktop Companion Robot

**ECE445 Senior Design · Team 6 · University of Illinois Urbana-Champaign**
*Jiajun Gao · Yuchen Shih · Zichao Wang*

A small **desktop robot** built on the **ESP32-S3** that listens for a wake word, talks to you with a streaming voice-AI pipeline, drives around your desk with cliff/obstacle safety, and shows a minimalist animated face on its onboard LCD. Internally, Jarvis is a custom-board fork of the open-source **XiaoZhi** AI chatbot stack, extended with our own PCB, motion subsystem, and face UI.

---

## 1. What Jarvis does

- **Wake on "Jarvis"** — offline WakeNet wake word, no cloud round-trip to wake.
- **Conversational AI** — streaming **ASR → LLM → TTS** through a local Mac server (Silero VAD, SenseVoice ASR, Gemini 2.5 Flash, Edge TTS).
- **Acts on your voice** — robot motion, mode switching, music, and device control via **MCP (Model Context Protocol)** tool calls.
- **Drives safely on a desk** — front IR obstacle stop and downward-facing cliff sensor that latches motors off at the edge.
- **Two control modes** — `ai` (voice-driven) and `human_control` (Wi-Fi browser joystick).
- **Animated face** — geometric LCD face that reflects *listening / thinking / speaking / moving / music / idle / error* states.

Reference timing on the integrated build (see [LATENCY_AND_SUCCESS_RATE.md](xiaozhi-local-project/docs/LATENCY_AND_SUCCESS_RATE.md)):

| Metric | Value |
|---|---|
| Wake-greeting response | < 100 ms |
| Direct-MCP voice command (e.g. "move forward") | ~2.2 s end-to-end |
| Conversational LLM turn (p50) | ~2.7 s end-to-end |
| MCP / LLM / TTS success rate | 100% over the recorded session |

---

## 2. Repository layout

```
ECE445_group6/
├── README.md                    ← you are here
├── Week 3.9                     ← weekly progress log
├── Gerber_SCH-MainBoard-Round1_2026-02-25 (1).zip   ← PCB Round 1 Gerbers
├── documents/
│   ├── proposal1.pdf            ← project proposal
│   └── design_document1.pdf     ← design document
└── xiaozhi-local-project/       ← all working code
    ├── ECE445_group6/           ← main firmware (ESP-IDF, ESP32-S3)
    │   └── main/boards/ece445-custom-s3/   ← our custom board profile
    ├── firmware/
    │   ├── xiaozhi-esp32/       ← upstream XiaoZhi reference firmware
    │   └── follow_user/         ← experimental vision-based "follow me" app
    ├── server/
    │   └── xiaozhi-esp32-server/   ← local Mac backend (VAD/ASR/LLM/TTS)
    ├── voiceprint/
    │   └── voiceprint-api/      ← optional speaker-recognition service
    ├── docs/                    ← build, test, latency, board, demo notes
    ├── notes/                   ← phase-by-phase next-steps log
    └── scripts/metrics/         ← latency / success-rate analysis scripts
```

---

## 3. System architecture

```
   ┌────────────────────────── ESP32-S3 (custom PCB) ──────────────────────────┐
   │                                                                           │
   │  ADC mic ──► AFE ──► WakeNet ──► Opus encoder ──┐                         │
   │                                                  │                        │
   │  Speaker ◄── PA  ◄── Opus decoder ◄──────────────┤                        │
   │                                                  │                        │
   │  ST7735 LCD (face)  ◄── FaceLcdDisplay/LVGL      │                        │
   │  Motors + drivers   ◄── robot.* MCP tools        │                        │
   │  Front IR / cliff IR ──► safety_manager          │   WebSocket            │
   │                                                  │   (binary audio +      │
   │                                                  │    JSON control)       │
   └──────────────────────────────────────────────────┼────────────────────────┘
                                                      │
                                                      ▼
   ┌────────────────────── Mac (xiaozhi-esp32-server) ────────────────────────┐
   │  VAD (Silero) → ASR (SenseVoice) → LLM (Gemini 2.5 Flash) → TTS (Edge)   │
   │  MCP dispatcher  ◄──►  device tools (robot.move, self.face.*, …)         │
   └──────────────────────────────────────────────────────────────────────────┘
```

**Transport:** WebSocket at `ws://<MAC_LAN_IP>:8000/xiaozhi/v1/`, with binary Opus frames plus JSON control messages (`stt`, `tts`, `llm`, `mcp`, `system`).

---

## 4. Hardware — `ece445-custom-s3`

Custom board profile lives in [xiaozhi-local-project/ECE445_group6/main/boards/ece445-custom-s3/](xiaozhi-local-project/ECE445_group6/main/boards/ece445-custom-s3/).

| Block | Detail |
|---|---|
| MCU | ESP32-S3 (Wi-Fi, dual-core, PSRAM) |
| Microphone | Analog mic on **GPIO4** (ADC path) |
| Speaker | PDM speaker + PA, **GPIO6/7/5** |
| Display | ST7735 80×160 SPI LCD (face) |
| Camera | OV5640 (optional — gated by `ECE445_ENABLE_CAMERA` in `config.h`) |
| Motors | Dual DC motor driver, controlled by `robot_app` |
| Safety | Front IR (obstacle), downward IR (cliff, ~7 cm threshold) |
| Boot button | GPIO0 |
| Power | 1S LiPo + boost/regulator (rail integrity is the dominant ASR-accuracy variable on battery) |

PCB: **Round 1** Gerbers are included at the repo root (`Gerber_SCH-MainBoard-Round1_2026-02-25 (1).zip`). Schematic and full design rationale are in [documents/design_document1.pdf](documents/design_document1.pdf).

---

## 5. Firmware

The shipping firmware is the **`ECE445_group6/`** ESP-IDF project. It is a fork of `xiaozhi-esp32` with our custom board, our `robot_app` motion stack, and a custom `FaceLcdDisplay`.

### Quick build (ESP-IDF ≥ 5.4)

```bash
. $HOME/esp/esp-idf/export.sh                          # adjust to your install
cd xiaozhi-local-project/ECE445_group6
unset IDF_TARGET
idf.py set-target esp32s3
idf.py menuconfig                                       # Xiaozhi Assistant → Board Type → ECE445 Custom S3
idf.py build
idf.py -p /dev/cu.usbserial-XXXX flash monitor
```

Full guide with troubleshooting: [xiaozhi-local-project/docs/BUILD_AND_FLASH.md](xiaozhi-local-project/docs/BUILD_AND_FLASH.md).

### Important compile-time switches

- `main/boards/ece445-custom-s3/config.h` → `ECE445_ENABLE_CAMERA` (`0` if unpopulated; otherwise OV5640 init runs and `self.camera.take_photo` is exposed via MCP).
- `Default Language` in `menuconfig` → set to **English** for our demo.

---

## 6. Backend (Mac)

Local server at [xiaozhi-local-project/server/xiaozhi-esp32-server/](xiaozhi-local-project/server/xiaozhi-esp32-server/).

1. Edit `main/xiaozhi-server/data/.config.yaml`:
   - `server.websocket: ws://<YOUR_MAC_LAN_IP>:8000/xiaozhi/v1/`
   - Set the LLM API key (Gemini / GLM as configured).
2. Drop the **SenseVoiceSmall** ASR model into `main/xiaozhi-server/models/SenseVoiceSmall/model.pt`.
3. Start the server. Confirm it logs:
   ```
   Websocket地址是 ws://<LAN_IP>:8000/xiaozhi/v1/
   OTA接口是      http://<LAN_IP>:8003/xiaozhi/ota/
   ```
4. Sanity-check the audio path with `main/xiaozhi-server/test/test_page.html` *before* flashing the board.

The Mac and the ESP32 must be on the same routable LAN (no Tailscale-only `100.x.x.x`). Full setup: [xiaozhi-local-project/docs/setup-and-test.md](xiaozhi-local-project/docs/setup-and-test.md).

### Optional: voiceprint (Phase 2)

Speaker recognition service at [xiaozhi-local-project/voiceprint/voiceprint-api/](xiaozhi-local-project/voiceprint/voiceprint-api/). Bring it up only after the base conversation loop works, then point the backend at it via `voiceprint.url`.

---

## 7. MCP tools Jarvis exposes

Implemented in firmware and reachable from the LLM as named tools.

**Robot motion** (custom — `robot_app.cc`):

| Tool | Args | Notes |
|---|---|---|
| `robot.move` | `direction` ∈ {`forward`, `backward`, `left`, `right`, `spin_left`, `spin_right`, `stop`}; optional `speed` 0–100 | Returns mode, IR/obstacle flags, safety reason (cliff latch, human-control block, …). |
| `robot.set_mode` | `mode` ∈ {`ai`, `human_control`} (`human`, `manual` are accepted aliases) | `human_control` enables Wi-Fi joystick driving. |
| `robot.get_status` | — | Compact mode + IR + safety string for the LLM. |
| `robot.dance` | — | Pre-programmed ~16 s routine. |

**Device / UX** (XiaoZhi common + our extensions):

| Tool | Notes |
|---|---|
| `self.get_device_status` | Speaker, screen, battery, network. |
| `self.audio_speaker.set_volume` | 0–100. |
| `self.screen.set_brightness` / `set_theme` | When backlight / themes are available. |
| `self.camera.take_photo` | Only when camera is initialized. |
| `self.face.set_playing_music` | Switches the face into the music/enjoying state. |

---

## 8. Face UI

`FaceLcdDisplay` ([main/boards/ece445-custom-s3/face_display.cc](xiaozhi-local-project/ECE445_group6/main/boards/ece445-custom-s3/face_display.cc)) renders a **minimalist geometric face** (circles, arcs, lines — not emoji, not pre-rendered sprites) on the ST7735 with double-buffering at 30–60 FPS. States are bound to the device state machine: **listening, thinking, speaking, moving, music, idle, error**, with ~200–500 ms transitions.

---

## 9. Demo flow

1. Wake — *"Jarvis"* → cached greeting plays, face → **listening**.
2. Chat — *"What's the weather like for a desk robot?"* → **thinking** → **speaking** (streaming TTS).
3. Drive — *"Move forward."* → `robot.move` → wheels turn, face → **moving**; cliff sensor stops it at the edge.
4. Hand off — *"Switch to human control."* → `robot.set_mode("human_control")` → drive from a phone browser.
5. Music — *"Play some music."* → `play_music` + `self.face.set_playing_music(true)` → **music** face.

A full talk-track is in [xiaozhi-local-project/docs/FINAL_PRESENTATION_SUMMARY.md](xiaozhi-local-project/docs/FINAL_PRESENTATION_SUMMARY.md).

---

## 10. Documentation index

Inside [xiaozhi-local-project/docs/](xiaozhi-local-project/docs/):

- [BUILD_AND_FLASH.md](xiaozhi-local-project/docs/BUILD_AND_FLASH.md) — ESP-IDF toolchain, board target, flash, troubleshooting.
- [setup-and-test.md](xiaozhi-local-project/docs/setup-and-test.md) — backend setup, IP selection, board-side bring-up, voiceprint Phase 2.
- [board-notes.md](xiaozhi-local-project/docs/board-notes.md) — pinout + verified upstream board match notes.
- [LATENCY_AND_SUCCESS_RATE.md](xiaozhi-local-project/docs/LATENCY_AND_SUCCESS_RATE.md) — per-turn latency, MCP/LLM/TTS success rates from a live session.
- [FINAL_PRESENTATION_SUMMARY.md](xiaozhi-local-project/docs/FINAL_PRESENTATION_SUMMARY.md) — one-page system overview for the demo.
- [metrics_report/](xiaozhi-local-project/docs/metrics_report) — generated metric outputs.

Top-level project documents:

- [documents/proposal1.pdf](documents/proposal1.pdf)
- [documents/design_document1.pdf](documents/design_document1.pdf)

---

## 11. Status & next steps

Current state (per [Week 3.9](Week 3.9) and [xiaozhi-local-project/notes/next-steps.md](xiaozhi-local-project/notes/next-steps.md)):

- ✅ End-to-end voice pipeline working on the custom board over local Mac backend.
- ✅ MCP-driven motion with cliff/obstacle safety and Wi-Fi human-control mode.
- ✅ Geometric LCD face wired to device state.
- ⏳ ASR word accuracy on battery (~50–60%) — limited by analog-mic rail sag; addressed by power-integrity fixes on PCB Round 2 (bulk caps, regulator sizing, shorter mic traces).
- ⏳ Vision-based "follow me" app under [firmware/follow_user/](xiaozhi-local-project/firmware/follow_user/) — separate experiment, not yet integrated into the main firmware.

---

## 12. Credits & licenses

Built on top of:

- [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) — upstream firmware (MIT).
- [xinnan-tech/xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server) — Python server.
- ESP-IDF, ESP-SR (WakeNet/AFE), LVGL, Silero VAD, SenseVoice, Edge TTS, Google Gemini.

Our additions (custom board, `robot_app`, `FaceLcdDisplay`, PCB) are released under the same MIT license as the upstream project unless otherwise noted in a subdirectory.

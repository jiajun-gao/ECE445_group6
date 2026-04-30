# Jarvis Desktop Robot — Final Presentation Summary

**ECE445 Team 6 · University of Illinois Urbana-Champaign**

---

## One-line pitch

A **small desktop robot** built on **ESP32-S3** that runs a **XiaoZhi-style voice AI pipeline**: offline wake word, streaming audio to a Mac server for **ASR + LLM + TTS**, **MCP tool calls** for motion and UI control, and a **minimal geometric face** on an LCD that reflects listening, thinking, speaking, moving, music, idle, and error states.

---

## What we built

| Layer | Description |
|--------|-------------|
| **Device** | Custom board **ece445-custom-s3**: ESP32-S3, **ADC microphone** (GPIO4), **PDM speaker** + PA (GPIO6/7/5), **ST7735 80×160** SPI LCD, optional OV5640 camera (can be disabled in firmware when not populated). |
| **Voice stack** | Offline **WakeNet** wake word (“Jarvis”), **AFE** audio front end, **Opus** streaming to the server. |
| **Server** | **xiaozhi-esp32-server** on Mac: **Silero VAD**, **Sherpa / SenseVoice ASR**, **Gemini 2.5 Flash** (via **google-genai** SDK), **Edge TTS**, WebSocket session per device. |
| **Control** | **MCP (Model Context Protocol)** over WebSocket: the LLM and fast-path handlers invoke **named tools** on the device (JSON-RPC style). |
| **UX** | Configurable **wake greeting**, **echo guard** for speaker→mic bleed, **human control mode** (Wi‑Fi manual drive), **latency-oriented** server tuning (streaming, fast replies, tool caching). |

---

## Technical architecture

```
ESP32-S3                              Mac (xiaozhi-esp32-server)
┌─────────────────┐                   ┌──────────────────────────────┐
│ Mic → AFE       │   Opus / WS       │ VAD → ASR → intent / LLM     │
│ Offline wake    │ ───────────────►  │ → TTS (streaming chunks)     │
│ LCD + motors    │ ◄──────────────  │ ← MCP tool calls → device    │
└─────────────────┘                   └──────────────────────────────┘
```

- **Transport:** WebSocket (e.g. `ws://<host>:8000/xiaozhi/v1/`), binary audio plus JSON control messages (`stt`, `tts`, `llm` emotion, `mcp`, `system`, etc.).
- **LLM:** **Gemini 2.5 Flash** with **function/tool calling**; provider replays **`thought_signature`** for reliable multi-turn tool use on “thinking” models.
- **Latency:** Streaming LLM output into TTS; optional **fast paths** for greetings and short robot phrases; **short-lived cache** for repeated read-only MCP results (e.g. status).

---

## End-to-end workflow

1. **Wake:** User speaks wake phrase → device detects offline → session active → server can play a **cached Jarvis greeting** (“Hello, I’m Jarvis, your desktop robot, developed by ECE445 team 6.”).
2. **Listen:** Device streams audio; **VAD** segments speech; **ASR** produces text.
3. **Think:** Server may use **fast intent** (direct robot commands / canned short replies) or full **LLM** with optional **MCP** tool execution on the device.
4. **Speak:** **Edge TTS** streams audio back; LCD shows **speaking** state; after **`tts.stop`**, a short **grace period** before returning to full listening reduces **acoustic echo** (no hardware AEC on this board).
5. **Act:** Motion and mode changes go through **`robot.*` MCP tools**; safety (obstacles, cliff, human control block) is reflected in tool responses; the face can show **moving** when the robot stack is in motion.

---

## Important MCP tools

### Common device tools (`McpServer::AddCommonTools`)

| Tool | Role |
|------|------|
| `self.get_device_status` | JSON: speaker, screen, battery/network hints, etc. |
| `self.audio_speaker.set_volume` | Volume **0–100** (call `get_device_status` first if unknown). |
| `self.screen.set_brightness` | Only registered if the board exposes a **backlight** API. |
| `self.screen.set_theme` | LVGL theme `light` / `dark` when themes are enabled. |
| `self.camera.take_photo` | Only if camera is initialized; disabled builds omit this entirely. |

### Robot tools (desk robot — `robot_app.cc`)

| Tool | Parameters / notes |
|------|---------------------|
| `robot.move` | `direction`: `forward` \| `backward` \| `left` \| `right` \| `spin_left` \| `spin_right` \| `stop`; optional `speed` **0–100**. Returns JSON with **mode**, **IR/obstacle** flags, **safety** (blocked in human control, cliff latch, backing off, etc.). |
| `robot.set_mode` | `mode`: **`ai`** \| **`human_control`** (aliases: `human`, legacy `manual`). Human control enables **Wi‑Fi manual driving**; AI voice motion may be blocked when appropriate. |
| `robot.get_status` | Compact **mode + IR + safety reason** string for the LLM. |

### Face / LCD (`FaceLcdDisplay`)

| Tool | Role |
|------|------|
| `self.face.set_playing_music` | `playing` **true/false** — toggles **music / enjoying** face while playback is treated as music. |

### User-only / system tools (when enabled in firmware)

Examples include `self.get_system_info`, `self.reboot`, `self.upgrade_firmware`, and optional screen snapshot/preview hooks behind compile flags.

---

## Face expression system

- **Visual style:** **Minimalist geometric face** (circles, arcs, lines) on a **light, high-contrast** background — **not** emoji and not pre-rendered image packs.
- **States:** **Listening**, **speaking**, **thinking / processing**, **moving**, **music / playing**, **idle**, **error / warning**.
- **Animation:** Smooth **transitions** (~200–500 ms), **double-buffered** drawing, target **30–60 FPS**; mouth can track **speaking**; **moving** can reflect motion.
- **Responsiveness:** Face updates are tied to **device state** so the UI stays informative while ASR, LLM, TTS, and MCP run.

---

## Reliability and polish (worth mentioning in Q&A)

| Topic | Detail |
|--------|--------|
| **Echo / no AEC** | PDM speaker + ADC mic: **echo guard** on the server for short ASR after TTS; firmware **delay after `tts.stop`** before aggressive listening. |
| **Dialogue health** | Gemini provider **sanitizes** incomplete assistant/tool turns after **barge-in / abort** so the model does not return empty replies. |
| **Power (battery)** | Small **1S LiPo** + Wi‑Fi + speaker peaks → **rail sag** can hurt wake word and ASR; mitigations: adequate **capacity**, **bulk capacitors**, **short/low-resistance** wiring, appropriate **regulator** (not undersized LDO on low battery). |
| **Terminology** | User-facing **“human control mode”** (replacing “manual”); MCP and voice parser accept **legacy aliases**. |

---

## Suggested live demo flow

1. Wake Jarvis → greeting → face **listening**.  
2. Short question → **thinking** → **speaking** (streaming TTS).  
3. “Move forward” / “stop” → **`robot.move`** → observe motion + **moving** face if applicable.  
4. “Switch to human control” → **`robot.set_mode`** → show **phone/browser** manual drive.  
5. Optional: music or long TTS → **`self.face.set_playing_music`** → **music** face.

---

## Closing statement

**We integrated embedded audio, WebSocket streaming AI, MCP-actuated robotics, and a real-time LVGL face into one coherent desktop assistant — Jarvis — as the capstone for ECE445 Team 6.**

---

*Generated for final presentation. Paths refer to this repo: `ECE445_group6/` (firmware), `server/xiaozhi-esp32-server/` (Mac backend).*

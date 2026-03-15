# Week 4 Engineering Notebook

**Date:** March 9 – March 15

## Objectives

- Stand up the first version of the web panel page (manual D-pad + mode toggle).
- Add latency instrumentation to the voice path and capture initial numbers.
- Wire xiaozhi MCP tool calls (`robot.move`, `robot.set_mode`, `robot.get_status`) into the `robot_core` event queue.
- Re-check Zichao's PCB schematic round 2 and confirm with Yuchen the front cliff sensor angle.

## Work Completed

I built a minimal web panel served from the device. It is a single static HTML page with a D-pad, a STOP button, a mode toggle, and a small status block. All of the logic on the page is one WebSocket connection that uses the schema I locked in Week 3. The intent is to keep this page small enough that it loads instantly on a phone browser.

I added per-stage timestamps in the voice pipeline so I can fill in real numbers for the latency budget from Week 3. Each stage appends a tagged log line, and a small Python helper on the laptop side parses the logs and computes the diffs.

For MCP integration: the xiaozhi backend now exposes `robot.move`, `robot.set_mode`, `robot.get_status`, and `robot.dance` (the last one as a placeholder that just sets the state; actual dance sequence is next). When the LLM emits a tool call, the backend forwards a `cmd` event with the same shape as a web command, so the device-side path is identical.

Discussion items with Zichao:

- Round 2 schematic: power switch and battery charging blocks were added (P-channel MOSFET for the load switch, TP4057 for charging). Confirmed firmware does not need to drive any charging logic — it is fully autonomous.
- We talked about the audio PA `PA_CTL` line. Plan is to keep PA muted during boot, and only unmute after the first state machine tick. This avoids the pop noise.

Discussion items with Yuchen:

- He printed a draft of the body. The cliff sensor slot is in roughly the right place but the front lip overhangs the IR module by ~2 mm, which could clip the field of view on the front sensor.
- We agreed to either trim the lip or move the IR module a few mm rearward in the next iteration.

## Design Notes

### Web panel (v1)

- Single HTML file embedded in firmware.
- Uses native `<input>` for the command field; D-pad buttons send `MOVE` commands with hold-to-repeat at ~10 Hz.
- Connection state visible in the status block.
- Intentionally no framework. The page is small and the device serves it directly.

### Voice path instrumentation

Stages I time:
- `t_wake`: from VAD trigger to first audio frame sent.
- `t_stream`: from first audio frame sent to last audio frame acknowledged by backend.
- `t_asr`: ASR finish.
- `t_llm`: first LLM token.
- `t_tts`: first TTS audio chunk produced.
- `t_play`: first audio sample played on device.

I am logging these as a single line per turn so I can `grep` and average.

### MCP integration

- Backend declares the tools.
- Tool call → `cmd` event over the existing WebSocket channel.
- Device-side dispatcher does not care whether a `cmd` came from the web panel or from MCP. Same `command_t`.

### Hardware notes from Zichao discussion

- Boot-time PA mute must happen before any digital toggle on `PDM_P/PDM_N` to avoid an audible click.
- `BOOT` (IO0) tactile switch is debounced in software because the PCB does not include a debounce R/C network.

### Exterior notes from Yuchen discussion

- Front lip overhang risks clipping the front cliff sensor. Decision: trim the lip first because moving the sensor would force a PCB harness re-route.
- Speaker grille on the head is sized so the speaker is audible without a resonant cavity; if we hear muffling we can enlarge the slots.

### Tradeoffs noted

- Hold-to-repeat at the panel vs. timed pulses on the device: I chose hold-to-repeat from the panel because it makes the safety stop trivial — release the button and commands stop flowing.
- Embedded HTML vs. external host: embedding wins for now; we can switch to a hosted page later if it grows.

## Diagrams and Figures

**Figure 1. Web panel v1 layout.**

Description: Sketch of the page — D-pad on the left, STOP in the center, mode toggle on the right, status row at the top. As shown in Figure 1, the page is a single screen with no navigation.

**Figure 2. MCP tool call → cmd event → state machine flow.**

Description: Shows that voice intent and a panel button take the same path through `command_t` into the state machine.

**Figure 3. Voice path timing diagram with the six stages tagged.**

Description: Annotated timeline showing where each `t_*` is captured.

## Equations and Calculations

End-to-end latency (re-stated from Week 3 with measurement labels):

\[
L_{e2e} = L_{wake} + L_{stream} + L_{ASR} + L_{LLM} + L_{TTS} + L_{play}
\]

**Equation 1. End-to-end voice latency, summed over instrumented stages.**

Initial measured values from this week's runs:
- \(L_{wake}\) = `[latency result]` ms
- \(L_{stream}\) = `[latency result]` ms
- \(L_{ASR}\) = `[latency result]` ms
- \(L_{LLM}\) = `[latency result]` ms (dominant term, as expected)
- \(L_{TTS}\) = `[latency result]` ms
- \(L_{play}\) = `[latency result]` ms

Concrete numbers will be recorded in the latency log file.

## Testing and Debugging

### Test 1: Web panel D-pad drives the dev board

**Goal:** Confirm panel buttons map to motor commands without going through xiaozhi.

**Setup:** Dev board on bench (no motors yet — loopback to logs only). Phone browser connected to the dev board's IP.

**Input Conditions:** Press and hold each D-pad button for ~2 seconds.

**Expected Result:** Device logs a `cmd` per repeat, state machine transitions to `MOVING`, and falls back to `IDLE` after release.

**Observed Result:** All four directions and STOP behave correctly. Hold-to-repeat at ~10 Hz.

**Analysis:** Path is healthy.

**Decision / Next Step:** Add a "connection lost" timeout that auto-stops if no cmd arrives for 500 ms while moving.

### Test 2: MCP `robot.move` from voice

**Goal:** Confirm a voice intent ends up as the same `cmd` shape as the web button.

**Setup:** Dev board running the new build. Backend on laptop.

**Input Conditions:** Wake word + "move forward."

**Expected Result:** Backend emits `robot.move(forward)`, device logs the resulting `cmd`, state machine transitions to `MOVING`.

**Observed Result:** Works. Logged `cmd` is byte-identical to the panel version (modulo `source`).

**Analysis:** The unified path is paying off.

**Decision / Next Step:** Keep the same shape for `robot.dance` and `robot.set_mode`.

### Test 3: Latency instrumentation collection

**Goal:** Capture an initial round of per-stage latency numbers.

**Setup:** 10 voice turns with the same short prompt.

**Input Conditions:** "Hello" wake + simple command.

**Expected Result:** Log file has one tagged line per stage per turn.

**Observed Result:** All 60 lines present. Median per-stage values recorded as `[latency result]`.

**Analysis:** LLM stage dominates. Network is small but not negligible on 2.4 GHz.

**Decision / Next Step:** Optimize buffer/chunk sizes next week and consider a smaller model for short commands.

## Problems and Proposed Solutions

**Problem:** Holding a D-pad button can leave the robot moving briefly after browser disconnect.
- **Evidence:** Killing the tab while holding "forward" left a queued command.
- **Possible cause:** No watchdog on the cmd stream.
- **Proposed solution:** 500 ms watchdog — if no `cmd` arrives during a `MOVING` state, auto-`STOP`. Will implement next week.

**Problem:** PA produces a small click on cold boot.
- **Possible cause:** `PA_CTL` toggling order at boot.
- **Proposed solution:** Hold `PA_CTL` low until after the first state-machine tick, then enable.

## Revisions

- Web panel: v1 page, single file, D-pad + STOP + mode toggle.
- Voice path: per-stage timestamps added.
- MCP: `robot.move`, `robot.set_mode`, `robot.get_status`, `robot.dance` registered.
- PCB feedback to Zichao: silkscreen for charge LED was off-center; he is fixing in round 3.
- Exterior feedback to Yuchen: trim the front lip rather than moving the IR module.

## References

[1] xiaozhi-esp32 GitHub Repository, `[link]`, accessed `[date]`.
[2] WebSocket Protocol RFC 6455, `[link]`, accessed `[date]`.
[3] TP4057 Linear Charger IC Datasheet, `[link]`, accessed `[date]`.
[4] NS4150B Class-D Audio Power Amplifier Datasheet, `[link]`, accessed `[date]`.

## Next Steps

- Add the 500 ms cmd watchdog.
- Implement boot-time PA mute and verify the click goes away.
- Begin LCD bring-up on the dev board (separate ST7789 module on a breakout).
- Capture a fuller latency sample (50+ turns) for a more honest median.
- Re-check Yuchen's body print after the front lip is trimmed.

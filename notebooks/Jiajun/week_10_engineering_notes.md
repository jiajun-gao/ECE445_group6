# Week 10 Engineering Notebook

**Date:** April 20 – April 26

## Objectives

- Build out the web panel into a fuller controller (not just D-pad).
- Continue testing the voice and MCP tool path on the assembled robot.
- Confirm `human_control` mode is predictable when switching back and forth from `ai`.
- Continue running the regression checklist before any longer demo session.

## Work Completed

The headline this week is the upgraded web controller. Up to now the panel has been a v1 D-pad with a STOP and a mode toggle. I rewrote the page to consolidate driving, mode switching, connection state, and status feedback in one screen so we can run the robot from a phone or laptop browser during bench tests and demos without relying on the voice path.

What's in the new panel:

- A connection indicator that reads "Connected" / "Disconnected" with the device IP.
- A mode pill ("Human Control" / "AI") that updates from device-pushed status messages.
- A D-pad with forward, back, left, right plus front-left and front-right corners.
- A center STOP button that's hard to miss.
- A "Switch back to AI" / "Switch to Human Control" toggle that calls `robot.set_mode`.
- The hold-to-repeat behavior is preserved — releasing a button issues `STOP`.

The panel still goes through the firmware's `human_control` path and uses the same motor and safety stack (cliff and front IR) as voice/MCP-driven motion. That means switching from voice → web → voice is just a mode change, not a code path change. This is the key reason I structured the firmware around a single `command_t` interface back in Week 2.

Voice/MCP testing on the assembled robot:

- End-to-end voice loops: wake → stream → ASR → LLM → TTS → device playback. Stable for extended runs.
- MCP tools `robot.move`, `robot.get_status`, `robot.set_mode`, `robot.dance` all dispatch correctly. I tightened the schemas so the LLM's tool calls match what `robot_core` expects byte-for-byte.
- Failure modes I have started cataloguing: tool name vs. argument mismatches, occasional ASR drops in noisy rooms, and an audio-path stability hiccup that I have not pinned down yet.

Mode switching:

- Voice → Human Control → Voice round trips behave as expected.
- During Human Control, voice intents are still ignored at the state machine (the dispatcher treats voice `cmd`s as no-ops in that mode), so an operator running the panel does not get surprised by a voice command.

Discussion items with Zichao:

- He asked whether I'd seen any unexpected current draw during dance; I had not. The PA mute and motor PWM stayed clean in the integrated case.
- We talked briefly about whether a future revision might want a board-level battery voltage ADC — useful, not critical for this project.

Discussion items with Yuchen:

- He asked about the visible state of the robot during a demo (which face is on, which mode is active, what the panel shows). We agreed the LCD face plus the panel's mode pill is enough.

## Design Notes

### Web panel v2

- Same WebSocket schema as v1 (no protocol break).
- Status messages drive the UI; the panel never polls.
- Mode toggle issues `SET_MODE` rather than toggling client-side, so the device stays the source of truth.
- Layout: connection state at the top, D-pad in the middle, mode toggle at the bottom. One screen, no scrolling.

### Tool/intent mismatch handling

- When an MCP tool call's argument shape doesn't match what `robot_core` expects, the dispatcher logs `"unknown_arg"` rather than dropping silently.
- I tightened the JSON schemas so the model emits arguments in a single canonical form.

### Hardware notes from Zichao discussion

- No new electrical issues from this week's testing.
- A board-level battery voltage ADC was discussed but not added — out of scope for this iteration.

### Exterior notes from Yuchen discussion

- LCD face + panel mode pill is sufficient for showing demo state.

### Tradeoffs noted

- Polling vs. pushed status: pushed wins because it removes wasted cycles and makes the UI feel live.
- Client-side optimistic mode toggle vs. device-as-source-of-truth: device wins. If the firmware rejects a mode change (e.g., during safety latch), the UI should reflect that, and that only works if the device tells the UI.

## Diagrams and Figures

**Figure 1. Web panel v2 layout (connection, D-pad, mode toggle, status).**

Description: As shown in Figure 1, the panel is one screen with no navigation. The mode pill is read-only — toggling it issues a `SET_MODE` command that the device decides to accept or reject.

**Figure 2. Voice → MCP → cmd dispatch path with schema-tightened arguments.**

Description: Tool argument validation is now closer to the protocol boundary so failures are observable.

**Figure 3. Mode-switch state diagram.**

Description: Shows that voice commands are dropped in `human_control`, and panel commands are accepted in both modes (panel mode toggle, panel STOP) but only drive motors in `human_control`.

## Equations and Calculations

Effective control loop period for hold-to-repeat:

\[
T_{repeat} = \frac{1}{f_{repeat}}
\]

**Equation 1. Repeat period for held D-pad commands.**

We are sending at `f_repeat ≈ 10 Hz`, which gives `T_repeat ≈ 100 ms`. The 500 ms watchdog from Week 5 covers any single dropped command without leaving the robot moving.

End-to-end voice latency target (re-stated):

\[
L_{e2e} = L_{wake} + L_{stream} + L_{ASR} + L_{LLM} + L_{TTS} + L_{play}
\]

**Equation 2. Voice latency budget.**

Per-stage values from this week's longer runs are recorded as `[latency result]`. LLM remains the dominant term.

## Testing and Debugging

### Test 1: Web panel v2 round trip on the assembled robot

**Goal:** Confirm v2 panel drives the robot identically to v1, with added mode toggle behavior.

**Setup:** Assembled robot on bench, panel on phone browser.

**Input Conditions:** Connect, drive forward/back/left/right, switch to AI, switch back, drive again.

**Expected Result:** All commands behave; mode pill updates from pushed status; STOP is immediate.

**Observed Result:** Behaves as expected.

**Analysis:** v2 is ready for demo use.

**Decision / Next Step:** Use this panel for the demo.

### Test 2: Voice and MCP tool calls in extended session

**Goal:** Surface intermittent issues over a long session.

**Setup:** Assembled robot, 30+ minutes of voice and MCP testing.

**Input Conditions:** Mixed: short commands ("stop", "forward"), longer phrases, and occasional dance triggers.

**Expected Result:** No state-machine wedges, clean recoveries from any audio glitches.

**Observed Result:** Mostly clean. Two ASR drops in a noisier moment; one "audio path hiccup" I noted but did not reproduce.

**Analysis:** Most failures are environmental. The unreproduced one needs targeted instrumentation.

**Decision / Next Step:** Add more granular logging on the audio path next week.

### Test 3: Mode-switch protection

**Goal:** Confirm voice commands are ignored in `human_control` and panel STOP works in both modes.

**Setup:** Assembled robot, panel + voice both available.

**Input Conditions:** Switch to `human_control`, send a voice command, then send panel STOP.

**Expected Result:** Voice command no-ops; panel STOP works.

**Observed Result:** Behaves as expected.

**Analysis:** Mode protection is solid.

**Decision / Next Step:** Document this in an operator checklist for the demo.

## Problems and Proposed Solutions

**Problem:** Occasional MCP tool call argument mismatch from the model.
- **Evidence:** Logs show `"unknown_arg"` warnings.
- **Possible cause:** Free-form natural language doesn't always map to one canonical argument shape.
- **Proposed solution:** Tightened schemas and a few-shot example in the system prompt for tools.

**Problem:** Unreproduced audio-path hiccup in a long session.
- **Possible cause:** Buffer underrun or websocket reconnect.
- **Proposed solution:** Add tagged logs on every audio-path state change so the next occurrence is diagnosable.

## Revisions

- Web panel v2: full controller layout, pushed status, device-as-source-of-truth mode toggle.
- MCP: argument schema tightened; `unknown_arg` warning instead of silent drop.
- Logging: per-mode-switch line includes the source.

## References

[1] xiaozhi-esp32 GitHub Repository, `[link]`, accessed `[date]`.
[2] WebSocket Protocol RFC 6455, `[link]`, accessed `[date]`.
[3] Model Context Protocol (MCP) reference, `[link]`, accessed `[date]`.
[4] ESP32-S3 Technical Reference Manual, Espressif Systems, `[link]`, accessed `[date]`.

## Next Steps

- Latency optimization pass on the voice path (chunk sizes, buffer sizes, model selection).
- Add granular audio-path logs to catch the rare hiccup.
- Lock the demo flow and rehearse it end-to-end.
- Final regression checklist run before demo recording.

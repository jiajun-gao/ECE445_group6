# Week 11 Engineering Notebook

**Date:** April 27 – May 4

## Objectives

- Run a focused latency optimization pass on the voice + MCP path.
- Lock in the dance routine and the final web panel layout for the demo.
- Run the regression checklist a final time before demo recording.
- Capture the demo and finish the engineering notebook PDF for submission.

## Work Completed

This is the final week of the senior design project. The integrated voice stack, MCP-driven behavior (including dance), web panel, and latency tuning are finalized for demo and documentation. There is no further project plan beyond this deliverable.

### Latency optimization

I instrumented and tuned each segment of the interactive loop:
- Wake → streaming capture: tightened the wake-to-stream handoff so we don't waste a frame.
- Server-side VAD/ASR: used a tighter endpoint and confirmed it did not hurt accuracy on short commands.
- LLM turnaround: this is the largest term and the least controllable. I tuned the system prompt to be terse so first-token time is faster.
- TTS → device playback: smaller initial chunk size so the first audio sample plays earlier.
- WebSocket buffering on the device side: reduced for the audio channel only.

The goal was to improve responsiveness for short commands and MCP tool turns without sacrificing stability. Latency for short commands is now noticeably better; conversational turns are still LLM-bound but have improved as well.

### Final dance routine

The dance behavior is exposed to the stack as `robot.dance`. The routine runs as a pre-programmed motion sequence on the order of tens of seconds, giving a concrete "personality" demo beyond simple directional moves. Safety locks (cliff latch) and face/UI state remain coherent throughout.

### Final web panel ("Jarvis Control Panel" layout)

The demo panel is the consolidated controller from Week 10 with a few additions for the demo:
- Robot status block (connected / disconnected, IP, current mode).
- Last-latency readout (so the audience can see the e2e number).
- Quick-action tiles: Dance, Follow, Human Mode, Stop.
- A free-text input that maps to a backend command.
- A manual-drive D-pad with a refresh tile to clear stuck state.
- A "Dance Status" + 1–10 rating panel for capturing demo feedback.

This is the panel I will use during the demo. It still uses the same WebSocket schema that the firmware has been speaking since Week 3.

### Regression checklist

Final pass:
1. Boot, panel reachable. ✓
2. All seven LCD states render. ✓
3. D-pad drives both motors. ✓
4. Voice commands → motor actions. ✓
5. Dance routine completes. ✓
6. Cliff preemption mid-dance. ✓
7. Follow mode enters/exits with adaptive threshold. ✓
8. Front + rear cliff stop. ✓
9. Battery vs. USB equivalence in fixed location. ✓
10. PA mute at boot (no click). ✓

All ten pass.

### Demo capture

Recorded the demo with the assembled robot, the new panel on a phone, and the local backend running on my Mac. The README in the repository has a link to the demo video.

Discussion items with Zichao:

- Sanity-checked the engineering notes covering the schematic and PCB sections so they describe what the assembled board actually does.
- Confirmed there is no further hardware change planned.

Discussion items with Yuchen:

- Confirmed the final exterior is the one used in the demo.
- Confirmed the final assembly photos are ready for the report.

## Design Notes

### Latency optimization tradeoffs

- Smaller buffers reduce latency but raise the chance of underruns. I A/B'd on real hardware and kept the smallest buffer that did not regress stability.
- Tighter VAD endpoint trades off against tail-cutting on slow speakers. Defaults are fine for our test phrases.
- A smaller / faster model could reduce LLM time further but at a quality cost; I left this as a future option.

### Dance vs. safety

- Cliff preemption was tested explicitly while running the final dance.
- The dance executor's table-driven design makes "abort cleanly" easy: it just jumps to a stop row.

### Hardware notes from Zichao discussion

- No further hardware change.
- The schematic-section engineering notes match the assembled board.

### Exterior notes from Yuchen discussion

- Final exterior locked.
- Assembly photos ready for the report.

### Tradeoffs noted

- Latency vs. stability: this entire week was effectively a series of these tradeoffs.
- Adding more demo features vs. polishing the existing ones: chose polish. The panel is already feature-complete.

## Diagrams and Figures

**Figure 1. Final web panel layout (Jarvis Control Panel).**

Description: Status block, command input, quick-action tiles (Dance, Follow, Human Mode, Stop), manual drive D-pad, and a Dance Status / rating panel. As shown in Figure 1, the panel is intentionally one screen for clean demos.

**Figure 2. Final voice path with optimization annotations.**

Description: Same six-stage breakdown as previous weeks, with this week's tuning labeled at each stage.

**Figure 3. Demo flow timeline.**

Description: Boot → wake-word → command → dance → safety stop → mode switch → manual drive. As shown in Figure 3, this is the ordered sequence captured in the demo video.

## Equations and Calculations

Latency relation (final):

\[
L_{e2e} = L_{wake} + L_{stream} + L_{ASR} + L_{LLM} + L_{TTS} + L_{play}
\]

**Equation 1. Final latency budget.**

Measured numbers (filled into the latency log as `[latency result]`) show that the dominant term is `L_LLM` and the rest are small.

Worst-case stop time after disconnect (carried over from Week 5):

\[
\Delta t_{stop} \le t_{watchdog} = 500\,\text{ms}
\]

**Equation 2. Disconnect → motors-stopped upper bound.**

Tested explicitly during the final regression; observed times are well under the 500 ms bound.

## Testing and Debugging

### Test 1: Latency tuning A/B

**Goal:** Confirm the tuning passes don't regress stability.

**Setup:** Final firmware with tuned buffers vs. previous build.

**Input Conditions:** Identical 20-turn voice script in both builds.

**Expected Result:** Equal or better stability, lower latency on short commands.

**Observed Result:** No regressions; `[latency result]` improvement on short commands.

**Analysis:** Tuning is keepable.

**Decision / Next Step:** Lock the build.

### Test 2: Final dance run with safety preemption

**Goal:** Make sure dance + safety still cooperates on the final assembled robot.

**Setup:** Assembled robot on a desk, dance triggered with rear near edge.

**Input Conditions:** Trigger dance; allow rear to approach the cliff while supported.

**Expected Result:** Cliff event preempts cleanly; dance does not resume.

**Observed Result:** Preempts cleanly. After recovery timeout and clear sensors, robot returns to `IDLE`, not `DANCE`.

**Analysis:** Behavior matches design.

**Decision / Next Step:** Use this in the demo.

### Test 3: Final regression checklist

**Goal:** All ten items pass before recording.

**Setup:** Assembled robot, both batteries, USB available.

**Input Conditions:** The 10-item list.

**Expected Result:** All pass.

**Observed Result:** All pass.

**Analysis:** Robot is demo-ready.

**Decision / Next Step:** Record demo.

### Test 4: End-to-end demo recording

**Goal:** Capture a clean run of all major features.

**Setup:** Robot on desk, phone browser with the final panel, backend on Mac.

**Input Conditions:** Follow the demo script (boot → voice → dance → safety stop → mode switch → manual drive).

**Expected Result:** Single take, no surprises.

**Observed Result:** Captured. Linked from the repository README.

**Analysis:** Done.

**Decision / Next Step:** Wrap up the engineering notebook.

## Problems and Proposed Solutions

**Problem:** Latency tuning is intrinsically a moving target.
- **Possible cause:** Small changes interact non-linearly across stages.
- **Proposed solution:** Document the final settings and a brief A/B procedure so a future operator can re-tune if conditions change.

**Problem:** Web UI demo footprint vs. desk safety: dance footprint must fit.
- **Evidence:** Earlier weeks' tuning.
- **Possible cause:** Dance table travel.
- **Proposed solution:** Final dance has been tuned to fit a typical desk; cliff preemption catches anything unexpected.

## Revisions

- Final voice-path tuning committed.
- Final dance routine committed.
- Final web panel layout committed.
- Engineering notes finalized.
- README updated with demo video link.

## References

[1] xiaozhi-esp32 GitHub Repository, `[link]`, accessed `[date]`.
[2] WebSocket Protocol RFC 6455, `[link]`, accessed `[date]`.
[3] Model Context Protocol (MCP) reference, `[link]`, accessed `[date]`.
[4] ESP32-S3 Technical Reference Manual, Espressif Systems, `[link]`, accessed `[date]`.
[5] ST7789 LCD Controller Datasheet, `[link]`, accessed `[date]`.
[6] TC118S H-Bridge Driver Datasheet, `[link]`, accessed `[date]`.
[7] TCRT5000 IR Reflective Sensor Datasheet, `[link]`, accessed `[date]`.

## Next Steps

This week we completed the senior design project. There is no further project plan beyond this deliverable. Possible future work, if the project were continued, would include:

- Microphone array for real direction-of-arrival in follow mode.
- Battery voltage ADC channel for runtime estimation.
- Cloud-hosted backend instead of local Mac for demo portability.
- Fleet-style automated regression harness if more than one robot were built.

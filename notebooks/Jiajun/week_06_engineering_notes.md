# Week 6 Engineering Notebook

**Date:** March 23 – March 29

## Objectives

- Implement the dance behavior sequence (timed forward / back / turn segments).
- Move LCD update to a dedicated task and confirm the SPI contention warning is gone.
- Diverge `dance` and `moving` face parameters.
- Begin follow mode skeleton.
- Confirm motor driver signal direction with Zichao now that the PCB has motor-driver footprints, and check Yuchen's next body print.

## Work Completed

I implemented the first version of the dance routine. It is a deterministic, table-driven sequence: each row is `(duration_ms, left_speed, right_speed)`, and the dance executor steps through the table while the state machine sits in `DANCE`. The sequence runs for ~30 seconds total. Importantly, the dance executor checks the safety latch on every tick — if `safety_stop` fires mid-dance, the table is abandoned cleanly.

The LCD update task is now its own FreeRTOS task pinned to the second core. The SPI contention warning is gone. As a side effect, transitions look smoother because the render task isn't fighting log bursts.

I diverged the `dance` and `moving` face parameters: `dance` now uses larger eyes, a wider mouth, and a fast eye-blink animation; `moving` is calmer.

Follow mode is skeleton-only this week. Without a camera, I'm starting with a rule-based "rotate toward last sound direction" approach, leaning on a future microphone array for direction-of-arrival. For now, the firmware just exposes `robot.set_mode(follow)` and the LCD shows the follow face; the actual rotation logic is stubbed.

Discussion items with Zichao on motor driver signals:

- Confirmed each TC118S takes two logic inputs per channel; PWM goes on one and direction on the other.
- He showed the pull-down resistors on `INA/INB` so the motors stay in High-Z if the MCU pins float at boot. That matches the firmware assumption of "pin defaults to 0 = motor stopped."
- Agreed PWM frequency target: ~20 kHz so it stays inaudible. Will fine-tune with the actual motors.

Discussion items with Yuchen on the second body print:

- Head is 3 mm shorter; proportions feel right.
- He mentioned the front face looks slightly bare with just the LCD; we agreed to keep the LCD as the only "feature" rather than adding decorative elements that compete with it.

## Design Notes

### Dance executor

- Table-driven so the choreography is data, not code.
- Rows are `(duration_ms, left_speed, right_speed)` with `speed ∈ [-1, 1]`.
- Safety preemption: each tick checks the safety latch; on latch, the executor jumps to a clean stop row and exits.
- LCD face stays on `dance` for the duration; sub-row face changes are not used.

### LCD task split

- Render runs on core 1, application/state machine on core 0.
- The two communicate via a small lock-free ring buffer of `face_event_t`.
- Logging stays on core 0 so it can't starve render anymore.

### Face parameter divergence

- `moving`: medium eye scale, neutral mouth, no blink.
- `dance`: large eye scale, wide mouth, periodic blink.
- Both parameter sets are tuned by eye on the LCD breakout, not measured.

### Follow mode (skeleton)

- Mode entry sets the LCD face and stops any pending motion.
- Behavior loop is a no-op stub that just logs "would rotate toward sound."
- The actual direction-of-arrival input will be a future hook.

### Hardware notes from Zichao discussion

- `MOTOR1_IN_A/B` and `MOTOR2_IN_A/B` are independent — the firmware can drive each side at different speeds, which is needed for differential turning.
- Pull-downs at the driver inputs make boot-time motor twitch impossible even if I forget to initialize the GPIO.

### Exterior notes from Yuchen discussion

- Head proportions agreed.
- LCD remains the single visual feature on the face.

### Tradeoffs noted

- Table-driven dance vs. behavior-tree: table wins for a 30-second canned routine. A behavior tree would matter if dances were composed on the fly.
- Rule-based follow vs. camera-based follow: camera-based was discussed but ruled out for this project — too much compute and too much mechanical change. Rule-based + sound-direction is the path.

## Diagrams and Figures

![Per-stage success rate of the voice + MCP pipeline](images/success_rate.png)

**Figure 1. Per-stage success rate measured on the dev-board sandbox after the dance, LCD-task split, and follow-mode skeleton work were in place. ASR commit, end-to-end TTS, MCP tool calls, and TTS synth all measured at 100% success across the runs (24/24, 24/24, 13/13, 24/24). The LLM time-to-first-token (TTFT) stage measured at 58% (14/24).**

This is the first quantitative look I have at how the pipeline holds up across the stages I touched. As shown in Figure 1, every stage I directly own — MCP dispatch, TTS synth, end-to-end TTS — is hitting full success. The drop on the LLM TTFT stage matches the qualitative observation from earlier weeks that LLM turnaround is the dominant and least-controllable term, and is what I'll have to attack from the other side (smaller prompts, smaller model for short commands) rather than from the firmware.

## Equations and Calculations

For the dance choreography, motor speed maps from a normalized input via the same Equation 1 from Week 2:

\[
D(s) = D_{min} + (D_{max} - D_{min}) \cdot s
\]

**Equation 1. PWM duty mapping reused for dance rows.**

PWM frequency target:

\[
f_{PWM} = \frac{1}{T_{PWM}}
\]

**Equation 2. PWM frequency — set so it is above audible range.**

We are targeting `f_PWM ≈ 20 kHz`. Final value will be tuned against the actual motors when the custom PCB is assembled.

## Testing and Debugging

### Test 1: Dance routine end-to-end (no motors yet)

**Goal:** Confirm the executor walks the table on schedule and yields cleanly on safety.

**Setup:** Dev board + LCD. Motor commands logged but not driven.

**Input Conditions:** Trigger dance via `robot.dance` and via web panel "Dance" button. Mid-dance, fire a fake cliff event.

**Expected Result:** Dance progresses through table; cliff event causes early exit through the safety stop row; LCD switches from `dance` to `safety_stop`.

**Observed Result:** Both triggers start the dance. Cliff preemption works on first try.

**Analysis:** Executor and state machine cooperate as designed.

**Decision / Next Step:** Re-test once the motors are physically connected.

### Test 2: LCD task split

**Goal:** Confirm the SPI contention warning is gone and frames don't drop during heavy logging.

**Setup:** Dev board with verbose logging enabled. LCD on dedicated task.

**Input Conditions:** Trigger 200 fast state transitions while spamming logs.

**Expected Result:** No dropped frames, no contention warning.

**Observed Result:** No contention warning. Visual frame rate stable.

**Analysis:** Two-core split is the right move.

**Decision / Next Step:** Keep this layout.

### Test 3: Follow mode entry

**Goal:** Confirm the mode switches and the LCD shows the right face.

**Setup:** Dev board + LCD.

**Input Conditions:** `robot.set_mode(follow)`.

**Expected Result:** State machine in `FOLLOW`, LCD shows follow face, behavior loop logs "would rotate toward sound."

**Observed Result:** All three.

**Analysis:** Skeleton is correct. Real behavior pending.

**Decision / Next Step:** Decide on a direction-of-arrival source — possibly a software-only beamforming hint from the existing single mic, since we don't have an array.

## Problems and Proposed Solutions

**Problem:** Dance routine occasionally feels "stuck" near the end because the last row holds zero-speed for too long.
- **Evidence:** Watching the dance with face changes; the last 2 seconds look like idle.
- **Possible cause:** Final stop row duration is too long.
- **Proposed solution:** Shorten the trailing stop row, or transition straight to `idle` instead of holding.

**Problem:** Without a camera or mic array, real follow mode is going to be approximate at best.
- **Possible cause:** Hardware limit.
- **Proposed solution:** Treat follow mode as "rotate toward where the wake word last came from" using whatever single-mic VAD energy gives us. Keep expectations modest.

## Revisions

- New `dance` executor module with safety preemption.
- LCD render moved to a dedicated task on core 1.
- Face parameters: `dance` and `moving` diverged.
- Follow mode skeleton (`robot.set_mode(follow)` + LCD face).

## Next Steps

- Receive the custom PCB and start hardware bring-up (power, USB, audio).
- Replace logged motor commands with real PWM once motors are wired.
- Pick a direction-of-arrival heuristic for follow mode (single-mic energy / VAD).
- Add per-state debug logs that include face parameters so we can correlate visual issues with logs.
- Continue PCB and exterior reviews with Zichao and Yuchen as their work converges.

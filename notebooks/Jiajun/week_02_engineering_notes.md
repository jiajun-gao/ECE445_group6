# Week 2 Engineering Notebook

**Date:** February 23 – March 1

## Objectives

- Convert the paper state machine from Week 1 into a stub firmware module that compiles and runs on a desk simulator.
- Define the command dispatch interface so both voice (xiaozhi) and the web panel can drive the same robot actions.
- Continue the pin map review with Zichao now that the PCB schematic is going out for fabrication.
- Sit in on Yuchen's first head/body sizing pass to confirm LCD opening dimensions.

## Work Completed

I implemented a first stub of the robot state machine as a C++/C module with:

- An enum for states (`IDLE`, `LISTENING`, `SPEAKING`, `MOVING`, `DANCE`, `FOLLOW`, `SAFETY_STOP`).
- A central `dispatch(event_t)` function that runs the transition table and returns the new state.
- A logging hook that prints `(prev_state, event, next_state)` for every transition.

The motivation is that the same module will run unchanged on the device and on a host, so I can write tests for transitions without hardware. This avoids burning bench time on logic bugs that are easier to hit on a laptop.

I also drafted the unified command interface that voice and web inputs both feed into:

```
command_t {
  type;        // MOVE / STOP / SET_MODE / DANCE / FOLLOW / FACE
  arg;         // direction, duration, target mode, etc.
  source;      // VOICE | WEB | INTERNAL
}
```

This is important because once xiaozhi MCP tools and the web panel buttons converge on the same shape, I do not need two parallel control paths.

Discussion items with Zichao:

- Confirmed cliff sensors will be on `IO17` (`EDGE_FRONT`) and `IO18` (`EDGE_RARE`).
- Reserved `IO5/IO6` for PDM audio after he flagged that those pins are friendly to the PA differential filter.
- Agreed motor driver inputs will live on the body PCB and connect to the head PCB through the expansion connector.

Discussion items with Yuchen:

- LCD opening is sized to a 1.69" round-corner ST7789-style panel; he is reserving a recess for the FPC tail and a 1 mm bezel.
- Front cliff sensor needs a downward-facing window in the body chassis; he is leaving a slot rather than a full hole so the IR LED is not visible in normal use.

## Design Notes

### State machine module

Transitions are stored as a flat table of `(state_in, event, guard, state_out, action)` rows. This keeps the logic readable and lets reviewers (Zichao, Yuchen) check it without reading control flow.

Guards I added so far:
- `cliff_*` events always force `SAFETY_STOP`, regardless of source.
- `DANCE`/`FOLLOW` are rejected if `safety_latched == true`.
- `STOP` from any source clears any active motion timer.

### Command dispatch

The single struct above means the web panel and the voice tool layer (`robot.move`, `robot.set_mode`, `robot.dance`) can both call into one function. I do not want to maintain two parsers.

### Hardware notes from Zichao discussion

- Cliff sensor signals are digital level (low when surface is detected on TCRT5000-style modules), so the firmware reads `EDGE_FRONT/EDGE_RARE` as plain GPIOs and debounces in software.
- Motor control: TC118S takes two logic inputs per channel. For PWM speed control, one input gets the PWM and the other is held at 0 or 1 to choose direction.

### Exterior notes from Yuchen discussion

- Head must have enough internal volume for the LCD module plus a small daughterboard.
- Body must keep cliff sensors aimed straight down at the desk surface, even with wheel suspension.

### Tradeoffs noted

- Digital vs. analog IR: starting digital because we only need a binary "cliff/no-cliff" signal. Analog would let us threshold in software but adds an ADC channel and noise filtering work.
- IR vs. ToF cliff sensor: IR wins on cost and update rate; ToF would only be worth it if we wanted a numeric height. Digital IR matches the safety logic cleanly.

## Diagrams and Figures

**Figure 1. Command dispatch flow from voice and web into the unified state machine.**

Description: Both xiaozhi MCP tool calls and the web panel WebSocket messages produce a `command_t` struct, which is enqueued and consumed by the state machine. As shown in Figure 1, only the state machine talks to the motor and LCD subsystems.

**Figure 2. Updated software state machine with explicit guards (safety latch, motion timer).**

Description: This refines Figure 2 from Week 1 by labeling each transition with its guard.

## Equations and Calculations

For initial speed scaling, motor speed command will be a linear map from a normalized input \(s \in [0,1]\) to a duty cycle:

\[
D(s) = D_{min} + (D_{max} - D_{min}) \cdot s
\]

**Equation 1. Linear PWM mapping for motor speed commands.**

Where:
- \(D_{min}\) is the minimum duty that actually moves the motor (`[measured]`).
- \(D_{max}\) is the safe upper duty (`[measured]`).
- \(s\) is the normalized speed input (0 = stop, 1 = max).

Concrete \(D_{min}\) and \(D_{max}\) will be measured once Zichao's PCB is assembled.

## Testing and Debugging

### Test 1: Host-side state machine transition test

**Goal:** Verify that the new transition table behaves the same as the paper version.

**Setup:** Compiled the state machine module on a host (no ESP32 yet) with a tiny test harness.

**Input Conditions:** Sequence of events: `wake_word, tts_start, tts_end, cmd_forward, cliff_front, cmd_stop, cliff_clear, recover_timeout, idle`.

**Expected Result:** End state is `IDLE`, with `SAFETY_STOP` visited exactly once, and dance/follow never entered.

**Observed Result:** Matches expected. Logging shows `MOVING → SAFETY_STOP` on the cliff event as intended.

**Analysis:** The safety latch and recovery timeout work as designed.

**Decision / Next Step:** Add a regression case where `dance_start` is sent while latched, and confirm it is rejected.

### Test 2: Command struct round-trip

**Goal:** Confirm that web panel JSON and a fake MCP tool call both produce the same `command_t`.

**Setup:** Hand-written JSON snippets and a fake MCP payload, both parsed in a host test.

**Input Conditions:** `{"type":"MOVE","arg":"FORWARD"}` and a synthetic MCP call equivalent.

**Expected Result:** Both produce identical `command_t` values modulo `source`.

**Observed Result:** Identical.

**Analysis:** The unified path is viable.

**Decision / Next Step:** Wire this into the device firmware once the board arrives.

## Problems and Proposed Solutions

**Problem:** First-pass transition table allowed `DANCE` to be entered while a motion timer from a prior `MOVING` was still running.
- **Evidence:** Host test showed two motor commands queued back-to-back.
- **Possible cause:** Missing guard.
- **Proposed solution:** Cancel any active motion timer on entry to `DANCE` and `FOLLOW`.
- **Tested:** Yes, on host. Passes now.

**Problem:** Cliff event source is ambiguous: front vs. rear matters when reversing.
- **Proposed solution:** Pass which sensor fired into the safety state so the recovery action can be direction-aware. Implementation deferred to next week.

## Revisions

- State machine: added `safety_latched` flag and explicit guards.
- Command interface: introduced `command_t` and `source` enum so voice and web share a path.
- Pin map: locked `IO17/IO18` for cliff sensors and `IO5/IO6` for PDM audio, per discussion with Zichao.

## References

[1] ESP32-S3 Technical Reference Manual, Espressif Systems, `[link]`, accessed `[date]`.
[2] TC118S H-Bridge Driver Datasheet, `[link]`, accessed `[date]`.
[3] TCRT5000 IR Reflective Sensor Datasheet, `[link]`, accessed `[date]`.
[4] xiaozhi-esp32 GitHub Repository, `[link]`, accessed `[date]`.

## Next Steps

- Finish the regression suite for the state machine on host.
- Stand up a minimal ESP32-S3 dev-board sandbox so the same module can be flashed and exercised over USB serial before our custom PCB returns.
- Define the JSON message schema for the web panel, since the web work will start in parallel with the firmware port.
- Re-check the pin map with Zichao after the PCB schematic round 1 review.

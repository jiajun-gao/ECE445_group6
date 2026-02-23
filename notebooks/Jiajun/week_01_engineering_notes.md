# Week 1 Engineering Notebook

**Date:** February 16 – February 22

## Objectives

- Lock down the high-level software architecture for the ESP32-S3 desktop companion robot.
- Decide how voice, web panel, sensor, and motor subsystems should be partitioned in firmware.
- Participate in early hardware discussion with Zichao on ESP32-S3 pin allocation.
- Participate in early exterior shape discussion with Yuchen so the LCD face and sensor mounting locations are reserved before the PCB is finalized.

## Work Completed

This week I focused on translating the project proposal into a concrete software architecture document so the rest of the team can plan hardware and mechanical work around it. I drafted a top-level block diagram showing the ESP32-S3 as the central controller, along with the audio in/out path, LCD face, motor driver, IR/ToF sensors, web panel over Wi-Fi, and battery rail.

I sketched a robot state machine with the states `idle`, `listening`, `speaking`, `moving`, `dance`, `follow`, and `safety_stop`. The intent is that any sensor-driven safety event can preempt all other states, and that the LCD facial expression is a function of the current state.

With Zichao, I went through a first pass on ESP32-S3 pin usage. We discussed:

- Reserving I2S/PDM pins for the microphone and speaker amplifier path.
- Reserving an SPI bank for the ST7789-style LCD.
- Reserving four GPIOs for the dual H-bridge motor driver inputs (`MOTOR1_IN_A/B`, `MOTOR2_IN_A/B`).
- Reserving two GPIOs for edge/cliff sensors (`EDGE_FRONT`, `EDGE_RARE`).
- A shared `BOOT` button on IO0 that can also serve as a runtime function key.

With Yuchen, I had a short discussion about the head/body split. The agreement was that the LCD must be in the head, ideally centered, with the microphone and speaker close enough to the LCD so a user reads them as a single "face." Final dimensions are deferred until we measure the manufactured PCB.

Why this matters: defining states and pin map this early prevents firmware rewrites later, and gives the hardware/exterior tracks a stable target.

## Design Notes

### Software architecture (initial)

- **Main control loop**: a cooperative loop on the ESP32-S3 application core. Each subsystem (audio, sensors, motor, LCD, web/WS, state machine) is a module with a `tick()` or task and exposes an event interface.
- **Voice command handling**: built on top of the `xiaozhi-esp32` framework. Wake word triggers a streaming session to a backend; the backend returns intents (or MCP tool calls) which are dispatched as robot actions.
- **Web panel command handling**: a minimal HTTP/WebSocket endpoint on the device for direct commands when the robot is in `human_control` mode.
- **State machine**: single source of truth for what the robot is "doing." LCD face and motor outputs are pure functions of the state plus per-state parameters.
- **Safety**: cliff/IR inputs raise a safety event that forces a transition to `safety_stop` regardless of current state.

### Hardware discussion with Zichao

- IR vs. ToF for cliff detection: leaning IR (TCRT5000 family) for cost and simplicity; ToF only if we need a numeric distance.
- Motor driver: TC118S-style H-bridge per wheel, controlled with two PWM-capable GPIOs each.
- Power: USB 5V plus a Li-ion battery via diode ORing; analog mic supply isolated through an RC filter.

### Exterior discussion with Yuchen

- Head houses LCD, mic, speaker.
- Body houses motor driver, motors, wheels, battery, and cliff sensors.
- Cliff sensors face downward at the front and rear edges of the body.

### Tradeoffs noted

- Voice control vs. web control: web control will likely be more reliable for early bring-up and demos; voice depends on network and wake-word tuning.
- Simple rule-based follow vs. camera-based follow: camera adds significant compute and mechanical complexity; we plan to start with a non-camera follow mode.

## Diagrams and Figures

**Figure 1. System block diagram showing ESP32-S3, motor driver, motors, LCD, microphone, speaker, IR cliff sensors, web panel over Wi-Fi, and battery.**

Description: This figure shows the ESP32-S3 in the center, with audio (mic + PA + speaker), LCD via SPI, motor driver via four PWM GPIOs, two cliff sensor GPIOs, and a Wi-Fi link to a backend server and to the web panel. As shown in Figure 1, every subsystem either sends events to or receives commands from the ESP32-S3.

**Figure 2. Software state machine with idle, listening, speaking, moving, dance, follow, and safety_stop states.**

Description: Transitions are driven by voice events, web panel commands, and sensor events. `safety_stop` can be entered from any state.

**Figure 3. LCD face rendering pipeline as a function of current state.**

Description: Per-state face assets are loaded into PSRAM and pushed over SPI; only the active region is updated each frame.

## Equations and Calculations

PWM duty cycle is the basic unit for motor speed commands:

\[
D = \frac{t_{on}}{T} \times 100\%
\]

**Equation 1. PWM duty cycle.**

Where:
- \(D\) is duty cycle in percent
- \(t_{on}\) is the time the PWM is high in seconds
- \(T\) is the PWM period in seconds

Approximate battery runtime (planning estimate):

\[
t = \frac{C}{I_{avg}}
\]

**Equation 2. Approximate battery runtime.**

Where:
- \(t\) is runtime in hours
- \(C\) is battery capacity (`[battery capacity]` Ah)
- \(I_{avg}\) is average current draw (`[measured current]` A)

Concrete numbers will be filled in once the PCB and battery are in hand.

## Testing and Debugging

No hardware was available this week, so testing was limited to a desk review of the architecture document.

### Test 1: State machine paper walkthrough

**Goal:** Confirm that every voice/web/sensor event has exactly one defined transition.

**Setup:** Architecture document plus state diagram (Figure 2).

**Input Conditions:** Hand-traced events: `wake_word`, `tts_start`, `tts_end`, `cmd_forward`, `cmd_stop`, `cliff_front_triggered`, `dance_start`, `follow_start`.

**Expected Result:** Every event lands in a defined transition or is explicitly ignored in that state.

**Observed Result:** All events covered. Two cases were initially undefined: `cliff_*` during `dance` and `dance_start` while already in `safety_stop`.

**Analysis:** Safety must always win, and `dance_start` should be rejected (not queued) while in `safety_stop`.

**Decision / Next Step:** Add an explicit "safety latch" rule: `safety_stop` can only be exited when sensors clear and a recovery timeout elapses.

## Problems and Proposed Solutions

**Problem:** Initial diagram showed dance mode and safety stop racing for motor control.
- **Evidence:** State walkthrough above.
- **Possible cause:** Missing priority rule.
- **Proposed solution:** Make `safety_stop` strictly preempt `dance` and `follow`.
- **Tested:** Not yet on hardware. To be re-validated when the device is up.

**Problem:** Pin map is still tentative; mic PDM and LCD SPI pins overlap on a draft assignment.
- **Possible cause:** ESP32-S3 strapping pins were not excluded from the candidate list.
- **Proposed solution:** Move strapping pins out of high-traffic peripheral roles in the next revision with Zichao.

## Revisions

- Architecture doc: added a "safety latch" rule to the state machine.
- Pin map (draft): flagged strapping pins for review with Zichao before PCB layout.

## References

[1] ESP32-S3 Technical Reference Manual, Espressif Systems, `[link]`, accessed `[date]`.
[2] xiaozhi-esp32 GitHub Repository, `[link]`, accessed `[date]`.
[3] ST7789 LCD Controller Datasheet, `[link]`, accessed `[date]`.
[4] TC118S H-Bridge Driver Datasheet, `[link]`, accessed `[date]`.

## Next Steps

- Finalize the pin map with Zichao before he completes the PCB schematic.
- Convert the state machine into a stub implementation (enums + transition table) so it can be reviewed in code, not just on paper.
- Confirm with Yuchen the rough head/body proportions so the LCD opening size can be reserved.
- Begin a `voice → action` mapping table for the most common commands (move, stop, dance, follow).

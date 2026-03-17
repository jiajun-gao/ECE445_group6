# Week 5 Engineering Notebook

**Date:** March 16 – March 22

## Objectives

- Bring up the ST7789 LCD on the dev board and render the first per-state face.
- Land the 500 ms command watchdog so the robot auto-stops if the panel disconnects.
- Add boot-time PA mute so cold-boot click is gone.
- Track latency more carefully (50+ turn sample) and look for cheap wins.
- Attend Zichao's PCB schematic round 3 review and Yuchen's second body print fitting.

## Work Completed

The big visible item this week is the LCD. I wired an ST7789 breakout to the dev board on the SPI pins reserved in the schematic and stood up a `lcd_face` module that renders one of `idle`, `listening`, `speaking`, `moving`, `dance`, `follow`, `safety_stop`. The face is just two eyes plus a mouth; geometry is parameterized so I can iterate without redrawing assets. The render path is driven from the state machine: every state transition emits a `face_change` event, the LCD module redraws.

I added the 500 ms command watchdog on the device. If the state machine is in `MOVING` and no `cmd` has arrived for 500 ms, it injects a synthetic `STOP`. This fixes the "held button, killed tab" failure from Week 4.

PA mute at boot is in. The PA_CTL line is held low until the state machine ticks once, and the cold-boot click is gone in casual listening. Will measure with a scope when the custom PCB returns.

I ran a 50-turn latency sample. The LLM stage is still dominant; everything else is small. I tried two cheap changes: a smaller chunk size on audio streaming and a slightly tighter VAD endpoint. Both give modest wins on the streaming and ASR stages.

Discussion items with Zichao on schematic round 3:

- Charging LED silkscreen fixed.
- He is also adding the speaker output filter caps (1 nF across `NS_PA_OUT+/-`) to suppress EMI; I confirmed the firmware does not need any change.

Discussion items with Yuchen on second body print:

- Front lip trimmed; cliff sensor field of view now clear.
- Speaker grille slot is the right size; audio sounds present and clear in a desk environment.
- Head still feels slightly tall; he will shorten it 3 mm in the next iteration.

## Design Notes

### LCD face module

- Geometry-driven: eyes (radius, position, blink phase) + mouth (shape, openness).
- One state → one parameter set. Animation is a smooth interp between sets.
- `lcd_face` consumes state events; it does not poll.
- Frame target: 30 Hz, which is more than enough for this style of face.

### Command watchdog

- Implemented as a per-state timer.
- Currently only armed in `MOVING`. `DANCE` has its own internal timing.
- Synthetic `STOP` is logged with `source = INTERNAL` so I can tell post-hoc whether a stop came from a user or from the watchdog.

### Boot-time PA mute

- `PA_CTL` is configured as output and pulled low in `app_main` before any audio block initializes.
- After the first state-machine tick, `PA_CTL` is set high, enabling audio.

### Hardware notes from Zichao discussion

- The 1 nF EMI caps across the speaker outputs are passive and have no firmware impact, but they confirm that the speaker wires were behaving like antennas.
- He also asked whether the firmware needs a battery voltage ADC channel; I said yes for runtime estimation later.

### Exterior notes from Yuchen discussion

- Head height reduced by 3 mm in the next print.
- Speaker placement is final.
- Front cliff sensor field of view confirmed clear.

### Tradeoffs noted

- Geometry-driven face vs. sprite assets: geometry wins for state-driven animation and saves PSRAM. Sprites would look richer but are more work for marginal gain at this stage.
- Watchdog only in MOVING vs. all motion-producing states: I picked MOVING-only first because DANCE owns its motion timeline. We can add coverage to FOLLOW once it lands.

## Diagrams and Figures

![Robot showing the rendered LCD face on the ST7789 display](images/face_expression.jpg)

**Figure 1. The robot on the bench with the ST7789 LCD rendering the first geometry-driven face. The two rounded rectangles on a light-blue background are the "eyes" produced by the `lcd_face` module. The body PCB power LED is visible glowing green at the bottom of the body.**

This is the first time the `lcd_face` module ran on the actual chassis and not just on a breakout. As shown in Figure 1, the face is intentionally minimal — just two eye shapes — because the geometry-driven approach lets me animate state transitions by interpolating eye size, position, and the mouth shape rather than swapping bitmaps. The framing on the head matches what Yuchen and I locked in earlier: LCD centered, with enough bezel that the screen reads as a "face" rather than a panel.

## Equations and Calculations

LCD frame update period:

\[
T_{frame} = \frac{1}{f_{frame}}
\]

**Equation 1. LCD frame period for face rendering.**

Where:
- \(T_{frame}\) is the per-frame period in seconds.
- \(f_{frame}\) is the target frame rate (we are using 30 Hz, so \(T_{frame} \approx 33\) ms).

Watchdog timing relation:

\[
\Delta t_{stop} \le t_{watchdog}
\]

**Equation 2. Worst-case time from disconnect to motor stop.**

Where:
- \(\Delta t_{stop}\) is the actual time from last received `cmd` to motors stopping.
- \(t_{watchdog}\) is the watchdog timeout (500 ms).

We will tighten `t_watchdog` once we have measured motor coast distance.

## Testing and Debugging

### Test 1: LCD face state coverage

**Goal:** Confirm every state has a visible, distinct face.

**Setup:** Dev board + LCD breakout + `robot_core`. Drive transitions over the serial console.

**Input Conditions:** Cycle through all seven states.

**Expected Result:** Each state shows a distinct face, transitions look smooth.

**Observed Result:** All seven render. `moving` and `dance` look similar; we may want to make `dance` more energetic (bigger eyes, wider mouth).

**Analysis:** Acceptable for now. Will tweak after the rest of the stack is up.

**Decision / Next Step:** Adjust `dance` parameters so it visually pops vs `moving`.

### Test 2: Command watchdog disconnect behavior

**Goal:** Confirm robot stops within 500 ms of browser disconnect.

**Setup:** Dev board + web panel on phone. Hold "forward" then kill the tab.

**Input Conditions:** Hold forward 2 s, then close browser tab.

**Expected Result:** Synthetic `STOP` logged within 500 ms of last `cmd`.

**Observed Result:** Synthetic `STOP` at `[latency result]` ms after last cmd, well under 500 ms.

**Analysis:** Watchdog works.

**Decision / Next Step:** Once motors are present, double-check that motor PWM is set to 0 within the same window.

### Test 3: Cold-boot PA click

**Goal:** Confirm the click is gone with PA mute at boot.

**Setup:** Dev board + speaker. Power on cold from USB.

**Input Conditions:** Cold boot 5 times in a row.

**Expected Result:** No audible click.

**Observed Result:** No click in 5/5 attempts. (Will scope on the custom PCB later.)

**Analysis:** Pop-noise mitigation working at the firmware level.

**Decision / Next Step:** Re-verify on custom PCB once it lands.

## Problems and Proposed Solutions

**Problem:** `moving` and `dance` faces look too similar.
- **Evidence:** Subjective from Test 1.
- **Possible cause:** Parameter sets are too close.
- **Proposed solution:** Diverge the dance parameters (eye scale, mouth motion) and re-review.

**Problem:** SPI bus contention warning during a busy log burst.
- **Evidence:** Occasional dropped LCD frames during heavy serial logging.
- **Possible cause:** Logging happens on the same task as LCD update.
- **Proposed solution:** Move LCD update to a dedicated task.

## Revisions

- LCD: `lcd_face` module added; geometry-driven faces.
- State machine: emits `face_change` on every transition.
- Networking: 500 ms command watchdog in `MOVING`.
- Audio: PA mute at boot.
- Logging: tagged `t_wake/t_stream/t_asr/t_llm/t_tts/t_play` per voice turn.

## Next Steps

- Diverge dance/moving face parameters.
- Move LCD update to a dedicated task to remove SPI contention.
- Implement the dance behavior sequence (timed forward/back/turn segments).
- Begin follow mode skeleton — start with a rule-based "approach last detected sound source" since we don't have a camera.
- Re-test on Yuchen's next body print (3 mm shorter head).

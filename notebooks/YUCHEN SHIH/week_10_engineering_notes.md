# Week 10 Engineering Notebook

**Date:** April 20 - April 26

## Objectives

- Continue system-level fitting after full hardware integration.
- Improve internal wire management inside the enclosure.
- Confirm that motion, sensing, display, microphone, and speaker can be tested without mechanical interference.
- Prepare the robot for final demonstration use.

## Work Completed

This week I focused on the transition from successful physical assembly to reliable system-level use. After the main modules fit into the enclosure, the next task was to make sure the assembled robot could actually operate without wires blocking moving parts, sensors being misaligned, or openings interfering with the interaction modules.

I inspected the internal wire routing and adjusted cable positions where necessary. The head contains the head PCB, head battery, screen, microphone, and speaker module, so cable routing in the head is especially important. The body contains the body PCB, body battery, two motors, four wheels, two sensors, rear charging access, and one axle, so the wires must stay clear of the wheel hub and center axle area. I also checked that the four snap-fit points still held the head/body interface and that the large pass-through opening did not pinch the wires between the two PCBs.

I also supported functional testing by checking whether the screen opening, microphone opening, speaker opening, front-facing sensor window, downward-facing sensor window, and rear charging opening remained correctly aligned after repeated handling. This step was important because a design that fits once may still fail during repeated testing if wires move or parts shift.

## Design Notes

### Wire management

- Keep display and head wires away from shell pinch points.
- Keep motor wires away from the wheel hub and center axle.
- Keep sensor wires from pushing the sensor modules out of alignment.
- Keep the rear charging opening clear and reachable.
- Keep the head/body pass-through opening clear for PCB-to-PCB wiring.
- Make sure the four snap-fit points do not loosen after repeated handling.
- Leave enough slack for assembly but not so much that wires move into functional areas.

### Functional alignment

- Screen must remain centered in the head opening.
- Microphone and speaker openings must remain clear.
- Front-facing sensor must point forward.
- Downward-facing sensor must point toward the desk surface.
- Rear charging opening must remain accessible from the back of the body.
- Four snap-fit connection points must remain aligned.
- Large head/body pass-through opening must remain clear for wiring.

### Demo preparation

- Exterior should look finished from normal viewing distance.
- Components should not need adjustment during a demo.
- The assembled robot should tolerate repeated power-on, handling, and movement tests.

## Diagrams and Figures

**Figure 1. Internal wire management layout.**

Description: Wires are routed around the screen, PCBs, two batteries, two motors, four wheels, sensors, axle, snap-fit interface, large PCB pass-through, and rear charging access while avoiding moving parts.

**Figure 2. Functional opening alignment check.**

Description: Screen, microphone, speaker, front sensor, downward sensor, rear charging opening, four snap-fit points, and PCB wiring pass-through remain aligned after assembly.

## Equations and Calculations

Wire slack condition:

\[
L_{wire} > L_{path}
\]

**Equation 1. Minimum wire length for safe routing.**

The wire length must exceed the routing path length, but excessive slack should be controlled to avoid interference with wheels or shell closure.

## Testing and Debugging

### Test 1: Repeated handling fit check

**Goal:** Confirm that internal parts do not shift after handling.

**Setup:** Fully assembled robot.

**Input Conditions:** Pick up, open/close, power on, and move the robot during testing.

**Expected Result:** No module shifts, no blocked openings, and no wire interference.

**Observed Result:** Assembly remained stable after wire routing adjustments.

**Analysis:** Internal routing is acceptable for demo-level use.

**Decision / Next Step:** Keep the current routing for final testing.

### Test 2: Sensor and opening alignment check

**Goal:** Confirm that the physical openings still match the modules after full integration.

**Setup:** Assembled robot with screen, microphone, speaker module, two sensors, two motors, four wheels, one axle, snap-fit interface, PCB wiring pass-through, and charging access installed.

**Expected Result:** All openings remain clear and aligned.

**Observed Result:** Openings and sensor windows remained aligned.

**Analysis:** Mechanical design supports system-level testing.

**Decision / Next Step:** Prepare final exterior and documentation photos.

## Problems and Proposed Solutions

**Problem:** Wire routing inside a compact enclosure is difficult after all modules are installed.
- **Proposed solution:** Adjust wire paths manually, keep wires away from wheel and axle areas, and preserve controlled slack.

**Problem:** Repeated handling can reveal fit issues not visible in a single assembly.
- **Proposed solution:** Re-check module alignment after repeated testing instead of only checking once.

## Revisions

- Improved internal wire management.
- Re-checked screen, microphone, speaker module, sensor, rear charging opening, snap-fit, and PCB wiring pass-through alignment.
- Confirmed wheel hub, four wheel openings, and center axle areas are not blocked by wires.

## References

[1] Integrated assembly inspection notes.  
[2] Functional test preparation notes.  
[3] Final enclosure and wiring photos.

## Next Steps

- Lock the final exterior configuration.
- Capture final assembly photos.
- Support final system-level debugging and demonstration recording.

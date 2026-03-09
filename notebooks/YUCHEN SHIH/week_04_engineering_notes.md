# Week 4 Engineering Notebook

**Date:** March 9 - March 15

## Objectives

- Develop a more detailed mechanical layout for the body.
- Improve the wheel, wheel hub, and center axle design.
- Plan how motors and wheels will connect to the body structure.

## Work Completed

This week I focused more specifically on the lower body and movement structure. The robot needs to drive on a desk surface, so the two motors, four wheels, wheel hubs, and single center axle are important mechanical features.

I refined the four-wheel placement and began shaping the body around the motor and axle area. The goal was to leave enough space for the two motors while keeping all four wheels aligned and stable. I also considered how the wheels would visually connect to the body so the robot would look intentional rather than like components attached to a shell.

The body model was updated to reserve space for the body PCB and body battery above the wheel/motor region, plus a rear charging access area. This required balancing compact size with internal clearance. If the body is too small, wiring and assembly will be difficult; if it is too large, the robot loses the compact desktop appearance.

## Design Notes

### Wheel hub and center axle

- Wheel hub geometry must align with the motor output location.
- The center axle region must leave enough clearance for rotation.
- Four wheel openings in the body should not rub against the wheels during movement.
- The structure should be strong enough for repeated assembly and testing.

### Body packaging

- Two motors occupy the lower body.
- Body PCB and body battery require protected internal volume.
- Wires need a path from the body PCB to motors, sensors, and rear charging area.

## Diagrams and Figures

**Figure 1. Wheel hub and center axle CAD detail.**

Description: The wheel interface and axle area were refined for mechanical alignment.

**Figure 2. Lower body packaging sketch.**

Description: Two motors and four wheels sit low, while body PCB, body battery, and rear charging access space is reserved above/behind them.

## Equations and Calculations

Wheel clearance:

\[
C_{wheel} = R_{opening} - R_{wheel}
\]

**Equation 1. Wheel clearance in body opening.**

Where \(R_{opening}\) is the body wheel opening radius and \(R_{wheel}\) is the wheel radius.

## Testing and Debugging

### Test 1: Wheel opening CAD check

**Goal:** Check that the wheel can rotate without body interference.

**Setup:** Fusion 360 model with wheel and body geometry.

**Expected Result:** Wheel has clearance around the body opening.

**Observed Result:** Clearance is acceptable in CAD, but physical print tolerance must be verified later.

**Decision / Next Step:** Keep extra clearance for the first printed version.

## Problems and Proposed Solutions

**Problem:** Compact body size conflicts with motor and wheel space.
- **Proposed solution:** Keep the lower body slightly wider and reserve clearance around moving parts.

**Problem:** CAD clearance may not match 3D print tolerance.
- **Proposed solution:** Avoid tight wheel openings until the first printed test.

## Revisions

- Refined four-wheel placement.
- Improved wheel hub and single center axle design.
- Updated body space planning for two motors, body PCB, body battery, rear charging access, and wires.

## References

[1] Fusion 360 wheel/body revision notes.  
[2] Motor and wheel physical measurements.

## Next Steps

- Continue refining sensor locations.
- Add more exact openings for the screen, microphone, and speaker.
- Prepare for PCB-based dimensional update.

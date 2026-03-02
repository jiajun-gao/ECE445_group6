# Week 3 Engineering Notebook

**Date:** March 2 - March 8

## Objectives

- Improve the first Fusion 360 exterior model.
- Refine the robot's visual proportions and make the head/body relationship more natural.
- Begin planning the positions of the screen, microphone, speaker, and sensors.

## Work Completed

This week I focused on making the robot shape more complete. The first CAD model proved that the head/body idea was feasible, so I refined the proportions and tried to make the exterior look more like a finished robot rather than a box around electronics.

I adjusted the head size so the screen could be centered and used as the face. I also kept space around the screen for future microphone and speaker openings. The goal was to make the user-facing side simple and readable.

On the body, I continued refining the wheel hub and center axle design. The four wheels must be placed low enough for stability, while the body still needs space for the body PCB, body battery, two motors, two sensors, wiring, and rear charging access. I also marked early positions for the front-facing sensor and downward-facing sensor.

## Design Notes

### Exterior proportion

- The head should be large enough for the screen but not so large that the robot becomes top-heavy.
- The body should visually support the head and contain the wheel system.
- Rounded edges were preferred because they make the robot look less like a raw electronics enclosure.

### Openings and sensor planning

- Screen opening should be centered on the head.
- Microphone opening should be close to the screen area.
- Speaker opening should be visible but not dominate the face.
- Front-facing body sensor should point forward for obstacle detection.
- Downward-facing body sensor should point toward the desk for edge detection.
- Rear body opening should be reserved for charging access.

## Diagrams and Figures

**Figure 1. Refined robot exterior in Fusion 360.**

Description: Head and body proportions were adjusted to create a more complete physical form.

**Figure 2. Early opening placement sketch.**

Description: Planned locations for screen, microphone, speaker, front sensor, and downward sensor.

## Equations and Calculations

Head-to-body height ratio:

\[
R_h = \frac{H_{head}}{H_{body}}
\]

**Equation 1. Proportion check for visual balance.**

The ratio is used qualitatively to keep the head expressive without making the robot unstable or visually top-heavy.

## Testing and Debugging

### Test 1: CAD proportion review

**Goal:** Evaluate whether the robot looks balanced and can still contain hardware.

**Setup:** Fusion 360 model with rough component blockouts.

**Expected Result:** The robot should look compact, friendly, and mechanically feasible.

**Observed Result:** The design direction is feasible, but detailed opening locations still need refinement.

**Decision / Next Step:** Continue refining component openings and wait for real head/body PCB measurements.

## Problems and Proposed Solutions

**Problem:** The head must contain multiple interaction features without looking crowded.
- **Proposed solution:** Make the screen the main feature and keep microphone/speaker openings simple.

**Problem:** Sensor placement must support function and exterior appearance.
- **Proposed solution:** Reserve sensor windows now, then adjust once real components are installed.

## Revisions

- Refined head/body proportions.
- Continued wheel hub and center axle design for the two-motor, four-wheel layout.
- Added early screen, microphone, speaker, body sensor, and rear charging placement plans.

## References

[1] Fusion 360 design revision notes.  
[2] Team discussion on screen-based robot face.

## Next Steps

- Continue improving the CAD model.
- Prepare for PCB-based measurement once the head PCB and body PCB arrive.
- Start thinking about the first physical print and assembly order.

# Week 2 Engineering Notebook

**Date:** February 23 - March 1

## Objectives

- Start the first Fusion 360 model for the robot exterior.
- Create rough head and body volumes before exact head/body PCB dimensions are available.
- Reserve early space for screen, head/body PCBs, head/body batteries, two motors, four wheels, two body sensors, axle, and internal wiring.

## Work Completed

This week I began translating the appearance idea into Fusion 360. I created the first rough head and body volumes to test whether the robot could have a compact desktop form while still containing the expected hardware.

For the head, I reserved a front area for the screen because the display is the most important visual interaction component. I also marked possible locations for the microphone and speaker. At this stage the openings were not final, but reserving the interaction area early helped prevent the head from becoming too small.

For the body, I created a rough internal space for the body PCB, body battery, two motors, four wheels, two sensors, and rear charging access. I also began sketching the wheel hub and center axle region. The wheel system must fit mechanically with the body while still allowing the robot to move smoothly, so I treated this as part of the body design rather than an afterthought.

## Design Notes

### Fusion 360 model setup

- Separate head and body components in CAD.
- Use simple rounded geometry first, then refine details later.
- Keep enough internal volume for real electronics and wiring.

### Reserved component areas

- Screen area on the front of the head.
- Microphone and speaker openings near the screen.
- Two motor positions and four wheel positions on the lower body.
- Two body sensor regions for obstacle detection and desk-edge detection.
- Rear charging access area on the body.
- Center axle and wheel hub geometry for wheel mounting.

## Diagrams and Figures

**Figure 1. First Fusion 360 head/body model.**

Description: Basic exterior volumes were created before final dimensions were available.

**Figure 2. Rough wheel hub and axle concept.**

Description: The wheel hub and center axle are reserved as part of the lower body structure.

## Equations and Calculations

Approximate body width planning:

\[
W_{body} \ge W_{PCB} + 2C_{side}
\]

**Equation 1. Minimum body width estimate.**

Where \(W_{PCB}\) is the expected PCB width and \(C_{side}\) is side clearance for shell thickness, wiring, and assembly tolerance.

## Testing and Debugging

### Test 1: Rough volume check

**Goal:** Check whether the robot form can contain the expected hardware.

**Setup:** Fusion 360 model with estimated component blockouts.

**Expected Result:** Head and body volumes should contain major components without obvious overlap.

**Observed Result:** The rough model can fit the expected parts, but exact head/body PCB dimensions are still needed.

**Decision / Next Step:** Keep the model flexible until the real head/body PCBs arrive.

## Problems and Proposed Solutions

**Problem:** Exact head/body PCB sizes were not available yet.
- **Proposed solution:** Use estimated blockouts and avoid designing tight mounting features too early.

**Problem:** Wheel hub and axle space compete with internal body space.
- **Proposed solution:** Reserve wheel geometry early and adjust PCB/battery placement around it.

## Revisions

- Created the first Fusion 360 exterior model.
- Added rough head, body, wheel hub, and axle regions.
- Reserved early locations for screen, microphone, speaker, head/body PCBs, head/body batteries, two motors, four wheels, two sensors, and rear charging access.

## References

[1] Fusion 360 CAD model revision notes.  
[2] Estimated module dimensions from team hardware plan.

## Next Steps

- Refine the exterior proportions.
- Add more accurate openings for screen, microphone, and speaker.
- Update the CAD model once real head/body PCB dimensions are measured.

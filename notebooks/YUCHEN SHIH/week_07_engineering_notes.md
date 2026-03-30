# Week 7 Engineering Notebook

**Date:** March 30 - April 5

## Objectives

- Measure the actual head PCB and body PCB dimensions after arrival.
- Use the head/body PCB dimensions to guide the robot's structural and spatial design.
- Create the initial PCB-based robot exterior design in Fusion 360.
- Reserve space for wheels, motors, front-facing sensor, and downward-facing sensor.

## Work Completed

This week we focused on the mechanical design and overall physical layout of the robot after receiving the PCBs. I measured the head PCB and body PCB carefully and used their physical dimensions as the foundation for planning the robot's external structure and internal component arrangement.

Using the PCBs as fixed references, I began planning how the head PCB would fit in the head and how the body PCB would be housed inside the body. This was important because the enclosure must match the actual board sizes, not just estimated dimensions, to make final assembly possible.

I recorded these early measurements and hand sketches in a separate PDF notebook page set: [mechanical_measurements_and_sketches.pdf](references/mechanical_measurements_and_sketches.pdf). These notes include PCB measurements, screen placement, speaker size, head/body battery layout, motor layout, sensor positions, wheel/axis planning, rear charging access, and early opening sketches.

I continued the robot exterior design in Fusion 360. The design focused on two main structural parts: the head and the body. At this stage, the goal was to establish the general physical form while also ensuring that the design could realistically accommodate the internal hardware.

I reserved locations for two motors, four wheels, one center axle, the front-facing sensor, the downward-facing sensor, and the rear charging opening. The front-facing sensor supports obstacle detection, while the downward-facing sensor monitors the desk surface and helps prevent the robot from falling from the edge. I also kept the four snap-fit head/body connection points and the large pass-through opening so the head PCB and body PCB could be wired together.

## Design Notes

### PCB-based layout

- Head PCB and body PCB dimensions became the main references for head/body size.
- Internal component placement was planned around the actual boards.
- The enclosure must support both mechanical fit and electrical connection access.

### Fusion 360 exterior design

- Head and body were treated as the main structural modules.
- The head supports interaction features.
- The body supports the body PCB, body battery, two motors, four wheels, two sensors, rear charging access, and wiring.

### Reserved hardware positions

- Two motors and four wheels are placed low in the body for movement and stability.
- Front-facing sensor points forward for obstacle detection.
- Downward-facing sensor points toward the desk for edge detection.
- Rear charging opening is reserved on the back of the body.
- Four snap-fit points connect the head and body, with a larger pass-through opening for PCB-to-PCB wiring.

## Diagrams and Figures

**Figure 1. PCB measurement reference diagram.**

Description: Actual head/body PCB length, width, connector positions, and clearance areas were used to guide the head and body design.

**Figure 2. PCB-based Fusion 360 exterior model.**

Description: Head and body structure were modeled around the real head/body PCB dimensions.

**Figure 3. Sensor and motor reserved positions.**

Description: Two motors, four wheels, front-facing sensor, downward-facing sensor, one center axle, rear charging access, four snap-fit points, and the PCB wiring pass-through are placed in the layout.

## Equations and Calculations

Body internal width:

\[
W_{internal} = W_{PCB} + 2C_{clearance}
\]

**Equation 1. Internal body width based on PCB size.**

Where \(W_{PCB}\) is the measured PCB width and \(C_{clearance}\) is side clearance for assembly and wiring.

## Testing and Debugging

### Test 1: PCB dimension measurement

**Goal:** Use real head/body PCB dimensions instead of estimated dimensions.

**Setup:** Head PCB, body PCB, ruler/caliper, Fusion 360 model.

**Input Conditions:** Measure PCB outlines and compare with head/body CAD space.

**Expected Result:** The head and body can be updated to fit the real PCBs.

**Observed Result:** PCB dimensions provided a clear reference for updating the head and body model.

**Analysis:** Using the actual PCBs reduced uncertainty in the enclosure design.

**Decision / Next Step:** Continue refining the CAD model around the measured PCBs.

## Problems and Proposed Solutions

**Problem:** The internal layout must balance compact size with enough space for all hardware.
- **Proposed solution:** Use the head PCB and body PCB as fixed references and reserve clearance for the other components around them.

**Problem:** Sensor locations must support both appearance and function.
- **Proposed solution:** Place the front-facing sensor and downward-facing sensor according to their detection directions, then refine through physical fitting.

## Revisions

- Measured the actual head PCB and body PCB after arrival.
- Updated the design approach to use PCB dimensions as the structural foundation.
- Created the PCB-based exterior layout in Fusion 360.
- Reserved space for two motors, four wheels, one center axle, front-facing sensor, downward-facing sensor, rear charging opening, four snap-fit head/body points, and one large PCB wiring pass-through.

## References

[1] PCB physical measurement notes.  
[2] Fusion 360 model revision, Week 3/30.  
[3] Team weekly progress log, Week 3/30.  
[4] [Mechanical measurements and sketches PDF](references/mechanical_measurements_and_sketches.pdf).

## Next Steps

- Refine the Fusion 360 model for better structural integration.
- Confirm detailed placement and mounting method for each component.
- Prepare for initial physical fitting and hardware integration.

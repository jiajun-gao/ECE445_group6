# Week 5 Engineering Notebook

**Date:** March 16 - March 22

## Objectives

- Refine the interaction openings in the robot head.
- Plan screen, microphone, and speaker placement more carefully.
- Continue adjusting sensor positions for obstacle and desk-edge detection.

## Work Completed

This week I focused on the robot head and interaction openings. Since the head is the part users see first, the openings need to support function while also keeping the exterior clean.

I refined the screen opening so the display can sit centered and become the robot's face. I also planned the microphone opening near the front of the head so it can receive voice input without being blocked by the shell. For the speaker module in the head, I worked on a grille/opening layout that allows sound to leave the enclosure while still looking integrated with the design.

I also started planning the head/body mechanical interface. The head and body need to connect securely, so I reserved four snap-fit connection points. In the center of the interface, I reserved one larger pass-through opening so wires could connect the head PCB and body PCB without being pinched.

I also reviewed the two body sensor opening positions again. The front-facing sensor is used for obstacle detection, while the downward-facing sensor supports desk-edge detection. Their positions must match the robot's movement direction and must not be blocked by the body shape or wheel area. I also kept the rear body area available for the charging opening so the robot could be charged without changing the main front appearance.

## Design Notes

### Head openings

- Screen opening should be centered and clean.
- Microphone opening should be small but not blocked.
- Speaker module and speaker opening should allow audio output without making the head look messy.
- The head should still look like one unified face area.

### Head/body interface

- Four snap-fit connection points hold the head and body together.
- One large pass-through opening is reserved between the head and body for PCB-to-PCB wiring.
- The pass-through opening must be large enough for wires but should not weaken the structure.

### Sensor function

- Front-facing sensor: obstacle detection.
- Downward-facing sensor: distance/edge detection to prevent falling from the desk.
- Sensor windows need functional alignment, not just visual placement.
- Rear charging opening: reserved on the back of the body for power access.

## Diagrams and Figures

**Figure 1. Head opening layout.**

Description: Screen, microphone, and speaker openings were arranged around the interaction area.

**Figure 2. Sensor placement plan.**

Description: The front-facing sensor points forward, and the downward-facing sensor points toward the desk surface.

**Figure 3. Head/body snap-fit and wiring pass-through plan.**

Description: Four snap-fit points connect the head and body, with one larger central opening for head PCB to body PCB wiring.

## Equations and Calculations

Opening margin:

\[
M_{opening} = D_{opening} - D_{module}
\]

**Equation 1. Margin between module and printed opening.**

The opening must be large enough for function and assembly but not so large that the robot looks unfinished.

## Testing and Debugging

### Test 1: Opening placement review

**Goal:** Check that the screen, mic, and speaker openings make sense visually and functionally.

**Setup:** Fusion 360 head model with component locations.

**Expected Result:** Openings align with component locations and preserve a clean exterior.

**Observed Result:** The layout is feasible, but physical assembly will be needed to confirm the exact fit.

**Decision / Next Step:** Keep the openings adjustable for later versions.

## Problems and Proposed Solutions

**Problem:** Too many openings on the head can make the face look crowded.
- **Proposed solution:** Make the screen the dominant feature and keep mic/speaker openings simple.

**Problem:** Sensor windows need accurate alignment with real modules.
- **Proposed solution:** Reserve sensor positions now and refine after physical fitting.

## Revisions

- Refined screen opening.
- Added microphone and speaker opening plans.
- Improved front-facing and downward-facing body sensor placement.
- Reserved rear body charging opening.
- Added four snap-fit connection points and a large head/body wiring pass-through opening.

## References

[1] Fusion 360 head design revision notes.  
[2] Screen, microphone, speaker, and sensor module dimensions.

## Next Steps

- Prepare the model for physical fit testing.
- Update dimensions once the real head/body PCBs arrive.
- Continue refining the body around two motors, four wheels, one axle, two sensors, and rear charging access.

# Week 8 Engineering Notebook

**Date:** April 6 - April 12

## Objectives

- Test initial two-motor and body PCB placement inside the robot body.
- Refine the head design by adding openings for the screen, microphone, and speaker.
- Perform trial assembly and identify fit problems.
- Modify multiple CAD versions to improve dimensional accuracy and hardware compatibility.

## Work Completed

This week we focused on initial hardware fitting, physical assembly validation, and iterative refinement of the robot structure. After completing the basic exterior design in Fusion 360 last week, I began testing whether the major internal components could be properly placed inside the designed robot structure.

I attempted to place the body PCB and two motors into the designed robot body to check whether the reserved spaces were accurate enough for real hardware integration. This helped identify assembly constraints such as component clearance, wire routing space, charging access, and the fit between electronic components and the enclosure.

I also refined the robot head design by adding openings for the screen, microphone, and speaker module. These openings are important because the head supports the robot's interaction functions: visual display, voice input, and audio output.

After placing the body PCB, two motors, four wheels, one axle, and interaction openings into the design, I performed an initial trial assembly. The purpose was to check whether the components fit properly inside the body and head, and whether the dimensions were accurate enough for physical assembly. Based on the fitting results, I created and modified multiple design versions to make the dimensions and operating logic more compatible.

The hand sketches and dimension notes in [mechanical_measurements_and_sketches.pdf](references/mechanical_measurements_and_sketches.pdf) were used during this process. They document the screen-centered head opening, microphone/speaker openings, two body sensor locations, motor/body clearance, rear charging area, wheel/axis layout, four snap-fit connection points, and the large pass-through opening needed for wiring between the head PCB and body PCB.

## Design Notes

### Initial motor and PCB placement

- Body PCB must fit inside the body without forcing the shell.
- Two motors must align with the wheel hub, four wheels, and center axle design.
- Wire routing space must be reserved around the body PCB, body battery, motors, sensors, and rear charging opening.

### Head interaction openings

- Screen opening supports the visual face.
- Microphone opening supports voice input.
- Speaker module and speaker opening support audio output.
- Openings must be positioned accurately for both function and appearance.

### Head/body connection

- Four snap-fit points were used so the head could attach to the body without a loose fit.
- A large central pass-through opening was reserved so wires can connect the head PCB and body PCB.
- The pass-through opening also makes assembly easier because the wiring does not need to be forced through a small hole.

### Iterative modification

- Body dimensions were adjusted for body PCB, two motors, four wheels, one axle, rear charging access, and wiring.
- Head dimensions were adjusted around screen, mic, and speaker openings.
- Multiple versions were compared to improve physical fit.

## Diagrams and Figures

**Figure 1. PCB and motor placement test.**

Description: The body PCB and two motors were placed inside the designed body to check real fit.

**Figure 2. Head opening design.**

Description: Screen, microphone, and speaker openings were added to the head.

**Figure 3. Trial assembly comparison across versions.**

Description: Multiple CAD versions were compared to improve dimensional accuracy.

**Figure 4. Snap-fit and PCB wiring pass-through detail.**

Description: Four snap-fit connection points secure the head to the body, and a larger opening allows wiring between the two PCBs.

## Equations and Calculations

Assembly tolerance:

\[
C_{fit} = C_{designed} - C_{used}
\]

**Equation 1. Remaining fit clearance after component placement.**

Where \(C_{designed}\) is the designed internal space and \(C_{used}\) is the space occupied by the real component and wiring.

## Testing and Debugging

### Test 1: Initial PCB and motor placement

**Goal:** Check whether the body PCB and two motors fit inside the body.

**Setup:** Designed body, body PCB, two motors, four wheels, axle, and wheel structure.

**Input Conditions:** Place body PCB and motors into the body space.

**Expected Result:** Components fit with enough clearance for wires.

**Observed Result:** Fit was possible, but some spaces required adjustment.

**Analysis:** Real components and wires require more clearance than CAD blockouts.

**Decision / Next Step:** Modify the body model and continue trial assembly.

### Test 2: Head opening placement

**Goal:** Confirm screen, microphone, and speaker openings can support interaction modules.

**Setup:** Head CAD model with component positions.

**Expected Result:** Openings align with modules and maintain clean appearance.

**Observed Result:** Openings were feasible but required dimensional refinement.

**Decision / Next Step:** Continue adjusting openings based on physical fit.

## Problems and Proposed Solutions

**Problem:** Body PCB, two motors, four wheels, axle, rear charging access, and wiring must fit in a compact enclosure.
- **Proposed solution:** Increase internal clearance where needed and avoid overly tight mounting geometry.

**Problem:** Screen, microphone, and speaker openings must satisfy both function and appearance.
- **Proposed solution:** Keep the screen as the main visual feature and refine mic/speaker openings through trial fitting.

## Revisions

- Tested initial body PCB placement inside the body.
- Checked two-motor, four-wheel, and axle fit in reserved positions.
- Added screen, microphone, and speaker module openings to the head.
- Added four snap-fit connection points and a large PCB wiring pass-through between head and body.
- Modified multiple design versions to improve dimensional accuracy and hardware compatibility.

## References

[1] Team weekly progress log, Week 4/6.  
[2] Fusion 360 iteration notes.  
[3] Physical fitting notes for body PCB, motors, screen, microphone, and speaker.  
[4] [Mechanical measurements and sketches PDF](references/mechanical_measurements_and_sketches.pdf).

## Next Steps

- Continue refining the CAD model based on trial assembly.
- Improve internal spacing for wires and hardware connections.
- Prepare for a more complete physical assembly of the robot.

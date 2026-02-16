# Week 1 Engineering Notebook

**Date:** February 16 - February 22

## Objectives

- Define the initial appearance direction for the interactive desktop companion robot.
- Decide the basic mechanical concept: head, body, wheels, and internal support structure.
- Connect the exterior design to the robot's intended interaction functions.

## Work Completed

This week I started the mechanical design by thinking about what the robot should look like before detailed CAD work. Since the project is an interactive desktop companion robot, the exterior should look friendly, compact, and stable on a desk. I decided that the robot should have a clear "face" area in the head and a lower body that contains the movement system.

The first design idea was to separate the robot into a head and body. The head would support interaction components such as the screen, microphone, speaker module, a head PCB, and a head battery. The body would support the body PCB, body battery, two motors, four wheels, two sensors, rear charging access, and the center axle/wheel structure. This split makes the robot easier to understand visually: the head handles user interaction, and the body handles movement, power support, sensing, and charging access.

I also began thinking about the wheel structure. The robot needs to be able to move on a desk surface, so the wheel placement must keep the body stable while leaving enough internal space for motors and wiring. I noted that the wheel hub and center axle area would need to be designed carefully because the wheel system affects both motion and enclosure size.

## Design Notes

### Appearance inspiration

- The robot should look like a small desktop companion rather than a bare hardware prototype.
- A simple head/body silhouette makes the robot easier to recognize.
- The screen should become the main facial expression area.
- The body should be compact but wide enough to support stable wheel placement.

### Initial mechanical concept

- Head: screen, microphone, speaker module/opening, head PCB, head battery, and expression area.
- Body: body PCB, body battery, two motors, four wheels, two sensors, rear charging opening, and wiring.
- Head/body interface: four snap-fit connection points plus a large pass-through opening for wiring between the head PCB and body PCB.
- Wheel hub and center axle: designed as the mechanical interface between motion hardware and exterior shell.

## Diagrams and Figures

**Figure 1. Initial exterior concept sketch.**

Description: A friendly robot form with a screen-based head and compact body.

**Figure 2. Head/body functional split.**

Description: The head is assigned to interaction modules, while the body is assigned to movement and control hardware.

## Equations and Calculations

Basic clearance planning:

\[
C_{total} = C_{component} + C_{assembly}
\]

**Equation 1. Required component clearance.**

Where \(C_{component}\) is the measured component size and \(C_{assembly}\) is extra space for tolerance, wiring, and assembly.

## Testing and Debugging

Testing this week was conceptual. I compared possible robot shapes against the project goals: desk use, mobility, interaction, and internal hardware space.

## Problems and Proposed Solutions

**Problem:** The robot needs to look compact but still fit many internal parts.
- **Proposed solution:** Use a head/body split so interaction parts and motion parts do not compete for the same space.

**Problem:** Wheel placement affects both stability and body size.
- **Proposed solution:** Reserve wheel and axle space early before finalizing the body shape.

## Revisions

- Chose the head/body exterior direction.
- Identified the screen as the main face.
- Identified wheel hub and center axle design as important mechanical tasks.

## References

[1] ECE 445 project proposal notes.  
[2] Team discussion on interactive desktop robot function.

## Next Steps

- Begin rough Fusion 360 modeling of the head and body.
- Estimate sizes for screen, head/body PCBs, head/body batteries, two motors, four wheels, one axle, and two body sensors.
- Continue developing the exterior style around the robot's interaction function.

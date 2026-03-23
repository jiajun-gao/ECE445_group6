# Week 6 Engineering Notebook

**Date:** March 23 - March 29

## Objectives

- Prepare the CAD model for the next stage of real hardware fitting.
- Review all reserved component spaces before PCB-based redesign.
- Identify likely assembly risks before the first full fitting attempt.

## Work Completed

This week I reviewed the complete mechanical model before using the actual PCB dimensions as the dimensional reference. I checked the head, body, wheel hub, center axle, two-motor region, four-wheel layout, two body sensor openings, and rear charging area together rather than as separate features.

The main goal was to make sure the design could be adjusted quickly after measuring the real head PCB and body PCB. I avoided finalizing tight mounting features because the real board dimensions and connector positions may differ slightly from earlier assumptions.

I also reviewed the internal layout for assembly logic. The head PCB, body PCB, two batteries, two motors, four wheels, two body sensors, screen, microphone, speaker, rear charging access, and wiring all need to be installed in an order that does not trap wires or block access. This week helped me identify that wire routing and connector clearance would likely become major issues during physical assembly.

## Design Notes

### Assembly risk review

- Head PCB and body PCB dimensions must become the foundation for the final head/body layout.
- Two motors, four wheels, and the center axle need enough clearance for both installation and movement.
- Wires must have paths between head and body.
- Openings for screen, microphone, speaker, two body sensors, and rear charging access must align with real components.

### CAD strategy

- Keep parametric dimensions easy to modify.
- Avoid overly tight internal clips before real fitting.
- Use the next PCB measurement step to lock the head and body sizes more accurately.

## Diagrams and Figures

**Figure 1. Full CAD assembly review.**

Description: Head, body, wheel hub, axle, two motors, four wheels, two body sensors, rear charging access, and internal electronics are checked together.

**Figure 2. Expected assembly order.**

Description: Body PCB and motors first, then sensors, wiring, batteries, screen, mic, speaker, and head PCB.

## Equations and Calculations

Assembly clearance estimate:

\[
C_{assembly} = C_{print} + C_{wire} + C_{tool}
\]

**Equation 1. Extra clearance needed for real assembly.**

Clearance must include print tolerance, wire thickness, and room for placing components by hand.

## Testing and Debugging

### Test 1: Pre-fitting CAD review

**Goal:** Identify risky areas before physical assembly.

**Setup:** Complete Fusion 360 model with estimated component positions.

**Expected Result:** Major components have reserved space and can be adjusted after real measurement.

**Observed Result:** The model is ready for PCB-based dimension refinement.

**Decision / Next Step:** Measure the head PCB and body PCB after arrival and rebuild the internal layout around them.

## Problems and Proposed Solutions

**Problem:** The real PCBs may change internal layout assumptions.
- **Proposed solution:** Use the head PCB and body PCB as fixed references once they arrive.

**Problem:** Wiring space is difficult to estimate in CAD.
- **Proposed solution:** Leave extra internal clearance and refine through trial assembly.

## Revisions

- Reviewed full mechanical model.
- Identified wire routing and connector clearance as key risks.
- Prepared the CAD model for PCB-based updates.

## References

[1] Fusion 360 full assembly review notes.  
[2] Estimated electrical module dimensions from team plan.

## Next Steps

- Measure the head PCB and body PCB after they arrive.
- Use PCB dimensions to guide structural design.
- Begin physical fitting with head/body PCBs, two motors, four wheels, two sensors, axle, and body.

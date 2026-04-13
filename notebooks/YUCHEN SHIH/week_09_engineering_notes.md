# Week 9 Engineering Notebook

**Date:** April 13 - April 19

## Objectives

- Validate the final physical fit of the 3D-printed robot enclosure.
- Integrate all major hardware modules into their designed positions.
- Power on the assembled system and check basic electrical integrity.

## Work Completed

This week we focused on validating the final physical fit of the 3D-printed robot enclosure and testing integration of all major hardware modules. After several rounds of design refinement, the overall dimensions and printed exterior successfully met the design requirements. The final enclosure size was very close to the intended measurements, and all modules were able to fit into their corresponding positions.

I tested the printed robot exterior and confirmed that the overall size, shape, and internal spacing were suitable for the hardware layout. This confirmed that the Fusion 360 design could be translated into a physical enclosure that supports the actual hardware components.

I placed all major modules into their designed locations: head PCB, body PCB, head battery, body battery, front-facing sensor, downward-facing sensor, screen in the head, microphone, speaker module, two motors, four wheels, one axle, and rear charging access. I also checked that the four snap-fit head/body connection points could align and that the large pass-through opening gave enough space for the wiring between the two PCBs. This confirmed that the mechanical design, internal layout, and component placement were aligned with the actual hardware dimensions.

After the modules were placed, we powered on the system to verify that the major modules could receive power correctly. We also checked soldering quality and circuit integrity to make sure the electrical connections were complete and stable before moving into full functional testing.

The physical assembly was checked against the earlier measurement/sketch PDF. This was useful because the PDF preserved intermediate decisions such as where the screen should be centered, where openings should be removed or enlarged, how the body height/thickness changed, and how the final holes and connection features should be placed.

## Design Notes

### Final enclosure validation

- Printed size matched the intended CAD dimensions closely.
- Head and body both provided enough internal space for assigned modules.
- The wheel hub and center axle area supported the two-motor, four-wheel body layout.

### Complete module placement

- Head PCB and body PCB fit inside their planned regions.
- Head battery and body battery fit in the reserved spaces.
- Screen, microphone, and speaker module aligned with head openings.
- Front-facing and downward-facing body sensors aligned with their windows.
- Rear charging opening remained accessible from the body back side.
- Four snap-fit points aligned between the head and body.
- Large pass-through opening allowed head PCB/body PCB wiring to route through the interface.

### Electrical integrity check

- Power was applied after physical placement.
- Modules were checked for power.
- Soldering and circuit connections were inspected before full system testing.

## Diagrams and Figures

**Figure 1. Final 3D-printed enclosure validation.**

Description: The printed head and body match the intended physical dimensions and support assembly.

**Figure 2. Complete module placement.**

Description: Head PCB, body PCB, two batteries, two sensors, screen, microphone, speaker module, two motors, four wheels, one axle, snap-fit interface, PCB wiring pass-through, and charging access are installed in their positions.

**Figure 3. Power and circuit integrity check.**

Description: The integrated system is powered on and inspected for stable electrical connections.

## Equations and Calculations

Final dimensional error:

\[
E_{dim} = |D_{printed} - D_{CAD}|
\]

**Equation 1. Difference between printed dimension and CAD dimension.**

Small dimensional error confirms that the 3D-printed enclosure is close enough to the Fusion 360 model for assembly.

## Testing and Debugging

### Test 1: Final enclosure fit

**Goal:** Confirm that the 3D-printed exterior meets the design requirements.

**Setup:** Printed head and body with all major hardware modules.

**Input Conditions:** Place hardware into the enclosure.

**Expected Result:** All modules fit in their corresponding positions.

**Observed Result:** All major modules fit smoothly.

**Analysis:** The final enclosure dimensions are accurate enough for assembly.

**Decision / Next Step:** Use this enclosure for integrated system testing.

### Test 2: Power supply and circuit integrity check

**Goal:** Confirm that modules receive power after integration.

**Setup:** Assembled robot with head/body PCBs, head/body batteries, two sensors, screen, microphone, speaker module, two motors, four wheels, one axle, snap-fit interface, PCB wiring pass-through, and rear charging access.

**Input Conditions:** Power on the system and inspect electrical connections.

**Expected Result:** Modules receive power and connections appear stable.

**Observed Result:** Modules powered correctly, and soldering/circuit integrity looked acceptable.

**Analysis:** The assembly is ready for functional testing.

**Decision / Next Step:** Continue testing communication, motion, sensing, display, microphone, and speaker functions.

## Problems and Proposed Solutions

**Problem:** All modules needed to fit in a compact enclosure with limited internal space.
- **Proposed solution:** Use multiple CAD iterations and physical trial assembly to converge on the final dimensions.

**Problem:** Wiring and soldering had to remain stable after placement.
- **Proposed solution:** Inspect connections carefully after installation and before functional testing.

## Revisions

- Validated the final 3D-printed enclosure.
- Installed head PCB, body PCB, head battery, body battery, sensors, screen, microphone, speaker module, two motors, four wheels, one axle, four snap-fit points, PCB wiring pass-through, and rear charging access.
- Powered on the system and checked soldering/circuit integrity.

## References

[1] Team weekly progress log, Week 4/13.  
[2] Final Fusion 360 model.  
[3] Final 3D-printed enclosure inspection notes.  
[4] [Mechanical measurements and sketches PDF](references/mechanical_measurements_and_sketches.pdf).

## Next Steps

- Continue testing each module after full physical integration.
- Verify communication between PCBs, sensors, and interaction modules.
- Test motion, sensing, display, microphone, and speaker functions as a complete system.
- Improve wire management inside the enclosure if necessary.

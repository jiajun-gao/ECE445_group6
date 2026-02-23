# Week 2 Engineering Notebook

**Date:** February 23 – March 1

---

## Objectives

- Resolve SDIO/SPI/LCD strap-pin conflicts against the ESP32-S3-WROOM module.
- Finish rev-A power tree: \(\mathrm{V_{BAT}}\) protection, charge path, \(\mathrm{V_{MOTOR}}\) vs \(\mathrm{V_{SYS}}\) separation.
- Select motor bridge IC/package and define **sense / fault** hookups.
- Export **Gerber Round 1** package for design review (**target Feb 25**—internal milestone); complete **DRC**, not fabrication yet.

---

## Work Completed

### My work

I closed the SDIO conflict from Week 1 by moving LCD SPI away from overlapping module pins per WROOM pinning table—Jiajun re-ran his pin spreadsheet and matched.

Power tree (**rev A**):

- Fuse + ideal diode OR (**placeholder symbol**) on \(\mathrm{V_{BAT}}\) input.
- **TP4056-class** lithium charge module footprint on body board edge (USB micro or Type-C)—team decision: Type-C receptacle purely for charging (no USB-PD stack on rev A to save risk).
- **AOZ part or similar buck** \(\rightarrow\) \(\mathrm{V_{SYS}} = 5\,\mathrm{V}\) hypothesis for motors + PA headroom; \(\mathrm{V_{BAT}}\) divider + ADC sense to MCU for SOC UI (divider high-impedance, verified leakage).

Motor driver narrowed to dual H-bridge with **PWM to INx** mode and \(\mathrm{V_\mathrm{motor,max}} \geq [battery\_max]\). Added **bulk cap** \(\mathrm{C_\mathrm{motor}}\) at bridge supply pins and \(\mathrm{V_\mathrm{motor}}\) pi-filter before bridge.

Exported **Gerber zip** labeled Round 1 (2026-02-25) for advisor review—not fabbed yet; still missing final speaker filter and ferrite BOM.

### Team discussions

- **Jiajun:** BOOT/IO0 test point access; ROM log UART route to test pad on body board edge.  
- **Yuchen:** Type-C centered on sternum for docked charging clears base stand by \([clearance\_mm]\)—adjust keep-out layer.

---

## Design Notes

### Hardware Design

- **PCB architecture:** Connector pinout draft for face↔body: **30-mil drills acceptable** on 2-layer if count limited; prioritized **GND pins interleaved with fast signals**.
- **Power design:** Star **PGND** at battery negative post; flood under motor driver with **thermal relief** suppressed on FET drains per vendor layout note.
- **Signal routing:** I2S lines matched within \(\pm [tolerance]\) (placement target)\*; discrete series resistors on SPI CLK/MOSI for EMI margin.
- **Sensor interface:** ESD TVS diodes (\(D_{\mathrm{TVS}}\)) on external sensor harness pads (budget allowing).
- **Motor control:** `nSLEEP`, `MODE` pins biased with explicit resistors—not floating.

\*Target set in CAD constraints; verified post-layout Week 6.

### Tradeoffs

- **Buck at 5 V vs direct battery to motors:** 5 V regulated gives predictable speed; battery-direct is higher efficiency but speed droops with \(\mathrm{V_{BAT}}\)—**rev A: regulated \(\mathrm{V_\mathrm{motor}} = V_\mathrm{SYS}\)** for simplicity.
- **2-layer vs 4-layer:** Cost/shorter fab time vs solid GND reference—**rev A stays 2-layer** with aggressive placement.

### System Integration

- ADC battery sense scaling: confirm **\(R_{top}/R_{bot}\)** so max \(\mathrm{V_{BAT}}\) never exceeds GPIO ADC range with tolerance.

---

## Diagrams and Figures

**Figure 1. Power tree (rev A)**  
Description: \(\mathrm{V_{BAT}}\) → fuse → charger → node; branch to buck/LDO; motor rail tap; BATT\_SENSE divider to ADC.

**Figure 2. Gerber output checklist screenshot (placeholder)**  
Description: Layers, drills, aperture, outline—annotated approvals.

---

## Equations and Calculations

**Battery divider to ADC:**

\[
V_\mathrm{ADC} = V_\mathrm{BAT}\,\frac{R_2}{R_1+R_2}
\]

Design for \(V_\mathrm{BAT,max}=[4.2]\,\mathrm{V}\Rightarrow V_\mathrm{ADC}\le [3.3]\cdot 0.9\,\mathrm{V}\) (10% MCU margin):

\[
\frac{R_1}{R_2} \ge \frac{V_\mathrm{BAT,max}}{V_\mathrm{ADC,target}}-1
\]

**Motor rail bulk cap (rule-of-thumb energy balance):**

\[
\frac{1}{2} C \Delta V^2 \approx \frac{1}{2} L_\mathrm{wire} I^2
\]

Use \([I]\) as worst-case step; pick \(C \ge [C_\mathrm{min}]\) after layout寄生 \(L\) estimated.

---

## Testing and Debugging

### Test 1: Buck board eval (TI/ALPS module on bench)

**Goal:** Measure ripple on \(\mathrm{V_{SYS}}\) with **motor load simulated** by power resistor steps.  
**Setup:** E-load or switched resistors; scope on \(\mathrm{V_{SYS}}\) at DC jack vs at load connection.  
**Input Conditions:** \(\mathrm{V_{IN}}=[3.7\text{–}4.2]\,\mathrm{V}\); \(I_\mathrm{out}=[0.2,0.6,1.0]\,\mathrm{A}\) steps.  
**Expected Result:** Ripple \(< [V_\mathrm{ripple,max}]\) mV pp at CC-CM.  
**Observed Result:** \([measured]\) mV pp; overshoot at step \([measured]\).  
**Analysis:** Informs output cap count and whether **LC pi filter** on motor branch is mandatory.  
**Decision / Next Step:** Add second **10 µF X5R** at buck output in schematic; review DCR of inductor.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Type-C CC pins left NC risks some chargers not sourcing 5 V | Advisor note | No Rp/Rd on CC | Add **5.1 kΩ** Rd on CC1/CC2 for UFP | Symbol updated; DRC clean |
| Motor cap placement creates void under flood | DRC density | Overly large via stitch | Relocate bulk to edge of FET island | *Layout debt Week 4* |

---

## Revisions

- **Schematic:** Motor driver symbol migrated from **part A** to **part B** after stock check.  
- **Requirements:** USB Type-C charging adopted (was micro-USB).

---

## References

- Espressif, *ESP32-S3-WROOM-1* module pinout.  
- Motor driver **[IC part number]** datasheet (layout checklist).  
- USB Type-C Specification (CC pull-down values for sunk device).

---

## Next Steps

- Complete speaker EMI filter footprints (parallel caps across PA outputs).  
- Run thermal estimates on \(\mathrm{V_{SYS}}\) inductor \(\mathrm{I_{RMS}}\).  
- Start board placement sketches for EMI separation review.

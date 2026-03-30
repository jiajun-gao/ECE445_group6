# Week 7 Engineering Notebook

**Date:** March 30 – April 5

---

## Objectives

- Receive **fabricated panels** (face + body); depanelize and sand mouse-bite stubs per mechanical note.
- Assemble **passive-first** (decouplers, terminates, ferrites) \(\rightarrow\) **visual + multimeter shorts** checklist before silicon.
- Solder ESP32‑S3 module, motor bridge, connectors; avoid **cold joints** on high-current pours.
- First **cold power-on**: current-limited \(\mathrm{V_{BAT}}\) only \(\rightarrow\) no firmware flash yet.

---

## Work Completed

### My work

**Incoming QC:** Checked annular rings, ENIG on FPC pads, soldermask webbing between two **0.5 mm** pitch pads—**acceptable**; no shorts on bare board E-test report from fab.

**Assembly order:** passives \(\rightarrow\) buck \(\rightarrow\) LDO \(\rightarrow\) USB Type-C passives \(\rightarrow\) motor driver \(\rightarrow\) module last (minimize reflow cycles on module). Used **low-temp profile** on hot air for accidental bridge on **0603 divider** \(\rightarrow\) cleaned.

Continuity checks (DMM diode mode where applicable):

- BATT+ to BATT−: **open** \(\rightarrow\) no dead short across input cap.
- \(V_{\mathrm{SYS}}\) to GND: **momentary beep** \(\rightarrow\) *expected* from output cap charge when probing both ways—logged.

**Smoke test \(\#\)1:** Bench \(\mathrm{V_{BAT}}=\) \[3.95\] V, CC \(\approx\) \[0.35\] A. Charger LED behavior **ambiguous** \(\rightarrow\) found **\(R_{\mathrm{LED}}\)** misplaced **\*one pad off\*** \(\rightarrow\) reworked \(\rightarrow\) `CHG` now toggles visibly when jumpering USB \(5\text{ V}\) *lab supply proxy* while battery disconnected per procedure.

MCU **not flashing yet** \(\rightarrow\) no digital activity aside from stray **XO** harmonic on scope **[small] mV** on **RTC** net—no latch-up sighted.

Motors **not connected** \(\rightarrow\) driver **INx** floated per checklist *(later learned: should bias disarmed state—tracked Week 9).*

### Team discussions

- **Jiajun:** Ready **UART-only** bootloader session once **3V3 rails** \(\pm\) **[tolerance]** confirmed—deferred flashing until divider fix fully validated across two boards.
- **Yuchen:** FPC folded radius **\{[r\_min]\}** mm \(\rightarrow\) I added **tape strain relief prototype** \(\rightarrow\) not a SCH change yet.

---

## Design Notes

### Hardware Design

- **PCB architecture:** Separate **bring-up jig** plywood with **GND banana** stitched to PSU negative for repeatable scope plane.
- **Power design:** Inrush \(I_{\mathrm{BATT}}\) peak **[meas]** \(\rightarrow\) \(\tau\) aligns with \(V_{\mathrm{BUS}}\) input cap slew—no UVLO chatter.

### Tradeoffs

- **Assemble face before body interconnect:** eased mic access test but delayed full **B2B** mating stress—chosen for **risk ordering** \(\rightarrow\) B2B mated Week 8.

### System Integration

- Type-C charger path **standalone verifiable** \(\rightarrow\) separates **MCU bring-up risks** vs **motor risks**.

---

## Diagrams and Figures

**Figure 1. Bare board QC photo grid (placeholder)**  
Description: Highlight FPC pads, motor island stitching, antenna keep-out clearance.

**Figure 2. Assembly sequence card (rev A)**  
Description: Ordered steps with checkpoints.

---

## Equations and Calculations

**Inrush current (cap charging, simple RC):**

\[
I_{\mathrm{peak}}\approx\frac{V_{\mathrm{BAT}}}{R_{\mathrm{series}}+ESR_{\mathrm{eq}}}
\]

Bench \(R_{\mathrm{series}}\) chosen to limit \(I_{\mathrm{peak}}\) to **[target] A** plateau.

---

## Testing and Debugging

### Test 1: Multimeter diode drop across batt TVS nets

**Goal:** Orientation verify **TVS** not reversed \(\rightarrow\) would clamp rail low.  
**Setup:** FLUKE diode mode BATT+ to BATT− \(\rightarrow\) read across protection branch.  
**Input Conditions:** Unpowered.  
**Expected Result:** Series chain consistent with datasheet forward threshold at **protection branch only** probing intended nodes.  
**Observed Result:** Readings plausible **[Vx]**—no smoking gun.  
**Analysis:** Non-trivial to interpret in-circuit—**trust E-test** + visual orientation dot.  
**Decision / Next Step:** Mark board **OK** proceed; log photo of **TVS dot** alignment.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Charger LED faint / off | Wrong pad | Assembly slip | Moved \(R_{\mathrm{LED}}\) \(\rightarrow\) correct | Visual OK |
| Bridge driver INx floated | Idle concern | Incomplete checklist bias | Tie disarmed \(\rightarrow\) **Week 9** add pulldowns ECO mentally | Captured |

---

## Revisions

- **Assembly doc:** Charger LED orientation photo added \(\rightarrow\) `docs/assy_revA_notes` *placeholder*.
- No PCB spin \(\rightarrow\) ECO backlog for \(\mathrm{V_{motor}}\) bias resistors \(\rightarrow\) **Week 11** if needed.

---

## References

- Module reflow guideline (Espressif).  
- USB Type-C receptacle datasheet (GND tab soldering).  

---

## Next Steps

- Mate face↔body; verify **GND loop** impedance \(\rightarrow\) no ground lift.
- Controlled **3V3 ripple** \(\rightarrow\) FFT at **PDM MIC** VDDA \(\rightarrow\) *Week 8*.
- Prep motor harness with polarity **locking clip**.

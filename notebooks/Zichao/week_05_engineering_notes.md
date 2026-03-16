# Week 5 Engineering Notebook

**Date:** March 16 – March 22

---

## Objectives

- **Schematic Round 3** review: charger indicator, passive networks, BOM scrub.
- Correlate speaker **HF bypass / differential caps** placement with EMI concern from Jiajun’s demo setup.
- Lock **motor current limit** resistor (if programmable) pending updated stall spreadsheet.
- Finalize fabrication pack **versioning** (“Round A fab” vs “review-only Round 1”).

---

## Work Completed

### My work

Round 3 edits:

- **Charging LED** silkscreen polarity and label `CHG` flipped to consistent **K-to-GND** direction per assembly feedback.
- Across \(\mathrm{PA_\mathrm{OUT}+/-}\): **\(1\,\mathrm{nF}\)** C0G parallel as discussed with Jiajun—confirmed **no firmware change** required.

Motor driver programmable \(I_{\mathrm{TRIP}}\) (if populated): provisional **\[R_{\mathrm{ILIM}}\]** sets \(I_{\mathrm{trip}}\approx [I_{\mathrm{value}}]\,\mathrm{A}\) from datasheet equation:

\[
I_{\mathrm{trip}}=\frac{V_{\mathrm{REF}}}{R_{\mathrm{SENSE}}}
\quad\text{or}\quad
f(R_{\mathrm{prog}})
\]

*BOM still lists alternate sense resistor DNP.*

Generated **fabrication readme**: layer stackup, impedance note “N/A 2-layer low-speed\", ENIG finish, IPC 600 Class 2.

### Team discussions

- **Jiajun:** Verified **cold-boot click mitigation** stays in firmware; agrees scope capture on \(V_{\mathrm{BAT}}\) **dip** during Wi‑Fi associate is useful for my power margin story.  
- **Yuchen:** Second printed body clearance for cliff sensor \(\rightarrow\) PCB **chin keep-out unchanged**—mechanical sanding only.

---

## Design Notes

### Hardware Design

- **PCB architecture:** Charger status LED resistor sized for **\[2\] mA** target at \(V_{\mathrm{f}} \approx [2.1]\,\mathrm{V}\).  
- **Power design:** **TVS \(\mathrm{V_{BAT}}\)** clamp selection check: \(\mathrm{V_{clamp}} < [motor\_driver.absmax_{\mathrm{V}}]\) but \(> nominal pack max.  
- **Signal routing:** SPI series resistors **27 Ω** settled (was 22–33 range).  
- **Sensor interface:** Pulldown on `nFAULT` if open-drain (weak **47 kΩ**).  
- **Motor control:** **Deadtime** internal to driver—document “no external shoot-through network”.

### Tradeoffs

- **ENIG vs HASL:** ENIG for fine-pitch FPC pads on face board; cost acceptable for low qty.  
- **Programmable current limit vs fuse-only:** limiter adds cost but saves **brushed arc wear**—kept.

### System Integration

- Safety: if driver `nFAULT` exists, OR with software policy—Jiajun to **latch stop** in firmware on fault edge.

---

## Diagrams and Figures

**Figure 1. Round 3 schematic diff (charger LED section)**  
Description: Before/after silk + diode orientation.

**Figure 2. PA output filter network**  
Description: Differential cap across speaker nodes with return path highlighted.

---

## Equations and Calculations

**Charging LED resistor:**

\[
R_{\mathrm{LED}}=\frac{V_{\mathrm{BUS}}-V_{\mathrm{f}}}{I_{\mathrm{f}}}
\]

Use \(V_{\mathrm{BUS}}\approx 5.0\,\mathrm{V}\) from USB LDO path, \(I_{\mathrm{f}}=2\,\mathrm{mA}\).

**Motor power (average, placeholder):**

\[
P_{\mathrm{motor}} \approx D \cdot I_{\mathrm{mean}} \cdot V_{\mathrm{SYS}}
\]

with duty \(D=[D]\), \(I_{\mathrm{mean}}=[I]\).

---

## Testing and Debugging

### Test 1: PA harmonics with/without 1 nF (eval board + clip-on)

**Goal:** Qualify radiated difference **near LCD SPI** bus.  
**Setup:** Near-field probe on SPI CLK line; PA playing **1 kHz** sine (Jiajun test bin).  
**Input Conditions:** Cap **out** vs **in** across speaker diff.  
**Expected Result:** Few dB reduction in **\[harmonic band\]** hash on SPI aggressor.  
**Observed Result:** **\[ΔdB\]** reduction at **\[freq\]**—*subjective; log photo in lab book*.  
**Analysis:** Acceptable trade; watch **high treble** roll-off with acoustic mic later.  
**Decision / Next Step:** Keep **1 nF** on rev fab; earmark acoustic A/B once PCB lands.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Wrong LED footprint pad width for 0603 rotated | BOM mismatch | Legacy footprint | Swap to rotated 0603 land pattern | Resolved pre-fab |

---

## Revisions

- **Schematic/silk:** Charging LED polarity + label.  
- **BOM:** SPI series Rs unified to **27 Ω**.

---

## References

- PA IC datasheet (typical application with **ZOBEL** commentary).  
- Motor driver datasheet (current limit program).  
- IPC-7351 footprint guidelines (LED).

---

## Next Steps

- Submit PCB order + stencil after final **DFM bot** check from fab.  
- Order passive kit + driver IC reels.  
- Draft bring-up checklist (rails sequence).

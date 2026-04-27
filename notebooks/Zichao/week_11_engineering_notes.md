# Week 11 Engineering Notebook

**Date:** April 27 – May 3

---

## Objectives

- Complete **hardware-facing pre-demo verification**: power rails, cliff and front IR behavior, motor polarity, harness strain relief, flex mate integrity.
- Freeze **ASSEMBLY_REV_A_PLUS** so **golden** and **backup** match on **INx pulldowns**, **\(V_{\mathrm{SYS}}\) bulk**, and **shell** trim revision.
- Decide with the advisor whether **PCB revision B** is justified before the demo (default: **no spin**; **ECO backlog** only).
- List **residual hardware risks** (\(V_{\mathrm{BAT}}\) droop, glossy surfaces, simultaneous charge + motion) with **operational mitigations**.

---

## Work Completed

### My work

**Golden build checks**

- Start-of-demo \(V_{\mathrm{BAT}}\): **[4.00–4.15] V**. Reconstructed voltage from **BATT_ADC** vs handheld DMM at **TP_BAT**: error **\< [err] %**.
- \(V_{\mathrm{SYS}}\) at **TP_5V0** under voice streaming + short motor bursts: ripple **\< [mV_budget] mV** pp (spring ground, **20 MHz** BW limit).
- Cliff + front IR scripted runs: **10 / 10** **pass** → `safety_stop` within **\[latency] ms** by stopwatch (LA capture optional).
- **Backup** unit had **\(M_\mathrm{R}\)** **A/B** swapped at housing → **corrected** harness → updated row in **`HARNESS_TABLE_rev2`** (spreadsheet placeholder).
- Piggy **\(10\,\mu\mathrm{F}\)** on \(\mathrm{V_{SYS}}\) clears internal rib after Yuchen’s trim → vertical clearance **≤ [Z_budget] mm**.

**Revision B triage (advisor)**

- **Frozen:** no second fab before demo.
- **ECO backlog:** clearer **`EDGE_*`** silk; populate **47 kΩ INx** pulls at fab; optional **LCD \(V_{DD}\)** ferrite; **\(V_{\mathrm{motor}}\)** fuse nail down to Week 9 **[I_stall]** numbers.

### Team discussions

- **Jiajun:** Safety latch cannot be cleared implicitly by mode changes → matches our **enable / default-off** intent from early pin planning.
- **Yuchen:** **Matte white** shell improves cliff margin vs glossy laminate → supports planned demo table choice.

---

## Design Notes

### Hardware Design

- **Two-board**: B2B **GND** stitching appears adequate — no subjective **SPI return** “buzz” when flex is exercised **[N]** cycles by hand.

### Tradeoffs

- **No rev B** → schedule win; we accept **manual** harness documentation and **prototype** variance.

### System Integration

- Single **\(V_{\mathrm{BAT}}\)** node feeds **radio + audio + motion** → **SOC** readout needs firmware filtering (ADC **±[x] %** quantization/wander observed in quick logs).

---

## Diagrams and Figures

**Figure 1. Demo hardware checklist matrix (placeholder)**  
Description: Rows for rail, safety, polarity, thermal columns with pass/fail.

**Figure 2. Harness correction (before/after)**  
Description: Photo of **\(M_\mathrm{R}\)** connector keying.

---

## Equations and Calculations

Aggregate power (rough):

\[
P_{\mathrm{tot}} \approx P_{\mathrm{ESP+radio}} + P_{\mathrm{audio}} + P_{\mathrm{LCD}} + \langle P_{\mathrm{motor}}\rangle
\]

Demo runtime order-of-magnitude:

\[
t \approx \frac{C_{\mathrm{Ah}}\,\eta_{\mathrm{conv}}}{\langle I_{\mathrm{BATT}}\rangle}
\]

Post-fill \(\langle I_{\mathrm{BATT}}\rangle\) from a scripted **\[duration] min** demo log.

---

## Testing and Debugging

### Test 1: USB charge + motor pulses (thermal)

**Goal:** Bound \(\Delta T\) on **bridge** and **cell** when **5 V** charge is present **and** motors pulse periodically.  
**Setup:** Thermocouples; start **\[SOC] %**; run **\[minutes]** script (voice idle + **\[D] %** bursts every **\[Δt] s**).  
**Input Conditions:** Ambient **\[T_a] °C**, no fan.  
**Expected Result:** \(\Delta T_\mathrm{bridge}\), \(\Delta T_\mathrm{pack}\) **\< [ΔT_cap] °C** above ambient; no foldback smell.  
**Observed Result:** **\[values] °C** → **pass** under **\[qualifier]**.  
**Analysis:** Informal only — **not** full charger FET SOA study.  
**Decision / Next Step:** Ops note: **avoid high-duty dance while charging**.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Backup wrong spin direction | Video | Harness mirror | Rework `M_R` pair | OK |
| Glossy desk eats margin | Week 10 data | Optics | Baffle / table policy | Documented |

---

## Revisions

- **ECO document** (no fab): `hw/eco_revB_candidates` *path TBD*.  
- **Build standard** ASSEMBLY_REV_A_PLUS dated **\[date]**.

---

## References

- Espressif ESP32-S3 hardware design guidelines.  
- Motor driver fault / retry behavior.  
- IR module distance vs reflectance.

---

## Next Steps

- After demo: quote **rev B** if funds allow.  
- Ten-point **\(V_{\mathrm{BAT}}\)** vs ADC calibration log for appendix.  
- Upload final scope PNG set to **`hw/measurements/demo_week/`** (placeholder folder).

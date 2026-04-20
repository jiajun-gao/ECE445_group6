# Week 10 Engineering Notebook

**Date:** April 20 – April 26

---

## Objectives

- Quantify **cliff sensor trip distance** \(d_{\mathrm{trip}}\) on **matte vs glossy** desk surfaces using Yuchen’s **final shell aperture**.
- Compare **\(V_{\mathrm{SYS}}\)** ripple after Jiajun enables **PWM slew** to the Week 8 capture (Wi‑Fi associate + motor transients).
- Reconcile **golden build** vs **backup unit** (extra \(V_{\mathrm{SYS}}\) bulk, INx pulldown rework).
- Log **bridge package temperature** \(\Delta T_{\mathrm{pak}}\) after **\[t_{\mathrm{dance}}]** seconds of scripted dance at moderate duty.

---

## Work Completed

### My work

**Cliff characterization:** Rolled the assembled robot over height blocks toward a desk edge. **Matte** laminate: mean \(d_{\mathrm{trip}}\) **\[d_matte]** mm (\(N=[\cdot]\) trials). **Glossy** sample: mean **\[d_gloss]** mm \(\rightarrow\) **\[Δd]** mm **closer** to the edge \(\rightarrow\) explains one “scary” office desk during dry run. Firmware debounce (Jiajun) stays the primary guard; **hardware** change deferred to **mechanical baffle** refinement.

**PWM slew vs ripple:** With ramp time **\[t_ramp]** ms, peak-to-peak \(V_{\mathrm{SYS}}\) during direction reversals dropped **\[pct]** % vs instantaneous step (**spring ground**, same probe point as Week 8). Confirms Week 9 risk: **thin current margin** is easier to live with when **di/dt** is controlled in software.

**Ground path check:** DMM **200 Ω** scale from **MCU digital GND test pad** to **pack negative** \(\rightarrow\) **\[R_G] Ω** \(\rightarrow\) plausible for prototype harness length. Visually confirmed **motor return** avoids **long detours** through face flex region.

**Thermal:** K-type thermocouple on bridge tab \(\rightarrow\) after **\[t_{\mathrm{dance}}]** s routine, \(\Delta T_{\mathrm{pak}}=[\cdot]\) **°C** \(\rightarrow\) below **\[limit] °C** budget.

### Team discussions

- **Yuchen:** Chin **lip trim** **\[x] mm** \(\rightarrow\) narrower spread in \(d_{\mathrm{trip}}\) across three desks; photos with ruler for appendix (placeholder).
- **Jiajun:** Web panel “jitter” \(\not\propto\) correlated \(V_{\mathrm{SYS}}\) dip in two short captures \(\rightarrow\) Wi‑Fi vs motor **coexistence** marked **low suspicion** on **power** side \(\rightarrow\) debug stays in **network/UI** if it recurs.

---

## Design Notes

### Hardware Design

- **Sensor optics:** Specular paths on glossy desks \(\rightarrow\) \(\theta_\mathrm{incidence}\) sensitivity \(\rightarrow\) mechanical **baffle height** trades off **field of view** \(\rightarrow\) coordinated with Yuchen’s CAD.

### Tradeoffs

- **Stronger IR emitter current** \(\rightarrow\) saturation / crosstalk risk \(\rightarrow\) avoided; **optical + debounce** preferred for rev A.

### System Integration

- **Dance RMS current** \(\times\) **voice + Wi‑Fi peaks** \(\rightarrow\) combined **\(V_{\mathrm{BAT}}\)** droop informs **SOC warning** threshold in UI \(\rightarrow\) joint team decision (not a schematic change this week).

---

## Diagrams and Figures

**Figure 1. Cliff trip distance — matte vs glossy (placeholder)**  
Description: Simple bar chart of mean \(\pm\) \(\sigma\) over **N** trials each.

**Figure 2. \(V_{\mathrm{SYS}}\) ripple — slew on vs off**  
Description: Annotated scope captures, same vertical scale.

---

## Equations and Calculations

First-order bridge loss estimate:

\[
P_{\mathrm{loss}} \approx 2\,R_{\mathrm{DS(on)}}\,\langle I\rangle^{2}
\]

(use tabulated \(R_{\mathrm{DS(on)}}\) at elevated \(T_\mathrm{J}\)).

PDM vs PWM **birdie** heuristic: check integer relationships \(|m f_{\mathrm{PWM}} - n f_{\mathrm{PDM,clk}}|\) against **\[audio BW]\)** \(\rightarrow\) FFT capture still on the **to-do** list for final report.

---

## Testing and Debugging

### Test 1: IR vs LCD brightness (static)

**Goal:** See if **LCD drive** injects enough **conducted/ near-field** coupling to corrupt **obstacle IR** quiescent voltage.  
**Setup:** Motors off; LCD brightness steps **0 / 50 / 100%**; scope on IR receiver output / GPIO net (where accessible).  
**Input Conditions:** \(\mathrm{V_{BAT}} \approx [full]\,\mathrm{V}\).  
**Expected Result:** Quiescent baseline shift **\< [ΔV_\mathrm{budget}]\,\mathrm{mV}** or **debounce swamps** it.  
**Observed Result:** Narrow spur at **\[f_spur] Hz**, **\[amplitude] mV** \(\rightarrow\) firmware sample cadence still dominant.  
**Analysis:** No mandatory **ferrite-on-LCD \(V_{DD}\)** change for Week 10.  
**Decision / Next Step:** Retry on **white glossy** desk Week 11 \(\rightarrow\) optional **ferrite** DNP pad population if needed.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Glossy desks eat cliff margin | Lower \(d_{\mathrm{trip}}\) | Specular reflection | Debounce + mechanical baffle | Acceptable for demo with known surfaces |
| Golden vs backup mismatch | Different noise floor | Assembly variance (bulk / pulldowns) | Written build standard | Ongoing |

---

## Revisions

- **Assembly standard (draft):** second \(V_{\mathrm{SYS}}\) bulk + INx **47 kΩ** pulldowns \(\rightarrow\) baseline for “integration build.”
- **Silk ECO backlog:** clearer `EDGE_*` names if **revision B** is ordered.

---

## References

- Optical reflectance / IR proximity application notes (vendor).  
- Motor driver \(R_{\mathrm{DS(on)}}(T_\mathrm{J})\) curves.

---

## Next Steps

- Week 11: **demo checklist** \(\rightarrow\) \(V_{\mathrm{BAT}}\) **full charge**, scripted thermal + safety dry runs.  
- Optional **revision B** \(\rightarrow\) populate default **INx bias** and **LED/charger** lands as already learned.  
- Optional **near-field** \(\rightarrow\) SPI **CLK** aggressor vs **mic** path (time-boxed).

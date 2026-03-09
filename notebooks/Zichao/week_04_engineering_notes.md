# Week 4 Engineering Notebook

**Date:** March 9 – March 15

---

## Objectives

- Finish **body board layout** for rev A: motor power island, buck placement, B2B connector escape.
- Run **pre-fab DRC/ERC** with fab house stackup (1.6 mm, 1 oz, min 6/6 mil).
- Define **test points** for bring-up: \(\mathrm{V_{BAT}}\), \(\mathrm{V_{SYS}}\), 3.3 V, GND, UART, motor nodes.
- IR sensor harness: define **RC filter** network values for ESD/EMI without killing edge speed.

---

## Work Completed

### My work

Placed dual H-bridge so **motor battery loop** curls **counterclockwise** into bulk caps \(C_{\mathrm{bulk1}}/C_{\mathrm{bulk2}}\) (100 µF + 10 µF X7R stack). Added **pour cut** around crystal keep-out copied from Espressif module reference.

Battery sense divider placed **Kelvin-ish** adjacent to BATT connector negative pin to reduce IR drop error.

Defined test pads **TP_BAT**, **TP_5V0**, **TP_3V3**, **TP_GND1/2**, **TP_TXD**, **TP_RXD**, **`nRST`**, **`BOOT`**.

Safety GPIO RC: series **\(R_s = 100\,\Omega\)**\* and **\(C_{hf} = 100\,\mathrm{pF}\)** to GND \(\rightarrow\) \(\tau \approx [10]\,\mathrm{ns}\), negligible vs human-scale obstacles; dominates RF rectification.

Exported **ODB++** sanity check—not sent.

\*Value may move to \(47\,\Omega\) if IRQ latency testing shows delay.

### Team discussions

- **Jiajun:** Web panel testing needs stable Wi-Fi antenna keep-out—I widened exclusion **3 mm** beyond module guideline near copper pour.  
- **Yuchen:** Second printed shell arrived—**motor wire exit** gouged plastic **1 mm**; adjust board corner radius \(\mathrm{R}=\mathrm{R_{old}}+1\) **CAD note sent**.

---

## Design Notes

### Hardware Design

- **PCB architecture:** B2B connector on body side uses **GND guard pins** surrounding SPI CLK.  
- **Power design:** **Schottky** OR before buck input on \(V_{\mathrm{SYS}}\) path (protection + reverse plug margin).  
- **Signal routing:** Motor PWM nets length-matched pairwise within **\[5\] mm**.  
- **Sensor interface:** Cliff sensor **shielded twisted pair** optional strain relief anchor on board edge.  
- **Motor interface:** Fuse on \(V_{\mathrm{motor}}\) populated—**rating TBD** after stall measure.

### Tradeoffs

- **Thermal relief on PGND pours:** easier solder for connectors vs higher Z—**selective relief** only on non-power pins.  
- **Test point density vs assembly time:** many TPs for bring-up; **DNP** silks on production if ever scaled.

### System Integration

- UART0 routed to edge for **silent log** capture without plugging USB—to speed bring-up alongside Jiajun’s flashing routine.

---

## Diagrams and Figures

**Figure 1. Layout screenshot — motor island (placeholder)**  
Description: Annotated \(\mathrm{V_\mathrm{motor}}\) inlet, \(\mathrm{C_\mathrm{bulk}}\), symmetry of high-current paths.

**Figure 2. Test pad map**  
Description: Readable silk `TP_xx` keyed to spreadsheet.

---

## Equations and Calculations

**Fuse \(I^{2}t\) sanity (placeholder):**

Select fuse rated current \(I_{\mathrm{r}}\) satisfying:

\[
I_{\mathrm{stall, worst}} < k \cdot I_{\mathrm{r}},\quad k\in[1.25,1.6]
\]

Iterate after motor stall \(I_{\mathrm{stall}}\) measured at \(V_{\mathrm{SYS,min}}\).

---

## Testing and Debugging

### Test 1: IR sensor ringing on eval harness

**Goal:** Confirm RC filter damping without missing short-range detection.  
**Setup:** Bench IR reflex module \(\rightarrow\) 10 cm coax pigtail \(\rightarrow\) series R sweep **{0, 47, 100} Ω**, shunt **{0, 100 pF}**; scope at MCU side (dev board stub).  
**Input Conditions:** Obstacle at **[distance]**.  
**Expected Result:** **\(<\) 20 ns overshoot** on rising edge \(\times\) repeatable detection interval.  
**Observed Result:** **100 Ω + DNP \(C\)**: clean edge; **[observed jitter]** µs variability at edge case distance.  
**Analysis:** Firmware debounce dominates; \(C_\mathrm{filt}\) **risk of stretch** pulses—use **\(R\) only** initially.  
**Decision / Next Step:** Rev A footprints for \(C_\mathrm{filt}\) **DNP**; \(R_\mathrm{s}=100\,\Omega\) **fitted**.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| DRC flagged necked trace on MOSI | CAD | Via cluster | Reroute + teardrops | Cleared |
| Buck feedback divider near noisy motor return | Quiet review intuition | Placement | Moved divider “quiet side” opposite motor | Reduced noise coupling risk |

---

## Revisions

- **Layout:** Antenna clearance increased.  
- **Mechanical note:** PCB corner \(\mathrm{R}\) update for strain relief clearance.

---

## References

- ESP32 hardware design guidelines (antenna clearance).  
- Motor driver datasheet layout Figure **\[n\]**.  
- Fabrication house DRU file **\[rev\]**.

---

## Next Steps

- Final ERC pass (floating inputs on driver disables).  
- Generate fab files Round 2 if Round 1 not submitted—align with procurement.  
- BoM cost rollup for advisor.

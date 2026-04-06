# Week 8 Engineering Notebook

**Date:** April 6 – April 12

---

## Objectives

- Bring up **regulated rails** sequentially: battery sense, \(V_{\mathrm{SYS}}\), 3.3 V MCU domain.
- Confirm **brown-out / reset** stability while Jiajun associates Wi‑Fi (**current spikes** concern).
- Start **motor driver output** verification with motors **blocked off table** \(\rightarrow\) small duty cycle pulses.
- Log **ripple** on \(V_{\mathrm{SYS}}\) with scope + short ground spring.

---

## Work Completed

### My work

**Rails-only session** (bench supply current limit **\[350\] mA**):

1. \(V_{\mathrm{BAT}}\) at **\[4.05\] V** \(\rightarrow\) ADC reads **\[V_{\mathrm{ADC,meas}}\]** \(\rightarrow\) estimated \(V_{\mathrm{BAT}}\) matches within **\([\Delta]\)%**.
2. \(V_{\mathrm{SYS}}\) **\[4.92–5.06\] V** unloaded; \(\Delta V \approx [\cdot]\,\mathrm{V}\) under **\([I_{\mathrm{motor,start}}]\)** inrush \(\rightarrow\) marginal but buck loop **rings \< 20 mV** visually—no chatter.

Wi‑Fi association (Jiajun running firmware): ripple on \(V_{\mathrm{SYS}}\) peaked **\[120\] mV** pp (**spring ground** capture). Piggy-backed **\(10\,\mu\mathrm{F}\)** X7R at bulk \(\rightarrow\) reduced to **[measured] mV** pp \(\rightarrow\) anecdotal improvement; screenshot in shared drive (informal; not audited numerically).

Motor micro-pulses: **\(D=5\%\)** at **\(f_{\mathrm{PWM}}=20\,\mathrm{kHz}\)** **100 ms** bursts, wheels clear of desk \(\rightarrow\) **no fault latch**, `nFAULT` quiet (populated alternate). Audible PWM whine \(\rightarrow\) expected.

Thermal spot \(\Delta T_{\mathrm{pak}}\) **\[small\] °C** during pulse bursts only.

### Team discussions

- **Jiajun:** **`EDGE_*` silk** ambiguity on harness \(\rightarrow\) I annotated spreadsheet + prototype sharpie \(\rightarrow\) ECO silk on **revision B** if we spin boards.
- **Yuchen:** Cable strain \(\rightarrow\) **tie-wrap anchor** CAD update; PCB unchanged.

---

## Design Notes

### Hardware Design

- **Power design:** “Charge **while motors run**” not in spec \(\rightarrow\) **thermal risk** flagged; taped **\[T_{\mathrm{cases}}\]** labels for future characterization.
- **PCB architecture:** Second bulk cap violates shell Z-height by \(\approx [\cdot]\,\mathrm{mm}\) \(\rightarrow\) Yuchen to trim internal rib (CAD ticket).

### Tradeoffs

- **More bulk \(\mathrm{V_{SYS}}\) vs height:** prioritized electrical margin for demo reliability.

### System Integration

- Safety GPIO polarity review: SCH **HIGH = obstacle asserted** aligns with Jiajun’s `safety_stop` \(\rightarrow\) no enum flip \(\rightarrow\) *verified on whiteboard.*

---

## Diagrams and Figures

**Figure 1. Ripple — \(V_{\mathrm{SYS}}\) Wi‑Fi associate**  
Description: Annotate \(V_{\mathrm{pp}}\) **before vs after** extra bulk.

**Figure 2. Micro-pulse burst timeline**  
Description: 100 ms on / **[off]** \(\rightarrow\) repetition period.

---

## Equations and Calculations

Duty-averaged motor voltage (idealized, ignores \(E_b\)):

\[
\langle V_{\mathrm{motor}}\rangle \approx D\,V_{\mathrm{SYS}}
\]

For \(D=0.05\), \(V_{\mathrm{SYS}}=5\,\mathrm{V}\), \(\langle V\rangle \approx 0.25\,\mathrm{V}\).

Capacitor droop first-order estimate:

\[
\Delta V \approx \frac{I\,\Delta t}{C}
\]

Solve for \(C\) given \(I_{\mathrm{burst}}\), association time budget \(\Delta t\).

---

## Testing and Debugging

### Test 1: Motor pulse — mean supply current

**Goal:** Bound mean \(I_{\mathrm{BATT}}\) at low \(D\) before stall campaigns.  
**Setup:** Ammeter series on bench HV path; MCU PWM from Jiajun’s **`human_control`** test pattern.  
**Input Conditions:** Motor **motor\_r** unloaded on desk \(\rightarrow\) negligible rolling friction \(\rightarrow\) *still not stall*.  
**Expected Result:** Mean \(I_{\mathrm{BATT}} \ll [I_{\mathrm{rated}}]\,\mathrm{A}\).

**Observed Result:** **\[I_{\mathrm{mean}}\]** \(\rightarrow\) order-of-magnitude matches \(K_v\) scribble \(\rightarrow\) rough consistency.  
**Analysis:** Stall not tested \(\rightarrow\) still open risk.  
**Decision / Next Step:** **Week 9** chocked-wheel stall + fuse sizing.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Rare brown-out during Wi‑Fi | One boot loop | \(V_{\mathrm{BAT}}\) sag toward **3.65 V** \(\rightarrow\) \(V_{\mathrm{SYS}}\) dip | Extra \(V_{\mathrm{SYS}}\) bulk; demo on **fresh charge** | Fewer resets |
| `EDGE_*` silk confusion | Wire swap question | Naming drift SCH vs CAD | Harness table + Sharpie \(\rightarrow\) ECO backlog | Accepted interim |

---

## Revisions

- **Assembly:** \(\mathrm{V_{SYS}}\) second \(\mu\)F bulk fitted on **golden** chassis build only \(\rightarrow\) log as **Variance B**.
- **Docs:** Upload scope PNGs \(\rightarrow\) `hw/measurements/`.

---

## References

- Oscilloscope probing best practices (\(\approx\) Tek/Keysight app notes, spring clip).  
- Motor driver datasheet (PWM frequency \(f_{\mathrm{PWM}}\)).

---

## Next Steps

- Stall current \(\rightarrow\) **fuse / trip** validation.  
- IR / cliff timing vs PWM noise floor.  
- Optional: micro **ferrite bead** on \(\mathrm{V_{SYS}}\) to bridge *if* height allows.

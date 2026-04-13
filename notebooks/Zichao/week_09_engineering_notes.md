# Week 9 Engineering Notebook

**Date:** April 13 – April 19

---

## Objectives

- Measure **motor stall current** \(I_{\mathrm{stall}}\) with mechanical chock and **rigorous time limit** \(\le 2\) s per sample to protect windings.
- Validate **fuse / current limit** headroom \(\rightarrow\) table for \(I_{\mathrm{trip}}\) vs \(I_{\mathrm{stall}}\).
- Correlate **IR front stop** latency with \(\mathrm{V_{PWM}}\) noise \(\rightarrow\) scope GPIO vs motor enable.
- Document **pulldowns** / default disarmed states for motor driver logic inputs \(\rightarrow\) ECO backlog if needed.

---

## Work Completed

### My work

**Stall sweep** (dual motors, \(\mathrm{V_{SYS}}\approx\) **[5.0] V**, wheels **locked** \(\rightarrow\) polyurethane pad chock \(\rightarrow\) axial slip \< **[x] mm**):

| Motor | \(I_{\mathrm{stall}}\) **[A]** | Notes |
|:---:|:---:|:---|
| L | **[I_L]** | \(\pm\) **[spread]** A over 3 trials |
| R | **[I_R]** | warmer housing after \[n\] pulses |

Compared to driver \(I_{\mathrm{trip}}\) setting \(\rightarrow\) **\[trip] A**\(\rightarrow\) ratio \(I_{\mathrm{trip}}/I_{\mathrm{stall}}\) lands in **[k_margin]** \(\rightarrow\) **margin is thin**\(\rightarrow\) Jiajun to add **PWM slew** (Week 10), not a hardware-only fix.

**PWM vs IR chatter:** Scoped **`OBSTACLE_IR`** \(\rightarrow\) **\[spikes] mV** ripple synchronized to edges when **duty** ramps **0 \(\rightarrow\) 70%** \(\rightarrow\) acceptable \(\rightarrow\) debounce \(\tau\) dominates in firmware.

**Floating INx recap:** Revised bring-up \(\#\)7 \(\rightarrow\) add **\[47\] kΩ** pulldowns to **INx** nets on **spare** second article board **(rework)** \(\rightarrow\) **no shoot-through** when MCU tri-states during reset brown-out \(\rightarrow\) **qualitative** improvement—**not** on all units yet.

### Team discussions

- **Jiajun:** Safety manager **latch** confirmed \(\rightarrow\) wants **optional hardware OR** of dual IR \(\rightarrow\) **rejected** on rev A \(\rightarrow\) **GPIO OR in FW** only.  
- **Yuchen:** Front IR window **glare** from LCD idle brightness \(\rightarrow\) mechanical **baffle sketch** \(\rightarrow\) no PCB change Week 9.

---

## Design Notes

### Hardware Design

- **Motor interface:** **Chock + thermocouple** \(\rightarrow\) winding temperature rise \< **[ΔT]** during **2 s** stall \(\rightarrow\) thermal \(\tau_\mathrm{th} \gg 2\) s \(\rightarrow\) spacing tests **30 s** apart stays conservative.
- **Sensor interface:** IR **pull-up** vs harness length \(\rightarrow\) \(R_{\mathrm{pu}}=[\cdot]\,\mathrm{k}\Omega\) \(\rightarrow\) \(t_r\approx 2.2\,R_{\mathrm{pu}}\,C_{\mathrm{in}}\) \(\rightarrow\) **[t_r] ns** vs firmware sample period.

### Tradeoffs

- **Hardware OR vs firmware OR:** fewer parts vs **fail-safe** complexity \(\rightarrow\) **FW OR** only **rev A**.

### System Integration

- **Motor PWM slew** couples to **mic** path \(\rightarrow\) scheduled **PDM clock ↔ PWM** frequency **non-integer ratio** test later \(\rightarrow\) *audio birdies* risk.

---

## Diagrams and Figures

**Figure 1. Stall test mechanical setup**  
Description: Chock geometry, current path DMM shunt position (if used), stopwatch procedure.

**Figure 2. IR line vs motor PWM (scope overlay)**  
Description: Two-channel capture with **ground spring** at sensor return.

---

## Equations and Calculations

Fuse hold vs motor inrush (rule-of-thumb):

\[
I_{\mathrm{fuse,rated}} \ge \frac{I_{\mathrm{stall}}}{k},\quad k\in[1.25,1.6]
\]

RC debounce (placeholder):

\[
\tau = R_{\mathrm{series}}\, C_{\mathrm{in,eff}}
\]

with \(C_{\mathrm{in,eff}}\) from GPIO + harness.

---

## Testing and Debugging

### Test 1: Stall current (repeatability)

**Goal:** Quantify \(I_{\mathrm{stall}}\) distribution.  
**Setup:** Bench \(V_{\mathrm{SYS}}\) regulated **\[5.00] V** \(\pm\) **[tol]**; ammeter in \(V_{\mathrm{motor}}\) feed; motors chocked.  
**Input Conditions:** **\(D_\mathrm{ramp}=100%\)** \(\le 2\) s pulses × **3**/motor with **cooling**.  
**Expected Result:** **\[I]** A scale \(\rightarrow\) **stable** \(\pm\[Δ]\%\).  
**Observed Result:** **[Table above]**.  
**Analysis:** Fuse / trip thresholds validated \(\rightarrow\) **margin thin** at low \(V_{\mathrm{BAT}}\).  
**Decision / Next Step:** Firmware **duty cap** \(\rightarrow\) Jiajun; HW **pulldown rework** \(\rightarrow\) me.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Occasional motor twitch at reset | Scope | INx float | 47 kΩ pulldowns (rework) | Reduced on second article |
| IR false trigger near LCD max brightness | Demo | optical crosstalk | Mechanical baffle (Yuchen) | *In progress* |

---

## Revisions

- **Bring-up doc:** Step 3 \(\rightarrow\) **explicit INx bias** after Week 7 lesson.  
- **Rework log:** Pulldowns on **unit B** only.

---

## References

- Motor driver datasheet (current sense / trip equations).  
- Brushed DC motor fundamentals (thermal limits on stall).  

---

## Next Steps

- Apply pulldowns on **golden** \(\rightarrow\) or schedule **revision B** if risky.  
- Full **desk edge cliff** characterization \(\rightarrow\) **Week 10** with actual shell aperture.  
- Thermals: charge **\+** motors corner case.

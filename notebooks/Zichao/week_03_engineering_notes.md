# Week 3 Engineering Notebook

**Date:** March 2 – March 8

---

## Objectives

- Complete face-board schematic sections: MIC (PDM), PA gain resistors, LCD power, backlight (if any).
- Add **speaker output EMI filter** and scope strategy for harmonic content.
- Lock motor connector pinout keyed for assembly (polarity idiot-proof).
- Begin body-board **component placement draft** prioritizing \(V_{\mathrm{SYS}}\) path length.

---

## Work Completed

### My work

Face board schematic: wired digital mic PDM rails with **explicit LC** on \(\mathrm{MIC_\mathrm{VDD}}\) branch off 3.3 V; added **\(0 Ω\)** series option to clock line for tweaking edge rate. PA (**NS4150-class**): set gain via \(\mathrm{R_{in}}\) pair; routed \(\mu\mathrm{Cap}\) on \(\mathrm{V_{BYPASS}}\) per datasheet.

LCD: \(\mathrm{V_{DD}}\) / \(\mathrm{V_{IO}}\) separated with ferrite \(L_{\mathrm{bulk}}\) toward the ESP-facing connector digital return (ferrite \(L_{\mathrm{bulk}}\) tracked on layout BOM).

Speaker filter: populated **\(1\,\mathrm{nF}\)** NPO/C0G placeholders across \(\mathrm{PA_\mathrm{OUT}+/-}\), in parallel with the speaker impedance model—Jiajun wanted EMI reduction **without firmware changes**.

Body board connector pinout (**JST PH** family target): \(V_{\mathrm{BAT}}\), \(V_{\mathrm{SYS}}\), \( \mathrm{GND} \times 3\), MOTOR nets… documented for bring-up (**`hw_pinout_revA.md`** placeholder—file TBD Week 7).

Mechanical keyed motor connector: mirrored pin 1 stripe relative to chassis—documented silkscreen **`M1:+`** on top layer.

Draft placement: MCU module at board center-ish; bridge along one edge \(\le 12\,\mathrm{mm}\) trace to connector to minimize loop area.

### Team discussions

- **Yuchen:** Cliff sensor bezel depth limits sensor height \(\le [Z_\mathrm{max}]\,\mathrm{mm}\).  
- **Jiajun:** Confirmed **motor deadband** characterization will use my **scope + low-side sense** jumpers if populated.

---

## Design Notes

### Hardware Design

- **PCB architecture:** Face board keeps **analog return** under mic/PA only; no motor return crossing (no motor on face board).  
- **Power design:** Backlight (if added): **separate LDO** from \(V_{\mathrm{SYS}}\) with soft-start RC on enable.  
- **Signal routing:** PDM data line length matched to CLK within **[50] ps** skew target (relaxed vs DDR—placement habit).  
- **Sensor interface:** Cliff IR module expected **open-collector**—add **weak pull-up** on board with **optional R series** to GPIO.  
- **Motor control:** Series gate resistors on driver if external FETs were used—**integrated bridge** uses internal; document **decoupling only**.

### Tradeoffs

- **PDM vs analog mic:** PDM routing simpler for noise; analog would need shielded cable—**PDM kept**.  
- **Filter caps on PA output:** Slight HF roll-off—verify with Jiajun’s audio band; **1 nF** deemed safe start.

### System Integration

- Motor PWM frequency vs PA noise: keep \(f_{\mathrm{PWM}} \ge [20]\,\mathrm{kHz}\) to stay above audible beat with Class-D images (firmware knob; hardware supports).

---

## Diagrams and Figures

**Figure 1. Face board audio power delivery**  
Description: 3.3 V → ferrite → MIC_VDD; PA power from same 3.3 with local bulk + HF bypass grid.

**Figure 2. Motor connector pinout (rev A)**  
Description: Table of pin index vs function; mechanical keying relative to housing.

---

## Equations and Calculations

**Class-D / PWM audibility guard (order of magnitude):**

Audible beat risk if \(|f_\mathrm{PWM}-f_\mathrm{audio}|\) within hearing—keep:

\[
f_\mathrm{PWM} \gg f_\mathrm{audio,max}
\]

Target \(f_\mathrm{audio,max} \approx 8\,\mathrm{kHz}\) content; choose \(f_\mathrm{PWM} \ge 20\,\mathrm{kHz}\).

**Voltage drop on motor harness (design check):**

\[
\Delta V = 2 I R_\mathrm{wire}
\]

Use \(I = [I_\mathrm{mean}]\), \(R_\mathrm{wire}=\rho L/A\) with \(L=[L_\mathrm{harness}]\), gauge \([AWG]\).

---

## Testing and Debugging

### Test 1: PDM mic on breakout (pre-PCB)

**Goal:** Validate PDM clock edge integrity with **longer vs shorter** hookup wire.  
**Setup:** Dev board PDM mic on breadboard; scope PDM_DATA vs CLK.  
**Input Conditions:** \(\mathrm{f_{CLK}}=[1.0]\,\mathrm{MHz}\) *typ.*  
**Expected Result:** Clean setup/hold margin at receiver.  
**Observed Result:** With **30 cm** dupont, occasional **runt pulses** in air (hand waving).  
**Analysis:** Confirms need for **short routing** and **series R** on face board.  
**Decision / Next Step:** Add **22–33 Ω** series on PDM data in layout.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| LCD FPC orientation ambiguous on silk | Review comment | No dot marker | Add **pin1 dot** + text `FPC1` | Silk updated |
| PA click on enable | Heard on eval | Pop from charge dump | Add **RC slow-start** on SD pin (if supported) / firmware mute | HW RC added where pin exists |

---

## Revisions

- **Schematic:** Added **33 Ω** PDM series default.  
- **BOM:** Swapped LCD connector vendor for **lower profile** FPC.

---

## References

- PA IC datasheet (NS4150 or chosen substitute).  
- Digital PDM mic datasheet (clocking, sensitivity).  
- LCD module drawing (FPC pin 1, voltage limits).

---

## Next Steps

- Freeze face board net classes in CAD.  
- Route critical SPI/PDM on inner prep (2-layer: **no inner**—route on top, reference bottom GND).  
- Motor island flood review with advisor.

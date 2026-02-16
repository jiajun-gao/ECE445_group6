# Week 1 Engineering Notebook

**Date:** February 16 – February 22

---

## Objectives

- Freeze a first-pass system hardware block diagram (power, ESP32-S3, audio, LCD, motors, sensors).
- Draft an initial BOM risk list (availability, package sizes, hand-solder vs reflow).
- Align ESP32-S3 pin allocation with Jiajun for I2S/PDM, SPI (LCD), motor PWM/DIR, and safety GPIOs.
- Capture exterior constraints from Yuchen for head vs body split, LCD window, and front sensor aperture.

---

## Work Completed

### My work

I translated the proposal into a **two-board concept**: a **face board** (LCD + mic front-end + speaker PA nearby) and a **body board** (battery input, charging/BMS interface, regulators, motor driver, ToF/IR connectors). The split is mechanical: Yuchen wants a hinge/neck clearance; routing every sensitive audio trace through flex would be noisy, so face board carries the “quiet” peripherals and exposes a board-to-board connector carrying I2S, SPI, motor control, UART/debug, power rails.

With Jiajun I walked through a provisional pin map. Key choices:

- Prefer **Dedicated SPI peripheral pins** for the LCD; avoid strap pins on reset.
- Motor control: reserve **four independent GPIO outputs** plus a shared **motor enable / nSLEEP** if the driver IC supports low-power standby.
- Safety inputs: allocate **interrupt-capable GPIO** for cliff IR and obstacle IR **with optional RC filter footprints** even if unfitted on rev A.

Discussion with Yuchen: front IR pair needs \(\sim\)15–20 mm vertical separation from desk for spec sheet range; cliff sensor prefers a downward \(\sim\)35–45° window through the chin. Nothing final—she will CAD a first shell; I noted **Keep-out zones** under the chin and front lip.

**Still pending:** Exact battery pack chemistry and connector (1S vs 2S) pending vendor quote; motor stall current not yet measured on the selected gearmotors.

### Team discussions

- **Jiajun:** State machine and safety override behavior—hardware must **default safe** if firmware crashes (pull-downs on enable, fail-open motor driver behavior TBD by IC choice).
- **Yuchen:** LCD centered; speaker/mic co-located with face; body volume for 18650 holder vs pouch cell.

---

## Design Notes

### Hardware Design

- **PCB architecture:** Face board focuses on localized analog/audio and short SPI runs; body board concentrates high di/dt (motors). Ground return strategy aims to **avoid routing motor ground under the MCU crystal / RF keep-out**.
- **Power design:** Preliminary rails: \(\mathrm{V_{BAT}}\) \(\rightarrow\) SEPIC or buck/boost (TBD if 1S \(\rightarrow\) 5 V) \(\rightarrow\) 3.3 V LDO for ESP32 analog domain. Charger on-module vs daughterboard decision deferred.
- **Signal routing intent:** Separate **STAR-ish** PGND connection at motor bridge; split analog vs digital pours on face board with a single choke point under the ADC mic section.
- **Sensor interface:** Cliff/obstacle wired as CMOS logic into ESP32 with **series R + optional \(\mathrm{C_\mathrm{filt}}\)** to limit ESD and RF pickup.
- **Motor interface:** H-bridge with current sense pads (even if \(\mathrm{R_\mathrm{sense}}\) DNP on rev A) for future instrumentation.

### Tradeoffs

- **Two-board vs single rigid PCB:** BOM and assembly cost ↑, but EMI and mechanical fit ↓ difficulty; aligns with hinge and LCD integration.
- **IR vs ToF for front stop:** IR is cheap and fast; ToF gives distance but power and optical crosstalk with LCD brightness—**rev A uses IR** on front stop, keep ToF connector as optional population.

### System Integration

- **Hardware–software:** Jiajun needs *latched* vs *momentary* semantics for safety—schematic will OR sensor flags into a single `nFAULT` style line only if the driver supports it; otherwise pure GPIO in firmware.
- **Safety interaction:** Motor enable held by default **off** during brown-out (targeting driver with proper UVLO).

---

## Diagrams and Figures

**Figure 1. System block diagram (Jarvis hardware)**  
Description: \(\mathrm{V_{BAT}}\) → protection → charger → motors (H-bridge) and 3.3 V domain; ESP32-S3; face board with LCD + audio; sensors into GPIO; B2B connector labeled with signal groups.

**Figure 2. Head/body split sketch (mechanical + electrical)**  
Description: Face board outline with LCD FPC, mic, speaker; body board with battery, drivers; flex or pin header interface.

---

## Equations and Calculations

**LDO power dissipation (first estimate):**

\[
P_\mathrm{D} \approx (V_\mathrm{in} - V_\mathrm{out}) \cdot I_\mathrm{out}
\]

Assuming pre-LDO rail \(V_\mathrm{in} = [5.0]\,\mathrm{V}\), \(V_\mathrm{out} = 3.3\,\mathrm{V}\), \(I_\mathrm{out} = [I_{3V3}]\,\mathrm{A}\):

\[
P_\mathrm{D} \approx (1.7\,\mathrm{V}) \cdot [I_{3V3}]
\]

If \([I_{3V3}] = 0.35\,\mathrm{A}\), then \(P_\mathrm{D} \approx 0.6\,\mathrm{W}\)—check thermal pad and copper area.

**Battery runtime (placeholder):**

\[
t \approx \frac{C_\mathrm{AH} \cdot \eta}{I_\mathrm{avg}}
\]

where \(C_\mathrm{AH}\) is pack Ah rating, \(\eta\) converter efficiency bucket \([0.80\text{–}0.92]\).

---

## Testing and Debugging

### Test 1: Motor pair bench smoke (unassembled parts)

**Goal:** Sanity-check unloaded current for two candidate gearmotors before locking driver current limit.  
**Setup:** Bench supply at \([V_\mathrm{motor}]\,\mathrm{V}\), Ammeter in series, motors floating on foam.  
**Input Conditions:** No load; stall briefly by hand \(\le 1\) s.  
**Expected Result:** No-load \(I_\mathrm{noload}\) in low tens of mA; stall \(I_\mathrm{stall}\) bounded by supply CC or motor heating.  
**Observed Result:** \([I_\mathrm{noload}] = {?}\,\mathrm{mA}\); stall \([I_\mathrm{stall}] = {?}\,\mathrm{A}\) — *log measured when parts arrive*.  
**Analysis:** Stall sets minimum **current capability** for MOSFETs/driver and capacitor ripple rating on motor rail.  
**Decision / Next Step:** Formalize \(\mathrm{V_\mathrm{motor}}\) tap (regulated 5 vs raw battery) once stall data recorded.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Pin map conflicts with SDIO/flash pinout on module | Spreadsheet flagged overlap | Incomplete ESP32-S3 module pinning check | Verify against **ESP32-S3-WROOM-1** pinout; move LCD SPI | *Open—fix Week 2* |

---

## Revisions

- None on PCB yet (schematic not started in CAD).  
- **Requirements change:** Two-board split adopted from prior “single PCB” idea for mechanical reasons.

---

## References

- Espressif, *ESP32-S3 Series Datasheet* (pinout, strapping, USB).  
- Motor vendor **[gear motor model TBD]** datasheet (stall, rated voltage).  
- TI or dialog app notes on **H-bridge layout and current recirculation** (general).

---

## Next Steps

- Enter schematic in CAD; create symbol/footprint for LCD FPC and motor driver candidate.  
- Confirm battery configuration with measured motor demand.  
- Second pin review with Jiajun after SDIO verification.

# Week 6 Engineering Notebook

**Date:** March 23 – March 29

---

## Objectives

- Submit **PCB fabrication** (face + body) and **steel stencil** order.
- Final **BOM parity** vs placed parts (orientation dots, polarization).
- Author **bring-up checklist**: power sequencing, dummy load on \(\mathrm{V_{SYS}}\), **smoke fuse** bench procedure.
- Pre-order **replacement passives kit** (\(±\)20% qty) for rework.

---

## Work Completed

### My work

Submitted **Gerber Round A** ZIP + drills + BOM **pick-and-place omitted** (hand assembly). Verified **soldermask clearance** \(\ge 4\) mil annular ring on \(\mathrm{V_{BAT}}\) vias flood.

Stencil: stepped reduction **10% area** locally on **fine-pitch DRV** footprints (fabricator advisory).

Bring-up checklist (excerpt logged in team drive):

1. Bench supply \(V_{\mathrm{BAT}}\) current-limited **[200] mA** through **fuse proxy** resistor.  
2. Verify **\[3.75–4.0] V** at charger input unloaded.  
3. Enable buck with motor driver \(V_{\mathrm{motor}}\) disconnected until MCU is alive; then open bridge inputs per checklist. *(Step ordering refined Week 9 after actual smoke path.)*

Replacement kit ordered: **100 Ω / 1 kΩ / 10 kΩ**, X7R **0.1 / 1 / 10 µF** kit.

### Team discussions

- **Jiajun:** Wants **`GPIO strapping`** readback on UART during first boot—I added **labeled strapping jumper** footprints DNP (`STRAP_JTAG`) for emergency.  
- **Yuchen:** Color anodizing **not affecting** EMI—note for aesthetics only.

---

## Design Notes

### Hardware Design

- **PCB architecture:** Fab panel with **mouse bites** \(\rightarrow\) breakout sand edges before flex stress on FPC region.  
- **Power design:** Test hook wire gauge specified **18 AWG** for \(\mathrm{V_{BAT}}\) bench harness to emulate pack IR drop during margin tests.  
- **Signal routing:** Post-fabric **no-patch** assumptions locked—golden Gerber archived `hw/pcb_revA/` *placeholder*.  
- **Sensor interface:** Pre-kit IR modules measured \(V_{\mathrm{high,min}}\) vs supply—documented **[value]** \(\rightarrow\) qualifies **3.3 V CMOS thresholds**.  
- **Motor interface:** Motor connector polarization photos in assembly CAD.

### Tradeoffs

- **HASL avoidance on FPC pads:** Paid ENIG on face despite body cost pressure—**risk reduction** priority.

### System Integration

- Shared GND Reference for scope: standardized **`GND_CLIP`** point beside \(\mathrm{V_{BAT}}\) negative lug for repeatable ripple captures.

---

## Diagrams and Figures

**Figure 1. Fabrication submission receipt (placeholder)**  
Description: Job ID **[id]**, lead time **[days]** EDA.

**Figure 2. Stencil aperture snapshot**  
Description: DRV region reduction annotated.

---

## Equations and Calculations

**Bench supply sag during inrush:**

\[
\Delta V_{\mathrm{supply}} \approx I_{\mathrm{inrush}} \cdot (R_{\mathrm{series}}+R_{\mathrm{wire}})
\]

Target \(\Delta V_{\mathrm{supply}}\) small enough buck **\(UV_{\mathrm{LO}}\)** doesn't chatter.

---

## Testing and Debugging

### Test 1: Pre-shipment netlist continuity check on gerber CAM

**Goal:** Automated **Opens/shorts sanity** CAM net compare.  
**Setup:** FAB CAM VS Gerber importer software.  
**Input Conditions:** N/A  
**Expected Result:** SPI/MOS net classes match SCH.  
**Observed Result:** **One false short** flagged on stitched pour—manual review dismissed.  
**Analysis:** Artefact from flood threshold.  
**Decision / Next Step:** Record screenshot; no design change.

---

## Problems and Proposed Solutions

| Problem | Evidence | Cause | Fix | Result |
|---|---|---|---|---|
| Panel tab too close to Type-C pads | CAM DFM | Panelization default | Moved tab outward **\[x\] mm** | Reexported gerber |
| Stencil aperture bridged pads in CAD preview | Rendering | Oversized paste | Reduced area locally | Accepted fab advisory |

---

## Revisions

- **Fabrication outputs:** Gerber regenerated after **panel tweak**.  
- **Docs:** Bring-up checklist v0.9.

---

## References

- Fab house **DFM guide** \[link in purchase order PDF\].  
- IPC-7351 stencil design guidelines summary note.  

---

## Next Steps

- Track shipment; freeze alternate **\(R_{\mathrm{LIM}}\)** values for quick tape-and-replace.  
- Prep hot-air rework profile for QFN bridges.  
- Motor assembly torque spec for gearbox screws (mechanical liaison).

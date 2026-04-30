# DeskBot Robot Module — Test Plan & Debugging Checklist

## Wiring recap
| Signal | GPIO | Notes |
|---|---|---|
| MOTOR1_IN_A (left)  | 17 | LEDC ch0 |
| MOTOR1_IN_B (left)  | 18 | LEDC ch1 |
| MOTOR2_IN_A (right) | 8  | LEDC ch2 |
| MOTOR2_IN_B (right) | 3  | LEDC ch3 — **strapping pin**, see risk note |
| TCRT5000 front DO   | 15 | points forward, detects obstacles |
| TCRT5000 bottom DO  | 16 | points down, detects desk / cliff |

Adjust each sensor's on-module pot so DO toggles cleanly at the distance you want:
- Front: trip when an object is ~3–5 cm away.
- Bottom: mount 5–10 mm above desk; LED should light when floor is present and drop when the sensor sees open air off the desk edge.

## Signal polarity
Most TCRT5000 modules drive DO **LOW** when they see a reflection. The code assumes this:

```cpp
// ir_manager.h
constexpr int FRONT_ACTIVE_LEVEL  = 0;  // LOW => obstacle
constexpr int BOTTOM_ACTIVE_LEVEL = 0;  // LOW => floor present
```

If your modules are inverted (onboard LED lit when there is NO reflection), flip the matching constant to `1` and rebuild.

Verify polarity on the first bring-up with a quick serial check:
- Wave hand in front: `robot.get_status` should show `front_obstacle=1`.
- Hold robot over the desk: `bottom_floor=1`. Off the edge: `bottom_floor=0`.

## Bench bring-up order

1. **Power-only test (motors unpowered).** Flash firmware, open serial. Expect:
   ```
   motor_ctrl: Motor PWM ready: 20000 Hz, 10-bit
   ir_mgr:     IR DO inputs ready (front GPIO15 active=0, bottom GPIO16 active=0)
   robot_app:  robot ready (AI mode)
   ```
2. **Motor sanity** — wheels off the ground. Via xiaozhi AI say "forward" etc. If a wheel spins the wrong way, swap its IN_A/IN_B wiring **or** flip the sign in `motion_manager.cc` for that wheel.
3. **IR polarity check** — call `robot.get_status` and confirm both flags match reality. Tune the sensor pots until DO transitions are sharp.
4. **Cliff latch** — set robot on desk edge such that the bottom sensor sees empty space. Serial: `CLIFF LATCH -> EMERGENCY_STOP`. `robot.move forward` does nothing. Move robot back on desk → `hazard cleared` after 500 ms.
5. **Front-obstacle veto** — hold hand in front. "forward" logs `veto FRONT_OBSTACLE`. "backward" and "stop" still work.
6. **Mode switch** — say "human control mode" (legacy "manual mode" still works). Serial: `-> MANUAL_MODE`, `wifi_manual: SoftAP up`. The MCP tool `robot.set_mode` takes `mode=human_control` (or `human` / `manual` as aliases).
7. **Phone test** — join `DeskBot-XXXX` / `deskbot123`, open `http://192.168.4.1/`. Buttons drive the robot. "Switch to AI Mode" button returns control.
8. **Cross-mode rejection** — in AI mode, `curl -X POST 192.168.4.1/cmd?m=FORWARD` is ignored. In human-control mode, voice "forward" is ignored. "stop" works in either.

## Common issues
- **Motor jitter at boot**: GPIO3 strapping. Add a 10 kΩ pull-up so the pin isn't driven low at reset.
- **Robot always emergency-stopped**: bottom sensor polarity inverted. Flip `BOTTOM_ACTIVE_LEVEL` to 1.
- **Robot never stops at cliff**: bottom sensor pot threshold too loose (still "sees" floor over edge). Tighten pot.
- **Forward immediately vetoed with nothing in front**: front polarity inverted. Flip `FRONT_ACTIVE_LEVEL` to 1.
- **SoftAP not appearing**: confirm serial shows `SoftAP up: SSID=…`. If not, STA may have failed earlier — the xiaozhi board init logs will show.
- **MCP tool not reachable**: verify `robot.move` in the tool list from your LLM backend. `RobotApp::RegisterMcpTools()` runs at boot.

## Tunables
- `motor_control.h : MOTOR_PWM_FREQ_HZ` — raise if audible whine.
- `motion_manager.h : default_speed_ / turn_speed_`
- `ir_manager.h : FRONT_ACTIVE_LEVEL, BOTTOM_ACTIVE_LEVEL, IR_DEBOUNCE_HITS`
- `safety_manager.h : HAZARD_CLEAR_MS`
- `wifi_manual_control.cc : AP_PASS`

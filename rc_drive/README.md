# ELRS Remote-Control Drive

Drives the four-motor drivetrain from an ExpressLRS (ELRS) receiver.
The receiver talks to the ESP32 over UART using the CRSF protocol.
Motor wiring is the same as in `drivetrain_test/`.

## Receiver wiring

| ELRS receiver pin | ESP32 pin |
|---|---|
| TX | GPIO16 (RX2) |
| RX | GPIO17 (TX2), optional, used only for telemetry |
| GND | GND (common ground) |
| 5V | 5V / VIN |

- Do not use GPIO1/GPIO3 (UART0). That port is USB serial for uploads and the Serial Monitor.
- ELRS receivers use 3.3 V logic, so no level shifter is needed.
- The receiver must be bound to the transmitter before it outputs anything.

## Controls (Mode 2 transmitter, AETR channel order)

| Channel | Stick / switch | Action |
|---|---|---|
| CH2 | Right stick up/down | Forward / reverse |
| CH1 | Right stick left/right | Strafe |
| CH4 | Left stick left/right | Rotate |
| CH5 (AUX1) | Arm switch | Must be high to drive |

## Safety behavior

- **Arming:** the motors only run after CH5 is switched high *while the sticks are centered*.
- **Failsafe:** if no valid frame arrives for 250 ms (lost link, unplugged receiver), all motors stop immediately.
- **Speed cap:** `MAX_DUTY` in `src/main.cpp` (default 128 ≈ 50%).
- **Ramping:** speed changes are ramped so the motors never reverse instantly.

## First test

1. Lift all wheels off the ground.
2. Upload, then open the Serial Monitor at 115200 baud.
3. With the transmitter on, check that `link=1` and that the channel values move with the sticks (about 172–1811, centered at 992).
4. Arm, push forward gently, and check that every wheel spins forward.
   If a wheel spins backward, set `inverted = true` for that motor in the `motors[]` table.
5. Test strafe and rotate. If strafing goes the wrong way, swap the sign of `x` in the mix.
6. Turn the transmitter off while driving and confirm that the motors stop.

## Limitations

- The mixing assumes mecanum or 45° X-drive omni wheels.
- No encoder feedback.
- Telemetry back to the transmitter is not implemented.

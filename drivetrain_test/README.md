# Four-Wheel Drivetrain Motor Test

Tests four DC motors using one ESP32 and four BTS7960 motor drivers.
USB Serial Monitor commands control all four motors together.

## Hardware

- DOIT ESP32 DEVKIT V1
- Four BTS7960 / IBT-2 motor drivers, one per motor
- Four DC motors rated for the motor supply voltage
- 12 V motor supply capable of supplying all four motors
- Regulated 5 V supply for driver logic
- USB data cable

## Wiring

Turn off power before changing connections.

### Power connections

Apply these connections to each BTS7960:

| Component / pin | Connect to |
|---|---|
| B+ | 12 V motor supply positive |
| B− | Motor supply negative |
| M+ / M− | Corresponding motor wires |
| VCC | Regulated 5 V |
| GND | Common ground |
| R_EN / L_EN | Regulated 5 V, always enabled |
| R_IS / L_IS | Unconnected |

Connect ESP32 GND, all driver grounds, and supply negative together.
Route motor current directly through the power wiring.

Confirm each driver module accepts 3.3 V control signals.

### ESP32 control pins

Each motor uses two independent PWM outputs.

| Wheel | LPWM GPIO | RPWM GPIO |
|---|---|---|
| Left front | GPIO13 | GPIO12 |
| Right back | GPIO14 | GPIO27 |
| Right front | GPIO26 | GPIO25 |
| Left back | GPIO33 | GPIO32 |

These numbers refer to GPIO numbers, not physical header positions.

GPIO12 is a boot-strapping pin. If the ESP32 fails to boot with the
drivers connected, check whether the driver is affecting that pin.
An alternative is GPIO23; update both the wiring and code if changed.

### ESP32 power during testing

Power the ESP32 through USB from the computer.
Disconnect the external regulator's positive wire from ESP32 VIN
while USB is connected. Keep the grounds connected.

Do not connect 12 V to ESP32 VIN.

A 0.1 µF capacitor is connected between ESP32 VIN and GND.
This filters high-frequency noise but does not provide bulk energy
storage for motor startup.

## PlatformIO Setup

Open `drivetrain_test` in VS Code with PlatformIO.

The project uses:

- Board: `esp32doit-devkit-v1`
- Framework: Arduino
- Serial Monitor baud rate: 115200
- PWM frequency: 20 kHz
- PWM resolution: 8 bits
- Test duty cycle: 64/255, approximately 25%

Example `platformio.ini`:

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
```

PlatformIO normally detects the COM port automatically.
If necessary, add:

```ini
upload_port = COM5
monitor_port = COM5
```

Replace COM5 with the port assigned to your ESP32.

## Testing

1. Lift all four wheels clear of the ground.
2. Verify wiring and power connections.
3. Build and upload the project.
4. Open the PlatformIO Serial Monitor at 115200 baud.
5. Send one of the commands below.

| Command | Action on all four drivers |
|---|---|
| `f` | RPWM at approximately 25% duty; LPWM LOW |
| `r` | LPWM at approximately 25% duty; RPWM LOW |
| `s` | Both PWM inputs LOW; electrical braking |

Press Enter after typing a command if needed.
Commands are lowercase.

The motors continue running until another command is received.
Each `f` or `r` command first stops all motors for one second.
Wait until the wheels have physically stopped before reversing.

The current code uses a blocking delay during this one-second interval,
so serial commands are not processed until the interval finishes.

The same electrical command is applied to every driver.
Physical rotation depends on motor wiring and mounting.
These commands do not yet represent coordinated robot forward,
reverse, turning, or sideways movement.

PWM duty cycle is not a measurement of actual motor speed.

## Changing Test Power

The initial duty cycle is defined in `src/main.cpp`:

```cpp
constexpr int TEST_DUTY = 64;
```

With 8-bit PWM, the range is 0–255:

| Value | Approximate duty cycle |
|---|---|
| 0 | 0% |
| 64 | 25% |
| 128 | 50% |
| 191 | 75% |
| 255 | 100% |

Stop the test if a motor hums without turning.

## Troubleshooting

- No serial output: check the COM port, baud rate, and USB data cable.
  Press the ESP32 reset button to display the startup message again.
- One motor does not turn: check that driver's power, enable pins,
  PWM wiring, and motor connections.
- No motors turn: check the shared power supply, common ground,
  and that the uploaded program is running.
- Motor hums without turning: stop and check for a stall or
  insufficient starting torque.
- ESP32 resets when motors start: check supply voltage, power wiring,
  noise, and decoupling.
- ESP32 does not boot: investigate GPIO12 and its boot-strapping role.
- Wheels rotate in different physical directions: this can result
  from mirrored mounting or motor wiring. Direction calibration
  is not implemented yet.

## Current Limitations

- All four motors receive the same duty cycle and direction command.
- No individual wheel commands or direction calibration.
- No coordinated omni-wheel movement.
- No encoder feedback or closed-loop speed control.
- No automatic stop on loss of the serial connection.
- Enable pins are permanently HIGH.
- Stop commands are delayed during the one-second blocking interval.
- Intended for supervised bench testing.

## Team Contributions

Keep drivetrain test code and documentation in this folder.
Update this README whenever wiring, pin assignments, commands,
or test behavior changes.

Exclude `.pio/` build output from Git.
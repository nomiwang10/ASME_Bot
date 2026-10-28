# Drivetrain Motor Test

Initial test of one DC motor using an ESP32 and a BTS7960 motor driver.
Control motor direction through the USB Serial Monitor.

## Hardware

- DOIT ESP32 DEVKIT V1
- BTS7960 / IBT-2 motor driver
- DC motor rated for the motor supply voltage
- 12 V motor supply
- Regulated 5 V supply for driver logic
- USB data cable

## Wiring

Turn off power before changing connections.

| Component / pin | Connect to |
|---|---|
| BTS7960 B+ | 12 V motor supply positive |
| BTS7960 B− | Motor supply negative |
| BTS7960 M+ / M− | Motor wires |
| BTS7960 VCC | Regulated 5 V |
| BTS7960 GND | Common ground |
| BTS7960 R_EN / L_EN | Regulated 5 V |
| BTS7960 RPWM | ESP32 GPIO25 |
| BTS7960 LPWM | ESP32 GPIO26 |
| BTS7960 R_IS / L_IS | Unconnected |

Connect ESP32 GND, driver GND, and supply negative together.
Route motor current directly through the power wiring.

Confirm the specific driver module accepts 3.3 V control signals.

### ESP32 power during testing

Power the ESP32 through USB from the computer.
Disconnect the external regulator's positive wire from ESP32 VIN
while USB is connected. Keep the grounds connected.

Do not connect 12 V to ESP32 VIN.

A 0.1 µF capacitor is connected between ESP32 VIN and GND.
This filters high-frequency noise but does not provide bulk energy
storage for motor startup.

## PlatformIO Setup

Open this folder, `drivetrain_test`, in VS Code with PlatformIO.

The project uses:

- Board: `esp32doit-devkit-v1`
- Framework: Arduino
- Serial Monitor baud rate: 115200
- PWM frequency: 20 kHz
- PWM resolution: 8 bits
- Test duty cycle: 64/255, approximately 25%

PlatformIO normally detects the COM port automatically.
If needed, set the correct ports in `platformio.ini`:

    upload_port = COM5
    monitor_port = COM5

Replace COM5 with the port assigned to your ESP32.

## Testing

1. Lift the wheels clear of the ground.
2. Verify wiring and power connections.
3. Build and upload the project.
4. Open the PlatformIO Serial Monitor at 115200 baud.
5. Send one of the commands below.

| Command | Action |
|---|---|
| f | Forward at approximately 25% duty |
| r | Reverse at approximately 25% duty |
| s | Stop using electrical braking |

Press Enter after typing a command if needed.

The motor continues running until another command is received.
Direction changes include a one-second stop interval.
Wait until the motor has physically stopped before reversing.

“Forward” depends on the motor wire orientation.
PWM duty cycle is not a measurement of actual motor speed.

## Troubleshooting

- No serial output: check the COM port, baud rate, and USB data cable.
- Motor does not turn: check motor power, common ground, and enable pins.
- Motor hums without turning: stop the test and check for a stall or
  insufficient starting torque.
- ESP32 resets: check supply voltage, wiring, noise, and decoupling.
- Direction is reversed: power off and swap the motor wires, or update
  the direction mapping in software.

## Current Limitations

- Tests one motor only.
- No encoder feedback or closed-loop speed control.
- No automatic stop on loss of the serial connection.
- Enable pins are permanently HIGH.
- Intended for supervised bench testing.

## Team Contributions

Keep drivetrain test code and documentation in this folder.
Update this README whenever wiring, pin assignments, commands,
or test behavior changes.

Exclude `.pio/` build output from Git.

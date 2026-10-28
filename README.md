# ASME Bot

Shared repository for the ASME robot’s software, electronics, and documentation.

## Repository structure

- `drivetrain_test/`: PlatformIO project for initial ESP32 and BTS7960 motor testing.
- Add folders for other subsystems as they are developed.

## How each subsystem should use this repository

1. Keep your subsystem’s files in a clearly named folder.
2. Include a README in that folder explaining:
   - What the subsystem does.
   - Hardware and required components.
   - Wiring and pin assignments.
   - How to build, upload, and test.
   - Current limitations and unfinished work.
3. Keep generated build files, passwords, and API keys out of Git.
4. Discuss changes to shared wiring, power connections, or interfaces with the team.

## Making changes

Clone this repository once, then open your subsystem’s project folder in VS Code.
For PlatformIO projects, open the folder containing `platformio.ini`.

Before starting work:

```bash
git switch main
git pull
git switch -c your-name/short-description
```

After making changes, run these commands from the repository root:

```bash
git add path/to/your-subsystem
git commit -m "Describe your changes"
git push -u origin HEAD
```

Open a pull request on GitHub so the team can review and merge your changes.

Start with a clean working directory before switching branches or pulling updates.

## Initial drivetrain test

Project: `drivetrain_test/`

- Board: DOIT ESP32 DEVKIT V1
- Framework: Arduino through PlatformIO
- GPIO25 → BTS7960 RPWM
- GPIO26 → BTS7960 LPWM
- Serial Monitor: 115200 baud

Commands:

| Command | Action |
|---|---|
| `f` | Forward at 25% PWM duty |
| `r` | Reverse at 25% PWM duty |
| `s` | Stop using electrical braking |

Lift the wheels before testing. Verify power wiring and common ground before enabling the motor.

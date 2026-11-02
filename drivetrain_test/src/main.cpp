#include <Arduino.h>
#include <esp_arduino_version.h>

constexpr int PWM_FREQ = 20000;
constexpr int PWM_BITS = 8;
constexpr int TEST_DUTY = 232;  // 64 - Approximately 25%, 128 - 50%, 128+64 = 232 is about 75%

struct Motor {
    uint8_t lpwm;
    uint8_t rpwm;
};

const Motor motors[] = {
    {13, 12},  // Left front
    {14, 27},  // Right back
    {26, 25},  // Right front
    {33, 32}   // Left back
};

constexpr int MOTOR_COUNT = sizeof(motors) / sizeof(motors[0]);

void setMotor(int index, int leftDuty, int rightDuty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(motors[index].lpwm, leftDuty);
    ledcWrite(motors[index].rpwm, rightDuty);
#else
    ledcWrite(index * 2, leftDuty);
    ledcWrite(index * 2 + 1, rightDuty);
#endif
}

void stopAll() {
    for (int i = 0; i < MOTOR_COUNT; i++) {
        setMotor(i, 0, 0);
    }
}

void runAll(bool forward) {
    // Remove drive from every motor before changing direction.
    stopAll();
    delay(1000);

    for (int i = 0; i < MOTOR_COUNT; i++) {
        if (forward) {
            setMotor(i, 0, TEST_DUTY);
        } else {
            setMotor(i, TEST_DUTY, 0);
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Hold all control signals LOW before configuring PWM.
    for (int i = 0; i < MOTOR_COUNT; i++) {
        pinMode(motors[i].lpwm, OUTPUT);
        pinMode(motors[i].rpwm, OUTPUT);
        digitalWrite(motors[i].lpwm, LOW);
        digitalWrite(motors[i].rpwm, LOW);
    }

    for (int i = 0; i < MOTOR_COUNT; i++) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcAttach(motors[i].lpwm, PWM_FREQ, PWM_BITS);
        ledcAttach(motors[i].rpwm, PWM_FREQ, PWM_BITS);
#else
        ledcSetup(i * 2, PWM_FREQ, PWM_BITS);
        ledcSetup(i * 2 + 1, PWM_FREQ, PWM_BITS);

        ledcAttachPin(motors[i].lpwm, i * 2);
        ledcAttachPin(motors[i].rpwm, i * 2 + 1);
#endif
    }

    stopAll();
    Serial.println("Four-wheel test ready.");
    Serial.println("f = all RPWM, r = all LPWM, s = stop all");
}

void loop() {
    if (!Serial.available()) return;

    char command = Serial.read();

    switch (command) {
        case 'f':
            runAll(true);
            Serial.println("All motors: RPWM at 75%");
            break;

        case 'r':
            runAll(false);
            Serial.println("All motors: LPWM at 75%");
            break;

        case 's':
            stopAll();
            Serial.println("All motors stopped");
            break;

        default:
            break;  // Ignore Enter and other characters.
    }
}
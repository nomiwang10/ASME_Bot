#include <Arduino.h>
#include <esp_arduino_version.h>

constexpr int RPWM = 25;
constexpr int LPWM = 26;
constexpr int PWM_FREQ = 20000;
constexpr int PWM_BITS = 8;
constexpr int TEST_DUTY = 230;  // 230/255 ≈ 50%

void writePWM(int forward, int reverse) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(RPWM, forward);
    ledcWrite(LPWM, reverse);
#else
    ledcWrite(0, forward);
    ledcWrite(1, reverse);
#endif
}

void setup() {
    Serial.begin(115200);

    pinMode(RPWM, OUTPUT);
    pinMode(LPWM, OUTPUT);
    digitalWrite(RPWM, LOW);
    digitalWrite(LPWM, LOW);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(RPWM, PWM_FREQ, PWM_BITS);
    ledcAttach(LPWM, PWM_FREQ, PWM_BITS);
#else
    ledcSetup(0, PWM_FREQ, PWM_BITS);
    ledcSetup(1, PWM_FREQ, PWM_BITS);
    ledcAttachPin(RPWM, 0);
    ledcAttachPin(LPWM, 1);
#endif

    writePWM(0, 0);
    Serial.println("f = forward, r = reverse, s = stop");
}

void loop() {
    if (!Serial.available()) return;

    char command = Serial.read();
    if (command != 'f' && command != 'r' && command != 's') return;

    // Stop before applying a new direction.
    writePWM(0, 0);

    if (command == 's') {
        Serial.println("Stopped");
        return;
    }

    delay(1000);  // Initial unloaded test: allow time to stop

    if (command == 'f') {
        writePWM(TEST_DUTY, 0);
        Serial.println("Forward at 50%");
    } else {
        writePWM(0, TEST_DUTY);
        Serial.println("Reverse at 50%");
    }
}
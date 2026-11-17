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

// ELRS receiver (CRSF protocol) on UART2, logged only; it does not drive the motors.
constexpr int CRSF_RX_PIN = 16;  // ESP32 RX2 <- receiver TX
constexpr int CRSF_TX_PIN = 17;  // ESP32 TX2 -> receiver RX (unused)
constexpr uint32_t CRSF_BAUD = 420000;
constexpr uint8_t CRSF_TYPE_RC_CHANNELS = 0x16;
constexpr int CRSF_MAX_FRAME = 64;
constexpr int LOG_CHANNELS = 10;          // the T8L sends up to 10 channels
constexpr uint32_t LINK_TIMEOUT_MS = 250;
constexpr uint32_t LOG_PERIOD_MS = 200;

uint16_t channels[16];
uint32_t lastFrameMs = 0;
uint32_t frameCount = 0;
uint8_t crsfBuf[CRSF_MAX_FRAME];
int crsfPos = 0;

uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
        }
    }
    return crc;
}

void unpackChannels(const uint8_t* payload) {
    // 16 channels x 11 bits, packed little-endian into 22 bytes
    uint32_t bits = 0;
    int bitCount = 0;
    int ch = 0;
    for (int i = 0; i < 22 && ch < 16; i++) {
        bits |= (uint32_t)payload[i] << bitCount;
        bitCount += 8;
        while (bitCount >= 11 && ch < 16) {
            channels[ch++] = bits & 0x7FF;
            bits >>= 11;
            bitCount -= 11;
        }
    }
}

void readCrsf() {
    // Frame: [sync][len][type][payload...][crc]; len counts type + payload + crc
    while (Serial2.available()) {
        uint8_t b = Serial2.read();

        if (crsfPos == 0 && b != 0xC8 && b != 0xEE) continue;
        if (crsfPos == 1 && (b < 2 || b > CRSF_MAX_FRAME - 2)) {
            crsfPos = 0;
            continue;
        }

        crsfBuf[crsfPos++] = b;

        if (crsfPos >= 2 && crsfPos == crsfBuf[1] + 2) {
            uint8_t len = crsfBuf[1];
            if (crsfBuf[2] == CRSF_TYPE_RC_CHANNELS && len == 24 &&
                crc8(&crsfBuf[2], len - 1) == crsfBuf[len + 1]) {
                unpackChannels(&crsfBuf[3]);
                lastFrameMs = millis();
                frameCount++;
            }
            crsfPos = 0;
        }
    }
}

void logChannels() {
    static uint32_t lastLog = 0;
    if (millis() - lastLog < LOG_PERIOD_MS) return;
    lastLog = millis();

    bool link = frameCount > 0 && millis() - lastFrameMs < LINK_TIMEOUT_MS;
    Serial.printf("link=%d frames=%lu |", link, (unsigned long)frameCount);
    for (int i = 0; i < LOG_CHANNELS; i++) {
        Serial.printf(" ch%d=%4u", i + 1, channels[i]);
    }
    Serial.println();
}

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

    Serial2.begin(CRSF_BAUD, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);

    Serial.println("Four-wheel test ready.");
    Serial.println("f = all RPWM, r = all LPWM, s = stop all");
    Serial.println("Logging ELRS receiver channels (GPIO16). Controller does not drive motors.");
}

void loop() {
    readCrsf();
    logChannels();

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
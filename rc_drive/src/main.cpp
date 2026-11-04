#include <Arduino.h>
#include <esp_arduino_version.h>

// ============================================================
// ELRS receiver (CRSF protocol) on UART2
// ============================================================
constexpr int CRSF_RX_PIN = 16;  // ESP32 RX2  <- receiver TX
constexpr int CRSF_TX_PIN = 17;  // ESP32 TX2  -> receiver RX (telemetry, optional)
constexpr uint32_t CRSF_BAUD = 420000;

constexpr uint8_t CRSF_SYNC_FC = 0xC8;
constexpr uint8_t CRSF_SYNC_ALT = 0xEE;
constexpr uint8_t CRSF_TYPE_RC_CHANNELS = 0x16;
constexpr int CRSF_MAX_FRAME = 64;

constexpr uint16_t CRSF_MID = 992;    // stick centered
constexpr float CRSF_SPAN = 820.0f;   // 172..1811 around 992

// Stop the motors if no valid RC frame arrives within this time.
// ELRS receivers stop sending channel frames when the link is lost.
constexpr uint32_t FAILSAFE_MS = 250;

// Channel assignments (0-based, standard AETR mapping on the transmitter)
constexpr int CH_STRAFE = 0;   // CH1 right stick left/right
constexpr int CH_FORWARD = 1;  // CH2 right stick up/down
constexpr int CH_ROTATE = 3;   // CH4 left stick left/right
constexpr int CH_ARM = 4;      // CH5 / AUX1 arm switch

constexpr float DEADBAND = 0.05f;

// ============================================================
// Motors (BTS7960)
// ============================================================
constexpr int PWM_FREQ = 20000;
constexpr int PWM_BITS = 8;
constexpr int MAX_DUTY = 128;            // speed cap, 0-255 (128 = 50%)
constexpr int RAMP_STEP = 6;             // max duty change per update
constexpr uint32_t UPDATE_MS = 10;       // motor update period

struct Motor {
    uint8_t lpwm;
    uint8_t rpwm;
    bool inverted;  // flip if this wheel spins backward when commanded forward
};

enum { LEFT_FRONT, RIGHT_BACK, RIGHT_FRONT, LEFT_BACK };

const Motor motors[] = {
    {13, 12, false},  // Left front
    {14, 27, false},  // Right back
    {26, 25, false},  // Right front
    {33, 32, false},  // Left back
};

constexpr int MOTOR_COUNT = sizeof(motors) / sizeof(motors[0]);

int currentDuty[MOTOR_COUNT] = {0};  // signed: + = RPWM, - = LPWM

// ============================================================
// CRSF parsing
// ============================================================
uint16_t channels[16];
uint32_t lastFrameMs = 0;
bool gotFrame = false;

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
    // 16 channels x 11 bits packed little-endian into 22 bytes
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

void handleFrame() {
    // Frame: [sync][len][type][payload...][crc]; len counts type+payload+crc
    uint8_t len = crsfBuf[1];
    uint8_t type = crsfBuf[2];
    if (crc8(&crsfBuf[2], len - 1) != crsfBuf[len + 1]) return;

    if (type == CRSF_TYPE_RC_CHANNELS && len == 24) {
        unpackChannels(&crsfBuf[3]);
        lastFrameMs = millis();
        gotFrame = true;
    }
}

void readCrsf() {
    while (Serial2.available()) {
        uint8_t b = Serial2.read();

        if (crsfPos == 0) {
            if (b != CRSF_SYNC_FC && b != CRSF_SYNC_ALT) continue;
        } else if (crsfPos == 1) {
            if (b < 2 || b > CRSF_MAX_FRAME - 2) {
                crsfPos = 0;
                continue;
            }
        }

        crsfBuf[crsfPos++] = b;

        if (crsfPos >= 2 && crsfPos == crsfBuf[1] + 2) {
            handleFrame();
            crsfPos = 0;
        }
    }
}

bool linkOk() {
    return gotFrame && (millis() - lastFrameMs) < FAILSAFE_MS;
}

float stick(int ch) {
    float v = ((int)channels[ch] - CRSF_MID) / CRSF_SPAN;
    v = constrain(v, -1.0f, 1.0f);
    return fabsf(v) < DEADBAND ? 0.0f : v;
}

// ============================================================
// Motor output
// ============================================================
void writeMotor(int i, int duty) {
    if (motors[i].inverted) duty = -duty;
    int l = duty < 0 ? -duty : 0;
    int r = duty > 0 ? duty : 0;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(motors[i].lpwm, l);
    ledcWrite(motors[i].rpwm, r);
#else
    ledcWrite(i * 2, l);
    ledcWrite(i * 2 + 1, r);
#endif
}

void stopAllNow() {
    for (int i = 0; i < MOTOR_COUNT; i++) {
        currentDuty[i] = 0;
        writeMotor(i, 0);
    }
}

void driveMotors(const int target[MOTOR_COUNT]) {
    // Ramp toward the target so direction changes pass through zero gently.
    for (int i = 0; i < MOTOR_COUNT; i++) {
        int diff = target[i] - currentDuty[i];
        diff = constrain(diff, -RAMP_STEP, RAMP_STEP);
        currentDuty[i] += diff;
        writeMotor(i, currentDuty[i]);
    }
}

// ============================================================
// Main
// ============================================================
bool armed = false;

void setup() {
    Serial.begin(115200);

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
    stopAllNow();

    for (auto& c : channels) c = CRSF_MID;
    Serial2.begin(CRSF_BAUD, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);

    Serial.println("ELRS drive ready. Flip CH5 (arm switch) with sticks centered to arm.");
}

void loop() {
    readCrsf();

    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate < UPDATE_MS) return;
    lastUpdate = millis();

    float y = stick(CH_FORWARD);
    float x = stick(CH_STRAFE);
    float r = stick(CH_ROTATE);

    bool link = linkOk();
    bool armSwitch = channels[CH_ARM] > 1500;

    // Only arm when the switch is on AND the sticks are centered,
    // so the robot never lurches the moment it is armed.
    if (!link || !armSwitch) {
        armed = false;
    } else if (!armed && x == 0 && y == 0 && r == 0) {
        armed = true;
    }

    if (!link) {
        stopAllNow();  // failsafe: cut immediately, no ramp
    } else {
        int target[MOTOR_COUNT] = {0};
        if (armed) {
            // Holonomic mix (mecanum or 45-degree X-drive omni)
            float mix[MOTOR_COUNT];
            mix[LEFT_FRONT] = y + x + r;
            mix[RIGHT_FRONT] = y - x - r;
            mix[LEFT_BACK] = y - x + r;
            mix[RIGHT_BACK] = y + x - r;

            float maxMag = 1.0f;
            for (float m : mix) maxMag = max(maxMag, fabsf(m));
            for (int i = 0; i < MOTOR_COUNT; i++) {
                target[i] = (int)(mix[i] / maxMag * MAX_DUTY);
            }
        }
        driveMotors(target);
    }

    static uint32_t lastPrint = 0;
    if (millis() - lastPrint >= 200) {
        lastPrint = millis();
        Serial.printf("link=%d armed=%d ch1=%4u ch2=%4u ch4=%4u ch5=%4u | LF=%4d RF=%4d LB=%4d RB=%4d\n",
                      link, armed, channels[0], channels[1], channels[3], channels[4],
                      currentDuty[LEFT_FRONT], currentDuty[RIGHT_FRONT],
                      currentDuty[LEFT_BACK], currentDuty[RIGHT_BACK]);
    }
}

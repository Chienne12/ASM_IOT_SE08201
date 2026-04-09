#define BLYNK_TEMPLATE_ID   "TMPL6TU9L6OPk"
#define BLYNK_TEMPLATE_NAME "IoT Sorter"
#define BLYNK_AUTH_TOKEN    "CGgyjqgCgKT6TcrXhkQsqRhe_nXbi-mT"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "Chien";
const char* WIFI_PASS = "12345678";

#define V_AUTO    V0
#define V_ESTOP   V1
#define V_STATUS  V2
#define V_COLOR   V3
#define V_RED     V4
#define V_GREEN   V5
#define V_BLUE    V6
#define V_TOTAL   V7

#define IR_PIN    34
#define PIN_BASE  13
#define PIN_UPPER 14
#define PIN_LOWER 18
#define PIN_GRIP  19

Servo s1, s2, s3, s4;

bool autoMode = true;
int  cntRed = 0, cntGreen = 0, cntBlue = 0;

BLYNK_CONNECTED() {
    autoMode = true;
    Blynk.virtualWrite(V_AUTO,   1);
    Blynk.virtualWrite(V_STATUS, "READY");
    Blynk.virtualWrite(V_COLOR,  "---");
    Blynk.virtualWrite(V_RED,    cntRed);
    Blynk.virtualWrite(V_GREEN,  cntGreen);
    Blynk.virtualWrite(V_BLUE,   cntBlue);
    Blynk.virtualWrite(V_TOTAL,  cntRed + cntGreen + cntBlue);
    Serial.println("[Blynk] Connected!");
}

BLYNK_WRITE(V_AUTO) {
    autoMode = (param.asInt() == 1);
    Blynk.virtualWrite(V_STATUS, autoMode ? "RUNNING" : "STOPPED");
    Serial.printf("[Blynk] autoMode=%d\n", autoMode);
}

BLYNK_WRITE(V_ESTOP) {
    if (param.asInt() == 1) {
        cntRed = cntGreen = cntBlue = 0;
        Blynk.virtualWrite(V_RED,    0);
        Blynk.virtualWrite(V_GREEN,  0);
        Blynk.virtualWrite(V_BLUE,   0);
        Blynk.virtualWrite(V_TOTAL,  0);
        Blynk.virtualWrite(V_COLOR,  "---");
        Blynk.virtualWrite(V_STATUS, "RESET");
        Serial.println("[Blynk] RESET");
    }
}

void goHome() {
    s3.write(80);  delay(600);
    s2.write(140); delay(800);
    s1.write(90);  delay(600);
    s4.write(10);  delay(500);
}

void pickObject() {
    s3.write(100); delay(1000);
    s2.write(115); delay(2000);
    s4.write(150); delay(1000);
}

void liftUp() {
    s2.write(140); delay(2000);
    s3.write(80);  delay(1000);
}

void dropRight() {
    s1.write(170); delay(1000);
    s3.write(100); delay(1000);
    s2.write(120); delay(2000);
    s4.write(10);  delay(1000);
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== IoT Sorter Boot ===");

    pinMode(IR_PIN, INPUT);

    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    s1.setPeriodHertz(50); s1.attach(PIN_BASE,  500, 2400);
    s2.setPeriodHertz(50); s2.attach(PIN_UPPER, 500, 2400);
    s3.setPeriodHertz(50); s3.attach(PIN_LOWER, 500, 2400);
    s4.setPeriodHertz(50); s4.attach(PIN_GRIP,  500, 2400);

    goHome();
    Serial.println("[OK] Servo HOME");

    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
    Serial.println("========== System Ready ==========");
}

void loop() {
    Blynk.run();

    if (!autoMode) { delay(200); return; }

    if (digitalRead(IR_PIN) == LOW) {
        delay(300);
        if (digitalRead(IR_PIN) != LOW) return;
Blynk.virtualWrite(V_STATUS, "PICKING");
        Serial.println("[IR] Co vat -> GAP!");

        pickObject();
        liftUp();
        dropRight();
        goHome();

        // Chưa có TCS → đếm vào total, chờ phân màu sau
        cntRed++;   // tạm đếm vào Red đến khi có TCS
        Blynk.virtualWrite(V_RED,   cntRed);
        Blynk.virtualWrite(V_TOTAL, cntRed + cntGreen + cntBlue);
        Blynk.virtualWrite(V_STATUS, "RUNNING");
        Serial.printf("[OK] Xong — tong: %d\n", cntRed + cntGreen + cntBlue);

        unsigned long t0 = millis();
        while (digitalRead(IR_PIN) == LOW && millis() - t0 < 5000) delay(50);
        delay(1000);
    }

    delay(30);
}
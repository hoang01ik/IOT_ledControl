#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"

// Cấu hình phần cứng
#define LED_PIN 18
#define FAN_PIN 22
#define PUMP_PIN 21
#define BUZZER_PIN 19

// Cấu hình WiFi
const char* WIFI_SSID = "Huong Tang 1";
const char* WIFI_PASSWORD = "68686868";

// Cấu hình Firebase
const char* API_KEY = "AIzaSyDCsHgRfDraiMLGlV9BFQ7IPIBOuYbpX9Y";
const char* DATABASE_URL = "https://hand-c4356-default-rtdb.firebaseio.com";
const char* FIREBASE_PATH_fingers_right = "/deviceControls/esp32_01/fingers";
const char* FIREBASE_PATH_fingers_left = "/deviceControls/esp32_01/brightness";

FirebaseData fbdoRight;
FirebaseData fbdoLeft;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastTokenRefresh = 0;
const unsigned long TOKEN_REFRESH_INTERVAL = 3300000;  // 55 phút

// Hàm điều khiển thiết bị
void controlDevices(uint8_t fingers_right, uint8_t fingers_left) {
  if (fingers_right == 1) digitalWrite(LED_PIN, HIGH);
  if (fingers_right == 2) digitalWrite(FAN_PIN, HIGH);
  if (fingers_right == 3) digitalWrite(PUMP_PIN, HIGH);
  if (fingers_right == 4) digitalWrite(BUZZER_PIN, HIGH);
  if (fingers_right == 5) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(FAN_PIN, HIGH);
    digitalWrite(PUMP_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  }

  if (fingers_left == 1) digitalWrite(LED_PIN, LOW);
  if (fingers_left == 2) digitalWrite(FAN_PIN, LOW);
  if (fingers_left == 3) digitalWrite(PUMP_PIN, LOW);
  if (fingers_left == 4) digitalWrite(BUZZER_PIN, LOW);
  if (fingers_left == 5) {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    digitalWrite(PUMP_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  Serial.printf("Right: %d | Left: %d | LED: %d | FAN: %d | PUMP: %d | BUZZER: %d | Heap: %d\n",
                fingers_right, fingers_left,
                digitalRead(LED_PIN),
                digitalRead(FAN_PIN),
                digitalRead(PUMP_PIN),
                digitalRead(BUZZER_PIN),
                ESP.getFreeHeap());
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = "halongwiz2003@gmail.com";
  auth.user.password = "Halongdz@2003";
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Cấu hình tối ưu
  fbdoRight.setBSSLBufferSize(1024, 1024);
  fbdoRight.setResponseSize(256);
  fbdoLeft.setBSSLBufferSize(1024, 1024);
  fbdoLeft.setResponseSize(256);
}

bool initRightStream() {
  if (Firebase.RTDB.beginStream(&fbdoRight, FIREBASE_PATH_fingers_right)) {
    Firebase.RTDB.setStreamCallback(
      &fbdoRight,
      [](FirebaseStream data) {
        if (data.dataTypeEnum() == fb_esp_rtdb_data_type_integer) {
          uint8_t val = constrain(data.intData(), 0, 5);
          controlDevices(val, 0); // chỉ cập nhật tay phải
        }
      },
      [](bool timeout) {
        if (timeout && Firebase.ready()) {
          Serial.println("Right hand stream timeout. Reconnecting...");
          initRightStream();
        }
      });
    Serial.println("Right stream initialized");
    return true;
  }
  Serial.println("Right stream failed: " + fbdoRight.errorReason());
  return false;
}

bool initLeftStream() {
  if (Firebase.RTDB.beginStream(&fbdoLeft, FIREBASE_PATH_fingers_left)) {
    Firebase.RTDB.setStreamCallback(
      &fbdoLeft,
      [](FirebaseStream data) {
        if (data.dataTypeEnum() == fb_esp_rtdb_data_type_integer) {
          uint8_t val = constrain(data.intData(), 0, 5);
          controlDevices(0, val); // chỉ cập nhật tay trái
        }
      },
      [](bool timeout) {
        if (timeout && Firebase.ready()) {
          Serial.println("Left hand stream timeout. Reconnecting...");
          initLeftStream();
        }
      });
    Serial.println("Left stream initialized");
    return true;
  }
  Serial.println("Left stream failed: " + fbdoLeft.errorReason());
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi connected: " + WiFi.localIP().toString());

  setupFirebase();
  while (!Firebase.ready()) delay(500);

  initRightStream();
  initLeftStream();
}

void loop() {
  // Tự động refresh token
  if (millis() - lastTokenRefresh > TOKEN_REFRESH_INTERVAL) {
    Firebase.refreshToken(&config);
    lastTokenRefresh = millis();
    Serial.println("Token refreshed");
  }

  if (!Firebase.ready() || WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting...");
    setup();
    return;
  }

  delay(10);
}

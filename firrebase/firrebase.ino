#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"

// Cấu hình phần cứng
const uint8_t LED_PINS[] = {18, 22, 21, 19};
const uint8_t LED_COUNT = sizeof(LED_PINS);

// Cấu hình WiFi
const char* WIFI_SSID = "Khoa CNTT";
const char* WIFI_PASSWORD = "";

// Cấu hình Firebase
const char* API_KEY = "AIzaSyDCsHgRfDraiMLGlV9BFQ7IPIBOuYbpX9Y";
const char* DATABASE_URL = "https://hand-c4356-default-rtdb.firebaseio.com";
const char* FIREBASE_PATH_fingers_right = "/deviceControls/esp32_01/fingers";
const char* FIREBASE_PATH_fingers_left = "/deviceControls/esp32_01/brightness";
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastTokenRefresh = 0;
const unsigned long TOKEN_REFRESH_INTERVAL = 3300000; // 55 phút (nhỏ hơn 1h)

void IRAM_ATTR controlLEDs(uint8_t fingers) {
  for(uint8_t i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], i < fingers ? HIGH : LOW);
  }
  Serial.printf("LEDs set: %d fingers | Heap: %d\n", fingers, ESP.getFreeHeap());
}

void setupFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = "halongwiz2003@gmail.com";
  auth.user.password = "Halongdz@2003";
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Cấu hình tối ưu stream
  fbdo.setBSSLBufferSize(1024, 1024);
  fbdo.setResponseSize(256);
}

bool initStream() {
  if(Firebase.RTDB.beginStream(&fbdo, FIREBASE_PATH_fingers_right)) {
    Firebase.RTDB.setStreamCallback(&fbdo, [](FirebaseStream data){
      if(data.dataTypeEnum() == fb_esp_rtdb_data_type_integer) {
        controlLEDs(constrain(data.intData(), 0, LED_COUNT));
      }
    }, [](bool timeout){
      if(timeout && Firebase.ready()) {
        Serial.println("Stream timeout. Reconnecting...");
        initStream();
      }
    });
    return true;
  }
  Serial.println("Stream error: " + fbdo.errorReason());
  return false;
}

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo LED
  for(uint8_t i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  // Kết nối WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi connected: " + WiFi.localIP().toString());

  // Khởi tạo Firebase
  setupFirebase();
  while(!Firebase.ready()) delay(500);
  
  // Khởi tạo stream
  if(!initStream()) {
    delay(1000);
    initStream();
  }
}

void loop() {
  // Tự động refresh token trước khi hết hạn
  if(millis() - lastTokenRefresh > TOKEN_REFRESH_INTERVAL) {
    Firebase.refreshToken(&config);
    lastTokenRefresh = millis();
    Serial.println("Token refreshed");
  }

  // Kiểm tra kết nối
  if(!Firebase.ready() || WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting...");
    setup();
    return;
  }
  
  delay(10);
}
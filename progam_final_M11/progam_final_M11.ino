#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <FirebaseESP8266.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// === Pin Konfigurasi ===
#define WIFI_RESET_PIN 2 // D4 (pastikan tidak nyangkut dengan boot mode)
#define Relay1 14        // D5
#define Relay2 5         // D1
#define Relay3 4         // D2
#define Relay4 13        // D7
#define BTN_PLUS 12      // D6
#define BTN_MINUS 16     // D0

// === EEPROM Config ===
#define EEPROM_SIZE 512
#define EEPROM_FLAG_ADDR 0
#define EEPROM_URL_ADDR 1
#define EEPROM_SECRET_ADDR 101
#define EEPROM_FLAG_VALID 123

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

char firebase_url[100];
char firebase_secret[100];

int val1, val2, val3, val4;
int currentSpeed = 1;  // posisi kecepatan saat ini
int targetSpeed = 1;   // target kecepatan dari Firebase

const int maxSpeed = 120;
const int minSpeed = 1;
const int pressTime = 100;          // ms
const int delayBetweenSteps = 300;  // ms

unsigned long lastSpeedUpdate = 0;  // untuk non-blocking

WiFiManager wm;

// === EEPROM Handling ===
void simpanKeEEPROM() {
  bool perluSimpan = false;

  for (int i = 0; i < 100; i++) {
    if (EEPROM.read(EEPROM_URL_ADDR + i) != firebase_url[i]) {
      EEPROM.write(EEPROM_URL_ADDR + i, firebase_url[i]);
      perluSimpan = true;
    }
  }
  for (int i = 0; i < 100; i++) {
    if (EEPROM.read(EEPROM_SECRET_ADDR + i) != firebase_secret[i]) {
      EEPROM.write(EEPROM_SECRET_ADDR + i, firebase_secret[i]);
      perluSimpan = true;
    }
  }

  if (perluSimpan) {
    EEPROM.write(EEPROM_FLAG_ADDR, EEPROM_FLAG_VALID);
    EEPROM.commit();
    Serial.println("EEPROM diperbarui dengan data baru.");
  } else {
    Serial.println("Data EEPROM masih sama. Tidak disimpan ulang.");
  }
}

bool dataEEPROMValid() {
  return EEPROM.read(EEPROM_FLAG_ADDR) == EEPROM_FLAG_VALID;
}

void bacaDariEEPROM() {
  if (!dataEEPROMValid()) {
    Serial.println("Data EEPROM tidak valid. Perlu konfigurasi ulang.");
    firebase_url[0] = '\0';
    firebase_secret[0] = '\0';
    return;
  }

  for (int i = 0; i < 100; i++) {
    firebase_url[i] = EEPROM.read(EEPROM_URL_ADDR + i);
    firebase_secret[i] = EEPROM.read(EEPROM_SECRET_ADDR + i);
  }
  Serial.println("Data Firebase dibaca dari EEPROM.");
}

// === Tombol Reset WiFi ===
void cekResetWifi() {
  static unsigned long tombolTekanStart = 0;
  static bool tombolSebelumnya = HIGH;

  bool tombolSekarang = digitalRead(WIFI_RESET_PIN);

  if (tombolSekarang == LOW && tombolSebelumnya == HIGH) {
    tombolTekanStart = millis();
  } else if (tombolSekarang == LOW && tombolSebelumnya == LOW) {
    if (millis() - tombolTekanStart > 3000) {
      Serial.println("Tombol reset WiFi ditekan > 3 detik. Reset WiFi...");
      wm.resetSettings();
      delay(500);
      ESP.restart();
    }
  }

  tombolSebelumnya = tombolSekarang;
}

// === SETUP ===
void setup() {
  Serial.begin(115200);

  pinMode(WIFI_RESET_PIN, INPUT_PULLUP);
  pinMode(Relay1, OUTPUT); digitalWrite(Relay1, HIGH);
  pinMode(Relay2, OUTPUT); digitalWrite(Relay2, HIGH);
  pinMode(Relay3, OUTPUT); digitalWrite(Relay3, HIGH);
  pinMode(Relay4, OUTPUT); digitalWrite(Relay4, HIGH);
  pinMode(BTN_PLUS, OUTPUT); digitalWrite(BTN_PLUS, LOW);
  pinMode(BTN_MINUS, OUTPUT); digitalWrite(BTN_MINUS, LOW);

  EEPROM.begin(EEPROM_SIZE);
  bacaDariEEPROM();

  WiFiManagerParameter custom_html("<h3>🔥 Konfigurasi Firebase</h3>");
  WiFiManagerParameter custom_url("url", "Firebase URL", firebase_url, 100);
  WiFiManagerParameter custom_secret("secret", "Firebase Secret", firebase_secret, 100);

  wm.addParameter(&custom_html);
  wm.addParameter(&custom_url);
  wm.addParameter(&custom_secret);

  if (!dataEEPROMValid()) {
    wm.resetSettings();
  }

  if (!wm.autoConnect("Bakar", "12345678")) {
    Serial.println("Gagal konek WiFi. Restart...");
    delay(2000);
    ESP.restart();
  }

  strcpy(firebase_url, custom_url.getValue());
  strcpy(firebase_secret, custom_secret.getValue());
  simpanKeEEPROM();

  Serial.println("WiFi terhubung ke: " + WiFi.SSID());
  Serial.println("IP: " + WiFi.localIP().toString());

  config.database_url = firebase_url;
  config.signer.tokens.legacy_token = firebase_secret;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Setup selesai.");
}

// === LOOP ===
void loop() {
  cekResetWifi();

  if (!Firebase.ready()) {
    Serial.println("Firebase belum siap.");
    delay(1000);
    return;
  }

  // === Kontrol Relay ===
  if (Firebase.getJSON(fbdo, "/IoT")) {
    String data = fbdo.to<FirebaseJson>().raw();
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, data);

    if (!error) {
      val1 = doc["L1"];
      val2 = doc["L2"];
      val3 = doc["L3"];
      val4 = doc["L4"];

      digitalWrite(Relay1, val1 == 1 ? LOW : HIGH);
      digitalWrite(Relay2, val2 == 1 ? LOW : HIGH);
      digitalWrite(Relay3, val3 == 1 ? LOW : HIGH);
      digitalWrite(Relay4, val4 == 1 ? LOW : HIGH);

      Serial.printf("Relay1: %s | Relay2: %s | Relay3: %s | Relay4: %s\n",
                    val1 ? "ON" : "OFF", val2 ? "ON" : "OFF",
                    val3 ? "ON" : "OFF", val4 ? "ON" : "OFF");
    } else {
      Serial.println("Gagal parsing JSON Firebase.");
    }
  } else {
    Serial.println("Error Firebase (Relay): " + fbdo.errorReason());
  }

  // === Ambil target kecepatan motor dari Firebase ===
  if (Firebase.getString(fbdo, "/IoT/kecepatan_motor")) {
    String speedStr = fbdo.stringData();
    Serial.print("Data string dari Firebase: ");
    Serial.println(speedStr);

    if (speedStr.length() > 0) {
      targetSpeed = constrain(speedStr.toInt(), minSpeed, maxSpeed);
      Serial.print("Target kecepatan konversi int: ");
      Serial.println(targetSpeed);
    }
  } else {
    Serial.print("Gagal membaca Firebase: ");
    Serial.println(fbdo.errorReason());
  }

  // === Update kecepatan motor secara non-blocking ===
  updateMotorSpeed();

  delay(50); // loop cepat tapi tetap ada delay kecil
}

// === Fungsi non-blocking update kecepatan motor ===
void updateMotorSpeed() {
  unsigned long now = millis();

  if (now - lastSpeedUpdate >= delayBetweenSteps) {
    if (targetSpeed > currentSpeed) {
      Serial.printf("Naik step: %d -> %d\n", currentSpeed, currentSpeed + 1);
      pressButton(BTN_PLUS, pressTime);
      currentSpeed++;
      lastSpeedUpdate = now;
    } else if (targetSpeed < currentSpeed) {
      Serial.printf("Turun step: %d -> %d\n", currentSpeed, currentSpeed - 1);
      pressButton(BTN_MINUS, pressTime);
      currentSpeed--;
      lastSpeedUpdate = now;
    }
  }
}

// === Fungsi untuk menekan tombol ===
void pressButton(int pin, int durationMs) {
  Serial.printf("Menekan tombol di pin %d\n", pin);
  digitalWrite(pin, HIGH);
  delay(durationMs);
  digitalWrite(pin, LOW);
}

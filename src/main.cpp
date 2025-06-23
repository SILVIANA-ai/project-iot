#include <TinyGPS++.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// The TinyGPS++ object
TinyGPSPlus gps;

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial gpsSerial(2);
Adafruit_MPU6050 mpu;

#define GPS_RX 16 // RX ESP32 ke TX GPS
#define GPS_TX 17 // TX ESP32 ke RX GPS

#define GPS_BAUD 9600

String chat_id = "1351783862"; 
String bot_token = "7857716095:AAEphwz658BW-1ekgPh6ybc_ftnWB0jK2Lc";

bool threatDetected = false;

void sendMessage(String message);

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Wire.begin();

  if (!mpu.begin()) {
    Serial.println("MPU6050 tidak ditemukan!");
    while (1);
  }
}

// Fungsi membaca GPS
void bacaGPS() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

// Kirim pesan ke Telegram
void sendMessage(String message) {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + bot_token + "/sendMessage?chat_id=" + chat_id + "&text=" + message;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.printf("Telegram Response code: %d\n", httpCode);
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.printf("Telegram HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send Telegram message.");
  }
}

// Deteksi gerakan sebagai ancaman
void deteksiGerakan() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (abs(a.acceleration.x) > 8 || abs(a.acceleration.y) > 8 || abs(a.acceleration.z) > 15) {
    threatDetected = true;
  } else {
    threatDetected = false;
  }
}

// Kirim lokasi ke user
void kirimLokasi() {
  if (gps.location.isValid()) {
    String latitude = String(gps.location.lat(), 6);
    String longitude = String(gps.location.lng(), 6);
    String message = "📍 Lokasi Ternak:\https://www.google.com/maps?q=" + latitude + "," + longitude;
    sendMessage(message);
  } else {
    sendMessage("⚠️ Gagal mendapatkan lokasi GPS.");
  }
}

// Cek apakah user meminta lokasi ternak via Telegram
void cekPermintaanUser() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + bot_token + "/getUpdates?limit=1&offset=-1";
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      payload.toLowerCase();
      if (payload.indexOf("lokasi") >= 0) {
        kirimLokasi();
      }
    } else {
      Serial.printf("Telegram getUpdates failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot check Telegram updates.");
  }
}

void loop() {
  bacaGPS();
  deteksiGerakan();
  cekPermintaanUser();

  if (threatDetected) {
    sendMessage("⚠️ Deteksi ancaman gerakan tidak biasa pada ternak!");
    kirimLokasi();
    threatDetected = false;
  }
  delay(5000); // Monitoring setiap 5 detik
}
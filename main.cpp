#include <HardwareSerial.h>
#include <TinyGPS++.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

HardwareSerial sim800(1); // UART1 untuk SIM800
HardwareSerial gpsSerial(2); // UART2 untuk GPS

TinyGPSPlus gps;
Adafruit_MPU6050 mpu;

#define SIM800_RX 18 // RX ESP32 ke TX SIM800
#define SIM800_TX 19 // TX ESP32 ke RX SIM800
#define GPS_RX 17      // RX ESP32 ke TX GPS
#define GPS_TX 16    // TX ESP32 ke RX GPS

String chat_id = "1351783862"; 
String bot_token = "7857716095:AAEphwz658BW-1ekgPh6ybc_ftnWB0jK2Lc";

bool threatDetected = false;

void sendMessage(String message);

void setup() {
  Serial.begin(115200);
  sim800.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Wire.begin();

  if (!mpu.begin()) {
    Serial.println("MPU6050 tidak ditemukan!");
    while (1);
  }

  // Tunggu SIM800 merespons AT
  bool simReady = false;
  Serial.println("Mengecek koneksi SIM800...");
  for (int i = 0; i < 5; i++) {
    sim800.println("AT");
    delay(500);

    while (sim800.available()) {
      String response = sim800.readStringUntil('\n');
      Serial.println("Respon SIM800: " + response);
      if (response.indexOf("OK") >= 0) {
        simReady = true;
        break;
      }
    }

    if (simReady) break;
  }

  if (simReady) {
    delay(2000); // Pastikan SIM800 siap penuh
    sendMessage("🚨 Sistem Smart Necklace Sheep aktif.");
  } else {
    Serial.println("❌ SIM800 tidak merespon. Cek koneksi dan power.");
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
  sim800.println("AT+HTTPTERM");
  delay(100);
  sim800.println("AT+HTTPINIT");
  delay(100);
  sim800.println("AT+HTTPPARA=\"CID\",1");
  delay(100);

  String url = "https://api.telegram.org/bot" + bot_token + "/sendMessage?chat_id=" + chat_id + "&text=" + message;
  sim800.println("AT+HTTPPARA=\"URL\",\"" + url + "\"");
  delay(500);
  sim800.println("AT+HTTPACTION=0");
  delay(5000);
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
    String message = "📍 Lokasi Ternak:\nhttps://www.google.com/maps?q=" + latitude + "," + longitude;
    sendMessage(message);
  } else {
    sendMessage("⚠️ Gagal mendapatkan lokasi GPS.");
  }
}

// Cek apakah user meminta lokasi ternak via Telegram
void cekPermintaanUser() {
  while (sim800.available()) {
    String response = sim800.readString();
    response.toLowerCase(); // Supaya tidak case sensitive
    if (response.indexOf("lokasi") >= 0) {
      kirimLokasi();
    }
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

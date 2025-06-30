#include <TinyGPS++.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// The TinyGPS++ object
TinyGPSPlus gps;

// Create an instance of the HardwareSerial class for Serial 2
HardwareSerial gpsSerial(2);
Adafruit_MPU6050 mpu;

#define GPS_RX 16 // RX ESP32 ke TX GPS
#define GPS_TX 17 // TX ESP32 ke RX GPS

#define GPS_BAUD 9600

#define TRIGGER_PIN 0

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool wm_nonblocking = false; // change to true to use non blocking

WiFiManager wm; // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )


String chat_id = "7057234968"; 
String bot_token = "7559166548:AAHi19aDlfC2oOaQWQNgnMA4vR-tLBlG89g";

bool threatDetected = false;

void sendMessage(String message);

void checkButton(){
  // check for button press
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if( digitalRead(TRIGGER_PIN) == LOW ){
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(10000); // reset delay hold
      if( digitalRead(TRIGGER_PIN) == LOW ){
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);
      
      if (!wm.startConfigPortal("SmartSheepIoT","kambing")) {
        Serial.println("Gagal terhubung ke WiFi");
        delay(3000);
        // ESP.restart();
      } else {
        //if you get here you have connected to the WiFi
        Serial.println("Tersambung ke WiFi");
      }
    }
  }
}


String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
}

void WifiManager() {
  pinMode(TRIGGER_PIN, INPUT);
  
  // wm.resetSettings(); // wipe settings

  if(wm_nonblocking) wm.setConfigPortalBlocking(false);

  // add a custom input field
  int customFieldLength = 40;


  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");
  
  // test custom html input type(checkbox)
  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
  
  // test custom html(radio)
  const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
  new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
  
  wm.addParameter(&custom_field);
  wm.setSaveParamsCallback(saveParamCallback);

  // custom menu via array or vector
  // 
  // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
  // const char* menu[] = {"wifi","info","param","sep","restart","exit"}; 
  // wm.setMenu(menu,6);
  std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  wm.setMenu(menu);

  // set dark theme
  wm.setClass("invert");


  //set static ip
  // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
  // wm.setShowStaticFields(true); // force show static ip fields
  // wm.setShowDnsFields(true);    // force show dns field always

  // wm.setConnectTimeout(20); // how long to try to connect for before continuing
  wm.setConfigPortalTimeout(180); // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  // wm.setAPClientCheck(true); // avoid timeout if client connected to softap

  // wifi scan settings
  // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
  // wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
  // wm.setShowInfoErase(false);      // do not show erase button on info page
  // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons
  
  // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("SmartSheepIoT","kambing123"); // password protected ap

  if(!res) {
    Serial.println("Gagal terhubung atau waktu habis");
    // ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("Tersambung ke WiFi...");
  }
}

void setup() {
  Serial.begin(115200);

  WifiManager();
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

void cekGerakan() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Membuat pesan untuk Telegram
  String pesan = "Accelerometer:\n";
  pesan += "X: " + String(a.acceleration.x, 2) + " m/s^2\n";
  pesan += "Y: " + String(a.acceleration.y, 2) + " m/s^2\n";
  pesan += "Z: " + String(a.acceleration.z, 2) + " m/s^2\n";
  pesan += "Gyroscope:\n";
  pesan += "X: " + String(g.gyro.x, 2) + " rad/s\n";
  pesan += "Y: " + String(g.gyro.y, 2) + " rad/s\n";
  pesan += "Z: " + String(g.gyro.z, 2) + " rad/s";

  sendMessage(pesan);
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

int lastUpdateId = 0;

// Cek apakah user meminta lokasi ternak via Telegram
void cekPermintaanUser() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + bot_token + "/getUpdates?offset=" + String(lastUpdateId + 1);
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Payload dari Telegram:");
      Serial.println(payload);

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error && doc["result"].size() > 0) {
        int updateId = doc["result"][0]["update_id"];
        if (updateId != lastUpdateId) {
          lastUpdateId = updateId;

          String messageText = doc["result"][0]["message"]["text"];
          Serial.print("Perintah dari user: ");
          Serial.println(messageText);

          messageText.toLowerCase();

          if (messageText == "/kirimlokasi") {
            kirimLokasi();
          } else if (messageText == "/cekkondisi") {
            deteksiGerakan();
            if (!threatDetected) {
              sendMessage("✅ Tidak Ada Bahaya.");
            } else {
              sendMessage("⚠️ Ada potensi ancaman gerakan.");
              kirimLokasi();
            }
          }
        } else {
          Serial.println("Perintah sama dengan sebelumnya, diabaikan.");
        }
      } else {
        Serial.println("Gagal parsing JSON atau tidak ada perintah baru.");
      }
    } else {
      Serial.printf("Telegram getUpdates failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi tidak terhubung.");
  }
}
static long lastMillis = 0;
const long interval = 15000; // 15 detik

void loop() {
  if(wm_nonblocking) wm.process(); // avoid delays() in loop when non-blocking and other long running code  
  checkButton();

  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= interval) {
      lastMillis = currentMillis;
      // put your main code here, to run repeatedly:
  bacaGPS();
  cekGerakan();
  deteksiGerakan();
  cekPermintaanUser();

  if (threatDetected) {
    sendMessage("⚠️ Deteksi ancaman gerakan tidak biasa pada ternak!");
    kirimLokasi();
    threatDetected = false;
  }
    Serial.println("Melakukan tugas setiap 15 detik...");
  }

}
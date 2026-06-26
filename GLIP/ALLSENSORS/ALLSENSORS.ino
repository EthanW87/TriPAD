#include "MPU6050.h"
#include <Wire.h>
#include <math.h>
#include "PMS.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

MPU6050 mpu;
HardwareSerial pmsSerial(2);
PMS pms(pmsSerial);
PMS::DATA data;

// Pin definitions
#define MIC_PIN 35

// Vibration thresholds (CCOHS)
#define VIBRATION_WARNING 2.5
#define VIBRATION_DANGER 5.0

// Noise thresholds (CCOHS)
#define NOISE_WARNING 80.0
#define NOISE_LIMIT 85.0

// Dust thresholds (WHO)
#define PM25_WARNING 15.0
#define PM25_DANGER 25.0

#define PM10_WARNING 45.0
#define PM10_DANGER 50.0

#define PM1_WARNING 15.0

#define WIFI_SSID "Rogers 1509"
#define WIFI_PASSWORD "hsu22200"

#define DATABASE_URL "https://glip-data-storage-default-rtdb.firebaseio.com/"
#define DATABASE_SECRET "3AeNxjTWc84tYS0gKJgNR5ATq1asQxIRBAmMdMu5"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const int buttonPin = 26;
int buttonState = 0;

float offsetX = 0.86, offsetY = 0.5, offsetZ = -11;

unsigned long lastNoiseUpdate = 0;
float sumSquares = 0;
long sampleCount = 0;
float currentDBA = 0.0; 

// Global variable to pace the Serial Monitor printing
unsigned long lastPrintTime = 0;

void updateNoiseNonBlocking() {
  int raw = analogRead(MIC_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float acSignal = voltage - 1.096; 
  
  sumSquares += (acSignal * acSignal);
  sampleCount++;

  if (millis() - lastNoiseUpdate >= 1000) {
    float rmsVoltage = sqrt(sumSquares / sampleCount);
    if (rmsVoltage < 0.0001) rmsVoltage = 0.0001;

    float dBA = 20.0 * log10(rmsVoltage) + 79.1;
    if (dBA <= 45.5){
      dBA = dBA - 13.0;
      if (dBA < 32.0) dBA = 32.0;
    }
    currentDBA = dBA; 
    sumSquares = 0;
    sampleCount = 0;
    lastNoiseUpdate = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Start the Wi-Fi connection
  WiFi.begin(ssid, password);

  // Wait until the ESP32 successfully connects to the router
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Connection successful
  Serial.println("");
  Serial.println("Wi-Fi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // Prints the local IP assigned to your ESP32

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;  

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase connected");

  pinMode(buttonPin, INPUT_PULLUP);
  
  
  pmsSerial.begin(9600, SERIAL_8N1, 16, 17); 
  
  Wire.begin(21, 22);
  delay(500);
  mpu.initialize();

  lastNoiseUpdate = millis();
  lastPrintTime = millis();
}

void loop() {
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW){

    updateNoiseNonBlocking();

    bool newPMDataAvailable = pms.read(data);

    if (millis() - lastPrintTime >= 1000) {
      lastPrintTime = millis(); 

      int16_t ax, ay, az;
      mpu.getAcceleration(&ax, &ay, &az);

      float axf = (ax / 16384.0 * 9.8) + offsetX;
      float ayf = (ay / 16384.0 * 9.8) + offsetY;
      float azf = (az / 16384.0 * 9.8) + offsetZ;
      float magnitude = sqrt(axf * axf + ayf * ayf + azf * azf);

      Serial.print("VIBRATION | ");
      Serial.print(magnitude, 2);
      Serial.print(" m/s²");

      if (magnitude >= VIBRATION_DANGER) {
        Serial.println(" ALERT: VIBRATION EXCEEDS 5.0 m/s²");
      } 
      else if (magnitude >= VIBRATION_WARNING) {
        Serial.println(" | >>> WARNING: EXCEEDS 2.5 m/s² <<<");
      } 
      else {
        Serial.println(" | STATUS: NORMAL");
      }

      Serial.print("NOISE     | ");
      Serial.print(currentDBA, 1);
      Serial.print(" dBA");

      if (currentDBA >= NOISE_LIMIT) {
        Serial.println(" | >>> ALERT: EXCEEDS 85 dBA <<<");
      } else if (currentDBA >= NOISE_WARNING) {
        Serial.println(" | >>> WARNING: APPROACHING 85 dBA <<<");
      } else {
        Serial.println(" | STATUS: NORMAL");
      }

      Serial.print("DUST METRICS | ");
      Serial.printf("PM1.0: %u ug/m³ | PM2.5: %u ug/m³ | PM10: %u ug/m³\n", 
                    data.PM_SP_UG_1_0, data.PM_SP_UG_2_5, data.PM_SP_UG_10_0);
      Serial.print("PM 2.5 STATUS:");
      if (data.PM_SP_UG_2_5 >= PM25_DANGER){
        Serial.println("ALERT: EXCEEDS EXPOSURE LIMIT (25 ug/m³)\n");
      }
      else if (data.PM_SP_UG_2_5 >= PM25_WARNING){
        Serial.println("WARNING: ELEVATED LEVELS (15 ug/m³)\n");
      }
      else{
        Serial.println("DUST LEVELS: NORMAL");
      }

      Serial.print("PM 10.-0 STATUS:");
      if (data.PM_SP_UG_10_0 >= PM10_DANGER){
        Serial.println("ALERT: EXCEEDS EXPOSURE LIMIT (50 ug/m³)\n");
      }
      else if (data.PM_SP_UG_10_0 >= PM10_WARNING){
        Serial.println("WARNING: ELEVATED LEVELS (45 ug/m³)\n");
      }
      else{
        Serial.println("DUST LEVELS: NORMAL");
      }

                    
      Serial.println("------------------------------------------");

      if (Firebase.ready()) {
      // We pass the actual live variables here instead of the test constants
      Firebase.RTDB.setFloat(&fbdo, "/tripAD/noise", currentDBA);
      Firebase.RTDB.setFloat(&fbdo, "/tripAD/vibration", magnitude);
      Firebase.RTDB.setInt(&fbdo, "/tripAD/pm25", data.PM_SP_UG_2_5);
      Firebase.RTDB.setInt(&fbdo, "/tripAD/pm10", data.PM_SP_UG_10_0);
      Firebase.RTDB.setInt(&fbdo, "/tripAD/pm1", data.PM_SP_UG_1_0);

      Serial.println(">>> Live data synced to Firebase! <<<");
    } else {
      Serial.println(">>> Firebase Error: Not Ready <<<");
    }
    }
  }
}
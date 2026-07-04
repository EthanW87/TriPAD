#include "MPU6050.h"
#include <Wire.h>
#include <math.h>
#include "PMS.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// --- New OLED Libraries ---
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // Common I2C address for 128x64 OLEDs
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

#define BOT_TOKEN "8886495077:AAHy_oqOITHPqDkuSWKE9S1nMoQ2MZfAzh0"
#define CHAT_ID "8015900696"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const int buttonPin = 26;
int buttonState = 0;

float offsetX = 0.86, offsetY = 0.5, offsetZ = -11;

unsigned long lastNoiseUpdate = 0;
float sumSquares = 0;
long sampleCount = 0;
float currentDBA = 0.0; 

// Global variables to hold the most recent VALID dust readings
uint16_t stablePM1_0 = 0;
uint16_t stablePM2_5 = 0;
uint16_t stablePM10_0 = 0;

// Global variable to pace the Serial Monitor printing
unsigned long lastPrintTime = 0;

bool vibrationAlertSent = false;
bool noiseAlertSent = false;
bool pm25AlertSent = false;
bool pm10AlertSent = false;
bool vibrationWarningSent = false;
bool noiseWarningSent = false;
bool pm25WarningSent = false;
bool pm10WarningSent = false;

// Global variables for display tracking
float globalMagnitude = 0.0;

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

// --- New function to handle screen drawing ---
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Title
  display.setCursor(0, 0);
  display.println("--- TriPAD METRICS ---");
  
  // Vibration Row
  display.setCursor(0, 16);
  display.printf("Vib: %.2f m/s2", globalMagnitude);
  if (globalMagnitude >= VIBRATION_DANGER) display.print(" !");
  
  // Noise Row
  display.setCursor(0, 28);
  display.printf("Noise: %.1f dBA", currentDBA);
  if (currentDBA >= NOISE_LIMIT) display.print(" !");
  
  // Dust Row
  display.setCursor(0, 40);
  display.printf("PM2.5: %u | PM10: %u", stablePM2_5, stablePM10_0);
  
  // System Status
  display.setCursor(0, 54);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("WiFi: Connected");
  } else {
    display.print("WiFi: Disconnected");
  }
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize I2C bus early so screen can show boot status
  Wire.begin(21, 22);
  delay(500);

  // --- Initialize OLED ---
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    // Don't freeze execution, but log it
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,20);
    display.println("Booting TriPAD...");
    display.display();
  }

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
  Serial.println(WiFi.localIP());

  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;  

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase connected");

  client.setInsecure();
  bot.sendMessage(CHAT_ID, "TriPAD Online - Monitoring Active", "");
  Serial.println("Telegram message sent");

  pinMode(buttonPin, INPUT_PULLUP);
  
  pmsSerial.begin(9600, SERIAL_8N1, 16, 17); 
  
  mpu.initialize();

  lastNoiseUpdate = millis();
  lastPrintTime = millis();
}

void loop() {

  // Continuously sample the microphone and compute RMS voltage
  updateNoiseNonBlocking();

  // Continuously read the PMS dust sensor stream into the data buffer
  bool newPMDataAvailable = pms.read(data);

  if (newPMDataAvailable) {
      stablePM1_0 = data.PM_SP_UG_1_0;
      stablePM2_5 = data.PM_SP_UG_2_5;
      stablePM10_0 = data.PM_SP_UG_10_0;
    }
  
  buttonState = digitalRead(buttonPin);
  
  // Only execute printing, uploading, and screen refreshes if the button is pressed
  if (buttonState == LOW) {

    // Pace the execution block to run exactly once per second
    if (millis() - lastPrintTime >= 1000) {
      lastPrintTime = millis(); 

      // Gather accelerometer data
      int16_t ax, ay, az;
      mpu.getAcceleration(&ax, &ay, &az);

      float axf = (ax / 16384.0 * 9.8) + offsetX;
      float ayf = (ay / 16384.0 * 9.8) + offsetY;
      float azf = (az / 16384.0 * 9.8) + offsetZ;
      globalMagnitude = sqrt(axf * axf + ayf * ayf + azf * azf); // assigned to global for OLED usage

      // --- Print Vibration Metrics ---
      Serial.print("VIBRATION | ");
      Serial.print(globalMagnitude, 2);
      Serial.print(" m/s²");
      if (globalMagnitude >= VIBRATION_DANGER) {
        Serial.println(" ALERT: VIBRATION EXCEEDS 5.0 m/s²"); 
        vibrationWarningSent = false; 
        if (!vibrationAlertSent){
          bot.sendMessage(CHAT_ID, "ALERT: VIBRATION EXCEEDS 5.0 m/s²", "");
          vibrationAlertSent = true;
        }
      } 
      else if (globalMagnitude >= VIBRATION_WARNING) {
        Serial.println("WARNING: EXCEEDS 2.5 m/s²");
        vibrationAlertSent = false;
        if (!vibrationWarningSent){
          bot.sendMessage(CHAT_ID, "WARNING: EXCEEDS 2.5 m/s²", "");
          vibrationWarningSent = true;
        }
      } 
      else {
        Serial.println(" | STATUS: NORMAL");
        vibrationAlertSent = false;
        vibrationWarningSent = false;
      }

      // --- Print Noise Metrics ---
      Serial.print("NOISE     | ");
      Serial.print(currentDBA, 1);
      Serial.print(" dBA");
      if (currentDBA >= NOISE_LIMIT) {
        Serial.println("ALERT: EXCEEDS 85 dBA");
        noiseWarningSent = false;
        if (!noiseAlertSent){
          bot.sendMessage(CHAT_ID, "ALERT: EXCEEDS 85 dBA", "");
          noiseAlertSent = true;
        }        
      } 
      else if (currentDBA >= NOISE_WARNING) {
        
        Serial.println("WARNING: APPROACHING 85 dBA");
        noiseAlertSent = false;
        if (!noiseWarningSent){
          bot.sendMessage(CHAT_ID, "WARNING: APPROACHING 85 dBA", "");
          noiseWarningSent = true;
        }
      } 
      else {
        Serial.println("STATUS: NORMAL");
        noiseAlertSent = false;
        noiseWarningSent = false;
      }

      // --- Print Dust Metrics ---
      Serial.print("DUST METRICS | ");
      Serial.printf("PM1.0: %u ug/m³ | PM2.5: %u ug/m³ | PM10: %u ug/m³\n", 
                    stablePM1_0, stablePM2_5, stablePM10_0);
                    
      Serial.print("PM 2.5 STATUS:");
      if (stablePM2_5 >= PM25_DANGER){
        Serial.println("ALERT: EXCEEDS EXPOSURE LIMIT (25 ug/m³)");
        pm25WarningSent = false;
        if (!pm25AlertSent){
          bot.sendMessage(CHAT_ID, "ALERT: EXCEEDS EXPOSURE LIMIT (25 ug/m³)\n", "");
          pm25AlertSent = true;
        }
      }
      else if (stablePM2_5 >= PM25_WARNING){
        Serial.println("WARNING: ELEVATED LEVELS (15 ug/m³)");
        pm25AlertSent = false;
        if (!pm25WarningSent){
          bot.sendMessage(CHAT_ID, "WARNING: ELEVATED LEVELS (15 ug/m³)\n", "");
          pm25WarningSent = true;
        }
      }
      else{
        Serial.println("DUST LEVELS: NORMAL");
        pm25AlertSent = false;
        pm25WarningSent = false;
      }

      Serial.print("PM 10.0 STATUS:");
      if (stablePM10_0 >= PM10_DANGER){
        Serial.println("ALERT: EXCEEDS EXPOSURE LIMIT (50 ug/m³)\n");
        pm10WarningSent = false;
        if (!pm10AlertSent){
          bot.sendMessage(CHAT_ID, "ALERT: EXCEEDS EXPOSURE LIMIT (50 ug/m³)", "");
          pm10AlertSent = true;
        }
      }
      else if (stablePM10_0 >= PM10_WARNING){
        Serial.println("WARNING: ELEVATED LEVELS (45 ug/m³)\n");
        pm10AlertSent = false;
        if (!pm10WarningSent){
          bot.sendMessage(CHAT_ID, "WARNING: ELEVATED LEVELS (45 ug/m³)", "");
          pm10WarningSent = true;
        }
      }
      else{
        Serial.println("DUST LEVELS: NORMAL");
        pm10AlertSent = false;
        pm10WarningSent = false;
      }       
      
      // --- Firebase Sync ---
      if (Firebase.ready()) {
        Firebase.RTDB.setFloat(&fbdo, "/tripAD/noise", currentDBA);
        Firebase.RTDB.setFloat(&fbdo, "/tripAD/vibration", globalMagnitude);
        Firebase.RTDB.setInt(&fbdo, "/tripAD/pm25", stablePM2_5);
        Firebase.RTDB.setInt(&fbdo, "/tripAD/pm10", stablePM10_0);
        Firebase.RTDB.setInt(&fbdo, "/tripAD/pm1", stablePM1_0);
        Serial.println(">>> Live data synced to Firebase! <<<");
      } else {
        Serial.println(">>> Firebase Error: Not Ready <<<");
      }
      
      // --- Refresh the OLED Screen with current data ---
      updateOLED();
      
      Serial.println("------------------------------------------");
    }
  }
}

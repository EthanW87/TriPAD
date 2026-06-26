#include "MPU6050.h"
#include <Wire.h>
#include <math.h>
#include "PMS.h"

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

// Calibration offsets for MPU6050
float offsetX = 0.86, offsetY = 0.5, offsetZ = -11;

// Global variables for Non-Blocking Noise Sampling
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
  
  pmsSerial.begin(9600, SERIAL_8N1, 16, 17); 
  Serial.println("--- PMS5003 Sensor Initialized ---");
  
  Wire.begin(21, 22);
  delay(500);
  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println(">>> MPU6050 ONLINE <<<");
  } else {
    Serial.println(">>> MPU6050 FAILED <<<");
    while (1);
  }
  
  lastNoiseUpdate = millis();
  lastPrintTime = millis();
}

void loop() {
  // Always update the microphone accumulator at maximum background speed
  updateNoiseNonBlocking();

  // Always check for incoming PM packets so the serial buffer doesn't overflow
  bool newPMDataAvailable = pms.read(data);

  // Print a unified telemetry update exactly once every 1000ms (1 second)
  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis(); // Reset the print clock

    Serial.println("\n--- SITE MONITORING REPORT ---");

    // ---- 1. VIBRATION ----
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
      Serial.println(" | >>> ALERT: EXCEEDS 5.0 m/s² <<<");
    } else if (magnitude >= VIBRATION_WARNING) {
      Serial.println(" | >>> WARNING: EXCEEDS 2.5 m/s² <<<");
    } else {
      Serial.println(" | STATUS: NORMAL");
    }

    // ---- 2. NOISE ----
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

    // ---- 3. DUST / AIR QUALITY ----
    // Note: The PMS5003 updates its data internally every 1-2 seconds anyway, 
    // so printing the most recent cached packet values here keeps everything synchronized.
    Serial.print("DUST METRICS | ");
    Serial.printf("PM1.0: %u ug/m³ | PM2.5: %u ug/m³ | PM10: %u ug/m³\n", 
                  data.PM_SP_UG_1_0, data.PM_SP_UG_2_5, data.PM_SP_UG_10_0);
                  
    Serial.println("------------------------------------------");
  }
}
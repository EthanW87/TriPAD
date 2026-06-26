#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

void setup(void) {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(21, 22);
  
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("Sensor not found");
    while (1) { delay(10); }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  Serial.println("Calibrating... keep sensor completely still");
  delay(2000);

  // Take 500 readings and average them
  float sumX = 0, sumY = 0, sumZ = 0;
  int samples = 500;

  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sumX += a.acceleration.x;
    sumY += a.acceleration.y;
    sumZ += a.acceleration.z;
    delay(5);
  }

  float offsetX = sumX / samples;
  float offsetY = sumY / samples;
  float offsetZ = (sumZ / samples) - 9.8; // subtract gravity

  Serial.println(">>> CALIBRATION COMPLETE <<<");
  Serial.print("Offset X: "); Serial.println(offsetX, 4);
  Serial.print("Offset Y: "); Serial.println(offsetY, 4);
  Serial.print("Offset Z: "); Serial.println(offsetZ, 4);
  Serial.println("Copy these values into your main sketch");
}

void loop() {}
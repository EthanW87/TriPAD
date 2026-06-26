#include "MPU6050.h"
#include <Wire.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(21, 22);
  delay(500);
  
  Serial.println("Initializing...");
  mpu.initialize();
  
  Serial.print("WHO_AM_I: 0x");
  Serial.println(mpu.getDeviceID(), HEX);
  
  Serial.print("Connection: ");
  Serial.println(mpu.testConnection() ? "SUCCESS" : "FAILED");
}

void loop() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  float axf = ax / 16384.0 * 9.8 + 0.86;
  float ayf = ay / 16384.0 * 9.8 + 0.5;
  float azf = az / 16384.0 * 9.8 - 11;
  float magnitude = sqrt(axf * axf + ayf * ayf + azf * azf);

  Serial.print(" X: "); Serial.println(axf, 2);
  Serial.print(" Y: "); Serial.println(ayf, 2);
  Serial.print(" Z: "); Serial.println(azf, 2);
  
  Serial.print(" Magnitude: "); Serial.print(magnitude, 2);
  Serial.println(" m/s2");
  
  if (magnitude >= 2.5){
    Serial.println(" Warning: Vibrations have exceeded 2.5 m/s^2 -- control measures required");
  }
  if (magnitude >= 5){
    Serial.println(" Alert: Dangerous -- vibrations have exceeded 5 m/s^2");
  }
  else{
    Serial.println(" Status is normal"); 
  }

  delay(200);





}
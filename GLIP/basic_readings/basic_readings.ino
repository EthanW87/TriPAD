#include <Wire.h>

const int MPU_ADDR = 0x68; 
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

void setup() {
  // Start serial immediately with zero delay traps
  Serial.begin(115200);
  delay(500); 
  
  Serial.println("\n=================================");
  Serial.println("   ESP32 BOOTING: SERIAL ONLINE  ");
  Serial.println("=================================");

  // Assign and start the verified I2C pins
  Wire.setPins(21, 22);
  Wire.begin();
  delay(100);

  Serial.println("Sending wakeup signal to 0x68...");
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); 
  Wire.write(0);    
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println("🏆 SUCCESS: Sensor awake! Starting stream... 🏆");
  } else {
    Serial.print("Hardware error code: ");
    Serial.println(error);
    while(1) { delay(10); }
  }
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); 
  Wire.endTransmission(false); 

  Wire.requestFrom(MPU_ADDR, 14, true);  
  
  AcX = Wire.read() << 8 | Wire.read();  
  AcY = Wire.read() << 8 | Wire.read();  
  AcZ = Wire.read() << 8 | Wire.read();  
  Tmp = Wire.read() << 8 | Wire.read();  
  GyX = Wire.read() << 8 | Wire.read();  
  GyY = Wire.read() << 8 | Wire.read();  
  GyZ = Wire.read() << 8 | Wire.read();  

  float tx = (Tmp / 340.00) + 36.53;

  Serial.print("X: "); Serial.print(AcX);
  Serial.print(" | Y: "); Serial.print(AcY);
  Serial.print(" | Z: "); Serial.print(AcZ);
  Serial.print(" | Temp: "); Serial.print(tx);
  Serial.println(" C");

  delay(200); 
}
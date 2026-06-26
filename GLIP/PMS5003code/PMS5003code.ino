#include "PMS.h"

// Define the ESP32 HardwareSerial port 2
HardwareSerial pmsSerial(2);
PMS pms(pmsSerial);
PMS::DATA data;

void setup() {
  // Initialize the standard USB Serial communication for the Serial Monitor
  Serial.begin(115200);
  
  // Initialize Serial2 for the PMS5003 at 9600 baud.
  // SERIAL_8N1 sets 8 data bits, no parity, 1 stop bit (standard UART).
  // 16 is RX pin (connected to LV1), 17 is TX pin (connected to LV2).
  pmsSerial.begin(9600, SERIAL_8N1, 16, 17); 

  Serial.println("--- PMS5003 Sensor Initialized ---");
}

void loop() {
  // readData() returns true only when a complete, valid 32-byte packet is received
  if (pms.read(data)) {
    Serial.println("=================================");
    Serial.println("Current Air Quality Readings:");
    Serial.println("=================================");
    
    // PM concentration units: Standard Particle (CF=1, ug/m3)
    Serial.print("PM 1.0: "); 
    Serial.print(data.PM_SP_UG_1_0); 
    Serial.println(" ug/m3");
    
    Serial.print("PM 2.5: "); 
    Serial.print(data.PM_SP_UG_2_5); 
    Serial.println(" ug/m3");
    
    Serial.print("PM 10.0: "); 
    Serial.print(data.PM_SP_UG_10_0); 
    Serial.println(" ug/m3");
    
    Serial.println(); // Blank line for readability
  }
}
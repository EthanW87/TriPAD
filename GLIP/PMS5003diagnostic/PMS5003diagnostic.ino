#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(115200);
  // Open Serial2 at 9600 baud using your pin configuration
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("--- Raw Serial Passthrough Test ---");
}

void loop() {
  // If even a single byte comes from the shifter, print it in HEX format
  if (Serial2.available()) {
    uint8_t c = Serial2.read();
    Serial.print(c, HEX);
    Serial.print(" ");
  }
}
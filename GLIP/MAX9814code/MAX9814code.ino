void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("MAX9814 Microphone Test");
}

float getSlowResponseDBA() {
  unsigned long startTime = millis();
  float sumSquares = 0;
  long sampleCount = 0;

  // 1. SLOW RESPONSE WINDOW: Average over 1000ms (1 second) as mandated for workplace noise
  while (millis() - startTime < 1000) { 
    int raw = analogRead(35);
    
    // Convert 12-bit ADC (0-4095) to Voltage (assuming 3.3V max)
    float voltage = raw * (3.3 / 4095.0);
  
    float acSignal = voltage - 1.096; 
    
    // Accumulate the squared values for RMS calculation
    sumSquares += (acSignal * acSignal);
    sampleCount++;
  }

  // Calculate Root Mean Square (RMS) voltage of the audio signal
  float rmsVoltage = sqrt(sumSquares / sampleCount);

  // Prevent log10(0) safety check
  if (rmsVoltage < 0.0001) rmsVoltage = 0.0001;

  
  float calibrationOffset = 79.1; // Adjust this value during calibration
  float dBA = 20.0 * log10(rmsVoltage) + calibrationOffset;

  if (dBA <= 45.5){
    dBA = dBA - 13.0;
    if (dBA < 32.0) dBA = 32.0;
  }

  return dBA;
}

void loop() {
  
  float dBA = getSlowResponseDBA();
  Serial.print(dBA, 1);
  Serial.println(" dBA");

  if (dBA > 85){
    Serial.println("Alert: Exceeds the 85 dBA limit.");
  }
  if (dBA > 80){
    Serial.println("Alert: Approaching 85 dBA limit.");
  }
  else{
    Serial.println("Status: Normal");
  }

}
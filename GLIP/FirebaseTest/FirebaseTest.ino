#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// WiFi credentials
#define WIFI_SSID "Rogers 1509"
#define WIFI_PASSWORD "hsu22200"

// Firebase credentials
  #define DATABASE_URL "https://glip-data-storage-default-rtdb.firebaseio.com/"
  #define DATABASE_SECRET "3AeNxjTWc84tYS0gKJgNR5ATq1asQxIRBAmMdMu5"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure Firebase
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;  

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase connected");
}

void loop() {
  if (Firebase.ready()) {
    // Send test values
    Firebase.RTDB.setFloat(&fbdo, "/tripAD/noise", 72.5);
    Firebase.RTDB.setFloat(&fbdo, "/tripAD/vibration", 0.3);
    Firebase.RTDB.setInt(&fbdo, "/tripAD/pm25", 12);
    Firebase.RTDB.setInt(&fbdo, "/tripAD/pm10", 18);
    Firebase.RTDB.setInt(&fbdo, "/tripAD/pm1", 8);

    Serial.println("Data sent to Firebase");
  }
  delay(5000); // Send every 5 seconds
}
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define WIFI_SSID "Rogers 1509"
#define WIFI_PASSWORD "hsu22200"
#define BOT_TOKEN "8886495077:AAHy_oqOITHPqDkuSWKE9S1nMoQ2MZfAzh0"
#define CHAT_ID "8015900696"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  client.setInsecure();
  
  // Send test alert
  bot.sendMessage(CHAT_ID, "TriPAD Online - Monitoring Active", "");
  Serial.println("Telegram message sent");
}

void loop() {}
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("=== IDENTIFICACAO DA PLACA ===");

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.println("================================");
}

void loop() {
}
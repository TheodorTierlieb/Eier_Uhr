#include <WiFi.h>
#include "wlan.h"

using namespace wlan;

const char* ssid = "iPhone von Niklas";
const char* password = "krausederlelek";

void wlan::connectToWiFi(){
  WiFi.begin(ssid, password);
  Serial.print("Verbindung zum WLAN...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nVerbunden! IP-Adresse: " + WiFi.localIP().toString());
}

void wlan::startAccessPoint() {
  WiFi.softAP("ESP32-AP", "Passwort123");
  Serial.println("ESP32 erstellt ein eigenes WLAN. SSID: ESP32-AP, Passwort: Passwort123");
  Serial.println("IP-Adresse im AP-Modus: " + WiFi.softAPIP().toString());
}

void wlan::changeWiFiCredentials(const char* newSSID, const char* newPassword) {
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(newSSID, newPassword);
  Serial.println("Verbindung zum neuen WLAN wird hergestellt...");
} 
#include <WiFi.h>
#include "wlan.h"

const char* ssid = "iPhone von Niklas";
const char* password = "krausederlelek";

void connectToWiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Verbindung zum WLAN...");
  
  for (int i = 0; i < 15; i++) {
    if (WiFi.status() != WL_CONNECTED){
      delay(1000);
      Serial.print(".");
    }
    else {
      Serial.println("\nVerbunden! IP-Adresse: " + WiFi.localIP().toString());
     return;
    }
  }
  Serial.println("\nWlanverbindung fehlgeschlagen");
  
}

void startAccessPoint() 
{
  WiFi.softAP("ESP32-AP", "Passwort123");
  Serial.println("ESP32 erstellt ein eigenes WLAN. SSID: ESP32-AP, Passwort: Passwort123");
  Serial.println("IP-Adresse im AP-Modus: " + WiFi.softAPIP().toString());
}

void changeWiFiCredentials(const char* newSSID, const char* newPassword) 
{
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(newSSID, newPassword);
  Serial.println("Verbindung zum neuen WLAN wird hergestellt...");
} 
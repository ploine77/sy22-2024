#include <WiFi.h>
#include "esp_wifi.h"
#include <string.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>

const char* ssidA = "servera";
const char* passwordA = "srvapass10";
const char* Host_NameA = "http://192.168.0.1";
const char* ssidB = "serverb";
const char* passwordB = "srvbpass10";
const char* Host_NameB = "http://192.168.1.1";

char ConnectedTo;
const char* host;


void sendData(String HOST_NAME, String PATH_NAME, String queryString) {
  HTTPClient http;
  http.begin(HOST_NAME + PATH_NAME);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("message=" + queryString);
  
  // httpCode will be negative on error
  if (httpCode > 0) {
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Response from server A :" + payload);
    } else {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST ... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] POST ... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();


}

void connectToBestWiFi() {
  int32_t rssiA = scanAndGetRSSI(ssidA);
  int32_t rssiB = scanAndGetRSSI(ssidB);

  // Serial.print("RSSI de servera: ");
  // Serial.println(rssiA);
  // Serial.print("RSSI de serverb: ");
  // Serial.println(rssiB);

  if (rssiA > rssiB) {
    connectToWiFi(ssidA, passwordA);
    ConnectedTo = 'A';
    host = Host_NameA;
  } else {
    connectToWiFi(ssidB, passwordB);
    ConnectedTo = 'B';
    host = Host_NameB;
  }
}

int32_t scanAndGetRSSI(const char* ssid) {
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; ++i) {
    if (WiFi.SSID(i) == ssid) {
      return WiFi.RSSI(i);
    }
  }
  return INT_MIN; // Si le réseau n'est pas trouvé, retourne la valeur minimale possible
}

void connectToWiFi(const char* ssid, const char* password) {
  Serial.print("Connexion à ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connexion établie!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  connectToBestWiFi();
}

void loop() {
  delay(10000); // Attente de 10 secondes
  connectToBestWiFi();
  sendData(String(host),"/message","fromclient/rssiA:" + String(scanAndGetRSSI(ssidA)) + "/rssiB:" + String(scanAndGetRSSI(ssidB)));
}
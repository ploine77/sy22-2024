#include <WiFi.h>
#include "esp_wifi.h"
#include <string.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>

const char* ssidA = "servera";
const char* passwordA = "srvapass10";
const char* ssidB = "serverb";
const char* passwordB = "srvbpass10";

// Coordonnées des serveurs
float serverAPosition[2] = {0.0, 0.0}; // Par exemple, (0, 0)
float serverBPosition[2] = {10.0, 0.0}; // Par exemple, (10, 0)

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

  Serial.print("RSSI de servera: ");
  Serial.println(rssiA);
  Serial.print("RSSI de serverb: ");
  Serial.println(rssiB);

  if (rssiA > rssiB) {
    connectToWiFi(ssidA, passwordA);
  } else {
    connectToWiFi(ssidB, passwordB);
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

void getPosition(){
  int32_t rssiA = scanAndGetRSSI(ssidA);
  int32_t rssiB = scanAndGetRSSI(ssidB);
  float distanceA = calculateDistance(rssiA);
  float distanceB = calculateDistance(rssiB);

  // Calculer la position de l'ESP
  float clientPosition[2] = {0.0, 0.0}; // Position initiale, peut être mise à jour avec des valeurs réelles

  // Trilatération pour estimer la position de l'ESP
  // Dans ce cas, nous supposerons que la position de l'ESP est le point d'intersection des cercles
  trilaterate(clientPosition, serverAPosition, serverBPosition, distanceA, distanceB);

  Serial.print("Position de l'ESP (x, y): ");
  Serial.print(clientPosition[0]);
  Serial.print(", ");
  Serial.println(clientPosition[1]);

}

float calculateDistance(int32_t rssi) {
  // À remplacer par une formule de conversion RSSI-distance appropriée
  // Cette formule peut dépendre de plusieurs facteurs, y compris la puissance de transmission des routeurs et les conditions environnementales
  // Cela pourrait nécessiter des tests et des ajustements pour correspondre à votre environnement spécifique
  return pow(10, (-rssi - 60) / (10 * 2.0));
}

void trilaterate(float clientPosition[], float serverAPosition[], float serverBPosition[], float distanceA, float distanceB) {
  // Calculer les coordonnées du point d'intersection des cercles
  float d = sqrt(pow((serverBPosition[0] - serverAPosition[0]), 2) + pow((serverBPosition[1] - serverAPosition[1]), 2));
  float a = (pow(distanceA, 2) - pow(distanceB, 2) + pow(d, 2)) / (2 * d);
  float h = sqrt(pow(distanceA, 2) - pow(a, 2));
  
  clientPosition[0] = serverAPosition[0] + (a * (serverBPosition[0] - serverAPosition[0])) / d;
  clientPosition[1] = serverAPosition[1] + (a * (serverBPosition[1] - serverAPosition[1])) / d;
}

void setup() {
  Serial.begin(115200);
  connectToBestWiFi();
}

void loop() {
  delay(10000); // Attente de 10 secondes
  connectToBestWiFi();
  //getPosition();
  serial.print(calculateDistance(scanAndGetRSSI(ssidA)))
}
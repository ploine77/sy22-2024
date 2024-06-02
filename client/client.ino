#include <WiFi.h>
#include "esp_wifi.h"
#include <string.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

const char* ssidA = "servera";
const char* passwordA = "srvapass10";
const char* Host_NameA = "192.168.0.1";
const char* ssidB = "serverb";
const char* passwordB = "srvbpass10";
const char* Host_NameB = "192.168.1.1";

char ConnectedTo;
const char* host;

// Username and Password for Basic Authentication
const char* http_username = "admin";
const char* http_password = "password";

struct RSSIValues {
  int32_t rssiA;
  int32_t rssiB;
};

// Déclaration globale de rssiValues
RSSIValues rssiValues;

AsyncWebServer server(80);

void process_msg(String message){
  DynamicJsonDocument doc(200); // Taille du document JSON en octets
  deserializeJson(doc, message);
  if (doc["Destination"] == "Client"){
    if (doc["data"] && doc["data"]["action"]){
      int pinNumber = doc["data"]["action"]["pinupmber"];
      bool pinStatus = doc["data"]["action"]["pinstat"];

      Serial.print("Pin Number: ");
      Serial.println(pinNumber);
      Serial.print("Pin Status: ");
      Serial.println(pinStatus);
      // Set the pin mode to OUTPUT
      pinMode(pinNumber, OUTPUT);

      // Set the pin state based on the pinStatus
      digitalWrite(pinNumber, pinStatus ? HIGH : LOW);

    }
  }
}

void HTTPSeverSetup() {
  // WebUI
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //   String response = "<h1>Bienvenue sur l'ESP32 B</h1>";
  //   response += "<p>IP Address (AP) : " + WiFi.softAPIP().toString() + "</p>";
  //   response += "<p>IP Adress (STATION) : " + WiFi.localIP().toString() + "</p>";
  //   response += "<p>(Deprecated)Internal Temperature : " + String(esp_internal_temp()) + "C</p>";
  //   response += "<p>Available memory : " + String(available_memory()) + " bytes</p>";
  //   response += "<p>Client Connected to B : " + String(client_connected_to_b) + "</p>";
  //   request->send(200, "text/html", response);
  // });
  // API Server
  server.on("/message", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    // Traitement du message reçu
    // Exemple : affichage du contenu du message dans la console série
    String message;
    if (request->hasParam("message", true)) {
      message = request->getParam("message", true)->value();
      process_msg(message);
      Serial.println("Message reçu : " + message);
    }
    request->send(200, "text/plain", "Message reçu avec succès par le client");
  });

  server.begin();
  Serial.println("API HTTP Client started");
    
}

// Send data over HTTP API
void sendData(String HOST_NAME, String PATH_NAME, String queryString) {
  HTTPClient http;
  http.begin("http://" + String(http_username) + ":" + String(http_password) + "@" + String(HOST_NAME) + String(PATH_NAME));
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("message=" + queryString);
  
  // httpCode will be negative on error
  if (httpCode > 0) {
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      //Serial.println("Response : " + payload);
    } else {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST ... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] POST ... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();


}

void connectToBestWiFi(int32_t rssiA = scanAndGetRSSI(ssidA), int32_t rssiB = scanAndGetRSSI(ssidB)) {
  // Vérifier si le RSSI de l'un ou l'autre SSID est supérieur à -120 dBm
  if (rssiA > -120 || rssiB > -120) {
    if (rssiA > rssiB && rssiA > -120) {
      if (ConnectedTo != 'A') {
        DynamicJsonDocument jsonDoc(100);
        // Ajoutez les valeurs au document JSON
        jsonDoc["Source"] = "Client";
        jsonDoc["Destination"] = "serva";
        jsonDoc["data"]["connection_data"]["ConnectedTo"] = "reconnecting";
        // Convertir le document JSON en chaîne
        String jsonString;
        serializeJson(jsonDoc, jsonString);
        sendData(String(host),"/message",jsonString);

        connectToWiFi(ssidA, passwordA);
        ConnectedTo = 'A';
        host = Host_NameA;
      }
    } else if (rssiB > -120) {
      if (ConnectedTo != 'B') {
        DynamicJsonDocument jsonDoc(100);
        // Ajoutez les valeurs au document JSON
        jsonDoc["Source"] = "Client";
        jsonDoc["Destination"] = "serva";
        jsonDoc["data"]["connection_data"]["ConnectedTo"] = "reconnecting";
        // Convertir le document JSON en chaîne
        String jsonString;
        serializeJson(jsonDoc, jsonString);
        sendData(String(host),"/message",jsonString);
        connectToWiFi(ssidB, passwordB);
        ConnectedTo = 'B';
        host = Host_NameB;
      }
    } else {
      Serial.println("Aucun réseau avec un RSSI acceptable trouvé.");
    }
  } else {
    Serial.println("Aucun réseau avec un RSSI acceptable trouvé.");
  }
}

// Get the RSSI of the wifi
int32_t scanAndGetRSSI(const char* ssid) {
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; ++i) {
    if (strcmp(WiFi.SSID(i).c_str(), ssid) == 0) {
      return WiFi.RSSI(i);
    }
  }
  return INT_MIN;
}

RSSIValues scanAndGetRSSIs(const char* ssidA, const char* ssidB) {
  RSSIValues rssiValues = {INT_MIN, INT_MIN};
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; ++i) {
    String foundSSID = WiFi.SSID(i);
    if (strcmp(foundSSID.c_str(), ssidA) == 0) {
      rssiValues.rssiA = WiFi.RSSI(i);
    }
    if (strcmp(foundSSID.c_str(), ssidB) == 0) {
      rssiValues.rssiB = WiFi.RSSI(i);
    }
  }
  return rssiValues;
}

String generateJsonMsg(int32_t rssiA, int32_t rssiB) {
  // Déclarez un document JSON avec une taille plus grande si nécessaire
  DynamicJsonDocument jsonDoc(256);
  // Ajoutez les valeurs au document JSON
  jsonDoc["Source"] = "Client";
  jsonDoc["Destination"] = "serva";
  jsonDoc["data"]["position"]["rssiA"] = rssiA;
  jsonDoc["data"]["position"]["rssiB"] = rssiB;
  jsonDoc["data"]["connection_data"]["mac"] = WiFi.macAddress();
  jsonDoc["data"]["connection_data"]["ip"] = WiFi.localIP().toString();
  jsonDoc["data"]["connection_data"]["ConnectedTo"] = String(ConnectedTo);

  // Convertir le document JSON en chaîne
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  // Vérifiez si la sérialisation s'est bien déroulée
  if (jsonString.length() == 0) {
    Serial.println("Erreur: JSON vide.");
  }

  return jsonString;
}

// Connect to the specify wifi
void connectToWiFi(const char* ssid, const char* password) {
  Serial.print("Connexion à ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();  // Enregistrer le temps de début de la tentative de connexion

  // Attendre la connexion, mais pas plus de 5 secondes
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime <5000) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connexion établie!");
    Serial.print("Adresse IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Connexion échouée : délai d'attente dépassé.");
    connectToWiFi(ssid,password);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("### THIS IS THE CLIENT ###");
  RSSIValues rssiValues = scanAndGetRSSIs(ssidA, ssidB);
  connectToBestWiFi(rssiValues.rssiA, rssiValues.rssiB);
  HTTPSeverSetup();
  
  //Serial.println(WiFi.macAddress());
}

void loop() {
  delay(3000); // Attente de 1 seconde
  // Update rssiValues
  RSSIValues rssiValues = scanAndGetRSSIs(ssidA, ssidB);
  Serial.print("RSSIA :" + String(rssiValues.rssiA));
  Serial.println(" | RSSIB :" + String(rssiValues.rssiB));
  connectToBestWiFi(rssiValues.rssiA, rssiValues.rssiB);
  sendData(String(host),"/message",generateJsonMsg(rssiValues.rssiA, rssiValues.rssiB));



  //Old data format sendData(String(host),"/message","fromclient/rssiA:" + String(scanAndGetRSSI(ssidA)) + "/rssiB:" + String(scanAndGetRSSI(ssidB)));
}
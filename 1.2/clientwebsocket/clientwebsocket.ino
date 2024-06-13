#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "DHT.h"

// Définition des broches et du type de capteur DHT
#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);  // Initialisation du capteur DHT

/// Définition des variables ///

// WiFi //
// Serveur A
const char* ssidA = "servera";
const char* passwordA = "srvapass10";
const char* Host_NameA = "192.168.0.1";
IPAddress gatewayA_IP(192, 168, 0, 1);
IPAddress localA_IP(192, 168, 0, 3);

// Serveur B
const char* ssidB = "serverb";
const char* passwordB = "srvbpass10";
const char* Host_NameB = "192.168.1.1";
IPAddress gatewayB_IP(192, 168, 1, 1);
IPAddress localB_IP(192, 168, 1, 3);

// Subnet utilisé dans le réseau, ici /24
IPAddress subnet(255, 255, 255, 0);

// IP de la gateway actuelle
IPAddress actualhost;

// Création de l'objet AsyncWebServer sur le port 80
AsyncWebServer server(80);

// Variables de roaming
char ConnectedTo;
const char* host;

// Structure pour les valeurs RSSI
struct RSSIValues {
  int32_t rssiA;
  int32_t rssiB;
};

RSSIValues rssiValues;  // Instance de la structure RSSIValues

// Debug //
const int verbose = 999;  // Niveau de verbosité pour les messages de debug

// Threads Handle //
TaskHandle_t wifiTaskHandle; // Tâche qui se connecte au meilleur wifi
TaskHandle_t dht11Handle; // Tâche qui récupère les données du capteur DHT11 et les envoi à l'ESP A 

// Délais //
const int ScanWifi_interval = 20000; // Interval entre chaque scan de wifi
const int DHT11_interval = 20000; // Interval entre chaque récupération des valeurs du capteur DHT11

// Etat //
bool ScanInProgress = false;


// Fonction de debug pour afficher les messages en fonction du niveau de verbosité
void debug(String debugmessage, int verboselvl){
  if (verbose >= verboselvl){
    Serial.println(debugmessage);
  }
}

// Fonction pour se connecter au meilleur WiFi en fonction du RSSI
void connectToBestWiFi(int32_t rssiA = rssiValues.rssiA, int32_t rssiB = rssiValues.rssiB) {
  if (rssiA > -120 || rssiB > -120) {
    if (rssiA > rssiB && rssiA > -120) {
      if (ConnectedTo != 'A') {
        DynamicJsonDocument jsonDoc(100);
        jsonDoc["source"] = "client";
        jsonDoc["destination"] = "servera";
        jsonDoc["data"]["connection_event"] = "reconnecting";
        String jsonString;
        serializeJson(jsonDoc, jsonString);
        senddata(actualhost,jsonString);
        ConnectedTo = 'A';
        connectToWiFi(ssidA, passwordA);
        actualhost = WiFi.gatewayIP();
      }
    } else if (rssiB > -120) {
      if (ConnectedTo != 'B') {
        DynamicJsonDocument jsonDoc(100);
        jsonDoc["source"] = "client";
        jsonDoc["destination"] = "servera";
        jsonDoc["data"]["connection_event"] = "reconnecting";
        String jsonString;
        serializeJson(jsonDoc, jsonString);
        senddata(actualhost,jsonString);
        ConnectedTo = 'B';
        connectToWiFi(ssidB, passwordB);
        actualhost = WiFi.gatewayIP();
      }
    } else {
      debug("Aucun réseau avec un RSSI acceptable trouvé.", 1);
    }
  } else {
    debug("Aucun réseau avec un RSSI acceptable trouvé.", 1);
  }
}

// Fonction pour scanner et obtenir les RSSI de deux SSID
RSSIValues scanAndGetRSSIs(const char* ssidA, const char* ssidB) {
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
  debug("Début du scan des SSIDs " + String(ssidA) + " et " + String(ssidB) + "...", 2);
  
  // Averti le serveur A qu'il execute un scan
  DynamicJsonDocument jsonDoc(100);
  jsonDoc["source"] = "client";
  jsonDoc["destination"] = "servera";
  jsonDoc["data"]["connection_event"] = "scanning";
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  senddata(actualhost,jsonString);
  ScanInProgress = true;
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
  WiFi.scanDelete();
  ScanInProgress = false;
  debug("Scan terminé.", 2);
  digitalWrite(12, LOW);
  return rssiValues;
}

// Fonction pour générer un message JSON avec les valeurs RSSI
String generateJsonMsg(int32_t rssiA, int32_t rssiB) {
  DynamicJsonDocument jsonDoc(256);
  jsonDoc["source"] = "client";
  jsonDoc["destination"] = "servera";
  jsonDoc["data"]["position"]["rssiA"] = rssiA;
  jsonDoc["data"]["position"]["rssiB"] = rssiB;
  jsonDoc["data"]["connection_data"]["ip"] = WiFi.localIP().toString();
  jsonDoc["data"]["connection_data"]["ConnectedTo"] = String(ConnectedTo);

  String jsonString;
  serializeJson(jsonDoc, jsonString);

  if (jsonString.length() == 0) {
    debug("Erreur: JSON vide.", 1);
  }

  return jsonString;
}

// Fonction pour se connecter au WiFi
void connectToWiFi(const char* ssid, const char* password) {
  debug("Connexion à " + String(ssid), 2);
  if (ConnectedTo == 'A') {
    if (!WiFi.config(localA_IP, gatewayA_IP, subnet)) {
      debug("Échec de la configuration STA", 1);
    }
  } else {
    if (!WiFi.config(localB_IP, gatewayB_IP, subnet)) {
      debug("Échec de la configuration STA", 1);
    }
  }
  
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  // Attendre la connexion, mais pas plus de 6 secondes
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 6000) {
    delay(1000);
    debug(".", 3);
  }

  if (WiFi.status() == WL_CONNECTED) {
    debug("Connexion établie! Adresse IP: " + WiFi.localIP().toString(), 2);
  } else {
    debug("Connexion échouée: délai d'attente dépassé.", 1);
    connectToWiFi(ssid, password);
  }
}

// Fonction pour envoyer des données à une adresse IP spécifique
void senddata(IPAddress ipdestination, String data){
  if (WiFi.status() == WL_CONNECTED){
    while (ScanInProgress){ // L'envoi de donné ne peut pas avoir lieu si un scan est en cours. Il faut attendre
      delay(500);
    }
    debug("Envoi du paquet " + data + " à http://" + ipdestination.toString() + "/API", 2);
    HTTPClient http;
    http.begin("http://" + ipdestination.toString() + "/API");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = http.POST("message=" + data);
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
      } else {
        debug("[HTTP] POST ... code: " + String(httpCode), 2);
      }
    } else {
      debug("[HTTP] POST ... failed, error: " + http.errorToString(httpCode), 1);
    }
    http.end();
  }
}

// Fonction pour traiter les messages reçus
void process_msg(String data) {
  DynamicJsonDocument doc(200);
  deserializeJson(doc, data);
  debug(data, 2);
  if (doc["destination"].as<String>() == "client") {
    if (doc["data"] && doc["data"]["action"]){
      int pinnumber = doc["data"]["action"]["pinupmber"];
      bool pinstat = doc["data"]["action"]["pinstat"];

      pinMode(pinnumber, OUTPUT);
      digitalWrite(pinnumber, pinstat ? HIGH : LOW);
      debug("Réglage de la broche: " + String(pinnumber) + " à: " + String(pinstat), 2);
    }
  }
}

// Tâche pour gérer le WiFi
void wifiTask(void * parameter) {
  for (;;) {
    RSSIValues rssiValues = scanAndGetRSSIs(ssidA, ssidB);
    connectToBestWiFi(rssiValues.rssiA, rssiValues.rssiB);
    senddata(actualhost, generateJsonMsg(rssiValues.rssiA, rssiValues.rssiB));
    vTaskDelay(ScanWifi_interval);
  }
}

// Tâche pour gérer le capteur DHT11
void dht11 (void * parameter) {
  for (;;) {
    vTaskDelay(DHT11_interval);
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      debug(F("Échec de la lecture du capteur DHT!"), 1);
    } else {
      DynamicJsonDocument jsonDoc(256);
      jsonDoc["source"] = "client";
      jsonDoc["destination"] = "servera";
      jsonDoc["data"]["sensor"]["dht11"]["temperature"] = t;
      jsonDoc["data"]["sensor"]["dht11"]["humidity"] = h;
      String jsonString;
      serializeJson(jsonDoc, jsonString);
      senddata(actualhost, jsonString);
    }
    
  }
}

void setup() {
  Serial.begin(115200);
  debug("### THIS IS THE CLIENT ###", 2);
  RSSIValues rssiValues = scanAndGetRSSIs(ssidA, ssidB);
  connectToBestWiFi(rssiValues.rssiA, rssiValues.rssiB);
  server.on("/API", HTTP_POST, [](AsyncWebServerRequest *request){
    String message;
    if (request->hasParam("message", true)) {
      message = request->getParam("message", true)->value();
      process_msg(message);
      request->send_P(200, "text/plain", "OK");
    }
  });
  xTaskCreatePinnedToCore(wifiTask, "WiFi Task", 4096, NULL, 1, &wifiTaskHandle, 0);
  xTaskCreate(dht11, "dht11 sensor", 4096, NULL, 1, &dht11Handle);
  server.begin();  // Démarrer le serveur
  dht.begin();  // Initialiser le capteur DHT
}

void loop() {
  // Boucle vide
}

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

/// Définition des variables  ///
// Wifi //
// Serveur A
const char* ssidA = "servera";
const char* passwordA = "srvapass10";
const char* Host_NameA = "192.168.0.1";
IPAddress gatewayA_IP(192, 168, 0, 1);
IPAddress localA_IP(192, 168, 0, 3);
IPAddress subnet(255, 255, 255, 0);
// Serveur B
const char* ssidB = "serverb";
const char* passwordB = "srvbpass10";
const char* Host_NameB = "192.168.1.1";
IPAddress gatewayB_IP(192, 168, 1, 1);
IPAddress localB_IP(192, 168, 1, 3);
// IP de la gateway actuelle
IPAddress actualhost;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Roaming //
char ConnectedTo;
const char* host;

struct RSSIValues {
  int32_t rssiA;
  int32_t rssiB;
};

RSSIValues rssiValues;

// Debug //
const int verbose = 999;

TaskHandle_t UDPserverTask;


void debug(String debugmessage, int verboselvl){
  if (verbose >= verboselvl){
    Serial.println(debugmessage);
  }
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
        senddata(actualhost,jsonString);
        ConnectedTo = 'A';
        connectToWiFi(ssidA, passwordA);
        
        actualhost = WiFi.gatewayIP();;
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
        senddata(actualhost,jsonString);
        ConnectedTo = 'B';
        connectToWiFi(ssidB, passwordB);
        
        actualhost = WiFi.gatewayIP();;
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
  jsonDoc["source"] = "client";
  jsonDoc["destination"] = "servera";
  jsonDoc["data"]["position"]["rssiA"] = rssiA;
  jsonDoc["data"]["position"]["rssiB"] = rssiB;
  // jsonDoc["data"]["connection_data"]["mac"] = WiFi.macAddress();
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

void connectToWiFi(const char* ssid, const char* password) {
  Serial.print("Connexion à ");
  Serial.println(ssid);
  // Configures static IP address
  if (ConnectedTo == 'A'){
    if (!WiFi.config(localA_IP, gatewayA_IP, subnet)) {
      Serial.println("STA Failed to configure");
    }
  }
  else{
    if (!WiFi.config(localB_IP, gatewayB_IP, subnet)) {
      Serial.println("STA Failed to configure");
    }
  }
  
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();  // Enregistrer le temps de début de la tentative de connexion

  // Attendre la connexion, mais pas plus de 5 secondes
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime <6000) {
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

void senddata(IPAddress ipdestination, String data){
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("Sending packet to " + ipdestination.toString() + " at http://" + ipdestination.toString() + "/API");
    HTTPClient http;
    http.begin("http://" + ipdestination.toString() + "/API");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    // Send HTTP POST request
    int httpCode = http.POST("message=" + data);
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
}


void process_msg(String data) {
  DynamicJsonDocument doc(200);
  deserializeJson(doc, data);
  Serial.println(data);
  if (doc["destination"].as<String>() == "client") {
    if (doc["data"] && doc["data"]["action"]){
      int pinnumber = doc["data"]["action"]["pinupmber"];
      bool pinstat = doc["data"]["action"]["pinstat"];

      pinMode(pinnumber, OUTPUT);
      digitalWrite(pinnumber, pinstat ? HIGH : LOW);
      Serial.println("Setting pin : " + String(pinnumber) + " to : " + String(pinstat));
    }
  }
}

TaskHandle_t wifiTaskHandle;
bool scanInProgress = true;

void wifiTask(void * parameter) {
  for (;;) {
    if (scanInProgress) {
      RSSIValues rssiValues = scanAndGetRSSIs(ssidA, ssidB); // Bloquant dans la tâche séparée
      //scanInProgress = false;
      //Serial.print("RSSIA :" + String(rssiValues.rssiA));
      //Serial.println(" | RSSIB :" + String(rssiValues.rssiB));
      connectToBestWiFi(rssiValues.rssiA, rssiValues.rssiB);
      senddata(actualhost, generateJsonMsg(rssiValues.rssiA, rssiValues.rssiB));
    }
    vTaskDelay(3000 / portTICK_PERIOD_MS); // Délai pour permettre le basculement des tâches
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println("### THIS IS THE CLIENT ###");
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
  xTaskCreatePinnedToCore(wifiTask, "WiFi Task", 4096, NULL, 1, &wifiTaskHandle,0);
  // Start server
  server.begin();
}

int loopcounter = 0;

void loop() {
}



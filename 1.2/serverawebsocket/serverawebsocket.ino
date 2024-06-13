#include <WiFi.h>
#include "esp_wifi.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ezButton.h>

// Paramètres du réseau WiFi
const char* ssid = "servera";
const char* password = "srvapass10";
const int channel = 10;                        // Numéro du canal WiFi entre 1 et 13
const bool hide_SSID = false;                  // Désactiver la diffusion SSID -> SSID n'apparaîtra pas dans un scan WiFi basique
const int max_connection = 10;                 // Nombre maximum de clients connectés simultanément sur l'AP

// Adresses IP
IPAddress local_ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress serverblocal_ip(192, 168, 0, 2);
IPAddress clientlocal_ip(192, 168, 0, 3);
IPAddress empty(0, 0, 0, 0);

const int verbose = 5;  // Niveau de verbosité pour les messages de debug

// Définition des boutons avec ezButton
ezButton button1(33);  // Bouton connecté à la broche 33
ezButton button2(32);  // Bouton connecté à la broche 32
ezButton button3(27);  // Bouton connecté à la broche 27

// Définition des LEDs
const int LED_PIN_A = 19;
const int LED_PIN_B = 18;
const int LED_PIN_C = 5;

bool previousLedStateA = false;
bool previousLedStateB = false;
bool previousLedStateC = false;

#define SHORT_PRESS_TIME 50  // Temps de pression court en millisecondes

// Variables de connexion client
bool client_connected_to_a = false;
bool client_connected_to_b = false;
bool client_reconnecting = false;
bool client_scanning = false;

// Structure pour les valeurs RSSI
struct RSSIValues {
  String rssiA;
  String rssiB;
};

RSSIValues rssiValues;  // Instance de la structure RSSIValues

// Création de l'objet AsyncWebServer sur le port 80
AsyncWebServer server(80);

// Variables pour les capteurs
float sensor_temperature = 0.0;
float sensor_humidity = 0.0;

// Fonction de debug pour afficher les messages en fonction du niveau de verbosité
void debug(String debugmessage, int verboselvl) {
  if (verbose >= verboselvl) {
    Serial.println(debugmessage);
  }
}

// Fonction pour configurer le WiFi
void WifiSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  debug("[+] Point d'accès A créé avec IP gateway " + WiFi.softAPIP().toString(), 1);
  debug("Configuration WiFi réussie", 1);
}

// Fonction pour envoyer des données à une adresse IP spécifique
void senddata(String destalias, String data) {
  HTTPClient http;
  IPAddress ipdestination;
  debug("Sending " + data + " to " + destalias,2);
  if (nexthop(destalias) != empty) {
      ipdestination = nexthop(destalias);
  }
  else{
    debug("Can't dertermine nexthop",2);
  }
  while (client_reconnecting | client_scanning){ // Si le client a annoncé étant en train de se changer de point d'accès ou de scanner, il est neccessaire d'attendre que celui-ci se soit reconnecter avant de lui envoyer des données
    delay(500);
    debug("Client is actualy reconnecting or scanning... waiting for him to finish",2);
  }
  http.begin("http://" + ipdestination.toString() + "/API");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("message=" + data);
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
    } else {
      debug("[HTTP] POST ... code: " + String(httpCode), 1);
    }
  } else {
    debug("[HTTP] POST ... échoué, erreur: " + String(http.errorToString(httpCode).c_str()), 1);
  }
  http.end();
}

// Fonction pour obtenir l'adresse IP suivante basée sur un alias de destination
IPAddress nexthop(String destalias) {
  if (destalias == "serverb") {
    return serverblocal_ip;
  } else if (destalias == "client") {
    if (client_connected_to_a) {
      return clientlocal_ip;
    } else if (client_connected_to_b) {
      return serverblocal_ip;
    } else {
      debug(destalias + " n'est pas connecté.", 1);
      return empty;
    }
  } else {
    debug(destalias + " inconnu. Il n'y a pas de table de routage pour cet alias.", 1);
    return empty;
  }
}

// Fonction pour traiter les messages reçus
void process_msg(String data) {
  debug(data, 2);
  DynamicJsonDocument doc(200); // Taille du document JSON en octets
  deserializeJson(doc, data);
  if (doc["destination"].as<String>() == "servera") {
    if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "B") {
      client_connected_to_b = true;
      client_connected_to_a = false;
      client_reconnecting = false;
      client_scanning = false;
      rssiValues.rssiA = doc["data"]["position"]["rssiA"].as<String>();
      rssiValues.rssiB = doc["data"]["position"]["rssiB"].as<String>();
    } else if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "A") {
      client_connected_to_b = false;
      client_connected_to_a = true;
      client_reconnecting = false;
      client_scanning = false;
      rssiValues.rssiA = doc["data"]["position"]["rssiA"].as<String>();
      rssiValues.rssiB = doc["data"]["position"]["rssiB"].as<String>();
    } else if (doc["data"]["connection_event"].as<String>() == "reconnecting") {
      client_reconnecting = true;
      debug("Le client se reconnect",1);
      rssiValues.rssiA = "0";
      rssiValues.rssiB = "0";
      } else if (doc["data"]["connection_event"].as<String>() == "scanning") {
      client_scanning = true;
      debug("Le client execute un scan",1);
      rssiValues.rssiA = "0";
      rssiValues.rssiB = "0";
    } else if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "None") {
      client_connected_to_b = false;
      client_connected_to_a = false;
      client_reconnecting = false;
      client_scanning = false;
      rssiValues.rssiA = "0";
      rssiValues.rssiB = "0";
    }
    if (doc["data"]["sensor"]["dht11"]) {
      sensor_temperature = doc["data"]["sensor"]["dht11"]["temperature"].as<float>();
      sensor_humidity = doc["data"]["sensor"]["dht11"]["humidity"].as<float>();
    }
  }
}

// Fonction pour générer un message JSON pour le contrôle des broches
String pincontroljson(int pinupmber, bool pinstat) {
  DynamicJsonDocument jsonDoc(256);
  jsonDoc["source"] = "servera";
  jsonDoc["destination"] = "client";
  jsonDoc["data"]["action"]["pinupmber"] = pinupmber;
  jsonDoc["data"]["action"]["pinstat"] = pinstat;
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  if (jsonString.length() == 0) {
    debug("Erreur: JSON vide.", 1);
  }
  return jsonString;
}

// Fonction de configuration initiale
void setup() {
  Serial.begin(115200);
  Serial.println("### THIS IS SERVER A ###\n\n");
  WifiSetup();
  button1.setDebounceTime(50); // Définir le temps de débounce à 50 millisecondes
  button2.setDebounceTime(50);
  button3.setDebounceTime(50);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String response = "<h1>Bienvenue sur le serveur A (Master)</h1>";
    response += "<p>RSSI A : " + String(rssiValues.rssiA) + "</p>";
    response += "<p>RSSI B : " + String(rssiValues.rssiB) + "</p>";
    response += "<p>client_connected_to_a : " + String(client_connected_to_a) + "</p>";
    response += "<p>client_connected_to_b : " + String(client_connected_to_b) + "</p>";
    response += "<p>client_reconnecting : " + String(client_reconnecting) + "</p>";
    response += "<p>client_scanning : " + String(client_scanning) + "</p>";
    response += "<p>Temperature : " + String(sensor_temperature) + "</p>";
    response += "<p>Humidity : " + String(sensor_humidity) + "</p>";
    request->send(200, "text/html", response);
  });
  server.on("/API", HTTP_POST, [](AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam("message", true)) {
      message = request->getParam("message", true)->value();
      process_msg(message);
      request->send_P(200, "text/plain", "OK");
    }
  });
  server.begin();  // Démarrer le serveur
}

// Boucle principale
void loop() {
  button1.loop(); // Appeler obligatoirement la fonction loop() en premier
  button2.loop();
  button3.loop();

  // Vérifier si le bouton 1 a été cliqué
  if (button1.isReleased()) {
    debug("Bouton 1 cliqué", 2);
    String message = pincontroljson(LED_PIN_A, !previousLedStateA);
    senddata("client", message);
    previousLedStateA = !previousLedStateA;
  }

  // Vérifier si le bouton 2 a été cliqué
  if (button2.isReleased()) {
    debug("Bouton 2 cliqué", 2);
    String message = pincontroljson(LED_PIN_B, !previousLedStateB);
    senddata("client", message);
    
    previousLedStateB = !previousLedStateB;
  }

  // Vérifier si le bouton 3 a été cliqué
  if (button3.isReleased()) {
    debug("Bouton 3 cliqué", 2);
    String message = pincontroljson(LED_PIN_C, !previousLedStateC);
    senddata("client", message);
    previousLedStateC = !previousLedStateC;
  }
}

#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// Identifiants Wifi pour se connecter à l'AP A
const char* wifi_network_ssid = "servera";
const char* wifi_network_password = "srvapass10";

// Paramètres Wifi pour l'AP B
const char *ssid = "serverb";
const char *password = "srvbpass10";
const int channel = 10;  // Numéro du canal WiFi entre 1 et 13
const bool hide_SSID = false;  // Pour désactiver la diffusion du SSID -> le SSID n'apparaîtra pas dans un scan WiFi basique
const int max_connection = 2;  // Nombre maximum de clients connectés simultanément à l'AP

// Paramètres réseau
IPAddress local_ip(192, 168, 1, 1);  // Adresse IP pour le point d'accès
IPAddress gateway(192, 168, 1, 1);  // Adresse de la passerelle
IPAddress subnet(255, 255, 255, 0);  // Masque de sous-réseau
IPAddress serveralocal_ip(192, 168, 0, 1);
IPAddress clientlocal_ip(192, 168, 1, 3);
IPAddress empty(0, 0, 0, 0);
IPAddress gatewayA_IP(192, 168, 0, 1);
IPAddress localA_IP(192, 168, 0, 2);

// Variables de statut
bool client_connected = false;
bool client_reconnecting = false;

// Niveau de verbosité pour le débogage
const int verbose = 999;

// Création d'un objet AsyncWebServer sur le port 80
AsyncWebServer server(80);

// Fonction de débogage
void debug(String debugmessage, int verboselvl) {
  if (verbose >= verboselvl) {
    Serial.println(debugmessage);
  }
}

// Fonction de routage local
IPAddress nexthop(String destalias) {
  if (destalias == "servera") {
    return serveralocal_ip;
  } else if (destalias == "client") {
    if (client_reconnecting) {
      debug(destalias + " est en train de se reconnecter. Veuillez patienter ...", 2);
      return empty;
    } else if (client_connected) {
      return clientlocal_ip;
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
  DynamicJsonDocument doc(200); // Taille du document JSON en octets
  deserializeJson(doc, data);
  debug("Données reçues: " + String(data), 2);
  
  if (doc["source"].as<String>() == "client") {
    client_connected = true;
    if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "reconnecting") {
      client_reconnecting = true;
      client_connected = false;
    }
  }
  
  if (doc["destination"].as<String>() != "serverb") {
    if (nexthop(doc["destination"].as<String>()) != empty) {
      senddata(nexthop(doc["destination"].as<String>()), data);
    }
  }
}

// Fonction appelée lorsque la station WiFi est déconnectée
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  debug("Déconnecté du point d'accès WiFi. Raison : " + String(info.wifi_sta_disconnected.reason) + " Reconnexion en cours ...", 1);
  WiFi.begin(wifi_network_ssid, wifi_network_password);
}

// Fonction pour connecter au réseau WiFi
void wifiConnect() {
  // Configuration de la station WiFi
  if (!WiFi.config(localA_IP, gatewayA_IP, subnet)) {
    debug("Échec de la configuration de la station", 1);
  }
  WiFi.begin(wifi_network_ssid, wifi_network_password);
  debug("Connexion au point d'accès A en cours", 2);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
    yield();
  }
  debug("Connecté au point d'accès A avec l'IP locale : " + WiFi.localIP().toString(), 1);
}

// Fonction pour configurer le WiFi
void WifiSetup() {
  // Configuration du WiFi
  wifiConnect();

  // Configuration de l'AP WiFi
  WiFi.mode(WIFI_AP_STA);
  debug("Création du point d'accès B", 1);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  debug("Point d'accès B créé avec l'IP de la passerelle : " + WiFi.softAPIP().toString(), 1);
  debug("Configuration WiFi réussie", 1);
}

// Fonction pour envoyer des données à une adresse IP spécifiée en utilisant le protocole UDP
void senddata(IPAddress ipdestination, String data) {
  HTTPClient http;
  http.begin("http://" + ipdestination.toString() + "/API");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Envoi de la requête HTTP POST
  int httpCode = http.POST("message=" + data);
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      debug("Réponse : " + payload, 2);
    } else {
      debug("HTTP POST ... code : " + String(httpCode), 1);
    }
  } else {
    debug("HTTP POST ... échoué, erreur : " + http.errorToString(httpCode), 1);
  }
  http.end();
}

// Fonction setup exécutée au démarrage
void setup() {
  Serial.begin(115200);
  debug("### CECI EST LE SERVEUR B ###\n\n", 1);

  // Connexion au réseau WiFi
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WifiSetup();
  
  // Configuration du serveur pour gérer les requêtes POST sur /API
  server.on("/API", HTTP_POST, [](AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam("message", true)) {
      message = request->getParam("message", true)->value();
      process_msg(message);
      request->send_P(200, "text/plain", "OK");
    }
  });
  
  // Démarrage du serveur
  server.begin();
}

// Boucle principale
void loop() {
  delay(50);
}

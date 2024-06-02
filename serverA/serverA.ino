//                    SERVEUR A 

#include <WiFi.h>
#include "esp_wifi.h"
#include <string.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_system.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>



//Initialisation des variables
const char* ssid     = "servera";
const char* password = "srvapass10";
const int   channel        = 10;                        // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;                     // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 3;                         // Maximum simultaneous connected clients on the AP

IPAddress local_ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

const char* mac_client = "C0:49:EF:CD:29:30";

bool client_connected_to_a;
bool client_connected_to_b;
int client_position;
struct RSSIValues {
  String rssiA;
  String rssiB;
};
RSSIValues rssiValues;
// Username and Password for Basic Authentication
const char* http_username = "admin";
const char* http_password = "password";

AsyncWebServer server(80);

void display_connected_devices()
{
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

    if (adapter_sta_list.num > 0)
        Serial.println("-----------");
    for (uint8_t i = 0; i < adapter_sta_list.num; i++)
    {
        tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
        Serial.print((String)"[+] Connected Device " + i + " | MAC : ");
        Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", station.mac[0], station.mac[1], station.mac[2], station.mac[3], station.mac[4], station.mac[5]);
        Serial.println((String) " | IP " + ip4addr_ntoa((const ip4_addr_t*)&(station.ip)));
    }
}
bool is_this_mac_address_connected(const char* mac_client)
{
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
    
    
    for (uint8_t i = 0; i < adapter_sta_list.num; i++)
    {
        char mac_addresse[18];
        tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
        snprintf(mac_addresse,18,"%02X:%02X:%02X:%02X:%02X:%02X",station.mac[0], station.mac[1], station.mac[2], station.mac[3], station.mac[4], station.mac[5]);
        if (strcmp(mac_client, mac_addresse) == 0)
        {
          // Mac adress found
          return true;
        }

    }
    // Mac adress not found
    return false;
  
}

float esp_internal_temp() {
  return (temperatureRead() - 32) / 1.8; // Température interne en degrés Celsius
}

size_t available_memory() {
  return ESP.getFreeHeap(); // Mémoire disponible en octets
}

void process_msg(String message){
  
  DynamicJsonDocument doc(200); // Taille du document JSON en octets
  deserializeJson(doc, message);
  Serial.println(String(message));
  if (doc["Destination"].as<String>() == "serva"){
    
    if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "B"){
      client_connected_to_b = true;
      client_connected_to_a = false;
      rssiValues.rssiA = doc["data"]["position"]["rssiA"].as<String>();
      rssiValues.rssiB = doc["data"]["position"]["rssiB"].as<String>();
    }
    else if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "A"){
      client_connected_to_b = false;
      client_connected_to_a = true;
      rssiValues.rssiA = doc["data"]["position"]["rssiA"].as<String>();
      rssiValues.rssiB = doc["data"]["position"]["rssiB"].as<String>();
    }
    else if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "reconnecting"){
      client_connected_to_b = false;
      client_connected_to_a = false;
      rssiValues.rssiA = "0";
      rssiValues.rssiB = "0";
    }
    else if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "None"){
      client_connected_to_b = false;
      client_connected_to_a = false;
      rssiValues.rssiA = "0";
      rssiValues.rssiB = "0";
    }

    
  }
}

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

String pincontroljson(int pinupmber, bool pinstat) {
  // Déclarez un document JSON avec une taille plus grande si nécessaire
  DynamicJsonDocument jsonDoc(256);
  // Ajoutez les valeurs au document JSON
  jsonDoc["Source"] = "serva";
  jsonDoc["Destination"] = "Client";
  jsonDoc["data"]["action"]["pinupmber"] = pinupmber;
  jsonDoc["data"]["action"]["pinstat"] = pinstat;
  // Convertir le document JSON en chaîne
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  // Vérifiez si la sérialisation s'est bien déroulée
  if (jsonString.length() == 0) {
    Serial.println("Erreur: JSON vide.");
  }
  return jsonString;
}

void startFunction(AsyncWebServerRequest *request) {
  // Code pour la fonction start
  Serial.println("Start LED 1");
  request->send(200, "text/plain", "Started");
  sendData("192.168.0.3","/message",pincontroljson(18,1));
  Serial.println("Message sent to the client");
}

void stopFunction(AsyncWebServerRequest *request) {
  // Code pour la fonction stop
  Serial.println("Stop LED 1");
  request->send(200, "text/plain", "Stopped");
  sendData("192.168.0.2","/message",pincontroljson(18,0));
  Serial.println("Message sent to the client");
}

void HTTPSeverSetup() {
  // WebUI
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String response = "<h1>Bienvenue sur l'ESP32 A</h1>";
    response += "<p>IP Address (AP): " + WiFi.softAPIP().toString() + "</p>";
    response += "<p>(Deprecated)Internal Temperature : " + String(esp_internal_temp()) + "C</p>";
    response += "<p>Available memory : " + String(available_memory()) + " bytes</p>";
    response += "<button onclick=\"sendRequest('/startled1')\">Start LED 1</button>";
    response += "<button onclick=\"sendRequest('/stopled1')\">Stop LED 1</button>";
    response += "<p id='status'></p>";
    response += "<script>";
    response += "function sendRequest(url) {";
    response += "  var xhr = new XMLHttpRequest();";
    response += "  xhr.open('GET', url, true);";
    response += "  xhr.onreadystatechange = function () {";
    response += "    if (xhr.readyState == 4 && xhr.status == 200)";
    response += "      document.getElementById('status').innerHTML = xhr.responseText;";
    response += "  };";
    response += "  xhr.send();";
    response += "}";
    response += "</script>";
    request->send(200, "text/html", response);
  });

  // Déclarez les routes pour les boutons
  server.on("/startled1", HTTP_GET, startFunction);
  server.on("/stopled1", HTTP_GET, stopFunction);

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
      // Serial.println("Message reçu : " + message);
    }
    request->send(200, "text/plain", "Message reçu avec succès par A");
  });

  server.begin();
  Serial.println("API HTTP Server A started");
    
}

void setup(){
  Serial.begin(115200);
  Serial.println("### THIS IS SERVER A ###");
  Serial.println("\n[*] Creating Access point A");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  Serial.print("[+] Access Point A created with IP gateway ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Wifi setup successfull");
  
  HTTPSeverSetup();
}

void loop(){
  delay(5000);
  Serial.println("RSSI A :" + String(rssiValues.rssiA) + " | RSSIB :" + String(rssiValues.rssiB));
  // Serial.println("Connected to B : " + String(client_connected_to_b));
  // Serial.println("Connected to A : " + String(client_connected_to_a));
  // if (is_this_mac_address_connected(mac_client)){
  //   client_connected_to_a = true;
  // }
  // else {
  //   client_connected_to_a = false;
  // }
  // Serial.println(client_connected_to_b);


  //display_connected_devices();
  //if (is_this_mac_address_connected(mac_client)){
  //  Serial.println("Client detected on server A");
   // }
  //delay(1000);
}

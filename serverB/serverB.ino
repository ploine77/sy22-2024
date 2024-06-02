//                    SERVEUR B
#include <WiFi.h>
#include "esp_wifi.h"
#include <string.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_system.h>
#include <ArduinoJson.h>
#include <ESP32Ping.h>

// Variable Init //

// Wifi login to connect to AP A
const char* wifi_network_ssid     = "servera";
const char* wifi_network_password =  "srvapass10";

// Wifi parameter for AP
const char *ssid          = "serverb";
const char *password      = "srvbpass10";
const int   channel        = 10;                        // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;                     // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 2;                         // Maximum simultaneous connected clients on the AP
IPAddress local_ip(192, 168, 1, 1);   // Adresse IP que vous souhaitez utiliser pour le point d'accès
IPAddress gateway(192, 168, 1, 1);    // Adresse de la passerelle (habituellement la même que l'adresse IP)
IPAddress subnet(255, 255, 255, 0);   // Masque de sous-réseau


String mac_client = "44:17:93:E3:3D:C4";
String ip_client;
//const char* mac_client = "06:2C:78:F4:46:D2"; //Iphone
bool client_connected_to_b;
int client_position[2];

AsyncWebServer server(80);

// Username and Password for Basic Authentication
const char* http_username = "admin";
const char* http_password = "password";

void jsonData(){
  StaticJsonDocument<200> jsonDoc; // Taille de 200 octets pour le document JSON
  jsonDoc["client_connected_to_b"] = client_connected_to_b;
  JsonObject clientPositionObj = jsonDoc.createNestedObject("client_position");
  // Ajouter les valeurs de client_position à l'objet JSON
  clientPositionObj["x"] = client_position[0];
  clientPositionObj["y"] = client_position[1];

  // Convertir l'objet JSON en chaîne JSON
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  Serial.println(jsonString);

}

float esp_internal_temp() {
  return (temperatureRead() - 32) / 1.8; // Température interne en degrés Celsius
}

size_t available_memory() {
  return ESP.getFreeHeap(); // Mémoire disponible en octets
}

void display_connected_devices(){
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

bool is_this_mac_address_connected(String mac_client) {
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;

    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

    for (uint8_t i = 0; i < adapter_sta_list.num; i++) {
        char mac_address[18];
        tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
        snprintf(mac_address, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                 station.mac[0], station.mac[1], station.mac[2],
                 station.mac[3], station.mac[4], station.mac[5]);

        if (mac_client.equals(String(mac_address))) {
            // Adresse MAC trouvée
            return true;
        }
    }
    // Adresse MAC non trouvée
    return false;
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
      Serial.println("Response : " + payload);
    } else {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST ... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] POST ... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();


}

void process_msg(String message){
  DynamicJsonDocument doc(200); // Taille du document JSON en octets
  deserializeJson(doc, message);
  if (doc["destination"] != "servb"){
    sendData("192.168.0.1","/message",message);
    if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "B"){
      client_connected_to_b = true;
    }
    if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "reconnecting"){
      client_connected_to_b = false;
    }
    mac_client = doc["data"]["connection_data"]["mac"].as<String>();
    ip_client = doc["data"]["connection_data"]["ip"].as<String>();
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
      //Serial.println("Message reçu : " + message);
    }
    request->send(200, "text/plain", "Message reçu avec succès par B");
  });

  server.begin();
  Serial.println("API HTTP Server B started");
    
}

void WifiSetup(){
  // WIFI SETUP
  // WIFI AP Setup
  WiFi.mode(WIFI_AP_STA);
  Serial.println("\n[*] Creating Access point B");
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  Serial.print("[+] Access Point B created with IP gateway ");
  Serial.println(WiFi.softAPIP());

  // WIFI Station Setup
  WiFi.begin(wifi_network_ssid, wifi_network_password);
  Serial.println("\n[*] Connecting to server A Access Point");
  while(WiFi.status() != WL_CONNECTED)
  {
      Serial.print(".");
      delay(100);
  }
  Serial.print("\n[+] Connected to Access Point A with local IP : ");
  Serial.println(WiFi.localIP());
  Serial.print("Wifi setup successfull");
}

bool CanIPingThisIp(String ipaddress){
  // Séparation de l'adresse IP en parties (octets)
  int parts[4];
  int partIndex = 0;
  int lastIndex = 0;

  for (int i = 0; i < ipaddress.length(); i++) {
    if (ipaddress.charAt(i) == '.') {
      parts[partIndex++] = ipaddress.substring(lastIndex, i).toInt();
      lastIndex = i + 1;
    }
    if (i == ipaddress.length() - 1) {
      parts[partIndex] = ipaddress.substring(lastIndex, i + 1).toInt();
    }
  }

  // Création de l'objet IPAddress
  IPAddress ip(parts[0], parts[1], parts[2], parts[3]);
 
 
  return Ping.ping(ip,3);
}

void connectionLostDetection(){
  if (client_connected_to_b && !CanIPingThisIp(ip_client)) {
    Serial.println("Connection lost with the client");
    // Déclarez un document JSON avec une taille plus grande si nécessaire
    DynamicJsonDocument jsonDoc(256);
    // Ajoutez les valeurs au document JSON
    jsonDoc["Source"] = "servb";
    jsonDoc["Destination"] = "serva";
    jsonDoc["data"]["connection_data"]["ConnectedTo"] = "None";

  // Convertir le document JSON en chaîne
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  // Vérifiez si la sérialisation s'est bien déroulée
  if (jsonString.length() == 0) {
    Serial.println("Erreur: JSON vide.");
  }
    sendData("192.168.0.1","/message",jsonString);
    client_connected_to_b = false;
  }
}

void setup(){
  Serial.begin(115200);
  Serial.print("### THIS IS SERVER B ###");
  WifiSetup();
  HTTPSeverSetup();
}

void loop() {
  //Serial.println(String(CanIPingThisIp("192.168.1.2")));
  //display_connected_devices();
  connectionLostDetection();
  // if (is_this_mac_address_connected(mac_client)){
  //   client_connected_to_b = true;
  // }
  // else {
  //   client_connected_to_b = false;
  // }
  //Serial.print(is_this_mac_address_connected(mac_client));
  //sendData("http://192.168.0.1","/message","fromsrvb/client_connected_to_b:" + String(client_connected_to_b));
  delay(1000);
  
  // display_connected_devices();
  // if (is_this_mac_address_connected(mac_client)){
  //     Serial.println("Client detected on server B");
  //     }
  // delay(5000);
  }

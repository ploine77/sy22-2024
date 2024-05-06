//                    SERVEUR A 

#include <WiFi.h>
#include "esp_wifi.h"
#include <string.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>



//Initialisation des variables
const char* ssid     = "servera";
const char* password = "srvapass10";
const int   channel        = 10;                        // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;                     // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 2;                         // Maximum simultaneous connected clients on the AP

IPAddress local_ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

const char* mac_client = "C0:49:EF:CD:29:30";
bool client_connected_to_a;
bool client_connected_to_b;
bool client_not_connected;
int client_position;

float internal_temp;

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

// float esp_internal_temp()
// {
//   temperature_sensor_enable();
//   return temperature_sensor_get_celsius();
// }

void setup()
{
  Serial.begin(115200);
  Serial.println("\n[*] Creating Access point B");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  Serial.print("[+] Access Point B created with IP gateway ");
  Serial.println(WiFi.softAPIP());

  //Serveur HTTP
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String response = "<h1>Bienvenue sur l'ESP8266</h1>";
    response += "<p>Adresse IP : " + WiFi.localIP().toString() + "</p>";
    //response += "<p>" + esp_internal_temp() +  "</p> ";
    request->send(200, "text/html", response);
  });
  server.on("/message", HTTP_POST, [](AsyncWebServerRequest *request){
    // Traitement du message reçu
    // Exemple : affichage du contenu du message dans la console série
    String message;
    if (request->hasParam("message", true)) {
      message = request->getParam("message", true)->value();
      Serial.println("Message reçu : " + message);
    }
    request->send(200, "text/plain", "Message reçu avec succès");
  });

  server.begin();
  Serial.println("Server started");
    
}

void loop()
{



  //display_connected_devices();
  //if (is_this_mac_address_connected(mac_client)){
  //  Serial.println("Client detected on server A");
   // }
  //delay(1000);
}
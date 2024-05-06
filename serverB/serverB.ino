//                    SERVEUR B

#include <WiFi.h>
#include "esp_wifi.h"
#include <string.h>
#include <HTTPClient.h>

// Variable Init
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


const char* serverAddress = "192.168.0.1'/message";

//const char* mac_client = "C0:49:EF:CD:29:30";
const char* mac_client = "06:2C:78:F4:46:D2"; //Iphone
bool client_connected_to_b;
int client_position;

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

void sendData() 
{
  String HOST_NAME   = "http://192.168.0.1"; // CHANGE IT
  String PATH_NAME   = "/message";      // CHANGE IT
  String queryString = "MonMessage"; // OPTIONAL

  HTTPClient http;
  http.begin(HOST_NAME + PATH_NAME);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(queryString);
  
  // httpCode will be negative on error
  if (httpCode > 0) {
    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    } else {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST ... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] POST ... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();


}


void setup()
{
  Serial.begin(115200);

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
  Serial.print("SUCCESFULL SETUP");
}

void loop() { 
  sendData();
  delay(5000);
  
  // display_connected_devices();
  // if (is_this_mac_address_connected(mac_client)){
  //     Serial.println("Client detected on server B");
  //     }
  // delay(5000);
  }
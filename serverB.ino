#include <WiFi.h>
#include "esp_wifi.h"

const char* wifi_network_ssid     = "servera";
const char* wifi_network_password =  "srvapass10";

const char *ssid          = "serverb";
const char *password      = "srvbpass10";
const int   channel        = 10;                        // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;                     // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 2;                         // Maximum simultaneous connected clients on the AP

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  Serial.println("\n[*] Creating serverb AP");
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  Serial.print("[+] serverb AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());

  WiFi.begin(wifi_network_ssid, wifi_network_password);
  Serial.println("\n[*] Connecting to WiFi Network");

  while(WiFi.status() != WL_CONNECTED)
  {
      Serial.print(".");
      delay(100);
  }

  Serial.print("\n[+] serverb connected to the servera WiFi network with local IP : ");
  Serial.println(WiFi.localIP());
  
}

void loop() { 
  }

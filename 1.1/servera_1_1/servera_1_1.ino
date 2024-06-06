#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

WiFiUDP udp;
char packetBuffer[255];
unsigned int localPort = 9999;
const char* ssid     = "servera";
const char* password = "srvapass10";
const int   channel        = 10;                        // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;                     // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 3;                         // Maximum simultaneous connected clients on the AP
IPAddress local_ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
const int verbose = 1;

AsyncWebServer server(80); // Serveur WEB

void debug(String debugmessage, int verboselvl) // Debug function with verbose level
{
  if (verboselvl >= verbose){
    Serial.println(debugmessage);
  }
}

void WifiSetup(){
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  Serial.print("[+] Access Point A created with IP gateway ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Wifi setup successfull");
}

void senddata(IPAddress ipdestination, String data){
  //Serial.print("Sending packet to");
  udp.beginPacket(ipdestination, localPort);
  udp.printf((data + "\r\n").c_str());
 	udp.endPacket();
}

void processdata(String data) // Define what to do with incomming messages
{
  data.replace("\n", "");
  data.replace("\r", "");
  DynamicJsonDocument doc(200); // Taille du document JSON en octets
  deserializeJson(jsondata, data);
  if (jsondata["Destination"] == "serva"){
    Serial.print(data)
  }
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
  server.begin();
  Serial.println("API HTTP Server started");
    
}

void setup() {
 	Serial.begin(115200);
 	// Connect to Wifi network.
 	WifiSetup();
 	udp.begin(localPort);
 	Serial.printf("UDP server : %s:%i \n", WiFi.localIP().toString().c_str(), localPort);
}

void udpserver(){
  int packetSize = udp.parsePacket();
 	if (packetSize > 0) {
    //Serial.print("Received packet from : "); Serial.println(udp.remoteIP());
    //Serial.print("Size : "); Serial.println(packetSize);
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0; // Pour savoir ou la chaîne de caractère se termine
    //Serial.printf("Data : %s\n", packetBuffer);
    processdata(String(packetBuffer) + "\n");
    //senddata(udp.remoteIP(),"OK\r\n");
  } 
}

void loop(){
 	udpserver();
 	delay(500);
}
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>


// Wifi login to connect to AP A
const char* wifi_network_ssid = "servera";
const char* wifi_network_password =  "srvapass10";

// Wifi parameter for AP
const char *ssid = "serverb";
const char *password = "srvbpass10";
const int channel = 10;                        // WiFi Channel number between 1 and 13
const bool hide_SSID = false;                     // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int max_connection = 2;                         // Maximum simultaneous connected clients on the AP

IPAddress local_ip(192, 168, 1, 1);   // Adresse IP que vous souhaitez utiliser pour le point d'accès
IPAddress gateway(192, 168, 1, 1);    // Adresse de la passerelle (habituellement la même que l'adresse IP)
IPAddress subnet(255, 255, 255, 0);   // Masque de sous-réseau
IPAddress serveralocal_ip(192,168,0,1);
IPAddress clientlocal_ip(192,168,1,3);
IPAddress empty(0,0,0,0);
IPAddress gatewayA_IP(192, 168, 0, 1);
IPAddress localA_IP(192, 168, 0, 2);


bool client_connected;
bool client_reconnecting;

const int verbose = 999;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void debug(String debugmessage, int verboselvl){
  if (verbose >= verboselvl){
    Serial.println(debugmessage);
  }
}

IPAddress nexthop(String destalias){ // Il s'agit de la fonction de routage locale
  if (destalias == "servera"){
    return serveralocal_ip;
  }
  else if (destalias == "client"){
    if (client_reconnecting){
      Serial.println(destalias + " is curently reconnecting. Please wait ...");
      return empty;
    }
    else if (client_connected){
      return clientlocal_ip;
    }
    else{
      Serial.println(destalias + " is not connected.");
      return empty;
    }
  }
  else{
    Serial.println(destalias + " not known. There is not route table for this alias.");
    return empty;
  }
}

void process_msg(String data){
  // data.replace("\n", "");
  // data.replace("\r", "");
  //Serial.print(data);
  // DynamicJsonDocument doc(200); // Taille du document JSON en octets
  // deserializeJson(jsondata, data);
  // if (jsondata["Destination"].as<String>() == "serva"){
  //   Serial.print(data)
  // }*
  DynamicJsonDocument doc(200); // Taille du document JSON en octets
  deserializeJson(doc, data);
  Serial.println(String(data));
  if (doc["source"].as<String>() == "client"){
    client_connected = true;
    if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "reconnecting"){
      client_reconnecting = true;
      client_connected = false;
    }
  }
  
  if (doc["destination"].as<String>() != "serverb"){
    if (nexthop(doc["destination"].as<String>()) != empty){
        senddata(nexthop(doc["destination"].as<String>()), data);
    }
  }

}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) //Reconnect to Wi-Fi Network After Lost Connection (Wi-Fi Events) 
{
  //debug("Disconnected from WiFi access point. Reason : " + String(info.wifi_sta_disconnected.reason) + " Reconnection ...",1);
  WiFi.begin(wifi_network_ssid, wifi_network_password);
}

void wifiConnect()
{
  // WIFI Station Setup
  if (!WiFi.config(localA_IP, gatewayA_IP, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(wifi_network_ssid, wifi_network_password);
  debug("[*] Connecting to server A Access Point",2);
  while(WiFi.status() != WL_CONNECTED)
  {
      Serial.print(".");
      delay(100);
      yield();
  }
  debug("[+] Connected to Access Point A with local IP : " + WiFi.localIP().toString(),1);
  
}

void WifiSetup() // Wifi Setup
{ 
  // WIFI SETUP
  wifiConnect();

  // WIFI AP Setup
  WiFi.mode(WIFI_AP_STA);
  debug("[*] Creating Access point B",1);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
  debug("[+] Access Point B created with IP gateway" + WiFi.softAPIP().toString(),1);
  debug("Wifi setup successfull",1);


}

void senddata(IPAddress ipdestination, String data) // Send a string of data to the specify IPAdress using UDP protocol
{
  //Serial.print("Sending packet to " + ipdestination.toString() + "at http://" + ipdestination.toString() + "/API");
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

void setup() // Setup
{
 	Serial.begin(115200);
  Serial.println("### THIS IS SERVER B ###\n\n");

 	// Connect to Wifi network.
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
 	WifiSetup();
 	//Serial.printf("UDP Client : %s:%i \n", WiFi.localIP().toString().c_str(), localPort);
  server.on("/API", HTTP_POST, [](AsyncWebServerRequest *request){
    String message;
    if (request->hasParam("message", true)) {
      message = request->getParam("message", true)->value();
      process_msg(message);
    request->send_P(200, "text/plain", "OK");
    }
  });
  // Start server
  server.begin();
}

void loop(){
 	delay(50);
}
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

WiFiUDP udp;
char packetBuffer[255];
unsigned int localPort = 9999;

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
IPAddress apIP(192, 168, 0, 1);
IPAddress hostgateway(192, 168, 0, 1);
const int verbose = 1;

void debug(String debugmessage, int verboselvl) // Debug function with verbose level
{
  if (verbose >= verboselvl){
    Serial.println(debugmessage);
  }
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) //Reconnect to Wi-Fi Network After Lost Connection (Wi-Fi Events) 
{
  debug("Disconnected from WiFi access point. Reason : " + String(info.wifi_sta_disconnected.reason) + " Reconnection ...",1);
  WiFi.begin(wifi_network_ssid, wifi_network_password);
}

void wifiConnect()
{
  // WIFI Station Setup
  WiFi.begin(wifi_network_ssid, wifi_network_password);
  debug("[*] Connecting to server A Access Point",2);
  while(WiFi.status() != WL_CONNECTED)
  {
      Serial.print(".");
      delay(100);
      yield();
  }
  debug("[+] Connected to Access Point A with local IP : " + String(WiFi.localIP()),1);
  debug("Wifi setup successfull",1);
  introductionhandshake();
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
  debug("[+] Access Point B created with IP gateway" + String(WiFi.softAPIP()),1);


}

void senddata(IPAddress ipdestination, String data) // Send a string of data to the specify IPAdress using UDP protocol
{
  debug("Sending packet ...", 2);
  udp.beginPacket(ipdestination, localPort);
  udp.printf("%s", data.c_str());
 	udp.endPacket();
}

void udpserver() // Listen to the UDP Port
{
  int packetSize = udp.parsePacket();
 	if (packetSize > 0) {
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len - 1] = 0;
    debug("Received packet from : " + String(udp.remoteIP()) + " with size of : " + String(packetSize) + " end data : " + String(packetBuffer),2); 
    //Serial.printf("Data : %s\n", packetBuffer);
    //processdata(packetBuffer);
    //senddata(udp.remoteIP(),"OK\r\n");
  }
} 

void introductionhandshake()
{
  senddata(hostgateway,"I AM SERVB");
}

void setup() // Setup
{
 	Serial.begin(115200);

 	// Connect to Wifi network.
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
 	WifiSetup();
  
 	udp.begin(localPort);
 	//Serial.printf("UDP Client : %s:%i \n", WiFi.localIP().toString().c_str(), localPort);
}

void loop(){
  udpserver();
  //senddata(hostgateway,"Message from Client");
 	delay(2000);
}
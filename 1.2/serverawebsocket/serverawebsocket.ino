#include <WiFi.h>
#include "esp_wifi.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>


const char* ssid     = "servera";
const char* password = "srvapass10";
const int   channel        = 10;                        // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;                     // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 10;                       // Maximum simultaneous connected clients on the AP
IPAddress local_ip(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
IPAddress serverblocal_ip(192,168,0,2);
IPAddress clientlocal_ip(192,168,0,3);
IPAddress empty(0,0,0,0);


const int verbose = 1;
#define BUTTON_PIN 33  // GPIO25 pin connected to button
#define SHORT_PRESS_TIME 500 // 500 milliseconds
// Variables will change:
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
bool previousLedState = false;


bool client_connected_to_a;
bool client_connected_to_b;
bool client_reconnecting;

struct RSSIValues {
  String rssiA;
  String rssiB;
};

RSSIValues rssiValues;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void debug(String debugmessage, int verboselvl){
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

IPAddress nexthop(String destalias){ // Il s'agit de la fonction de routage locale
  if (destalias == "serverb"){
    return serverblocal_ip;
  }
  else if (destalias == "client"){
    if (client_reconnecting){
      Serial.println(destalias + " is curently reconnecting. Please wait ...");
      return empty;
    }
    else if (client_connected_to_a){
      return clientlocal_ip;
    }
    else if (client_connected_to_b){
      return serverblocal_ip;
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
  if (doc["destination"].as<String>() == "servera"){
    
    if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "B"){
      client_connected_to_b = true;
      client_connected_to_a = false;
      client_reconnecting = false,
      rssiValues.rssiA = doc["data"]["position"]["rssiA"].as<String>();
      rssiValues.rssiB = doc["data"]["position"]["rssiB"].as<String>();
    }
    else if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "A"){
      client_connected_to_b = false;
      client_connected_to_a = true;
      client_reconnecting = false,
      rssiValues.rssiA = doc["data"]["position"]["rssiA"].as<String>();
      rssiValues.rssiB = doc["data"]["position"]["rssiB"].as<String>();
    }
    else if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "reconnecting"){
      client_connected_to_b = false;
      client_connected_to_a = false;
      client_reconnecting = true;
      rssiValues.rssiA = "0";
      rssiValues.rssiB = "0";
    }
    else if (doc["data"]["connection_data"]["ConnectedTo"].as<String>() == "None"){
      client_connected_to_b = false;
      client_connected_to_a = false;
      client_reconnecting = false,
      rssiValues.rssiA = "0";
      rssiValues.rssiB = "0";
    }
  }
}

String pincontroljson(int pinupmber, bool pinstat) {
  // Déclarez un document JSON avec une taille plus grande si nécessaire
  DynamicJsonDocument jsonDoc(256);
  // Ajoutez les valeurs au document JSON
  jsonDoc["source"] = "servera";
  jsonDoc["destination"] = "client";
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


void setup() {
 	Serial.begin(115200);
  Serial.println("### THIS IS SERVER A ###\n\n");
 	// Connect to Wifi network.
 	WifiSetup();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String response = "<h1>Bienvenue sur le serveur A (Master)</h1>";
    //response += "<p>IP Address (AP) : " + WiFi.softAPIP().toString() + "</p>";
    //response += "<p>IP Adress (STATION) : " + WiFi.localIP().toString() + "</p>";
    //response += "<p>(Deprecated)Internal Temperature : " + String(esp_internal_temp()) + "C</p>";
    //response += "<p>Available memory : " + String(available_memory()) + " bytes</p>";
    response += "<p>RSSI A : " + String(rssiValues.rssiA) + "</p>";
    response += "<p>RSSI B : " + String(rssiValues.rssiB) + "</p>";
    response += "<p>client_connected_to_a : " + String(client_connected_to_a) + "</p>";
    response += "<p>client_connected_to_b : " + String(client_connected_to_b) + "</p>";
    response += "<p>client_reconnecting : " + String(client_reconnecting) + "</p>";
    request->send(200, "text/html", response);
  });
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

void buttonpress(){
  currentState = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && currentState == LOW)       // button is pressed
    pressedTime = millis();
  else if (lastState == LOW && currentState == HIGH) { // button is released
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if ( pressDuration < SHORT_PRESS_TIME )
      Serial.println("A short press is detected");
      String message = pincontroljson(12, !previousLedState);
      Serial.println("Sending : " + message);
      if (nexthop("client") != empty){
        senddata(nexthop("client"), message);
      }
      //senddata(clientlocal_ip, message);
      
      previousLedState = !previousLedState;

      
  }

  // save the the last state
  lastState = currentState;
}

void loop(){
  //Serial.println ("In the looop");
  buttonpress();
  // if (client_connected_to_a){

  // };
  //senddata(clientlocal_ip, "Hey");
  delay(200);
  //Serial.println("RSSI A : " + String(rssiValues.rssiA) + " | RSSI B : " + String(rssiValues.rssiB + "\r"));
  //Serial.println("A : " + rssiValues.rssiA + " | B : " + rssiValues.rssiB);
  //Serial.println("IP" + client_ip.toString());
  
}

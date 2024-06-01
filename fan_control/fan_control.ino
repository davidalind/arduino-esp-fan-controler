/*
ESP8266 Arduino 4 pin fan controler

Code for the webserver and websocket was found here
https://randomnerdtutorials.com/esp8266-nodemcu-websocket-server-arduino/


*/

// Import required libraries

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <ESP8266mDNS.h>
//#include <AsyncElegantOTA.h>
//#include <PubSubClient.h>
#include "credentials.h"
//#include "html.h"
#include "eepromconf.h"


const char *ap_ssid PROGMEM = AP_SSID;
const char *ap_password PROGMEM = AP_PASSWORD;
AsyncWebServer server(80);
//AsyncWebSocket ws("/ws");
IPAddress apIP(192, 168, 4, 1);  // Private network address: local & gateway
IPAddress subnet(255,255,255,0);


void setup_wifi_ap() {
  delay(10);

//  WiFi.mode(WIFI_AP_STA);
  WiFi.mode(WIFI_AP);  

  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
/*
  Serial.print("sizeof(macAddr): ");
  Serial.println(sizeof(macAddr));
  Serial.print("strlen(ap_ssid): ");
  Serial.println(strlen(ap_ssid));
*/

  char uniquessid[sizeof(macAddr)*2 + 1 + strlen(ap_ssid)+1];
  sprintf(uniquessid, "%s-%02x%02x%02x%02x%02x%02x\0", ap_ssid, macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  Serial.print("uniquessid: ");
  Serial.println(uniquessid);
//  Serial.print("ap_password: ");
//  Serial.println(ap_password);


  /*
  // add mac adress to ssid
  char uniquessid[sizeof(deviceId) + 1 + sizeof(ap_ssid)];
  sprintf(uniquessid, "%s-%s", ap_ssid, deviceId);
  */

  // AP setup
  
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(apIP, apIP, subnet) ? "Ready" : "Failed!");
  //Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");  

  // if (WiFi.softAP(uniquessid, ap_password, 1, 0, 2) == true) {
//  if (WiFi.softAP(uniquessid) == true) {    
  if (WiFi.softAP(uniquessid) == true) {    
    Serial.print(F("Access Point is Created with SSID: "));
    Serial.println(uniquessid);
    Serial.print(F("Access Point IP: "));
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println(F("Unable to Create Access Point"));
  }
}





void setup() {
//  randomSeed(micros());

  Serial.begin(9600);
  delay(5000);
  Serial.println("hello.");

  // setup EEPROM
  init_conf();

  // setup wifi ap
  setup_wifi_ap();


  // setup /conf GET
  server.on("/conf", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(F("{"));
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(!p->isFile()){
        Serial.println("set conf:");
        Serial.println(p->name().c_str());
        Serial.println(p->value().c_str());
        set_conf(p->name().c_str(), p->value().c_str());
      }
    }
    if(params > 0 && conf_initalized) {
      response->print(F("\"message\":\"Configuration saved.\""));
    }


    if(conf_initalized) {
      int len = sizeof(conf)/sizeof(t_conf);
      for (int i = 0; i < len; i++) {
        if(!conf[i].hidden) {
          response->print(F("\""));
          response->print(conf[i].name);
          response->print(F("\":\""));
          response->print(conf[i].data);
          response->print(F("\""));
          if(i < len - 1) {
            response->print(F(","));
          }
        }
      }
    }
    response->print(F("}"));
    request->send(response);
  });

  // Start web server
  server.begin();
}





void loop() {
  Serial.print(".");
  /*
  Serial.print("mqtt_broker_port: ");
  Serial.println(get_conf((char *)"mqtt_broker_port")->data);

  Serial.print("mqtt_broker_host: ");
  Serial.println(get_conf((char *)"mqtt_broker_host")->data);
  */
//  set_conf((char *)"mqtt_broker_host", (char *)"brock");
//  read_conf();
//  Serial.print("mqtt_broker_host: ");
//  Serial.println(get_conf((char *)"mqtt_broker_host")->data);

//  delay(100);
}

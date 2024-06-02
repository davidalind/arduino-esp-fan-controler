/*
ESP8266 Arduino 4 pin fan controler

Code for the webserver and websocket was found here
https://randomnerdtutorials.com/esp8266-nodemcu-websocket-server-arduino/


*/

// Import required libraries

//https://arduino-esp8266.readthedocs.io/en/latest/index.html

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <ESP8266mDNS.h>
#include <AsyncElegantOTA.h>
//#include <PubSubClient.h>
#include "credentials.h"
#include "html.h"
#include "eepromconf.h"


const char *ap_ssid PROGMEM = AP_SSID;
const char *ap_password PROGMEM = AP_PASSWORD;
AsyncWebServer server(80);
//AsyncWebSocket ws("/ws");
IPAddress apIP(192, 168, 4, 1);  // Private network address: local & gateway
IPAddress subnet(255,255,255,0);
char *response_buf;
int response_maxLen = 0;



void setup_wifi_ap() {
  delay(10);

//  WiFi.mode(WIFI_AP_STA);
  WiFi.mode(WIFI_AP_STA);  

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


void setup_wifi_sta() {
  // STA WiFi setup
  WiFi.disconnect();
  delay(100);
  WiFi.scanNetworks(true);
//  if(atoi(get_conf((char *)"wifi_enabled").data)) {
//    WiFi.begin(get_conf((char *)"wifi_ssid").data, get_conf((char *)"wifi_password").data);
//  }
}


String processor(const String& var)
{
  if(var == "HEADER")
    return header_html;
  else if (var == "FOOTER")
    return footer_html;
  return String();
}




void setup() {
//  randomSeed(micros());

  Serial.begin(115200);
  delay(5000);
  Serial.println("hello.");

  // setup EEPROM
  init_conf();

  // setup wifi ap
  setup_wifi_ap();

  // setup wifi sta
  setup_wifi_sta();

/*
  //find max html content lenght
  response_maxLen += strlen(header_html);
  response_maxLen += strlen(footer_html);
  int content_len = strlen(index_html);
  if (strlen(wifi_html) > content_len) content_len = strlen(wifi_html);
  if (strlen(mqtt_html) > content_len) content_len = strlen(mqtt_html);
  response_maxLen += 1;
  // allocate buffer
  response_buf = (char *) calloc(response_maxLen, sizeof(char));
*/


//TODO: change to String response (see wifiscan)
  // setup /conf GET
  server.on("/conf", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
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
      response->print(F("\"message\":\"Configuration saved.\","));
    }

/*
{
  "message": "Configuration saved.",
  "conf": [
    { "name": "mqtt_broker_port", "value": "1883"}
  ]
}
*/
    response->print(F("\"conf\": [ "));
    if(conf_initalized) {
      int len = sizeof(conf)/sizeof(t_conf);
      for (int i = 0; i < len; i++) {
        if(!conf[i].hidden) {
          response->print(F("{ \"name\": \""));
          response->print(conf[i].name);
          response->print(F("\", \"value\": \""));
          response->print(conf[i].data);
          response->print(F("\" }"));
          if(i < len - 1) {
            response->print(F(","));
          }
        }
      }
    }
    response->print(F(" ]"));
    response->print(F(" }"));
    request->send(response);
  });

    // setup /wifi
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", wifi_html, processor);
    request->send(response);
  });



  /*
  {
    "message": "Error: SSIB scan failed.",
    "networks": [
      { "ssid": "asus", "rssi": "-65"},
      { "ssid": "cisco", "rssi": "-35"},
    ]
  }
  */

/*
  server.on("/wifiscan", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("create response");
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    Serial.println("start printing to reponse");
    response->print("{");

    Serial.println("disconnecting");

//    Serial.println("switch to WIFI_STA");
//    WiFi.mode(WIFI_STA);

    Serial.println("disconnecting");
    WiFi.disconnect(); //Running WiFi.disconnect() is to shut down a connection to an access point that module may have automatically made using previously saved credentials.

    Serial.println("delay");
    delay(100);

    Serial.println("start scan");

    system_soft_wdt_stop(); // disable the software watchdog
    system_soft_wdt_feed(); //this feeds the software AND the hardware watchdog
    // now, you can place your code here -- it has ~ 6 seconds until the hardware watchdog bites
    int n = WiFi.scanNetworks();
    system_soft_wdt_feed(); // feed again
    system_soft_wdt_restart(); // enable software watchdog again

//    Serial.println("switch to WIFI_AP_STA");
//    WiFi.mode(WIFI_AP_STA);  

    Serial.print(n);
    Serial.println(" network(s) found");

    char *rssi_buf = (char *) calloc(10, sizeof(char));

    if(n < 1) {
      response->print(F("\"message\":\"No networks found.\","));
    } else {
      response->print(F("\"networks\": [ "));
      for (int i = 0; i < n; i++)     {
        Serial.println(WiFi.SSID(i));

        response->print(F("{ \"ssid\": \""));
        response->print(WiFi.SSID(i).c_str());

        response->print(F("\", \"rssi\": \""));
        response->print(itoa(WiFi.RSSI(i), rssi_buf, 10));

        response->print(F("\" }"));  

        if(i<n-1) response->print(F(","));
      }
      response->print(F(" ]"));
    }

    response->print(F(" }"));
    request->send(response);
    WiFi.scanDelete();
  });
*/

// https://github.com/me-no-dev/ESPAsyncWebServer/issues/85
  server.on("/wifiscan", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"networks\": [ ";
    int n = WiFi.scanComplete();
    if(n == -2){
      WiFi.scanNetworks(true);
    } else if(n){
      for (int i = 0; i < n; ++i){
        if(i) json += ",";
        json += "{";
        json += "\"ssid\":\""+WiFi.SSID(i)+"\"";
        json += ", \"rssi\":\""+String(WiFi.RSSI(i))+"\"";
        json += "}";
      }
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    json += "}";    
    request->send(200, "text/json", json);
    json = String();
  });

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server);  
  //AsyncElegantOTA.begin(&server, "username", "password");


  // Start web server
  server.begin();
}





void loop() {
  //Serial.print(".");
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

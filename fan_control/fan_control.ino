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
#include <PubSubClient.h>
#include "credentials.h"
#include "html.h"
#include "eepromconf.h"

// ID string for SSID and MQTT clientid.
char *unique_id_str;

// WiFi
const char *ap_ssid PROGMEM = AP_SSID;
const char *ap_password PROGMEM = AP_PASSWORD;
AsyncWebServer server(80);
//AsyncWebSocket ws("/ws");
IPAddress apIP(192, 168, 4, 1);  // Private network address: local & gateway
IPAddress subnet(255,255,255,0);
//char *response_buf;
//int response_maxLen = 0;

int lastnetworkscan = 0;
#define WL_CONNECTING 99 // extend the wifi states with own state
long last_connection_attempt = 0;
long connection_attempt_timeout = 30000; // ms
bool connection_attempt_started = false;


//  MQTT
WiFiClient espClient;
PubSubClient client(espClient);
long mqtt_last_connection_attempt = 0;
long mqtt_connection_attempt_timeout = 5000; // ms
bool mqtt_connection_attempt_started = false;


void setup_mqtt() {
  // add mac adress to client id
//0  sprintf(mqtt_unique_client_id, "%s-%s", mqtt_client_id, deviceId);
//  if(atoi(get_conf("mqtt_enabled").data)) {
//    client.setServer(get_conf("mqtt_broker_host").data, atoi(get_conf("mqtt_broker_port").data));
    client.setCallback(mqtt_on_message);
//  }
}


void mqtt_on_message(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("MQTT: "));
  Serial.print(topic);
  Serial.print(F(" <- "));
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


void mqtt_loop() {

  if (client.connected() && !atoi(get_conf("mqtt_enabled")->data)) {
    Serial.println("mqtt disconnecting.");
    client.disconnect();
  }

  if(
    !mqtt_connection_attempt_started &&
    WiFi.status() == WL_CONNECTED &&  // need sta wifi 
    !client.connected() &&          // check we're not already connected
    atoi(get_conf("mqtt_enabled")->data)   // check if wifi is enabled in conf
  ){
    Serial.print("mqtt connecting...");
    client.setServer(get_conf("mqtt_broker_host")->data, atoi(get_conf("mqtt_broker_port")->data));
    client.connect(unique_id_str);
    mqtt_connection_attempt_started = true;
    mqtt_last_connection_attempt = millis();
  }

  if (mqtt_connection_attempt_started) {
      if (client.connected()) {
        Serial.println(" mqtt connected");
        mqtt_connection_attempt_started = false;
        // Once connected, publish an announcement...
        mqtt_publish("ip", WiFi.localIP().toString());
        // ... and resubscribe
        //  client.subscribe("inTopic");
      }
      if (millis() > mqtt_last_connection_attempt + mqtt_connection_attempt_timeout) {
        mqtt_connection_attempt_started = false;
      }
  }
  client.loop();
}

void mqtt_publish(String t, String p) {
  t = String(get_conf("mqtt_root")->data) + "/" + String(unique_id_str) + "/" + t;
  client.publish(t.c_str(), p.c_str());
  Serial.println(p + " -> " + t);
}


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

  unique_id_str = (char *)calloc(sizeof(macAddr)*2 + 1 + strlen(ap_ssid)+1, sizeof(char));
  sprintf(unique_id_str, "%s-%02x%02x%02x%02x%02x%02x\0", ap_ssid, macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  Serial.print("unique_id_str: ");
  Serial.println(unique_id_str);
//  Serial.print("ap_password: ");
//  Serial.println(ap_password);


  /*
  // add mac adress to ssid
  char unique_id_str[sizeof(deviceId) + 1 + sizeof(ap_ssid)];
  sprintf(unique_id_str, "%s-%s", ap_ssid, deviceId);
  */

  // AP setup
  
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(apIP, apIP, subnet) ? "Ready" : "Failed!");
  //Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");  

  // if (WiFi.softAP(unique_id_str, ap_password, 1, 0, 2) == true) {
//  if (WiFi.softAP(unique_id_str) == true) {    
  if (WiFi.softAP(unique_id_str) == true) {    
    Serial.print(F("Access Point is Created with SSID: "));
    Serial.println(unique_id_str);
    Serial.print(F("Access Point IP: "));
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println(F("Unable to Create Access Point"));
  }
}

// STA WiFi setup
void setup_wifi_sta() {
  // start by doing a scan for avail networks
  WiFi.disconnect();
  delay(200);
  WiFi.scanNetworks(true);


//  if(atoi(get_conf("wifi_enabled").data)) {
//    WiFi.begin(get_conf("wifi_ssid").data, get_conf("wifi_password").data);
//  }
}


/*
-4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
-3 : MQTT_CONNECTION_LOST - the network connection was broken
-2 : MQTT_CONNECT_FAILED - the network connection failed
-1 : MQTT_DISCONNECTED - the client is disconnected cleanly
0 : MQTT_CONNECTED - the client is connected
1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
*/
#define MQTT_CONNECTING 99 // extend the states with own state
String get_mqtt_status_str(int status) {
  String s = "";
//  int s = WiFi.status();
  switch(status) {
    case MQTT_CONNECTION_TIMEOUT:
      s = "Connection timeout.";
      break;
    case MQTT_CONNECTION_LOST:
      s = "Connection lost.";
      break;
    case MQTT_CONNECT_FAILED:
      s = "Connecting failed.";
      break;
    case MQTT_DISCONNECTED:
      s = "Disconnected.";
      break;
    case MQTT_CONNECTED:
      s = "Connected.";
      break;
    case MQTT_CONNECT_BAD_PROTOCOL:
      s = "Bad protocol.";
      break;
    case MQTT_CONNECT_BAD_CLIENT_ID:
      s = " Client id rejected.";
      break;    
    case MQTT_CONNECT_UNAVAILABLE:
      s = " Connection not accepted.";
      break;
    case MQTT_CONNECT_BAD_CREDENTIALS:
      s = "Wrong username/password.";
      break;
    case MQTT_CONNECT_UNAUTHORIZED:
      s = "Client rejected.";
      break;
    case MQTT_CONNECTING:
      s = "Connecting...";
      break;
    default:
      s = "Idle";
      break;
  }
  return s;
}


/*
typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_WRONG_PASSWORD   = 6,
    WL_DISCONNECTED     = 7
} wl_status_t;
*/
#define WL_CONNECTING 99 // extend the states with own state
String get_wifi_status_str(int status) {
  String s = "";
//  int s = WiFi.status();
  switch(status) {
    case WL_IDLE_STATUS:
      s = "Idle.";
      break;    
    case WL_DISCONNECTED:
      s = "Disconnected.";
      break;
    case WL_CONNECTED:
      s = "Connected to " + WiFi.SSID() + " (Local IP: " + WiFi.localIP().toString() + ")";
      break;
    case WL_CONNECTING:
      s = "Connecting to " + WiFi.SSID() + "...";
      break;
    case WL_NO_SSID_AVAIL:
      s = "AP not found.";
      break;
    case WL_CONNECT_FAILED:
      s = "Connecting failed.";
      break;
    case WL_CONNECTION_LOST:
      s = "Connection lost.";
      break;      
    case WL_WRONG_PASSWORD:
      s = "Wrong password.";
      break;
    default:
      s = "Connecting failed: " + String(status);
      break;
  }
  return s;
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



/*
{
  "messages": [
    { "type": "ok", "text": "Configuration saved."}
  ]
  "conf": [
    { "name": "mqtt_broker_port", "value": "1883"},
    ...
  ]
}
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
        Serial.print("set conf: ");
        Serial.print(p->name().c_str());
        Serial.print(", ");
        Serial.println(p->value().c_str());
        set_conf(p->name().c_str(), p->value().c_str());
      }
    }

    if(params > 0 && conf_initalized) {
      response->print(F("\"messages\": [ "));
      response->print(F("{\"type\":\"ok\", \"text\":\"Configuration saved.\"}"));
      response->print(F("],"));
    }


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

    // setup /mqtt
  server.on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", mqtt_html, processor);
    request->send(response);
  });



  /*
  {
    "messages": [
      { "type": "ok", "text": "Error: SSID scan failed."},
      { "type": "fail", "text": "hej"}
    ]
    
    "networks": [
      { "ssid": "asus", "rssi": "-65"},
      { "ssid": "cisco", "rssi": "-35"}
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

    // check if we need to send messages
    int m = 0;
    json += F("\"messages\": [ ");
    if (WiFi.status() == WL_CONNECTED) {
      json += F("{\"type\":\"fail\", \"text\":\"Warning: Cannot scan for networks when connected to network. SSID list might be outdated. Disable WiFi and try again.\"}");
      m++;
    }
    
    int n = lastnetworkscan;

    if (n<1) {
      if(m) json += F(",");
      json += F("{\"type\":\"fail\", \"text\":\"No networks found.\"}");
    }

    json += F("]");

    // build network json
    if(n) {
      json += F(", \"networks\": [ ");
      for (int i = 0; i < n; ++i){
        if(i) json += F(",");
        json += F("{");
        json += "\"ssid\":\""+WiFi.SSID(i)+"\"";
        json += ", \"rssi\":\""+String(WiFi.RSSI(i))+"\"";
        json += F("}");
      }
      json += F("]");
    }

    // delete scan result and trigger rescan for networks
    if (WiFi.status() == WL_DISCONNECTED) {
      WiFi.scanDelete();
      WiFi.scanNetworks(true);
    }

    json += F("}");    
    request->send(200, F("text/json"), json);
    json = String();
  });



  /*
  {
    "status": [
      { "name": "wifi_status": text: "[WiFi] Connected to: laban."},
      ...
    ]
  }
  */
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";

    json += F("\"status\": [ ");

    json += F("{\"name\":\"wifi_status\",");
    json += F("\"text\":\"WiFi: ") + (connection_attempt_started ? get_wifi_status_str(WL_CONNECTING) : get_wifi_status_str(WiFi.status())) + F("\"}");
	  json += F(",");
    json += F("{\"name\":\"mqtt_status\",");
    json += F("\"text\":\"MQTT: ") + (mqtt_connection_attempt_started ? get_mqtt_status_str(MQTT_CONNECTING) : get_wifi_status_str(client.state())) + F("\"}");
	  json += F(",");
    json += F("{\"name\":\"client_id\",");
    json += F("\"text\":\"Client ID: ") + String(unique_id_str) + F("\"}");
    json += F(",");
    json += F("{\"name\":\"free_memory\",");
    json += F("\"text\":\"Free memory: ") + String(ESP.getFreeHeap()) + F("\"}");

    json += F("]");
    json += F("}");    

    request->send(200, F("text/json"), json);

    json = String();
  });

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server);  
  //AsyncElegantOTA.begin(&server, "username", "password");


  // Start web server
  server.begin();
}






void loop() {


// wifi_loop() {
  if (WiFi.status() == WL_CONNECTED && atoi(get_conf("wifi_enabled")->data) == 0) {
    Serial.println("disconnecting.");
    WiFi.disconnect();
    delay(100);
  }

  int n = WiFi.scanComplete();
  if (n>-1) {
      lastnetworkscan = n;
  }

  if(
    n != -1 &&                                // check ssid scan is not running
    !connection_attempt_started &&
    WiFi.status() != WL_CONNECTED &&          // check we're not already connected
    WiFi.getMode() & WIFI_STA &&           // check we're in staion mode
    atoi(get_conf("wifi_enabled")->data)   // check if wifi is enabled in conf
  ){
    Serial.print("connecting...");
    WiFi.begin(get_conf("wifi_ssid")->data, get_conf("wifi_password")->data);
    connection_attempt_started = true;
    last_connection_attempt = millis();

  }

  if (connection_attempt_started) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" connected");
        connection_attempt_started = false;
      }
      if (millis() > last_connection_attempt + connection_attempt_timeout) {
        connection_attempt_started = false;
      }
  }
// }



  mqtt_loop();


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

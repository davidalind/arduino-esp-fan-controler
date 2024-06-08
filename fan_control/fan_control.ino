/*
ESP8266 Arduino 4 pin fan controler

Code for the webserver and websocket was found here
https://randomnerdtutorials.com/esp8266-nodemcu-websocket-server-arduino/


*/

// Import required libraries

//https://arduino-esp8266.readthedocs.io/en/latest/index.html

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <ESP8266mDNS.h>
#include <AsyncElegantOTA.h>

#include "credentials.h"
#include "html.h"
#include "kludda_eeprom.h"
#include "kludda_wifi.h"
#include "kludda_mqtt.h"


// webserver
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");



// PWM conf
const byte PWM_PIN = D1;
#define PWM_FREQ_HZ 25000 // PWM frequency in HZ

// TACHO conf
const byte TACHO_PIN = D4;


int dutycycle = 0;
unsigned int rpm = 0;
volatile unsigned int num_tacho = 0;
unsigned long lastrpmcalctime = 0;
unsigned long lasttacho = 0;
unsigned long lastrpmduration = 0;
unsigned long lastrpmsendtime = 0;

int lasttachostate = LOW;
unsigned long lasttachopoll = 0;






void webSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// assumes key=value
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    //data[len] = 0;

    Serial.print("WS len: ");
    Serial.println(len);


    String message = "";

    for(int i = 0; i < len; i++) {
      message += (char)data[i];
    }
  //  message += '\0';



//    String message = String((char*)data);
//    message.trim(); // remove leading + trailing whitespace

    Serial.print("WS message: ");
    Serial.println(message);


    int pos = message.indexOf("=");
    if (pos != -1) {
      String key = message.substring(0, pos);
   //   key.trim();
      String value = message.substring(pos + 1);
   //   value.trim();

      Serial.print("WS key: ");
      Serial.println(key);

      Serial.print("WS value: ");
      Serial.println(value);

      if (key == "dutycycle") {
        Serial.print("Got new DC: ");
        Serial.println(key);

        dutycycle = value.toInt();

        Serial.print("DC: ");
        Serial.println(dutycycle);

        set_dc(dutycycle);
      }
    }

    message = String();



// ~~ https://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string ~~
// https://forum.arduino.cc/t/serial-input-basics-updated/382007/3

/*
    char key[20] = {0};
    char value[20] = {0};    
    char * strtokIndx; // this is used by strtok() as an index
    strtokIndx = strtok((char *)data,",");      // get the first part - the key
    strcpy(key, strtokIndx); // copy it to key
    strtokIndx = strtok(NULL,",");      // get the first part - the value
    strcpy(value, strtokIndx); // copy it to value

    if (strcmp(key, (const char*)"dutycycle") == 0) {
      dutycycle = atoi(value);
      Serial.print("Got new DC: ");
      Serial.println(dutycycle);
      //dutycycle = message.toInt();
      if (dutycycle < 0) { dutycycle = 0; }
      if (dutycycle > 100) { dutycycle = 100; }
      analogWrite(PWM_PIN, dutycycle);
      ws.textAll("{\"dutycycle\":" + String(dutycycle) + "}");
    }
*/
/*
    else if (strcmp(key, (const char*)"ssid") == 0) {
      strcpy(ssid, value);
      writeEEPROM(EEPROM_SSID_START,EEPROM_SSID_LEN,value);
    }
    else if (strcmp(key, (const char*)"scanssid") == 0) {
      scanssid();
    }
    else if (strcmp(key, (const char*)"password") == 0) {
      strcpy(password, value);
      writeEEPROM(EEPROM_PASSWORD_START,EEPROM_PASSWORD_LEN,value);
    }
*/
  }
}

void set_dc(int dc) {
  if (dc < 0) dc = 0;
  if (dc > 100) dc = 100;
  dutycycle = dc;
  analogWrite(PWM_PIN, dutycycle);
  ws.textAll("{\"dutycycle\":" + String(dutycycle) + "}");

}
/*
void mqtt_on_message(char* topic, byte* payload, unsigned int length) {
  String p = "";
  String t = String(topic);

  for(int i = 0; i < length; i++) {
    p += (char)payload[i];
  }

  Serial.println("Got MQTT in: ");
  Serial.println(t);
  Serial.println(p);
  t = mqtt_trim(t);

  Serial.println(t);
  Serial.println(p.toInt());

  if (t == "/fan/dutycycle/set") {
    Serial.println("got matching dc set");
    set_dc(p.toInt());
  } 

  p = String();
  t = String();

}
*/



void on_mqtt_dc_set (String *t, String *p) {
  set_dc(p->toInt());
}





String processor(const String& var)
{
  if(var == "HEADER")
    return header_html;
  else if (var == "FOOTER")
    return footer_html;
  else if (var == "UNIQUEID")
    return String(unique_id_str);
  return String();
}




void setup() {
//  randomSeed(micros());

  Serial.begin(115200);
  delay(100);
  Serial.println("hello.");



  // Setup PWM output
  pinMode(PWM_PIN, OUTPUT);      // Configure PWM pin to output
  analogWriteRange(100);         // Configure range to be 0-100 to skip conversion later
  analogWriteFreq(PWM_FREQ_HZ);  // Configure PWM frequency
  analogWrite(PWM_PIN, 0);

  // setup tacho
  pinMode(TACHO_PIN, INPUT_PULLUP);  // Configure TACHO pin to output
  // attachInterrupt triggers on noise. reverting to polling.
  // attachInterrupt(digitalPinToInterrupt(TACHO_PIN), counttacho, FALLING); // 2 interrupts per revolution


  // setup EEPROM
  init_conf();

  // setup wifi
  setup_wifi();

  // setup wifi
  setup_mqtt();
  mqtt_subscribe("fan/dutycycle/set",  on_mqtt_dc_set);
  
//  mqtt_client.setCallback(mqtt_on_message);


  // setup websocket
  ws.onEvent(webSocketEvent);  // if there's an incomming websocket message, go to function 'webSocketEvent'
  server.addHandler(&ws);
  Serial.println(F("WebSocket server started."));

  // setup /
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", fan_html, processor);
    request->send(response);
  });

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
    json += F("\"text\":\"WiFi: ") + get_wifi_status_str() + F("\"}");
	  json += F(",");
    json += F("{\"name\":\"mqtt_status\",");
    json += F("\"text\":\"MQTT: ") + get_mqtt_status_str() + F("\"}");
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



//ICACHE_RAM_ATTR void counttacho() {
//  num_tacho++;

  //    lasttacho = millis();
  /*
    if(num_tacho > 39) {
      //rpm = int(float(num_tacho / float((millis() - lastrpmcalctime)/1000)) * float(30)); //60 / 2;
//      rpm = int(float(float(num_tacho) / float((millis() - lastrpmcalctime))) * float(30)); //60 / 2;
//      rpm = int(12000 / (millis() - lastrpmcalctime));
//      rpm = int(300000 / (millis() - lastrpmcalctime));
      //rpm = (num_tacho / (millis() - lastrpmcalctime)) * 30000;
      lastrpmduration = millis() - lastrpmcalctime;
      num_tacho = 0;
      lastrpmcalctime = millis();
    }
    */
//}





void loop() {
  wifi_loop();
  mqtt_loop();


  // Check state of tacho input @1000 Hz.
  // If changed: count.
  // Fans have 2 pulses / rev => 4 changes / rev
  if (micros() - lasttachopoll > 1000) {
    int pinstate = digitalRead(TACHO_PIN);
    if (pinstate != lasttachostate) {
      num_tacho++;
      lasttachostate = pinstate;
    }
    lasttachopoll += 1000;
  }



  // Push RPM
  if (lastrpmsendtime + 1000 < millis()) {
	  // number of pulses
    // noInterrupts();
    unsigned long mytachos = num_tacho;
    num_tacho = 0;
    // interrupts();

	  // calc RPM from pulses and duration since last calulation
	  // rpm = (mytachos * (unsigned long)30000) / (millis() - lastrpmsendtime); // 2 counts per rev
    rpm = (mytachos * (unsigned long)15000) / (millis() - lastrpmsendtime); // 4 counts per rev
	  lastrpmsendtime = millis();

  	
  	// Push JSON object with data to WS clients
    Serial.println("RPM: " + String(rpm));
    ws.textAll("{\"rpm\":" + String(rpm) + ", \"dutycycle\":" + String(dutycycle) + "}");
    mqtt_publish("fan/rpm", String(rpm));
    mqtt_publish("fan/dutycycle", String(dutycycle));    
    mqtt_publish("mcu/freeheap", String(ESP.getFreeHeap()));
//    ws.textAll("{\"rpm\":" + String(rpm) + ", \"dutycycle\":" + String(dutycycle) + ", \"freemem\":" + String(ESP.getFreeHeap())  + "}");
  

    /*
    //Serial.println(WiFi.localIP());
    if (WiFi.status() != WL_CONNECTED) {
      ws.textAll("{\"wifi\":\"not connected\"}");
    } else {
      ws.textAll("{\"wifi\":\"connected (IP: " + WiFi.localIP() + "\"}");
    }
    */

  }

}

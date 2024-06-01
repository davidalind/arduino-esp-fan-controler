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
#include <AsyncElegantOTA.h>
#include <PubSubClient.h>
#include "credentials.h"
#include "html.h"

char deviceId[12]; // mac adress w/o :


// PWM conf
const byte PWM_PIN = D1;
#define PWM_FREQ_HZ 25000 // PWM frequency in HZ

// TACHO conf
const byte TACHO_PIN = D4;

// Web server conf
//const char *mdnsName PROGMEM = "fancontrol";  // Domain name for the mDNS responder. This makes the ESP crash...
const char *ap_ssid PROGMEM = AP_SSID;
const char *ap_password PROGMEM = AP_PASSWORD;
#define SSID_LIST_MAXLEN = 30;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
IPAddress apIP(192, 168, 4, 1);  // Private network address: local & gateway

const char* mqtt_client_id = MQTT_CLIENTID;
const char* mqtt_unique_client_id = (char*) calloc(strlen(deviceId) + strlen(mqtt_client_id) + 2 * sizeof(char));
WiFiClient espClient;
PubSubClient client(espClient);
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];



char ledState = LOW;

int dutycycle = 0;
unsigned int rpm = 0;
volatile unsigned int num_tacho = 0;
unsigned long lastrpmcalctime = 0;
unsigned long lasttacho = 0;
unsigned long lastrpmduration = 0;
unsigned long lastrpmsendtime = 0;

int lasttachostate = LOW;
unsigned long lasttachopoll = 0;



// populate select
// https://stackoverflow.com/questions/73708331/how-to-populate-a-select-dropdown-with-a-list-of-json-values-using-js-fetch
// ssidlist shall be an array

void setup_wifi_ap() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_AP_STA);
 
  // add mac adress to ssid
  char uniquessid[sizeof(deviceId) + 1 + sizeof(ap_ssid)];
  sprintf(uniquessid, "%s-%s", ap_ssid, deviceId);


  // AP setup
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  if (WiFi.softAP(uniquessid, ap_password, 1, 0, 2) == true) {
    Serial.print(F("Access Point is Creadted with SSID: "));
    Serial.println(uniquessid);
    Serial.print(F("Access Point IP: "));
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println(F("Unable to Create Access Point"));
  }
}


void setup_wifi_sta() {
  // STA WiFi setup
  if(atoi(get_conf("wifi_enabled").data)) {
    WiFi.begin(get_conf("wifi_ssid").data, get_conf("wifi_password").data);
  }
}


void setup_mqtt() {
  // add mac adress to client id
  sprintf(mqtt_unique_client_id, "%s-%s", mqtt_client_id, deviceId);
  if(atoi(get_conf("mqtt_enabled").data)) {
    client.setServer(get_conf("mqtt_broker_host").data, atoi(get_conf("mqtt_broker_port").data));
    client.setCallback(onMQTTMessage);
  }
}

void reconnect_mqtt() {
  // Loop until we're reconnected
//  while (!client.connected()) {
//    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(mqtt_unique_client_id)) {
//      Serial.println("connected");
      // Once connected, publish an announcement...
//      client.publish("outTopic", "hello world");
      // ... and resubscribe
//      client.subscribe("inTopic");
    } // else {
  //    Serial.print("failed, rc=");
  //    Serial.print(client.state());
  //    Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
  //    delay(5000);
  //  }
//  }
}





void setup() {
//  randomSeed(micros());

  Serial.begin(115200);
  delay(100);

  // setup EEPROM
  init_conf();

  // set a unique device id
  setDeviceId();

  // Setup PWM output
  pinMode(PWM_PIN, OUTPUT);      // Configure PWM pin to output
  analogWriteRange(100);         // Configure range to be 0-100 to skip conversion later
  analogWriteFreq(PWM_FREQ_HZ);  // Configure PWM frequency
  analogWrite(PWM_PIN, dutycycle);

  // setup tacho
  pinMode(TACHO_PIN, INPUT_PULLUP);  // Configure TACHO pin to output
  // attachInterrupt triggers on noise. reverting to polling.
  // attachInterrupt(digitalPinToInterrupt(TACHO_PIN), counttacho, FALLING); // 2 interrupts per revolution

  // setup WiFi AP
  setup_wifi_ap();

  // setup WiFi STA
  setup_wifi_sta();

  // setup websocket
  ws.onEvent(webSocketEvent);  // if there's an incomming websocket message, go to function 'webSocketEvent'
  server.addHandler(&ws);
  Serial.println(F("WebSocket server started."));


  /*
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
  */

  // setup /
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(header_html);
    response->print(index_html);
    response->print(footer_html);
    // https://github.com/me-no-dev/ESPAsyncWebServer#send-large-webpage-from-progmem-containing-templates
    request->send(response);
  });

  // setup /wifi
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(header_html);
    response->print(wifi_html);
    response->print(footer_html);
    request->send(response);
  });
  
  // setup /mqtt
  server.on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(header_html);
    response->print(mqtt_html);
    response->print(footer_html);
    request->send(response);
  });


  // setup /conf GET
  server.on("/conf", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(F("{"));
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(!p->isFile()){
        set_conf(p->name().c_str(), p->value().c_str());
      }
    }
    if(params > 0 && conf_initalized) {
      response->print(F("\"message\":\"Configurations saved.\""));
    }

    if(conf_initalized) {
      len = sizeof(conf)/sizeof(t_conf);
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

/*  
  // setup /mqtt POST
  server.on("/mqttconf", HTTP_POST, [](AsyncWebServerRequest *request) {

    if(request->hasParam("mqttbroker", true)) {
      AsyncWebParameter* b = request->getParam("mqttbroker", true);
      AsyncWebParameter* p = request->getParam("mqttport", true);
      AsyncWebParameter* e = request->getParam("mqttenabled", true);

      bool ebool = false;
      if(atoi(e->value().c_str()) > 0 {
        ebool = true;
      }
      writeEEPROMconf_mqtt(ebool, b->value().c_str(), p->value().c_str()); 
    }

    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(header_html);
    response->print(mqtt_html);
    response->print(footer_html);
    request->send(200, response);
  });
*/

/*

  server.on("/mqttget", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/json");
    response->print("{\"mqttenabled\":");
    if (mqtt_enabled) {
      response->print("true");
    } else {
      response->print("false");
    }
    response->print(", \"mqtthost\":\"" + mqtt_broker_host + "\", "mqttport":\"" + mqtt_broker_port + "\"}");
    request->send(200, response);
  });

  server.on("/conf", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/json");
    response->print("{\"wifienabled\":");
    if (wifi_enabled) {
      response->print("true");
    } else {
      response->print("false");
    }
    response->print(", \"wifissid\":\"" + wifi_ssid + "\"}");
    request->send(200, response);
  });

*/
  server.on("/wifiscan", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/json");
    response->print("{");

    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++)     {
      response->print("\"" + WiFi.SSID(i) "\":1");
      if(i<n-1) { response->print(","); }
    }
    response->print("}");
    request->send(response);
  });


 // /wifi
 // /wifiscan
 // /mqtt
 // /conf
 

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server);  

  // Start web server
  server.begin();
}





void loop() {
  // check if dutu cycle input from Arduino CLI
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    dutycycle = command.toInt();
    analogWrite(PWM_PIN, dutycycle);

    Serial.println("You wrote: " + command + " | Duty cycle set to " + String(dutycycle));
  }

  // mqtt
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();



  // Check state of tacho input @1000 Hz.
  // If changed: count.
  // Fans have 2 pulses / rev => 4 changes / rev
  if (micros() - lasttachopoll > 1000) {
    int pinstate = digitalRead(TACHO_PIN);
    if (pinstate != lasttachostate) {
      num_tacho++;
      //counttacho();
      lasttachostate = pinstate;
    }
    lasttachopoll += 1000;
  }



  // Push RPM
  if (lastrpmsendtime + 2000 < millis()) {
    //   Serial.println("RPM: " + String((num_tacho / (millis() - lastrpmtime)) / 2 * 1000 * 60 ));
    //   rpm = (num_tacho / ((millis() - lastrpmtime)/1000)) * 60 / 4;
    //   rpm = (unsigned int)((unsigned long)1200000 / lastrpmduration);
    //   rpm = (unsigned int)((unsigned long)1200000 / lastrpmduration);

	// number of pulses
    //    noInterrupts();
    unsigned long mytachos = num_tacho;
    num_tacho = 0;
    //    interrupts();

	// calc RPM from pulses and duration since last calulation
	//  rpm = (mytachos * (unsigned long)30000) / (millis() - lastrpmsendtime); // 2 counts per rev
    rpm = (mytachos * (unsigned long)15000) / (millis() - lastrpmsendtime); // 4 counts per rev
	lastrpmsendtime = millis();
	
	// Push JSON object with data to WS clients
    Serial.println("RPM: " + String(rpm));
    ws.textAll("{\"rpm\":" + String(rpm) + ", \"dutycycle\":" + String(dutycycle) + ", \"freemem\":" + String(ESP.getFreeHeap())  + "}");

/* mqtt
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
*/
//Serial.println(WiFi.localIP());
    if (WiFi.status() != WL_CONNECTED) {
      ws.textAll("{\"wifi\":\"not connected\"}");
    } else {
      ws.textAll("{\"wifi\":\"connected (IP: " + WiFi.localIP() + "\"}");
    }

  }


  ws.cleanupClients();
  //  MDNS.update();
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
    data[len] = 0;
//    String message = (char *)data;
//    message.trim();

// ~~ https://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string ~~
// https://forum.arduino.cc/t/serial-input-basics-updated/382007/3

    char key[20] = {0};
    char value[20] = {0};    
    char * strtokIndx; // this is used by strtok() as an index
    strtokIndx = strtok((char *)data,",");      // get the first part - the key
    strcpy(key, strtokIndx); // copy it to key
    strtokIndx = strtok(NULL,",");      // get the first part - the value
    strcpy(value, strtokIndx); // copy it to value

    if (strcmp(key, (const char*)"dutycycle") == 0) {
      dutycycle = atoi(value);
      //dutycycle = message.toInt();
      if (dutycycle < 0) { dutycycle = 0; }
      if (dutycycle > 100) { dutycycle = 100; }
      analogWrite(PWM_PIN, dutycycle);
      ws.textAll("{\"dutycycle\":" + String(dutycycle) + "}");
    }
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



void scanssid() {
  String ssid;
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t *bssid;
  int32_t channel;
  bool hidden;
  int scanResult;

//  const int SSIDLIST_MAXLEN = 10;  // array capacity
//  const char *SSIDist[SSID_LIST_MAXLEN] = {};


  Serial.println(F("Starting WiFi scan..."));

  scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

  String ssidlist = "{\"ssidlist\":[";

  if (scanResult == 0) {
    Serial.println(F("No networks found"));
    ssidlist += "\"No networks found\"]";
  } else if (scanResult > 0) {
    Serial.printf(PSTR("%d networks found:\n"), scanResult);

    // Print unsorted scan results
    for (int8_t i = 0; i < scanResult; i++) {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);
      if(i > 0) {
        ssidlist += ", ";
      }

      ssidlist += "\"" + ssid + "\""; 
/*
      // get extra info
      const bss_info *bssInfo = WiFi.getScanInfoByIndex(i);
      String phyMode;
      const char *wps = "";
      if (bssInfo) {
        phyMode.reserve(12);
        phyMode = F("802.11");
        String slash;
        if (bssInfo->phy_11b) {
          phyMode += 'b';
          slash = '/';
        }
        if (bssInfo->phy_11g) {
          phyMode += slash + 'g';
          slash = '/';
        }
        if (bssInfo->phy_11n) {
          phyMode += slash + 'n';
        }
        if (bssInfo->wps) {
          wps = PSTR("WPS");
        }
      }
*/
 //     Serial.printf(PSTR("  %02d: [CH %02d] [%02X:%02X:%02X:%02X:%02X:%02X] %ddBm %c %c %-11s %3S %s\n"), i, channel, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], rssi, (encryptionType == ENC_TYPE_NONE) ? ' ' : '*', hidden ? 'H' : 'V', phyMode.c_str(), wps, ssid.c_str());
      yield();
    }
  } else {
    Serial.printf(PSTR("WiFi scan error %d"), scanResult);
    ssidlist += F("\"WiFi scan error\"]");
  }

  ws.textAll(ssidlist);
}


void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("MQTT: "));
  Serial.print(topic);
  Serial.print(F(" <- "));
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}






setDeviceId() {
  //https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/station-class.html#macaddress
   uint8_t macAddr[6];
   WiFi.macAddress(macAddr);
   sprintf(deviceId, "%02x%02x%02x%02x%02x%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

/*

// You must free the result if result is non-NULL.
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}
*/


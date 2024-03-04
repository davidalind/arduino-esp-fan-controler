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
#include "credentials.h"

// PWM conf
const byte PWM_PIN = D1;
#define PWM_FREQ_HZ 25000 // PWM frequency in HZ

// TACHO conf
const byte TACHO_PIN = D4;

// Web server conf
//const char *mdnsName PROGMEM = "fancontrol";  // Domain name for the mDNS responder. This makes the ESP crash...
const char *ssid PROGMEM = SSID;
const char *password PROGMEM = PASSWORD;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
IPAddress apIP(192, 168, 4, 1);  // Private network address: local & gateway




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



const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>DACAB Fan Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
    .textinput {
    background-color: #FFFFFF;
    font-size: 24px;
    text-align: center;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>D&Aring;C AB Fan Control</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Speed</h2>
      <p class="state"><span id="rpm">--</span> RPM</p>
      <h2>PWM</h2>
      <p class="state">Duty Cycle: <span id="dutycycle">--</span> %</p>
    </div>

    <p></p>

    <div class="card">
      <h2>Set Duty Cycle</h2>
      <p><input class="textinput" type="text" size="3" id="newdutycycle" onkeydown="senddc()"><span class="state"> %</span></p>
      <p><button id="setdutycycle" class="button">Send</button></p>
    </div>

    <p></p>

    <div class="card">
      <h2>Connection status</h2>
      <p class="state"><span id="status">--</span></p>
      <h2>Free memory</h2>
      <p class="state"><span id="freemem">--</span></p>
    </div>

  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onerror   = onError;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
    document.getElementById('status').innerHTML = "Connected";
  }
  function onClose(event) {
    console.log('Connection closed');
    document.getElementById('status').innerHTML = "Disconnected. Refresh to reconnect.";
    resetvalues();
//    setTimeout(initWebSocket, 2000);
  }
  function onError(event) {
    console.log("WebSocket error: ", event);
    document.getElementById('status').innerHTML = "Error: " & event;
    resetvalues();
//    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
    }
  }

  function onLoad(event) {
    initWebSocket();
    initButton();
  }

  function initButton() {
    document.getElementById('setdutycycle').addEventListener('click', setdutycycle);
  }
  function setdutycycle(){
    websocket.send(document.getElementById('newdutycycle').value);
  }
  function senddc() {
    if(event.key === 'Enter') {
        setdutycycle();        
    }
  }
  function resetvalues(){
    document.getElementById('dutycycle').innerHTML = "--";
    document.getElementById('rpm').innerHTML = "--";
    document.getElementById('freemem').innerHTML = "--";
  }
</script>
</body>
</html>
)rawliteral";



void setup() {

  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);


  // Setup PWM output
  pinMode(PWM_PIN, OUTPUT);      // Configure PWM pin to output
  analogWriteRange(100);         // Configure range to be 0-100 to skip conversion later
  analogWriteFreq(PWM_FREQ_HZ);  // Configure PWM frequency
  analogWrite(PWM_PIN, dutycycle);

  // setup tacho
  pinMode(TACHO_PIN, INPUT_PULLUP);  // Configure TACHO pin to output
  // attachInterrupt(digitalPinToInterrupt(TACHO_PIN), counttacho, FALLING); // 2 interrupts per revolution


  // setup WiFi AP
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  //if(WiFi.softAP(ssid,password)==true)
  if (WiFi.softAP(ssid, password, 1, 0, 2) == true) {
    Serial.print(F("Access Point is Creadted with SSID: "));
    Serial.println(ssid);
    Serial.print(F("Access Point IP: "));
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println(F("Unable to Create Access Point"));
  }



  ws.onEvent(webSocketEvent);  // if there's an incomming websocket message, go to function 'webSocketEvent'
  server.addHandler(&ws);
  Serial.println(F("WebSocket server started."));


  /*
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
  */

  // setup webserver
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

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

    // blink onboard LED
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(LED_BUILTIN, ledState);
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

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char *)data;
    message.trim();
    dutycycle = message.toInt();
    if (dutycycle < 0) { dutycycle = 0; }
    if (dutycycle > 100) { dutycycle = 100; }
    analogWrite(PWM_PIN, dutycycle);
    ws.textAll("{\"dutycycle\":" + String(dutycycle) + "}");
  }
}

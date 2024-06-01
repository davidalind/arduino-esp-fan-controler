

const char header_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head><title>Fan Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {
 font-family: Arial, Helvetica, sans-serif;
 margin: 0;
}
p {
 font-size: 20px;
}
.content {
 padding: 30px;
 max-width: 600px;
 margin: 0 auto;
}
.card {
 background-color: #F8F7F9;
 padding:10px;
 text-align: center;
 margin-top:20px;
}
.textinput {
 background-color: #FFFFFF;
 font-size: 18px;
 padding:5px;
 text-align: center;
}
 
.button {
 padding: 10px 30px;
 font-size: 24px;
 text-align: center;
 outline: none;
 color: #fff;
 background-color: #0f8b8d;
 border: none;
 border-radius: 5px;
}

.topnav {
 overflow: hidden;
 background-color: #000;
 position: relative;
}

.topnav #myLinks {
 display: none;
}

.topnav a {
 color: white;
 padding: 14px 16px;
 text-decoration: none;
 font-size: 17px;
 display: block;
}

.topnav a.icon {
 background: black;
 display: block;
 position: absolute;
 font-weight: bold;
 right: 0;
 top: 0;
}

.topnav a:hover {
 background-color: #ddd;
 color: black;
}

.active {
 background-color: #0f8b8d;
 color: white;
}
</style>
</head>
<body>
<div class="topnav">
 <a href="/" class="active">Fan control</a>
 <div id="myLinks">
  <a href="/">Dashboard</a>
  <a href="/wifi">WiFi</a>
  <a href="/mqtt">MQTT</a>
 </div>
 <a href="javascript:void(0);" class="icon" onclick="toggleMenu()">&#9776;</a>
</div>
<script>
function toggleMenu() {
  var x = document.getElementById("myLinks");
  if (x.style.display === "block") {
    x.style.display = "none";
  } else {
    x.style.display = "block";
  }
}
</script>
<div class="content">
)rawliteral";


const char footer_html[] PROGMEM = R"rawliteral(
</div>
</body>
</html>
)rawliteral";


const char index_html[] PROGMEM = R"rawliteral(
 <div class="card">
  <h2>Speed</h2>
  <p><span id="rpm">--</span> RPM</p>
  <h2>PWM</h2>
  <p>Duty Cycle: <span id="dutycycle">--</span> %</p>
 </div>

 <div class="card">
  <h2>Set Duty Cycle</h2>
  <p><input class="textinput" type="text" size="3" id="newdutycycle" onkeydown="senddc()"> %</p>
  <p><button id="setdutycycle" class="button">Send</button></p>
 </div>

 <div class="card">
  <p>WS connection status: <span id="status">--</span></p>
  <p>Sta WiFi status: <span id="wifi">--</span></p>
  <p>Free mem: <span id="freemem">--</span></p>
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
    websocket.send("dutycycle=" & document.getElementById('newdutycycle').value);
  }
  function senddc() {
    if(event.key === 'Enter') {
        setdutycycle();        
    }
  }
  function resetvalues(){
    document.getElementById('dutycycle').innerHTML = "--";
    document.getElementById('rpm').innerHTML = "--";
    document.getElementById('wifi').innerHTML = "--";
    document.getElementById('freemem').innerHTML = "--";
  }
</script>
)rawliteral";

// /mqtt      post variables?
// /mqttget   return current mqtt settings as json
const char mqtt_html[] PROGMEM = R"rawliteral(
<form id="conf">
 <div class="card">
  <h2>MQTT</h2>
  <p><input type="checkbox" id="mqtt_enabled"> Enabled</p>
  <p>Broker/p>
  <p><input class="textinput" type="text" size="20" id="mqtt_broker_host"></p>
  <p>Port (default: 1883)/p>
  <p><input class="textinput" type="text" size="4" id="mqtt_broker_port"></p>
  <p><button id="mqtt_save" class="button">Save</button></p>
 </div>
</form>

<script>
  function onLoad(event) {
    initButton();
    getconf(new URLSearchParams());
  }
  function initButton() {
    document.getElementById('mqtt_save').addEventListener('click', conf);
  }
  function saveconf(){
    websocket.send("dutycycle=" & document.getElementById('newdutycycle').value);
    document.getElementById('mqtt_save').innerHTML = "Saving...";
    document.getElementById('mqtt_save').disabled = true;
    getconf(new URLSearchParams(new FormData(document.getElementById("form"))););
    document.getElementById('mqtt_save').disabled = false;
  }
  function getconf(data) {
   fetch("/conf", {
    method: 'get',
    body: data,
  })
   .then(res => res.json())
   .then(data => {
    data.forEach(conf => {
      document.getElementById(conf.name)
      
    });
  });
 }
</script>
)rawliteral";

// /wifi      post variables?
// /wifiscan  return list of networks as json


const char wifi_html[] PROGMEM = R"rawliteral(
 <div>
  <p>Configure this if you want the device to connect to an existing WiFi. Disable if you want to use access point only. Acess point is always enabled.</p>
 </div>
 <div class="card">
  <h2>Stationary WiFi</h2>
  <p><input type="checkbox" id="wifienabled"> Enabled</p>
  <p>SSID/p>
  <p><input class="textinput" type="text" size="20" id="wifissid"></p>
  <p>Password/p>
  <p><input class="textinput" type="text" size="4" id="wifipassword"></p>
  <p><button id="savemqtt" class="button">Save</button></p>

  <p>Scan and select from list to populate SSID input above.</p>
  <p><button id="scanssid" class="button">Scan SSID</button></p>
  <select name="ssidlist" id="ssidlist"></select>
 </div>


<script>
 function wifiscan(){
  document.getElementById('scanssid').innerHTML = "Scanning...";
  document.getElementById('scanssid').disabled = true;
  //clear list
  var i, L = document.getElementById('ssidlist').options.length - 1;
  for(i = L; i >= 0; i--) {
   document.getElementById('ssidlist').remove(i);
  }
  let ssids = document.getElementById('ssidlist');
  fetch('/wifiscan')
   .then(res => res.json())
   .then(data => {
    data.forEach(ssid => {
     let option = new Option(ssid, ssid);
     ssidlist.add(option);
    });
  });
  document.getElementById('scanssid').disabled = false;
 }
 function onLoad(event) {
  initButton();
 }
 function initButton() {
  document.getElementById('scanssid').addEventListener('click', wifiscan);
  document.getElementById("list").onchange = ssidselected;
 }
 function ssidselected(){
  var e = document.getElementById("ssidlist");
  document.getElementById("wifissid").value = e.options[e.selectedIndex].text);
 }
</script>
)rawliteral";


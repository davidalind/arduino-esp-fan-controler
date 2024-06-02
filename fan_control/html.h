

/*
header_html
footer_html
index_html
wifi_html
mqtt_html
*/


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
 margin-top:20px;
}
.textinput {
 background-color: #FFFFFF;
 font-size: 18px;
 padding:5px;
 width:250px;
}
.selectinput {
 background-color: #FFFFFF;
 font-size: 18px;
 padding:5px;
 width:250px;
}
input[type="checkbox"] {
    zoom: 1.5;
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
 font-size: 21px;
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
  <a href="/update">Update FW</a>  
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
%HEADER%
 <div class="card">
  <h2>Connect to WiFi</h2>
  <p>Enable if you want the device to connect to an existing 2.4 GHz WiFi network.</p>
  <p>Access point is always enabled.</p>
 </div>
 <div class="card">
 <p><span id="message"></span></p>
 </div>

 <div class="card">
  <form id="conf_form">
  <p>Network list<br>
  <select class="selectinput" name="wifi_ssid_list" id="wifi_ssid_list"></select></p>
  <div id="ssidinput" style="display:block"><p>SSID<br>
  <input class="textinput" type="text" name="wifi_ssid" id="wifi_ssid"></p></div>
  <p>Password<br>
  <input class="textinput" type="text" name="wifi_password" id="wifi_password"></p>
  <p><input  type="checkbox" name="wifi_enabled" id="wifi_enabled"> WiFi Enabled</p>
  <p><button type="button" id="do_save" class="button">Save</button></p>
  </form>
 </div>


<script>

window.addEventListener('load', onLoad);

function do_wifi_ssid_selected(){
  var e = document.getElementById("wifi_ssid_list");
  if(e.options[e.selectedIndex].value === "showssid") {
    document.getElementById("ssidinput").style.display = "block";
  } else {
    document.getElementById("ssidinput").style.display = "none";
    document.getElementById("wifi_ssid").value = e.options[e.selectedIndex].value;
  }
}

function do_save(){
  console.log("saving");
  document.getElementById('do_save').innerHTML = "Saving...";
  document.getElementById('do_save').disabled = true;

  var formData = new FormData(document.getElementById('conf_form'));
  
	let wifi_enabled = '0';

  const params = new URLSearchParams();
  for (const pair of new FormData(document.getElementById("conf_form"))) {
    console.log("pair[0]" + pair[0]);
    console.log("pair[1]" + pair[1]);    
	if (pair[0] == "wifi_enabled") {
		wifi_enabled = '1';
	} else if (pair[1] != '') {
		params.append(pair[0], pair[1]);
	}
  }
  params.append("wifi_enabled", wifi_enabled);

  if(params.get("wifi_ssid")) {
	  getconf(params);
  } else {
    document.getElementById('message').innerHTML = "SSID cannot be empty.";
	}

  document.getElementById('do_save').innerHTML = "Save";
  document.getElementById('do_save').disabled = false;
}

function getconf(params) {
  console.log("conf params:" + params.toString());
  fetch("/conf?" + params.toString())
    .then(res => res.json())
    .then(data => {
      for (const conf of data.conf) {
        console.log("setting input: " + conf.name + " to: " + conf.value);
      
        if(document.getElementById(conf.name)) {
        if (conf.name.endsWith('_enabled')) {
          if (conf.value == '1') {  
            document.getElementById(conf.name).checked = true;
          } else {
            document.getElementById(conf.name).checked = false;
          }
        } else {
          document.getElementById(conf.name).value = conf.value;
        }
      }
    }

    if (data.hasOwnProperty('message')) {
     document.getElementById('message').innerHTML = data.message;
    }
  });
}

function wifiscan() {
//  document.getElementById('do_wifi_scan_ssid').innerHTML = "Scanning...";
//  document.getElementById('do_wifi_scan_ssid').disabled = true;

  //clear list
  var i, L = document.getElementById('wifi_ssid_list').options.length - 1;
  for(i = L; i >= 0; i--) {
   document.getElementById('wifi_ssid_list').remove(i);
  }

  let wifi_ssid_list = document.getElementById('wifi_ssid_list');
  fetch("/wifiscan")
   .then(res => res.json())
   .then(data => {
    let option = new Option("Enter SSID or select...", "showssid");    
    wifi_ssid_list.add(option);
    for (const n of data.networks) {
      console.log("network ssid: " + n.ssid + " rssi: " + n.rssi);
      let option = new Option(n.ssid + " (" + n.rssi + "dBm)", n.ssid);
      wifi_ssid_list.add(option);
    }

    if (data.hasOwnProperty('message')) {
     document.getElementById('message').innerHTML = data.message;
    }
  });

//  document.getElementById('do_wifi_scan_ssid').innerHTML = "Scan SSID";
//  document.getElementById('do_wifi_scan_ssid').disabled = false;
}

function onLoad(event) {
  console.log("init btn");
  initButton();  
  console.log("init get conf");
  getconf(new URLSearchParams());
  wifiscan();
 }

 function initButton() {
  document.getElementById('do_save').addEventListener('click', do_save);
  document.getElementById("wifi_ssid_list").onchange = do_wifi_ssid_selected;
 }
</script>
%FOOTER%
)rawliteral";


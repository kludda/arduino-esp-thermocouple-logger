
#ifndef html_h
#define html_h

/*
header_html
footer_html
index_html
wifi_html
mqtt_html
*/


const char header_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head>
<title>%UNIQUEID%</title>
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
.name {
 font-size: 18px;
 line-height: 28px;
}
.value {
 font-size: 24px;
 font-weight: bold;
}
.status {
 font-size: 12px;
}
.fail {
 background-color: #FF8080;
}
.ok {
 background-color: #80FF80;
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
 <a href="/" class="active">%UNIQUEID%</a>
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
<div class="card">
  <div class="status" id="status"></div>
  <div class="status" id="ws_status"></div>
</div>  

<script>
(function(){
  var f = function() {
    fetch("/status")
      .then(res => res.json())
      .then(data => {
        let div = document.getElementById("status");
        if(div) {
          div.innerHTML = "";
          for (const s of data.status) {
            div.innerHTML += s.text + "<br>";
          }
        }
      })
      .catch(error => {
        document.getElementById("status").innerHTML = "Disconnected.";
      });
  };
  window.setInterval(f, 5000);
  f();
})();
</script>

</div>
</body>
</html>
)rawliteral";


const char temp_html[] PROGMEM = R"rawliteral(
%HEADER%

 <div>
  <h2>Temp sensors</h2>
 </div>

 <div class="card" style="display:none" id="message"></div>

 <div id="log">
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
    websocket.onmessage = onMessage;
  }
  function onOpen(event) {
    console.log('Connection opened');
    document.getElementById('ws_status').innerHTML = "WS: Connected";
  }
  function onClose(event) {
    console.log('Connection closed');
    document.getElementById('ws_status').innerHTML = "WS: Disconnected. Refresh to reconnect.";
    resetvalues();
//    setTimeout(initWebSocket, 2000);
  }
  function onError(event) {
    console.log("WebSocket error: ", event);
    document.getElementById('ws_status').innerHTML = "WS: Error: " + event;
    resetvalues();
//    setTimeout(initWebSocket, 2000);
  }
  
  function writecard(id, value) {
	let e = document.getElementById(id);
	if (e) {
	  	clearTimeout(e.getAttribute('timerid'));
	}
    e.innerHTML = "<span class=\"name\">" + id + "</span><br><span class=\"value\">" + value + " &deg;C</span>";
	e.setAttribute('timerid', setTimeout(writecard, 3000, id, "--"));
  }
  
  function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);

    if (document.getElementById(myObj.id)) {
//		var timerid = document.getElementById(myObj.id).timerid;
//		clearTimeout(timerid);
//		var timeoutid = setTimeout(writecard(myObj.id, "---"), 3000);
		writecard(myObj.id, myObj.value)
//      document.getElementById(myObj.id).innerHTML = "<div timerid=" + timeoutid + "<p>" + myObj.id + "<br><h2>" + myObj.value + " &deg;C</p>";
    } else {
      document.getElementById('log').innerHTML += "<div class=\"card\" id=\"" + myObj.id + "\"></div>";
	  writecard(myObj.id, myObj.value)
    }
  }
  function onLoad(event) {
    initWebSocket();
  }
  function resetvalues(){
    //document.getElementById('dutycycle').innerHTML = "--";
    //document.getElementById('rpm').innerHTML = "--";
    // document.getElementById('wifi').innerHTML = "--";
    // document.getElementById('freemem').innerHTML = "--";
  }
</script>

%FOOTER%

)rawliteral";

/*

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

*/

const char wifi_html[] PROGMEM = R"rawliteral(
%HEADER%
 <div>
  <h2>Connect to WiFi</h2>
  <p>Enable if you want the device to connect to an existing 2.4 GHz WiFi network.</p>
  <p>The device access point is always enabled.</p>
 </div>

 <div class="card" style="display:none" id="message">
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
    } else if (pair[0] == "wifi_ssid") {
      if (pair[1] != '') {
        params.append(pair[0], pair[1]);
      }
    } else {
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

    let m = document.getElementById('message');
    m.innerHTML = "";
    m.style.display = "none";
    if (data.hasOwnProperty('messages')) {
      for (const mess of data.messages) {
        m.style.display = "block";
        m.innerHTML += "<p class=\"" + mess.type + "\">" + mess.text + "</p>";
//        document.getElementById('message').classList.remove("ok", "fail");
      }
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

    let m = document.getElementById('message');
    m.innerHTML = "";
    m.style.display = "none";
    if (data.hasOwnProperty('messages')) {
      for (const mess of data.messages) {
        m.style.display = "block";
        m.innerHTML += "<p class=\"" + mess.type + "\">" + mess.text + "</p>";
      }
    }

//    if (data.hasOwnProperty('message')) {
//     document.getElementById('message').innerHTML = data.message;
//    }
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


const char mqtt_html[] PROGMEM = R"rawliteral(
%HEADER%
 <div>
  <h2>MQTT</h2>
 </div>

 <div class="card" style="display:none" id="message">
  </div>

 <div class="card">
  <form id="conf_form">
  <p>Broker host<br>
  <input class="textinput" type="text" name="mqtt_broker_host" id="mqtt_broker_host"></p>
  <p>Port (default: 1883)<br>
  <input class="textinput" type="text" name="mqtt_broker_port" id="mqtt_broker_port"></p>
  <p>Root topic<br>
  <input class="textinput" type="text" name="mqtt_root" id="mqtt_root"></p>
  <p><input  type="checkbox" name="mqtt_enabled" id="mqtt_enabled"> MQTT Enabled</p>

  <p><button type="button" id="do_save" class="button">Save</button></p>
  </form>
 </div>

<script>

window.addEventListener('load', onLoad);

function do_save(){
  console.log("saving");
  document.getElementById('do_save').innerHTML = "Saving...";
  document.getElementById('do_save').disabled = true;

  var formData = new FormData(document.getElementById('conf_form'));
  
	let mqtt_enabled = '0';

  const params = new URLSearchParams();
  for (const pair of new FormData(document.getElementById("conf_form"))) {
    console.log("pair[0]" + pair[0]);
    console.log("pair[1]" + pair[1]);    
    if (pair[0] == "mqtt_enabled") {
      mqtt_enabled = '1';
    } else {
      params.append(pair[0], pair[1]);
    }
  }
  params.append("mqtt_enabled", mqtt_enabled);

  if(params.get("mqtt_broker_host") && params.get("mqtt_broker_port")) {
	  getconf(params);
  } else {
    document.getElementById('message').innerHTML = "Host and port are required.";
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

    let m = document.getElementById('message');
    m.innerHTML = "";
    m.style.display = "none";
    if (data.hasOwnProperty('messages')) {
      for (const mess of data.messages) {
        m.style.display = "block";
        m.innerHTML += "<p class=\"" + mess.type + "\">" + mess.text + "</p>";
//        document.getElementById('message').classList.remove("ok", "fail");
      }
    }
  });
}

function onLoad(event) {
  initButton();  
  getconf(new URLSearchParams());
}

function initButton() {
  document.getElementById('do_save').addEventListener('click', do_save);
}
</script>
%FOOTER%
)rawliteral";


#endif


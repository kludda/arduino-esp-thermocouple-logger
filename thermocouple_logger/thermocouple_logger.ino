/*
ESP8266 Arduino DS18B20 temp logger

Code for the webserver and websocket was found here
https://randomnerdtutorials.com/esp8266-nodemcu-websocket-server-arduino/

Other useful resources:
https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf
https://arduino-esp8266.readthedocs.io/en/latest/index.html
https://www.pjrc.com/teensy/td_libs_OneWire.html
https://github.com/milesburton/Arduino-Temperature-Control-Library



*/

// Import required libraries


#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <ESP8266mDNS.h>
#include <AsyncElegantOTA.h>

#include <OneWire.h>
#include <DallasTemperature.h>


#include "credentials.h"
#include "html.h"
#include "kludda_eeprom.h"
#include "kludda_wifi.h"
#include "kludda_mqtt.h"

// one wire Data wire 
#define ONE_WIRE_BUS D3

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

int num_sensors = 0;


// webserver
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");





unsigned long lastsendtime = 0;

  




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
    String message = "";
    for(int i = 0; i < len; i++) {
      message += (char)data[i];
    }

    int pos = message.indexOf("=");
    if (pos != -1) {
      String key = message.substring(0, pos);
      String value = message.substring(pos + 1);

/*
      if (key == "dutycycle") {
        set_dc(dutycycle);
      }
*/
    }

    message = String();
  }
}

/*
void mqtt_on_message(char* topic, byte* payload, unsigned int length) {
  String p = "";
  String t = String(topic);

  for(int i = 0; i < length; i++) {
    p += (char)payload[i];
  }

  t = mqtt_trim(t);


  p = String();
  t = String();


}
*/

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



/*
void on_mqtt_prop1_set (String *t, String *p) {
  Serial.print(F("prop1: "));
  Serial.print(t->c_str());
  Serial.print(F(" <- "));
  Serial.println(p->c_str());
}

void on_mqtt_prop2_set (String *t, String *p) {
  Serial.print(F("prop2: "));
  Serial.print(t->c_str());
  Serial.print(F(" <- "));
  Serial.println(p->c_str());
}
*/
/*
t_subscribe subs[] = {
//t_subscribe_arr subs = {
  {
    .topic = "node/prop1/set",
    .callback = on_mqtt_prop1_set,
  
  },
  {
    .topic = "node/prop2/set",
    .callback = on_mqtt_prop2_set,
  
  }
};
*/

void setup() {
//  randomSeed(micros());

  Serial.begin(115200);
  delay(100);
  Serial.println("hello.");


  // setup EEPROM
  init_conf();

  // setup wifi
  setup_wifi();


/*
  Serial.print("subs len.");
  Serial.println(sizeof(subs) / sizeof(t_subscribe));
*/
  // setup wifi 
  setup_mqtt(); //subs);
/*
  mqtt_subscribe("node/prop1/set",  on_mqtt_prop1_set);
  mqtt_subscribe("node/prop2/set",  on_mqtt_prop2_set);
*/

  //mqtt_client.setCallback(mqtt_on_message);


  // setup websocket
  ws.onEvent(webSocketEvent);  // if there's an incomming websocket message, go to function 'webSocketEvent'
  server.addHandler(&ws);
  Serial.println(F("WebSocket server started."));

  // setup /
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", temp_html, processor);
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


  // Start up the library
  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
delay(100);
  // locate devices on the bus
/*
  Serial.print("Locating devices...");
  Serial.print("Found ");
  num_sensors = sensors.getDeviceCount();
  Serial.print(num_sensors, DEC);
  Serial.println(" devices.");
*/

}


// function to print a device address
String ow_address_str(DeviceAddress deviceAddress)
{
  String astr = "";

  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    astr += String(deviceAddress[i], HEX);
  }
  return astr;
}


// function to print the temperature for a device
String ow_temp_str(DeviceAddress deviceAddress)
{
  String tstr = "";
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    tstr = String(tempC, 2);
  }
  return tstr;
}



DeviceAddress tempadr;

void loop() {
  wifi_loop();
  mqtt_loop();



  // Push 
  if (lastsendtime + 1000 < millis()) {

  Serial.print("Locating devices...");
  Serial.print("Found ");
  num_sensors = sensors.getDeviceCount();
  Serial.print(num_sensors, DEC);
  Serial.println(" devices.");

    for(int i = 0; i< num_sensors; i++) {
      if (sensors.getAddress(tempadr,i)) {
    //    sensors.getTempCByIndex(i)
        String t = ow_address_str(tempadr);
        String p = ow_temp_str(tempadr);

        ws.textAll("{\"id\":\"" + t + "\",\"value\":\"" + p + "\"}");
        t += "/temperature";
        mqtt_publish(t, p);
        delay(10);
      }
    }

    mqtt_publish("mcu/freeheap", String(ESP.getFreeHeap()));
/*
    Serial.println("RPM: " + String(rpm));
    ws.textAll("{\"rpm\":" + String(rpm) + ", \"dutycycle\":" + String(dutycycle) + "}");
    mqtt_publish("fan/rpm", String(rpm));
    mqtt_publish("fan/dutycycle", String(dutycycle));    
//    ws.textAll("{\"rpm\":" + String(rpm) + ", \"dutycycle\":" + String(dutycycle) + ", \"freemem\":" + String(ESP.getFreeHeap())  + "}");
  
*/

    // call sensors.requestTemperatures() to issue a global temperature 
    // request to all devices on the bus
    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    Serial.println("DONE");

    lastsendtime = millis();

  }

}

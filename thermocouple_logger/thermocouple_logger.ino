/*
ESP8266 Arduino DS18B20 temp logger

Code for the webserver and websocket was found here
https://randomnerdtutorials.com/esp8266-nodemcu-websocket-server-arduino/

Other useful resources:
https://arduino-esp8266.readthedocs.io/en/latest/index.html



*/

// Import required libraries


#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <ESP8266mDNS.h>
#include <AsyncElegantOTA.h>

#include <SPI.h>
#include <Adafruit_MAX31856.h>

#include "credentials.h"
#include "html.h"
#include "kludda_eeprom.h"
#include "kludda_wifi.h"
#include "kludda_mqtt.h"

// use hardware SPI, just pass in the CS pin
// CLK = D5
// DI = D6
// DO = D7
// CS = D8 (not used)

Adafruit_MAX31856 maxthermo[] = {
  Adafruit_MAX31856(D0),
  Adafruit_MAX31856(D2),
  Adafruit_MAX31856(D3)
};


int num_sensors = 3;
bool conversion_started[3] = { false, false, false };



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
    for (int i = 0; i < len; i++) {
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

String processor(const String &var) {
  if (var == "HEADER")
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
  setup_mqtt();  //subs);
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
    for (int i = 0; i < params; i++) {
      AsyncWebParameter *p = request->getParam(i);
      if (!p->isFile()) {
        Serial.print("set conf: ");
        Serial.print(p->name().c_str());
        Serial.print(", ");
        Serial.println(p->value().c_str());
        set_conf(p->name().c_str(), p->value().c_str());
      }
    }

    if (params > 0 && conf_initalized) {
      response->print(F("\"messages\": [ "));
      response->print(F("{\"type\":\"ok\", \"text\":\"Configuration saved.\"}"));
      response->print(F("],"));
    }


    response->print(F("\"conf\": [ "));
    if (conf_initalized) {
      int len = sizeof(conf) / sizeof(t_conf);
      for (int i = 0; i < len; i++) {
        if (!conf[i].hidden) {
          response->print(F("{ \"name\": \""));
          response->print(conf[i].name);
          response->print(F("\", \"value\": \""));
          response->print(conf[i].data);
          response->print(F("\" }"));
          if (i < len - 1) {
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
  server.on("/wifiscan", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";

    // check if we need to send messages
    int m = 0;
    json += F("\"messages\": [ ");
    if (WiFi.status() == WL_CONNECTED) {
      json += F("{\"type\":\"fail\", \"text\":\"Warning: Cannot scan for networks when connected to network. SSID list might be outdated. Disable WiFi and try again.\"}");
      m++;
    }

    int n = lastnetworkscan;

    if (n < 1) {
      if (m) json += F(",");
      json += F("{\"type\":\"fail\", \"text\":\"No networks found.\"}");
    }

    json += F("]");

    // build network json
    if (n) {
      json += F(", \"networks\": [ ");
      for (int i = 0; i < n; ++i) {
        if (i) json += F(",");
        json += F("{");
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\"";
        json += ", \"rssi\":\"" + String(WiFi.RSSI(i)) + "\"";
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
      { "name":"wifi_status", "text":"[WiFi] Connected to: laban."},
      ...
    ]
  }
  */
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
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


  for (int i = 0; i < num_sensors; i++) {
    if (!maxthermo[i].begin()) {
      Serial.print("Could not initialize thermocouple ");
      Serial.println(i);
    }
  }

  for (int i = 0; i < num_sensors; i++) {
    maxthermo[i].setThermocoupleType(MAX31856_TCTYPE_K);
  }

  for (int i = 0; i < num_sensors; i++) {
    //maxthermo[i].setConversionMode(MAX31856_ONESHOT_NOWAIT);
    maxthermo[i].setConversionMode(MAX31856_CONTINUOUS);
  }
}



/*
{
    "nodes": [
        {
            "node": "thermocouple1",
            "name": "Thermocouple 1",
            "properties": [
                {
                    "property": "temperature",
                    "name": "Temperature",
                    "value": 30,
                    "unit": "C"
                },
                {
                    "property": "cjtemperature",
                    "name": "Temperature",
                    "value": 30,
                    "unit": "C"
                }
            ]
        },
        {
            "node": "thermocouple1",
            "name": "Thermocouple 1",
            "properties": [
                {
                    "property": "temperature",
                    "name": "Temperature",
                    "value": 30,
                    "unit": "C"
                },
                {
                    "property": "cjtemperature",
                    "name": "Temperature",
                    "value": 30,
                    "unit": "C"
                }
            ]
        }
    ]
}  
*/


void loop() {
  wifi_loop();
  mqtt_loop();

  String t;
  String p;
  String s;
  String json = "";
  uint8_t fault;

  if (lastsendtime + 1000 < millis()) {

    json += F("{\"nodes\":[");

    for (int i = 0; i < num_sensors; i++) {
//    if (maxthermo[i].conversionComplete()) {
//      conversion_started[i] = false;
      t = "thermocouple" + String(i);
      p = "";
      s = "";

      fault = maxthermo[i].readFault();
      if (fault) {
        if (fault & MAX31856_FAULT_CJRANGE) s += "Cold Junction Range Fault";
        if (fault & MAX31856_FAULT_TCRANGE) s += "Thermocouple Range Fault";
        if (fault & MAX31856_FAULT_CJHIGH) s += "Cold Junction High Fault";
        if (fault & MAX31856_FAULT_CJLOW) s += "Cold Junction Low Fault";
        if (fault & MAX31856_FAULT_TCHIGH) s += "Thermocouple High Fault";
        if (fault & MAX31856_FAULT_TCLOW) s += "Thermocouple Low Fault";
        if (fault & MAX31856_FAULT_OVUV) s += "Over/Under Voltage Fault";
        if (fault & MAX31856_FAULT_OPEN) s += "Thermocouple Open Fault";
        p = "-1000";
      } else {
        s += "Ok";
        p = String(maxthermo[i].readThermocoupleTemperature(), 2);
      }


      json += F("{");
      json += "\"node\":\"thermocouple" + String(i) + "\"";
      json += F(",");
      json += "\"name\":\"Thermocouple " + String(i) + "\"";
      json += F(",");

      json += F("\"properties\":[");

      json += F("{");
      json += "\"property\":\"temperature\"";
      json += F(",");
      json += "\"name\":\"Temperature\"";
      json += F(",");
      json += "\"value\":\"" + p + "\"";
      json += F(",");
      json += "\"unit\":\"°C\"";
      json += F("}");

      json += F(",");

      json += F("{");
      json += "\"property\":\"cjtemperature\"";
      json += F(",");
      json += "\"name\":\"Cold junction temperature\"";
      json += F(",");
      json += "\"value\":\"" + String(maxthermo[i].readCJTemperature(), 2) + "\"";
      json += F(",");
      json += "\"unit\":\"°C\"";
      json += F("}");


      json += F("]");

      json += F(",");
      json += "\"status\":\"" + s + "\"";

      json += F("}");

      if (i+1 < num_sensors) {
        json += F(",");
      }


      mqtt_publish(t + "/temperature", p);
 //     mqtt_publish(t + "/status", s);

//    } else if (!conversion_started[i]) {
//      maxthermo[i].triggerOneShot();
//      conversion_started[i] = true;
    }

    json += F("]}");

    ws.textAll(json);



    mqtt_publish("mcu/freeheap", String(ESP.getFreeHeap()));
      //    ws.textAll("{\"freemem\":" + String(ESP.getFreeHeap())  + "}");
    lastsendtime = millis();
  
  }




  json = String();
  t = String();
  p = String();
  s = String();
}
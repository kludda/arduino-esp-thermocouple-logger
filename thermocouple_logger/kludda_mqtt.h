#ifndef kludda_mqtt_h
#define kludda_mqtt_h

#include <PubSubClient.h>
#include "kludda_eeprom.h"
#include "kludda_wifi.h"

//  MQTT
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
long mqtt_last_connection_attempt = 0;
long mqtt_connection_attempt_timeout = 5000; // ms
bool mqtt_connection_attempt_started = false;
#define MQTT_CONNECTING 99 // extend the states with own state


typedef struct _subscribe {
  const char *topic;
  void (*callback)(String*, String*);
  struct _subscribe *next;
} t_subscribe;
//typedef t_subscribe t_subscribe_arr[];

//t_subscribe_arr *mqtt_subscribe_to;
t_subscribe *mqtt_subscribe_list = NULL;



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
String get_mqtt_status_str() {
  int status = (mqtt_connection_attempt_started) ? MQTT_CONNECTING : mqtt_client.state();
  String s = "";
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


void mqtt_publish(String t, String p) {
  t = String(get_conf("mqtt_root")->data) + "/" + String(unique_id_str) + "/" + t;
  mqtt_client.publish(t.c_str(), p.c_str());
  Serial.print(p);
  Serial.print(" -> ");
  Serial.println(t);
}



String mqtt_trim(String t) {
  int pos = t.lastIndexOf("/");
  pos = t.lastIndexOf("/", pos - 1);
  if (t.endsWith("/set")) pos = t.lastIndexOf("/", pos - 1);
  if (pos != -1) {
    t = t.substring(pos + 1); // +1 to remove first /
  }
  return t;  
}



void mqtt_on_message(char* topic, byte* payload, unsigned int length) {

  String p = "";
  String t = String(topic);

  for(int i = 0; i < length; i++) {
    p += (char)payload[i];
  }

  t = mqtt_trim(t);

  Serial.println(t + " <- " + p);

  t_subscribe *node = mqtt_subscribe_list;
  while (node) {
    if (t == node->topic) {
      node->callback(&t,&p);
      break;
    }

    node = node->next;
  }


  /*
  for (int i = 0; i < sizeof(mqtt_subscribe_to) / sizeof(t_subscribe); i++) {
    if (t == (*mqtt_subscribe_to)[i].topic) {
      (*mqtt_subscribe_to)[i].callback(&t,&p);
    }
    break;
  }
  */

  p = String();
  t = String();
}



void mqtt_subscribe(const char* t,  void (*callback)(String*, String*)) {
  t_subscribe *node = NULL;
  node = (t_subscribe*) malloc(sizeof(t_subscribe));
  node->topic = t;
  node->callback = callback;
  node->next = NULL;

  if (mqtt_subscribe_list == NULL) {
    mqtt_subscribe_list = node;
  } else {
    mqtt_subscribe_list->next = node;
  }
}

void mqtt_subscribe_do() {
  t_subscribe *node = mqtt_subscribe_list;
  while (node) {
    String t = String(get_conf("mqtt_root")->data) + "/" + String(unique_id_str) + "/" + node->topic;
    Serial.print(F("Subscribing to:"));
    Serial.println(t);
    mqtt_client.subscribe(t.c_str());
    t = String();

    node = node->next;
  }

  /*
  Serial.print("subscribing. len: ");
  Serial.println(sizeof(mqtt_subscribe_to) / sizeof(t_subscribe));
  

  for (int i = 0; i < sizeof(mqtt_subscribe_to) / sizeof(t_subscribe); i++) {
    String t = String(get_conf("mqtt_root")->data) + "/" + String(unique_id_str) + "/" + (*mqtt_subscribe_to)[i].topic;
    Serial.print(F("Subscribing to:"));
    Serial.println(t);
    mqtt_client.subscribe(t.c_str());
    t = String();
  }
  */
}


//void setup_mqtt(t_subscribe (*subs)[]) {
//void setup_mqtt(t_subscribe *subs) {
void setup_mqtt() {
  // add mac adress to client id
//0  sprintf(mqtt_unique_client_id, "%s-%s", mqtt_client_id, deviceId);
//  if(atoi(get_conf("mqtt_enabled").data)) {
//    mqtt_client.setServer(get_conf("mqtt_broker_host").data, atoi(get_conf("mqtt_broker_port").data));
//    mqtt_client.setCallback(mqtt_on_message);
//  }
 // Serial.print("setup_mqtt subs len: ");
  //Serial.println(sizeof(subs) / sizeof(t_subscribe));

//  mqtt_subscribe_to = subs;


  mqtt_client.setCallback(mqtt_on_message);
}



void mqtt_loop() {

  if (mqtt_client.connected() && !atoi(get_conf("mqtt_enabled")->data)) {
    Serial.println("mqtt disconnecting.");
    mqtt_client.disconnect();
  }

  if(
    !mqtt_connection_attempt_started &&
    WiFi.status() == WL_CONNECTED &&  // need sta wifi 
    !mqtt_client.connected() &&          // check we're not already connected
    atoi(get_conf("mqtt_enabled")->data)   // check if wifi is enabled in conf
  ){
    Serial.print("mqtt connecting...");
    mqtt_client.setServer(get_conf("mqtt_broker_host")->data, atoi(get_conf("mqtt_broker_port")->data));
    mqtt_client.connect(unique_id_str);
    mqtt_connection_attempt_started = true;
    mqtt_last_connection_attempt = millis();
  }

  if (mqtt_connection_attempt_started) {
      if (mqtt_client.connected()) {
        Serial.println(" mqtt connected");
        mqtt_subscribe_do();
        mqtt_connection_attempt_started = false;
/*
        String t = String(get_conf("mqtt_root")->data) + "/" + String(unique_id_str) + "/fan/dutycycle/set";
        
        Serial.print(F("Subscribing to:"));
        Serial.println(t);

        mqtt_client.subscribe(t.c_str());
        t = String();
*/
        // Once connected, publish an announcement...
        mqtt_publish("ip", WiFi.localIP().toString());
        // ... and resubscribe
        //  mqtt_client.subscribe("inTopic");
      }
      if (millis() > mqtt_last_connection_attempt + mqtt_connection_attempt_timeout) {
        mqtt_connection_attempt_started = false;
      }
  }
  mqtt_client.loop();
}







#endif

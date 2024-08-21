#ifndef kludda_wifi_h
#define kludda_wifi_h


#include <ESP8266WiFi.h>
#include "kludda_eeprom.h"
#include "credentials.h"


// ID string for SSID and MQTT clientid.
char *unique_id_str;

// WiFi
const char *ap_ssid PROGMEM = AP_SSID;
const char *ap_password PROGMEM = AP_PASSWORD;

IPAddress apIP(192, 168, 4, 1);  // Private network address: local & gateway
IPAddress subnet(255,255,255,0);


int lastnetworkscan = 0;
#define WL_CONNECTING 99 // extend the wifi states with own state
long last_connection_attempt = 0;
long connection_attempt_timeout = 30000; // ms
bool connection_attempt_started = false;




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
String get_wifi_status_str() {
  String s = "";
  int status = (connection_attempt_started) ? WL_CONNECTING : WiFi.status();

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


// STA WiFi setup
void setup_wifi() {
  // setup wifi ap
  setup_wifi_ap();

  // setup wifi sta
  setup_wifi_sta();
}



void wifi_loop() {
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
}







#endif

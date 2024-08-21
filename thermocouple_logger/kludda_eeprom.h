#ifndef kludda_eeprom_h
#define kludda_eeprom_h


#include <EEPROM.h>

//https://www.tutorialspoint.com/compile_c_online.php


#define EEPROM_START 0
#define EEPROM_SIZE 512
//char jsonbuf[600] = { 0 };  // buffer for json string
int conf_initalized = 0;


typedef struct _conf {
  const char *name;
  char *data;
  int len;
  const char *def;
  int hidden;
} t_conf;


t_conf conf[] = {
  {
    .name = "magic_string",
    .len = 5,
    //    .def = {0xA5,0xA5,0xA5,0xA5}
    .def = "mage",
    .hidden = 1    
  },
  // wifi conf
  {
    .name = "wifi_enabled",
    .len = 2,
    .def = "0" 
  },
  {
    .name = "wifi_ssid",
    .len = 33 
  },
  {
    .name = "wifi_password",
    .len = 33,
    .hidden = 1 
  },
  // mqtt conf
  {
    .name = "mqtt_enabled",
    .len = 2,
    .def = "0" 
  },
  { 
    .name = "mqtt_broker_host",
    .len = 33,
    .def = "localhost" 
  },
  { 
    .name = "mqtt_broker_port",
    .len = 5,
    .def = "1883" 
  },
  { .name = "mqtt_root",
    .len = 33,
    .def = "home" 
  }
};

//startAdr: offset (bytes), writeString: String to be written to EEPROM
void write_eeprom(int startAdr, int maxlength, const char *writeString) {
//  Serial.println("eeprom begin");
  EEPROM.begin(EEPROM_SIZE);  //Max bytes of eeprom to use
//  Serial.println("yield");
//  yield();

 
  int len = strlen(writeString);
//  Serial.print("len: ");
//  Serial.println(len);
  //write to eeprom
  for (int i = 0; i < len && i < maxlength; i++) {
    EEPROM.write(startAdr + i, writeString[i]);
 //   Serial.print("eeprom write: startAdr: ");
 //   Serial.println(startAdr + i);
 //   Serial.print(" writeString: ");
//    Serial.println(writeString[i]);
  }

  // always end string with null
  if(len < maxlength){
    EEPROM.write(startAdr + len, '\0');
  }
  
  // always end eeprom buffer with null
  EEPROM.write(startAdr + maxlength - 1, '\0');

  EEPROM.commit();
  EEPROM.end();
}

void read_eeprom(int startAdr, int maxLength, char *dest) {
  EEPROM.begin(EEPROM_SIZE);
  delay(10);
  for (int i = 0; i < maxLength; i++) {
    dest[i] = EEPROM.read(startAdr + i);
  }
  EEPROM.end();
}

int write_conf() {
  int mem_counter = EEPROM_START;
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    write_eeprom(mem_counter, conf[i].len, conf[i].data);
    mem_counter += conf[i].len;
  }
  return 1;
}

void read_conf() {
  int mem_counter = EEPROM_START;
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    read_eeprom(mem_counter, conf[i].len, conf[i].data);
    mem_counter += conf[i].len;
  }
}



void init_conf() {
  // initalize conf array

  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    conf[i].data = (char *) calloc(conf[i].len, sizeof(char));
    if (conf[i].def != NULL) {
      strcpy(conf[i].data, conf[i].def);
    }
  }


  // check if magic string
  //https://www.cs.umb.edu/cs341/Lab03/index.html
  char *magic = (char *)calloc(conf[0].len, sizeof(char));
  read_eeprom(EEPROM_START, conf[0].len, magic);
  if (strcmp(magic, conf[0].data)) {
    // no, write default conf to EEPROM
    write_conf();
  } else {
    // yes, read conf
    read_conf();
  }
  free(magic);
  conf_initalized = 1;
}

t_conf *get_conf(const char *name) {
  if (!conf_initalized) return 0;
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    if (!strcmp(name, conf[i].name)) {
      return &conf[i];
    }
  }
  return NULL;
}

int set_conf(const char *name, const char *data) {
  if (!conf_initalized) return 0;
  int mem_counter = EEPROM_START;
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    if (!strcmp(name, conf[i].name)) { // 0 = strings equal
      // update struct
      strcpy(conf[i].data, data);
      // write to eeprom
      write_eeprom(mem_counter, conf[i].len, data);
      return 1;
    }
    mem_counter += conf[i].len;
  }
  return 0;
}



#endif

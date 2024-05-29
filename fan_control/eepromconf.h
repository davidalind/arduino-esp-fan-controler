#include <EEPROM.h>

//https://www.tutorialspoint.com/compile_c_online.php


#define EEPROM_START 0
#define EEPROM_SIZE 512
char jsonbuf[600] = { 0 };  // buffer for json string
int conf_initalized = 0;


typedef struct _conf {
  const char *name;
  char *data;
  int len;
  char *def;
  int hidden;
} t_conf;

t_conf conf[] = {
  { .name = "magic_string",
    .len = 5,
    //    .def = {0xA5,0xA5,0xA5,0xA5}
    .def = "magi" },

  // wifi conf
  {
    .name = "wifi_enabled",
    .len = 2,
    .def = "0" },
  { .name = "wifi_ssid",
    .len = 33 },
  { .name = "wifi_password",
    .len = 33,
    .hidden = 1 },

  // mqtt conf
  {
    .name = "mqtt_enabled",
    .len = 2,
    .def = "0" },
  { .name = "mqtt_broker_host",
    .len = 33,
    .def = "localhost" },
  { .name = "mqtt_broker_port",
    .len = 5,
    .def = "1883" },
  { .name = "mqtt_root",
    .len = 33,
    .def = "home/" }
};

void init_conf() {
  // initalize conf array
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    conf[i].data = calloc(conf[i].len, sizeof(char *));
    if (conf[i].def != NULL) {
      strcpy(conf[i].data, conf[i].def);
    }
  }
  // check if magic string
  //https://www.cs.umb.edu/cs341/Lab03/index.html
  char *magic = calloc(conf[0].len, sizeof(char *));
  //  readEEPROM(EEPROM_START, conf[0].len, magic);
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

int write_conf() {
  int mem_counter = EEPROM_START;
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    //writeEEPROM(mem_counter, conf[i].len, conf[i].data);
    mem_counter += conf[i].len;
  }
  return 1;
}

void read_conf() {
  int mem_counter = EEPROM_START;
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    //readEEPROM(mem_counter, conf[i].len, conf[i].data);
    mem_counter += conf[i].len;
  }
}


t_conf *get_conf(char *name) {
  if (!conf_initalized) return 0;
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    if (!strcmp(name, conf[i].name)) {
      return &conf[i];
    }
  }
  return NULL;
}

t_conf *get_conf_a() {
  if (!conf_initalized) return &conf;
  return NULL;
}

/*
int get_conf_json(char *dest) {
  if(!conf_initalized) return 0;
//    char* result = calloc(1);

    for (int i = 0; i < sizeof(conf)/sizeof(t_conf); i++) {
      if(!conf[i].hidden) {
        strlen(jsonbuf
        char* conf = {0};
        char* temp = {0};
        conf = calloc(strlen(conf[i].name) + strlen(conf[i].data) + 6 * sizeof(char));
        strcat(conf, "\"");
        strcat(conf, conf[i].name);
        strcat(conf, "\":\"");
        strcat(conf, conf[i].data);
        strcat(conf, "\",");
        temp = calloc(strlen(dest) + strlen(conf));
        strcpy(temp, dest);
        strcat(temp, conf);
        free(conf);
        free(dest);
        dest = calloc(strlen(temp));
        free(temp);
      }
    }


  return NULL;
}
*/

char *astrcat(const char *x, const char *y) {
  size_t nx = strlen(x), ny = strlen(y);
  char *z = malloc(nx + ny + 1);
  if (!z)
    return NULL;
  memcpy(z, x, nx);
  memcpy(z + nx, y, ny + 1);
  return z;
}

/*
int get_conf(char *name, char *dest) {
  if(!conf_initalized) return 0;
  for (int i = 0; i < sizeof(conf)/sizeof(t_conf); i++) {
    if(!strcmp(name, conf[i].name)) {
      strcpy(dest,conf[i].data);
      return 1;
    }
  }
  return 0;
}
*/

int set_conf(char *name, char *data) {
  if (!conf_initalized) return 0;
  int mem_counter = EEPROM_START;
  for (int i = 0; i < sizeof(conf) / sizeof(t_conf); i++) {
    if (!strcmp(name, conf[i].name)) {
      // update struct
      strcpy(conf[i].data, data);
      // write to eeprom
      //writeEEPROM(mem_counter, conf[i].len, data);
      return 1;
    }
    mem_counter += conf[i].len;
  }
  return 0;
}



/*
int main () {

  init_conf();
  for (int s = 0; s < sizeof(conf)/sizeof(t_conf); s++) {
    printf("name: %s, len: %d, data: %s, def: %s\n", conf[s].name, conf[s].len, conf[s].data, conf[s].def);
  }

  char value[32] = {0};
  get_conf("magic_string", value);
  printf("value: %s",value);

   return(0);
}
*/

//startAdr: offset (bytes), writeString: String to be written to EEPROM
void writeEEPROM(int startAdr, int laenge, char *writeString) {
  EEPROM.begin(EEPROM_SIZE);  //Max bytes of eeprom to use
  yield();
  //  Serial.println();
  //  Serial.print("writing EEPROM: ");
  //write to eeprom
  for (int i = 0; i < laenge; i++) {
    EEPROM.write(startAdr + i, writeString[i]);
    //      Serial.print(writeString[i]);
  }
  EEPROM.commit();
  EEPROM.end();
}

void readEEPROM(int startAdr, int maxLength, char *dest) {
  EEPROM.begin(EEPROM_SIZE);
  delay(10);
  for (int i = 0; i < maxLength; i++) {
    dest[i] = EEPROM.read(startAdr + i);
  }
  EEPROM.end();
  //  Serial.print("ready reading EEPROM:");
  //  Serial.println(dest);
}

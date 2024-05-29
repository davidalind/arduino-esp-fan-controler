

typedef struct _conf
    {
        const *char name;
        void *data;
    } t_conf;


t_conf conf = [
  {
    .name = "wifi_ssid",
    .data = (char *) malloc(sizeof(char) * 32)
  },
  {
    .name = "wifi_password",
    .data = (char *) malloc(sizeof(char) * 32)
  },
  {
    .name = "wifi_enabled",
    .data = (char *) malloc(sizeof(char) * 1)
  }
];




// EEPROM conf
//check for valid data. eg if it has been set.
//https://www.cs.umb.edu/cs341/Lab03/index.html
bool eeprom_valid_data = false;
const long eeprom_magic_number[4] = [0xA5,0xA5,0xA5,0xA5];
const int eeprom_magic_number_start PROGMEM = 1;
const int eeprom_magic_number_len PROGMEM = sizeof(eeprom_magic_number);

const int eeprom_storage_start PROGMEM = eeprom_magic_number_start + eeprom_magic_number_len;

const int eeprom_wifi_enabled_start PROGMEM = eeprom_storage_start;
const int eeprom_wifi_enabled_len PROGMEM = 1;
bool wifi_enabled = false;

const int eeprom_wifi_ssid_start PROGMEM = eeprom_wifi_enabled_start + eeprom_wifi_enabled_len;
const int eeprom_wifi_ssid_len PROGMEM = 32;
const char* wifi_ssid = (char*) malloc(eeprom_wifi_ssid_len + 1);

const int eeprom_wifi_password_start PROGMEM = eeprom_wifi_ssid_start + eeprom_wifi_ssid_len;
const int eeprom_wifi_password_len PROGMEM = 32;
const char* wifi_password = (char*) malloc(eeprom_wifi_password_len + 1);

const int eeprom_mqtt_enabled_start PROGMEM = eeprom_wifi_password_start + eeprom_wifi_password_len;
const int eeprom_mqtt_enabled_len PROGMEM = 1;
bool mqtt_enabled = false;

const int eeprom_mqtt_broker_host_start PROGMEM = eeprom_mqtt_enabled_start + eeprom_mqtt_enabled_len;
const int eeprom_mqtt_broker_host_len PROGMEM = 50;
const char* mqtt_broker_host = (char*) malloc(eeprom_mqtt_broker_host_len + 1);

const int eeprom_mqtt_broker_port_start PROGMEM = eeprom_mqtt_broker_host_start + eeprom_mqtt_broker_host_len;
const int eeprom_mqtt_broker_port_len PROGMEM = 4;
const char* mqtt_broker_port = (char*) malloc(eeprom_mqtt_broker_port_len + 1);

const int eeprom_storage_end PROGMEM = eeprom_mqtt_broker_port_start + eeprom_mqtt_broker_port_len;



void readEEPROMconf() {
  readEEPROMconf_mqtt();
  readEEPROMconf_wifi();
}

void readEEPROMconf_mqtt() {
  if (readEEPROM(eeprom_mqtt_enabled_start) > 1) {
    mqtt_enabled = true;
  } else {
    mqtt_enabled = false;
  }
  readEEPROM(eeprom_mqtt_broker_host_start, eeprom_mqtt_broker_host_len, mqtt_broker_host);
  mqtt_broker_host[eeprom_mqtt_broker_host_len + 1] = 0;
  readEEPROM(eeprom_mqtt_broker_port_start, eeprom_mqtt_broker_port_len, mqtt_broker_port);
  mqtt_broker_port[eeprom_mqtt_broker_port_len + 1] = 0;
}

void readEEPROMconf_wifi() {
  if (readEEPROM(eeprom_wifi_enabled_start) > 1) {
    wifi_enabled = true;
  } else {
    wifi_enabled = false;
  }
  readEEPROM(eeprom_wifi_ssid_start, eeprom_wifi_ssid_len, wifi_ssid);
  wifi_ssid[eeprom_wifi_ssid_len + 1] = 0;
  readEEPROM(eeprom_wifi_password_start, eeprom_wifi_password_len, wifi_password);
  wifi_password[eeprom_wifi_password_len + 1] = 0;
}

void writeEEPROMconf_mqtt(bool enabled, char* host, char* port) {
  if (enabled) {
    writeEEPROM(eeprom_mqtt_enabled_start, (char)1);
  } else {
    writeEEPROM(eeprom_mqtt_enabled_start, (char)0);
  }
  writeEEPROM(eeprom_mqtt_broker_host_start, eeprom_mqtt_broker_host_len, host);
  writeEEPROM(eeprom_mqtt_broker_port_start, eeprom_mqtt_broker_port_len, port);
}

void writeEEPROMconf_wifi(bool enabled, char* ssid, char* password) {
  if (enabled) {
    writeEEPROM(eeprom_wifi_enabled_start, (char)1);
  } else {
    writeEEPROM(eeprom_wifi_enabled_start, (char)0);
  }
  writeEEPROM(eeprom_wifi_ssid_start, eeprom_wifi_ssid_len, ssid);
  writeEEPROM(eeprom_wifi_password_start, eeprom_wifi_password_len, password);
}
  

// check if we have valid data in EEPROM
bool isvalidEEPROM() {
  // check if EEPROM contains valid data
  for (int i = 0; i < eeprom_magic_number_len; i++) {
    if (readEEPROM(eeprom_magic_number_start + i) != eeprom_magic_number[i]) return false;
  }
  return true;
}



// write null to EEPROM
void resetEEPROM() {
  for (int i = eeprom_storage_start; i < eeprom_storage_end + 1; i++) {
    writeEEPROM(i, char(0));
  }
}

void writeEEPROMmagicnumber() {
  // check if EEPROM contains valid data
  for (int i = 0; i < eeprom_magic_number_len; i++) {
    writeEEPROM(eeprom_magic_number_start + i, eeprom_magic_number[i]);
  }
}

void writeEEPROM(int Adr, char data) {
  EEPROM.write(Adr, data);
}

//startAdr: offset (bytes), writeString: String to be written to EEPROM
void writeEEPROM(int startAdr, int laenge, char* writeString) {
  EEPROM.begin(512); //Max bytes of eeprom to use
  yield();
  Serial.println();
  Serial.print("writing EEPROM: ");
  //write to eeprom 
  for (int i = 0; i < laenge; i++)
    {
      writeEEPROM(startAdr + i, writeString[i]);
      Serial.print(writeString[i]);
    }
  EEPROM.commit();
  EEPROM.end();           
}

char readEEPROM(int Adr) {
  return char(EEPROM.read(Adr)
}

void readEEPROM(int startAdr, int maxLength, char* dest) {
  EEPROM.begin(512);
  delay(10);
  for (int i = 0; i < maxLength; i++)
    {
      dest[i] = readEEPROM(startAdr + i);
    }
  EEPROM.end();    
  Serial.print("ready reading EEPROM:");
  Serial.println(dest);
}

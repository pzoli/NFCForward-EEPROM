#include <SPI.h>
#include "EEpromWriteAnything.h"
#include <Ethernet.h>
#include <MFRC522.h>

struct NetConfig
{
  char state[10];
  uint8_t mac[6];
  uint8_t useDHCP;
  uint8_t serverip[4];
  uint8_t ip[4];
  uint8_t subnet[4];
  uint8_t gateway[4];
  uint8_t dnsserver[4];
} conf;

byte* p = (byte*)(void*)&conf;
unsigned int i = 0;

EthernetServer server(80);
String serverAddress;
boolean useDHCP = true;
boolean configured = false;
#define SD_SS_PIN 4
//#define RESETCONFIG true
#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          7         // Configurable, see typical pin layout above
#define DEBUG true

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
String defRequest = "/log.php?readerId=%RID%&cardId=%CID%";
String request = defRequest;

const byte default_id[4] = {192, 168, 1, 14};
const uint8_t default_mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const uint8_t default_subnet[4] = {255, 255, 255, 0};
const uint8_t default_gateway[4] = {192, 168, 1, 1};
const uint8_t default_dnsserver[4] = {192, 168, 1, 1};

char charArrayOfServerAddress[16];
String stringOfMACAddress;

void readConfig() {
  EEPROM_readAnything(0, conf);
  Serial.println();
  sprintf(charArrayOfServerAddress, "%d.%d.%d.%d", conf.serverip[0], conf.serverip[1], conf.serverip[2], conf.serverip[3]);
  char charArrayOfMACAddress[17];
  sprintf(charArrayOfMACAddress, "%02X:%02X:%02X:%02X:%02X:%02X", conf.mac[0], conf.mac[1], conf.mac[2], conf.mac[3],conf.mac[4],conf.mac[5]);
  stringOfMACAddress = String((char*)charArrayOfMACAddress);
  printConfig(conf);
}

void printIPToSerial(String ipname, IPAddress addr) {
  char ipno[16];
  sprintf(ipno, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
  Serial.print(ipname);
  Serial.print(ipno);
}

void printConfig(NetConfig oconf) {
  Serial.println("{");
  Serial.print(F("'state':'"));
  Serial.print(oconf.state);
  Serial.println("',");
  Serial.print(F("'mac':'"));
  Serial.print(stringOfMACAddress);
  Serial.println("',");
  Serial.print(F("'useDHCP':'"));
  Serial.print(oconf.useDHCP);
  Serial.println("',");
  printIPToSerial(F("'ip':'"), oconf.ip);
  Serial.println("',");
  printIPToSerial(F("'subnet':'"), oconf.subnet);
  Serial.println("',");
  printIPToSerial(F("'gateway':'"), oconf.gateway);
  Serial.println("',");
  printIPToSerial(F("'dnsserver':'"), oconf.dnsserver);
  Serial.println("',");
  printIPToSerial(F("'serverip':'"), oconf.serverip);
  Serial.println("',");
  Serial.print(F("'request':'"));
  Serial.print(request);
  Serial.println("'\n}");
}

void resetConfig() {
  strncpy(conf.state, "reseted  ", 10);
  memcpy(conf.mac, default_mac, 6);
  conf.useDHCP = true;
  memcpy(conf.ip, default_id, 4);
  memcpy(conf.subnet, default_subnet, 4);
  memcpy(conf.gateway, default_gateway, 4);
  memcpy(conf.dnsserver, default_dnsserver, 4);
  EEPROM_writeAnything(0, conf);
}

void initRC522() {
  mfrc522.PCD_Init();   // Init MFRC522
#ifdef DEBUG
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
#endif
}

void initEthernet() {
  if (conf.useDHCP == 1)
    Ethernet.begin(conf.mac);
  else
    Ethernet.begin(conf.mac, conf.ip, conf.dnsserver, conf.gateway, conf.subnet);
  server.begin();
#ifdef DEBUG
  Serial.print("reader ip: ");
  Serial.println(Ethernet.localIP());
#endif
}

void initPins() {
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

#ifdef RESETCONFIG
  resetConfig();
#endif

#ifdef DEBUG
  while (!Serial) {
    ;
  }
#endif
  initPins();
  readConfig();
  if (String(conf.state).startsWith("run")) {
    initEthernet();
    initRC522();
    configured = true;
#ifdef DEBUG
    Serial.println(F("Setup ready"));
#endif
  }
#ifdef DEBUG
  else
    Serial.println(F("Not configured yet."));
#endif
}


EthernetClient webClient;
String command;
bool dataSent = false;
bool firstLine = true;
String webResult = "";

void processRequest() {
  #ifdef DEBUG
      Serial.println();
      Serial.println(F("webclient stop."));
      Serial.println();
      Serial.println(F("webresult:"));
      Serial.println(webResult);
#endif
      int resultStartIdx = webResult.indexOf("{\"");
      if (resultStartIdx > -1) {
        int resultEndIdx = webResult.indexOf("\"}");
        if (resultEndIdx > -1) {
          webResult = webResult.substring(resultStartIdx,resultEndIdx+2);
#ifdef DEBUG
          Serial.println();
          Serial.println(F("server result:"));
          Serial.println(webResult);
#endif
          resultEndIdx = webResult.indexOf("\"}");
          String allowed = webResult.substring(resultEndIdx-1,resultEndIdx);
#ifdef DEBUG
          Serial.println();
          Serial.print(F("Allowed:"));
          Serial.println(allowed);
#endif    
          if (allowed == "t") {
#ifdef DEBUG
            Serial.println();
            Serial.println(F("Beengedem"));
#endif    
          } else {
#ifdef DEBUG
            Serial.println();
            Serial.println(F("Kitiltom"));
#endif                      
          }
        }        
      }
      webResult = "";
}
void loop() {
  if (configured) {
    if (webClient.available()) {
      char buff[64];
      int cnt = webClient.read(buff, sizeof(buff));
      for(int i=0;i<cnt;i++) {
        webResult += buff[i];
      }
#ifdef DEBUG
      Serial.write(buff, cnt);
#endif
    } 

    if (!webClient.connected() && dataSent) {
      webClient.stop();
      dataSent = false;
      processRequest();
    }
    // Look for new cards
    if ( mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String cardId;
      for ( uint8_t i = 0; i < 4; i++) {
        byte readCard = mfrc522.uid.uidByte[i];
        char chrHex[2];
        sprintf(chrHex, "%02X", readCard);
        cardId += chrHex;
      }
      mfrc522.PICC_HaltA(); // Stop reading
#ifdef DEBUG
      printIPToSerial(F("connect to"),conf.serverip);
      Serial.println();
      Serial.print(F("cardid:"));
      Serial.println(cardId);
#endif
      if (webClient.connect(conf.serverip, 80)) {
        webClient.print(F("GET "));
        request.replace(F("%RID%"),stringOfMACAddress);
        request.replace(F("%CID%"),cardId);
#ifdef DEBUG
      Serial.println();
      Serial.print(F("request:"));
      Serial.println(request);
#endif
        webClient.print(request);        
        webClient.println(F(" HTTP/1.1"));
        webClient.print(F("Host: "));
        webClient.print(charArrayOfServerAddress);
        webClient.println();
        webClient.println(F("Connection: close"));
        webClient.println();
        dataSent = true;
        request = defRequest;
      } else {
#ifdef DEBUG
        Serial.println(F("connection failed"));
#endif
      }
    }
  }

  if (Serial.available()) {
    if (i < sizeof(conf)) {
      *p++ = Serial.read();
      i++;
#ifdef DEBUG
      Serial.print("i(");
      Serial.print(sizeof(conf));
      Serial.print("):");
      Serial.println(i);
#endif
    }
    if (i == sizeof(conf)) {
      p = (byte*)(void*)&conf;
      i = 0;
      if (String(conf.state) == "configure") {
        strncpy(conf.state, "run      ", 10);
        EEPROM_writeAnything(0, conf);
        readConfig();
        initEthernet();
      } else {
        Serial.print(F("unknow command ["));
        Serial.print(conf.state);
        Serial.println("]");
      }
    }
  }
}

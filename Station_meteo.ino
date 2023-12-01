#include <SoftwareSerial.h>
#include <Wire.h>
#include <DS1307.h>
#include <SPI.h>
#include <SD.h> 
#include <Adafruit_BME280.h>    // Inclusion de la librairie BME280 d'Adafruit
#include <RTClib.h>                           
#include <ChainableLED.h>
#include <EEPROM.h>

#define adresseI2CduBME280 0x76     
#define boutonRouge 3
#define boutonVert 2
#define chipSelect 4
#define analogPin 2
byte config = digitalRead(boutonRouge);

const byte numChars = 12;
char receivedChars[numChars];

// Instanciation de la librairie BME280
Adafruit_BME280 bme;
RTC_DS3231 rtc;

const int nombre_variables = 15;
long int inact;

int data = 0;
int code = 0;
int fichier = 0;
int i = 0;
int mode = 0; /* Changement de mode par cette variable : 
0 : Standard
1 : Maintenance
2 : Economie
3 : Configuration */
unsigned long time_now  = 0;

char file0[13];
char file1[14];

char timeBuffer[9];  
char dataString[12];

const char *variableNames[] = {
  "LUMIN", "LUMIN_LOW", "LUMIN_HIGH", "TEMP_AIR", "MIN_TEMP_AIR",
  "MAX_TEMP_AIR", "HYGR", "HYGR_MINT", "HYGR_MAXT", "PRESSURE",
  "PRESSURE_MIN", "PRESSURE_MAX", "LOG_INTERVALL", "FILE_MAX_SIZE", "TIMEOUT"
};

int variableInter[nombre_variables][2] = { { 0, 1 },
                                           { 0, 1023 },
                                           { 0, 1023 },
                                           { 0, 1 },
                                           { -40, 85 },
                                           { -40, 85 },
                                           { 0, 1 },
                                           { -40, 85 },
                                           { -40, 85 },
                                           { 0, 1 },
                                           { 300, 1100 },
                                           { 300, 1100 },
                                           { 1, 300 },
                                           { 2048, 16384 }, 
                                           { 5, 120 } };

DS1307 clock;//define a object of DS1307 class RTC Clock on I2C port on Grove Shield
volatile bool flagRouge = 0;
volatile bool flagVert = 0;
unsigned long start = 0;
unsigned long temp = 0;


ChainableLED leds(4, 5, 1);

void initialisation_sd(){
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    code = 9;
    while (1){allumage();}
  }
  Serial.println(F("Carte initialisée"));
}

void initialisation_interruption() {
  attachInterrupt(digitalPinToInterrupt(boutonRouge), basculerRouge , CHANGE);
  attachInterrupt(digitalPinToInterrupt(boutonVert), basculerVert, CHANGE);
}

void initialisation_mon_serie() {
  Serial.begin(9600);
}

void initialisation_boutons() {
  pinMode(boutonRouge, INPUT);
  pinMode(boutonVert, INPUT);
}

void initialisation_bme() {
  if(!bme.begin(adresseI2CduBME280)) {
    code = 6;
    while(1){allumage();}                    
  } 
}

void formatTime(char *buffer) {
  DateTime now = rtc.now();
  snprintf(buffer, sizeof(timeBuffer), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
}


int para_pos(int i){
  byte h = EEPROM.read(i*2);
  byte l = EEPROM.read(i*2+1);
  int val = word(h,l);
  return val;
}

int para_neg(int i){
  int val;
  EEPROM.get(i*2+1, val);
  return val;
}

int Mesures(){
  int mesures = 0;
  for (int i = 0; i<4; i++){
    if(para_pos(i*3) == 1){
      mesures++;
    }
  }
  return mesures;
}

void Recuperation() {
  int data = 0;
  int mesures = Mesures();
  formatTime(timeBuffer);
  snprintf(dataString, sizeof(dataString), "%s ; ", timeBuffer);
  if (mode != 1){
  ecriture(mesures, dataString);}
  else{Serial.print(dataString);}

  if (para_pos(0) == 1){
    if (mode != 1){
    ecriture(mesures,  F("Luminosité : "));}
    else{Serial.print(F("Luminosité : "));}
    data++;
    if (mode != 1){
    ecritureValues(mesures,data, analogRead(0));}
    else{Serial.print(analogRead(0));
    Serial.print(" ; ");}
  }


  if (para_pos(3) == 1){
    if (mode != 1){
    ecriture(mesures,  F("Temperature : "));}
    else{Serial.print(F("Temperature : "));}
    data++;
    if (mode != 1){
      if(!bme.init()){
        code = 6; 
        allumage();
        ecriture(mesures, F("NA ; "));
      }
      else if (bme.readTemperature() < para_neg(4) || bme.readTemperature() > para_neg(5)){
        code = 7;
        while(1){allumage();}
      }
      else{ecritureValues(mesures,data,  bme.readTemperature());}}
    else{
      if(!bme.init()){
        code = 6; 
        allumage();
        ecriture(mesures, F("NA ; "));
      }
      else{
        Serial.print(bme.readTemperature());
        Serial.print(" ; ");}
    }
  }

  if (para_pos(6) == 1){
    if (mode != 1){
    ecriture(mesures,  F("Pression : "));}
    else{Serial.print(F("Pression : "));}
    data++;
    if (mode != 1){
      if(!bme.init()){
        code = 6; 
        allumage();
        ecriture(mesures, F("NA ; "));
      }
      else if (bme.readPressure()/100 < para_pos(10) || bme.readPressure()/100 > para_pos(11)){
        code = 7;
        while(1){allumage();}
      }
      else{ecritureValues(mesures,data,  bme.readPressure()/100);}}
    else{
      if(!bme.init()){
        code = 6; 
        allumage();
        ecriture(mesures, F("NA ; "));
      }
      else{
        Serial.print(bme.readPressure()/100);
        Serial.print(" ; ");}
    }
    }

  if (para_pos(9) == 1){
    if (mode != 1){
    ecriture(mesures,  F("Humidite : "));}
    else{Serial.print(F("Humidite : "));}
    data++;
    if (mode != 1){
      if(!bme.init() || (bme.readTemperature() < para_neg(7) || bme.readTemperature() > para_neg(8))){
        code = 6; 
        allumage();
      DateTime now = rtc.now();
        snprintf(file0, sizeof(file0), "%02d%02d%02d_%d.LOG", now.year() - 2000, now.month(), now.day(), fichier);
        File DataFile = SD.open(file0, FILE_WRITE);
        if (DataFile) {
          DataFile.println(" NA ;");
          DataFile.close();
        }
      }
      else {ecritureValues(mesures,data, bme.readHumidity());}}
    else{
      if(!bme.init()){
        code = 6; 
        allumage();
        ecriture(mesures, F("NA ; "));
      }
      else{
        Serial.print(bme.readHumidity());
        Serial.println(" ; ");}
    }
    }
  data = 0;
}


int ecriture(int mesures, String dataString) {
    DateTime now = rtc.now();
  snprintf(file0, sizeof(file0), "%02d%02d%02d_%d.LOG", now.year() - 2000, now.month(), now.day(), fichier);
  File DataFile = SD.open(file0, FILE_WRITE);
  if (DataFile) {
    DataFile.print(dataString);
    DataFile.close();
  }
  else{
    code = 9;
    while(1){allumage();}
  }
}


int ecritureValues(int mesures, int data, float capteur) {
    DateTime now = rtc.now();
    snprintf(file0, sizeof(file0), "%02d%02d%02d_%d.LOG", now.year() - 2000, now.month(), now.day(), fichier);
  File DataFile = SD.open(file0, FILE_WRITE);
  if (DataFile) {
    if(data == mesures){
      DataFile.print(capteur);
      DataFile.println(F(" ; "));
      if (DataFile.size() > para_pos(13)){
        fichier++;}
    DataFile.close();
    }
    else{
      DataFile.print(capteur);
      DataFile.print(F(" ; "));
      DataFile.close();
    }
  }
  else{
    code = 9;
    while(1){allumage();}
  }
}

void basculerRouge(){
  start = millis();
  flagRouge = not(flagRouge);
  return;
}  

void basculerVert(){
  start = millis();
  flagVert = not(flagVert);
  return;

}

void allumage(){
//CODES MODES ALLUMAGES CONTINUS
//0:standard 1:config 2:eco 3:maintenance
//CODES ERREURS ALLUMAGES CLIGNOTANT
//4:horlogeRTC 5:GPS 6:dataCapteur 7 :Données incohérentes 8:SDfull 9:AccesSD
  int i=0;
  if (code == 0){
    leds.setColorRGB(0,0,255,0);
  }
  else if(code==1){
    leds.setColorRGB(0,255,127,0);
    }
  else if(code==2){
    leds.setColorRGB(0,0,0,255);
    }
  else if(code==3){
    leds.setColorRGB(0,255,255,0);
    }
  else if(code==4){
  for(i=0;i<3;i++){
    leds.setColorRGB(0,0,0,255);
    delay(500);
    leds.setColorRGB(0,255,0,0);
    delay(500);
  }leds.setColorRGB(0,0,0,0);}
  else if(code==5){
  for(i=0;i<3;i++){
    leds.setColorRGB(0,248,255,103);
    delay(500);
    leds.setColorRGB(0,255,0,0);
    delay(500);
  }leds.setColorRGB(0,0,0,0);}
    else if(code==6){
  for(i=0;i<3;i++){
    leds.setColorRGB(0,0,255,0);
    delay(500);
    leds.setColorRGB(0,255,0,0);
    delay(500);
  }leds.setColorRGB(0,0,0,0);}
  else if(code==7){
  for(i=0;i<3;i++){
    leds.setColorRGB(0,255,0,0);
    delay(500);
    leds.setColorRGB(0,0,255,0);
    delay(1000);
  }leds.setColorRGB(0,0,0,0);}
  else if(code==8){
  for(i=0;i<3;i++){
    leds.setColorRGB(0,255,0,0);
    delay(500);
    leds.setColorRGB(0,255,255,255);
    delay(500);
  }leds.setColorRGB(0,0,0,0);}
  else if(code==9){
  for(i=0;i<3;i++){
    leds.setColorRGB(0,255,0,0);
    delay(500);
    leds.setColorRGB(0,255,255,255);
    delay(1000);
  }leds.setColorRGB(0,0,0,0);}
}

void setup() {
  // Initialisation de la carte Arduino
  initialisation_mon_serie();
  initialisation_boutons();
  initialisation_bme();
  initialisation_sd();
  initialisation_interruption();
  Wire.begin();
  if(!rtc.begin()){
    code = 4;
    while(1){allumage();}
  }
  if (config == LOW){
    mode = 3;
    code = mode;
    allumage();
  }
  else{allumage();}
}

void loop() {
  int long timing = millis();
  if (flagRouge or flagVert){
    temp = millis();
    if(temp - start > 5000){
      if(flagRouge){
        if (mode==0){mode=1;code=mode;
        allumage();
        }
        else if (mode==1){mode=0;code=mode;
        allumage();
        }
      }
      if(flagVert){
        if (mode==0){mode=2;code=mode;
        allumage();
        }
        else if (mode == 2){mode=0;code=mode;
        allumage();
        }
      }
    flagRouge = 0;
    flagVert = 0;
  }
  }
  if (millis() >= time_now + 2500/*para_pos(12)*60000*/ && (mode == 0 || mode == 1)){
    time_now += 2500;
    Recuperation();
  }
  if (millis() >= time_now + 10000/*para_pos(12)*60000*2*/ && mode == 2){
    time_now += 10000;
    Recuperation();
  }
  if (timing >= inact + 30000 && mode == 3){
    mode=0;
    code=mode;
    allumage();
  }

  if (mode == 3){
  int valeur = 0;
  char parametre[15];
  bool parametreTrouve = false;

  if (Serial.available()) {
    String entree = Serial.readStringUntil('\n');

    if (entree == "SETTINGS") {
      int j = 0;
      byte lowByte, highByte;
      
      for (int i = 0; i < nombre_variables; i++) {
        if (j == 8 || j == 10 || j == 14 || j == 16) {
          EEPROM.get(j + 1, valeur);
          Serial.print(variableNames[i]);
          Serial.print(" : ");
          Serial.println(valeur);
          j+=2;
        } else {
          highByte = EEPROM.read(j);
          lowByte = EEPROM.read(j + 1);
          valeur = word(highByte, lowByte);
          Serial.print(variableNames[i]);
          Serial.print(" : ");
          Serial.println(valeur);
          j+=2;
        }
      inact = timing;
      }
    
    } else if (entree == F("RESET")) {
      // 1
      EEPROM.write(0, 0);
      EEPROM.write(1, 1);
      // 255
      EEPROM.write(2, 0);
      EEPROM.write(3, 255);
      // 768
      EEPROM.write(4, 3);
      EEPROM.write(5, 0);
      // 1
      EEPROM.write(6, 0);
      EEPROM.write(7, 1);
      // -10
      EEPROM.put(8, 0);
      EEPROM.put(9, 0-10);
      // 60
      EEPROM.put(10, 0);
      EEPROM.put(11, 60);
      // 1
      EEPROM.write(12, 0);
      EEPROM.write(13, 1);
      // 0
      EEPROM.put(14, 0);
      EEPROM.put(15, 0);
      // 50 
      EEPROM.put(16, 0);
      EEPROM.put(17, 50);
      // 1
      EEPROM.write(18, 0);
      EEPROM.write(19, 1);
      // 850
      EEPROM.write(20, 3);
      EEPROM.write(21, 82);
      // 1080
      EEPROM.write(22, 4);
      EEPROM.write(23, 56);
      // 10
      EEPROM.write(24, 0);
      EEPROM.write(25, 10);
      // 4096
      EEPROM.write(26, 16);
      EEPROM.write(27, 0);
      // 30
      EEPROM.write(28, 0);
      EEPROM.write(29, 30);



      Serial.println(F("Valeurs réinitialisées"));
      inact = timing;

    } 
    else if (entree == F("VERSION")) {
      Serial.println(F("Version : 1.01"));
      Serial.println(F("Numéro de lot : 23021"));
      inact = timing;}

     else if (sscanf(entree.c_str(), "%s = %d", parametre, &valeur) == 2) {
      entree = String(parametre);
      int i = 0;

      for (int j = 0; j < 15; j++) {
        if (entree == variableNames[j]) {
          parametreTrouve = true;
          bool intervalOk = intervalVerification(valeur, i);
          if (intervalOk) {
            writeValue(entree, valeur);
          }
          break;
        } else {
          i++;
        }
        inact = timing;
      }


      if (!parametreTrouve) {
        Serial.println(F("ERROR"));
        Serial.print("\n");
        parametre[0] = '\0';
      }
    } else {
      Serial.println(F("ERROR"));
      Serial.print("\n");
      parametre[0] = '\0';
    }
  }
  }
}

bool intervalVerification(int valeur, int i) {
  bool intervalOk;
  if (valeur >= variableInter[i][0] && valeur <= variableInter[i][1]) {
    intervalOk = true;
  } else {
    Serial.println(F("ERROR"));
    Serial.print("\n");
    intervalOk = false;
  }
  return intervalOk;
}

void writeValue(String entree, int valeur) {
  int j = 0;
  for (int i = 0; i < nombre_variables ; i++) {
    if (entree == variableNames[i]) {
      if (entree == variableNames[4] || entree == variableNames[5] || entree == variableNames[7] || entree == variableNames[8]) {
        EEPROM.put(j, 0);
        EEPROM.put(j + 1, valeur);
        Serial.println(F("Valeur changée"));
        break;
      } else {
        EEPROM.write(j, valeur >> 8);
        EEPROM.write(j + 1, valeur & 0xFF);
        Serial.println(F("Valeur changée"));
        break;
      }
    } else {
      j+=2;
    }
  }
}

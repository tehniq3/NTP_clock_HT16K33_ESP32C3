/*******************************************************************************
// Adaptat pentru ESP32-C3 Mini (Compatibil cu Core 2.0.5)
// DST Calculat Manual
// Parsare JSON imbunatatita (cauta strict in sectiunea "current")
 *******************************************************************************/

#include <WiFi.h>                 
#include <WiFiManager.h>          
#include <Wire.h>
#include <HTTPClient.h>           
#include <time.h>                 
#include "NoiascaHt16k33.h"      
#include <math.h>                 

Noiasca_ht16k33_hw_14_ext display = Noiasca_ht16k33_hw_14_ext(); 

const char* openMeteoUrl = "http://api.open-meteo.com/v1/forecast?latitude=44.3191&longitude=23.8001&current=temperature_2m,relative_humidity_2m&timezone=Europe/Bucharest";
unsigned long lastWeatherUpdate = 600000 * -1; 
const unsigned long weatherUpdateDelay = 600000; 

const char* ntpServer = "pool.ntp.org";

unsigned long tpafisare;
unsigned long tpinfo = 60000;  

struct tm timeinfo; 

int i = 0;
int j = 4;

int pauzamica = 100;  
int pauzamare = 3000; 
int pauzamedie = 300;  

int ora, minut;

int humidity = 0;
int temperature = 0;
int temperature2 = 0;
int zi = 0;

int n = 0;

const String weekDays[7][2] = {
  "    Saturday    ", "    Duminica    ",
  "    Monday    ", "    Luni    ",
  "    Tuesday    ", "    Marti    ",
  "    Wednesday    ", "    Miercuri    ",
  "    Thursday   ", "    Joi    ",
  "    Friday    ", "    Vineri    ",
  "    Sunday    ", "    Sambata    "
  };

String informatii[5][2] = {
  "    Clock    ", "    Ora    ",   
  "    Date    ", "    Data    ",   
  "    Year    ", "    An    ",     
  "    Temperature    ", "    Temperatura    ",     
  "    Humidity    ", "    Umiditate    "     
  };
byte b = 0; 
String info1;
char info2[20];

const String intro = {"    NTP clock with Meteo API v-8 AutoDST by niq_ro     "};

bool getLocalTimeWithDST(struct tm *info) {
  time_t now = time(nullptr);
  if (now < 100000) return false; 
  
  struct tm utcTime;
  gmtime_r(&now, &utcTime); 
  
  bool dst = false;
  int month = utcTime.tm_mon + 1; 
  int day = utcTime.tm_mday;      
  int dow = utcTime.tm_wday;      
  int hour = utcTime.tm_hour;     
  
  if (month > 3 && month < 10) {
    dst = true;
  } else if (month == 3) {
    if (day - dow >= 25) { 
      if (day - dow == 25) dst = (hour >= 1); 
      else dst = true;
    }
  } else if (month == 10) {
    if (day - dow >= 25) { 
      if (day - dow == 25) dst = (hour < 1); 
      else dst = false;
    } else {
      dst = true; 
    }
  }
  
  int offsetSec = dst ? 3 * 3600 : 2 * 3600;
  time_t localTime = now + offsetSec;
  localtime_r(&localTime, info);
  
  return true;
}

void fetchWeatherData() {
  if (WiFi.status() == WL_CONNECTED && millis() - lastWeatherUpdate >= weatherUpdateDelay) {
    HTTPClient http;
    http.begin(openMeteoUrl);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      
      // 1. Găsim secțiunea unde sunt datele reale (care începe cu "current":{)
      int currentSection = payload.indexOf("\"current\":{");
      
      if (currentSection != -1) {
        // 2. Căutăm temperatura DOAR în secțiunea găsită mai sus
        int tempIndex = payload.indexOf("\"temperature_2m\":", currentSection);
        if (tempIndex != -1) {
          int valStart = payload.indexOf(':', tempIndex) + 1;
          int valEnd = payload.indexOf(',', valStart);
          if (valEnd == -1) valEnd = payload.indexOf('}', valStart); // safety
          String tempStr = payload.substring(valStart, valEnd);
          temperature = (int)round(tempStr.toFloat());
        }

        // 3. Căutăm umiditatea DOAR în secțiunea găsită mai sus
        int humIndex = payload.indexOf("\"relative_humidity_2m\":", currentSection);
        if (humIndex != -1) {
          int valStart = payload.indexOf(':', humIndex) + 1;
          int valEnd = payload.indexOf('}', valStart);
          String humStr = payload.substring(valStart, valEnd);
          humidity = (int)round(humStr.toFloat());
        }
      }

      Serial.println("Meteo: Temp=" + String(temperature) + "C, Hum=" + String(humidity) + "%");
      lastWeatherUpdate = millis();
    } else {
      Serial.println("Eroare HTTP Meteo: " + String(httpCode));
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nStart..."));

  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
      
  Serial.println("WiFi connected!");

  configTime(0, 0, ntpServer);

  Serial.print("Se sincronizează NTP...");
  time_t now = time(nullptr);
  while (now < 100000) { 
    Serial.print(".");
    delay(500);
    now = time(nullptr);
  }
  Serial.println("\nNTP Sincronizat cu succes!");
  
  fetchWeatherData();  
  
  Wire.begin();                                  
  display.begin(0x70, 1);                        
  if (display.isConnected() == false)            
    Serial.println(F("E: display error"));
  display.setDigits(4);                          
  display.setBrightness(2);                      
  display.clear();

  delay(4000);
  char intro_char[65];
  intro.toCharArray(intro_char,intro.length()); 
  Serial.println (intro_char);
  
  j = 0;
  while (j <= intro.length()-4) {
    display.print(intro_char[j]);
    display.print(intro_char[j+1]);
    display.print(intro_char[j+2]);
    display.print(intro_char[j+3]);
    delay(pauzamedie);
    j++;
  }
  display.clear();
}

void loop() {
  if (getLocalTimeWithDST(&timeinfo)) {
    ora = timeinfo.tm_hour;
    minut = timeinfo.tm_min;
    zi = timeinfo.tm_wday; 
  }

  fetchWeatherData();

  // --- CLOCK ---
  b = 0; 
  info1 = informatii[b][i%2];
  info1.toCharArray(info2,info1.length()); 
 
  j = 0;
  while (j <= info1.length()-4) {
    display.print(info2[j]);
    display.print(info2[j+1]);
    display.print(info2[j+2]);
    display.print(info2[j+3]);
    delay(pauzamedie);
    j++;
  }
  display.clear();

  String intrare = "    ";
  if (ora < 10) intrare = intrare + " ";
  else intrare = intrare + ora/10;
  intrare = intrare + ora%10;
  intrare = intrare + minut/10;
  intrare = intrare + minut%10;    
  char intrare_char[9];
  intrare.toCharArray(intrare_char,intrare.length()+1); 
        
  j = 0;
  while (j <= 4) {
    display.print(intrare_char[j]);
    display.print(intrare_char[j+1]);
    display.print(intrare_char[j+2]);
    display.print(intrare_char[j+3]);
    delay(pauzamica);
    j++;
  }
  display.clear();

  int s = 0;
  while (s < 20) {
    if (ora < 10) display.print(F(" "));
    display.print(ora);
    if (millis()/1000%2 == 0) display.print(F("."));
    if (minut < 10) display.print(F("0"));
    display.print(minut);  
    delay(500); 
    s++;
  }
  display.clear();
  
  String iesire = "";
  if (ora < 10) iesire = iesire + " ";
  else iesire = iesire + ora/10;
  iesire = iesire + ora%10;
  iesire = iesire + minut/10;
  iesire = iesire + minut%10;
  iesire = iesire + "    "; 
  char iesire_char[9];
  iesire.toCharArray(iesire_char,iesire.length()+1); 

  j = 0;
  while (j <= 4) {
    display.print(iesire_char[j]);
    display.print(iesire_char[j+1]);
    display.print(iesire_char[j+2]);
    display.print(iesire_char[j+3]);
    delay(pauzamica);
    j++;
  }
  display.clear();

  // --- WEEKDAY ---
  String weekDay = weekDays[zi][i%2];
  char numezi[20];
  weekDay.toCharArray(numezi,weekDay.length()); 
    
  j = 0;
  while (j <= weekDay.length()-4) {
    display.print(numezi[j]);
    display.print(numezi[j+1]);
    display.print(numezi[j+2]);
    display.print(numezi[j+3]);
    delay(pauzamedie);
    j++;
  }
  display.clear();

  // --- DATE ---
  b = 1; 
  info1 = informatii[b][i%2];
  info1.toCharArray(info2,info1.length()); 
  display.clear();   
  j = 0;
  while (j <= info1.length()-4) {
    display.print(info2[j]);
    display.print(info2[j+1]);
    display.print(info2[j+2]);
    display.print(info2[j+3]);
    delay(pauzamedie);
    j++;
  }
  display.clear();

  int day = timeinfo.tm_mday;
  int month = timeinfo.tm_mon + 1; 
  int year = timeinfo.tm_year + 1900; 

  String intrare3 = "    ";
  intrare3 = intrare3 + day/10;
  intrare3 = intrare3 + day%10;
  intrare3 = intrare3 + month/10;
  intrare3 = intrare3 + month%10;    
  char intrare_char3[9];
  intrare3.toCharArray(intrare_char3,intrare3.length()+1); 
        
  j = 0;
  while (j <= 4) {
    display.print(intrare_char3[j]);
    display.print(intrare_char3[j+1]);
    display.print(intrare_char3[j+2]);
    display.print(intrare_char3[j+3]);
    delay(pauzamica);
    j++;
  }
  display.clear();

  if (day < 10) display.print(F("0"));
  display.print(day);
  display.print(F("."));
  if (month < 10) display.print(F("0"));
  display.print(month); 
  display.print(F("."));
  delay(pauzamare);
  display.clear();

  display.print(year);
  display.print(F("."));
  delay(pauzamare);
  display.clear();

  String iesire4 = "";
  iesire4 = iesire4 + year;
  iesire4 = iesire4 + "    "; 
  char iesire_char4[9];
  iesire4.toCharArray(iesire_char4,iesire4.length()+1); 

  j = 0;
  while (j <= 4) {
    display.print(iesire_char4[j]);
    display.print(iesire_char4[j+1]);
    display.print(iesire_char4[j+2]);
    display.print(iesire_char4[j+3]);
    delay(pauzamica);
    j++;
  }
  display.clear(); 

  // --- TEMPERATURE ---
  b = 3;  
  info1 = informatii[b][i%2];
  info1.toCharArray(info2,info1.length()); 
  display.clear();   
  j = 0;
  while (j <= info1.length()-4) {
    display.print(info2[j]);
    display.print(info2[j+1]);
    display.print(info2[j+2]);
    display.print(info2[j+3]);
    delay(pauzamedie);
    j++;
  }
  display.clear();

  String intrare2 = "    ";
  if (temperature >= 0) {
    temperature2 = temperature;
    if (temperature2/10 == 0) intrare2 = intrare2 + " ";
    else intrare2 = intrare2 + temperature2/10;
    intrare2 = intrare2 + temperature2%10;
    intrare2 = intrare2 + "%";
    intrare2 = intrare2 + "C"; 
  } else {
    temperature2 = -temperature;
    intrare2 = intrare2 + "-";
    if (temperature2 < 10) {
      intrare2 = intrare2 + temperature2%10;
      intrare2 = intrare2 + "%";
      intrare2 = intrare2 + "C";
    } else {
      intrare2 = intrare2 + temperature2/10;
      intrare2 = intrare2 + temperature2%10;
      intrare2 = intrare2 + "C"; 
    } 
  }
  char intrare_char2[9];
  intrare2.toCharArray(intrare_char2,intrare2.length()+1); 
        
  j = 0;
  while (j <= 4) {
    display.print(intrare_char2[j]);
    display.print(intrare_char2[j+1]);
    display.print(intrare_char2[j+2]);
    display.print(intrare_char2[j+3]);
    delay(pauzamica);
    j++;
  }

  if (temperature >= 0) {
    temperature2 = temperature;
    if (temperature2 < 10) display.print(F(" "));
    display.print(temperature2);
    display.print("%C"); 
  } else {
    temperature2 = -temperature;
    display.print("-");
    display.print(temperature2);
    if (temperature2 < 10) display.print("%C"); 
    else display.print("C");  
  }
  delay(pauzamare);
  display.clear();

  String iesire2 = "";
  if (temperature >= 0) {
    temperature2 = temperature;
    if (temperature2/10 == 0) iesire2 = iesire2 + " ";
    else iesire2 = iesire2 + temperature2/10;
    iesire2 = iesire2 + temperature2%10;
    iesire2 = iesire2 + "%";
    iesire2 = iesire2 + "C"; 
  } else {
    temperature2 = -temperature;
    iesire2 = iesire2 + "-";
    if (temperature2 < 10) {
      iesire2 = iesire2 + temperature2%10;
      iesire2 = iesire2 + "%";
      iesire2 = iesire2 + "C";
    } else {
      iesire2 = iesire2 + temperature2/10;
      iesire2 = iesire2 + temperature2%10;
      iesire2 = iesire2 + "C"; 
    } 
  }
  iesire2 = iesire2 + "    "; 
  char iesire_char2[9];
  iesire2.toCharArray(iesire_char2,iesire2.length()+1); 

  j = 0;
  while (j <= 4) {
    display.print(iesire_char2[j]);
    display.print(iesire_char2[j+1]);
    display.print(iesire_char2[j+2]);
    display.print(iesire_char2[j+3]);
    delay(pauzamica);
    j++;
  }
  display.clear();

  // --- HUMIDITY ---
  b = 4;  
  info1 = informatii[b][i%2];
  info1.toCharArray(info2,info1.length()); 
  display.clear();   
  j = 0;
  while (j <= info1.length()-4) {
    display.print(info2[j]);
    display.print(info2[j+1]);
    display.print(info2[j+2]);
    display.print(info2[j+3]);
    delay(pauzamedie);
    j++;
  }
  display.clear();

  if (humidity >= 100) humidity = 99;

  String intrare1 = "    ";
  if (humidity/10 == 0) intrare1 = intrare1 + " ";
  else intrare1 = intrare1 + humidity/10;
  intrare1 = intrare1 + humidity%10;
  intrare1 = intrare1 + "%";
  intrare1 = intrare1 + "o";    
  char intrare_char1[9];
  intrare1.toCharArray(intrare_char1,intrare1.length()+1); 
        
  j = 0;
  while (j <= 4) {
    display.print(intrare_char1[j]);
    display.print(intrare_char1[j+1]);
    display.print(intrare_char1[j+2]);
    display.print(intrare_char1[j+3]);
    delay(pauzamica);
    j++;
  }

  if (humidity < 10) display.print(F(" "));
  display.print(humidity);
  display.print("%o"); 
  delay(pauzamare);
  display.clear();

  String iesire1 = "";
  if (humidity/10 == 0) iesire1 = iesire1 + " ";
  else iesire1 = iesire1 + humidity/10;
  iesire1 = iesire1 + humidity%10;
  iesire1 = iesire1 + "%";
  iesire1 = iesire1 + "o";
  iesire1 = iesire1 + "    "; 
  char iesire_char1[9];
  iesire1.toCharArray(iesire_char1,iesire1.length()+1); 

  j = 0;
  while (j <= 4) {
    display.print(iesire_char1[j]);
    display.print(iesire_char1[j+1]);
    display.print(iesire_char1[j+2]);
    display.print(iesire_char1[j+3]);
    delay(pauzamica);
    j++;
  }
  display.clear(); 

  n++;
  if (n > 4) n=0;
  i++;
  if (i>6) i=0;
}  // end main loop

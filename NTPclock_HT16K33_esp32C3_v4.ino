.
/*******************************************************************************
// Adaptat pentru ESP32-C3 Mini (Compatibil cu Core 2.0.5)
// DST Calculat Manual
// Adaugat: Cod Vreme WMO tradus in Romana/Engleza
 *******************************************************************************/

#include <WiFi.h>                 
#include <WiFiManager.h>          
#include <Wire.h>
#include <HTTPClient.h>           
#include <time.h>                 
#include "NoiascaHt16k33.h"      
#include <math.h>                 

Noiasca_ht16k33_hw_14_ext display = Noiasca_ht16k33_hw_14_ext(); 

// Am adăugat "weather_code" la finalul link-ului
const char* openMeteoUrl = "http://api.open-meteo.com/v1/forecast?latitude=44.3191&longitude=23.8001&current=temperature_2m,relative_humidity_2m,weather_code&timezone=Europe/Bucharest";
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
int weatherCode = 0; // Variabilă nouă pentru codul vremii

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

// Am mărit array-ul de la 5 la 6 pentru a include și "Vremea"
String informatii[6][2] = {
  "    Clock    ", "    Ora    ",   
  "    Date    ", "    Data    ",   
  "    Year    ", "    An    ",     
  "    Temperature    ", "    Temperatura    ",     
  "    Humidity    ", "    Umiditate    ",
  "    Weather    ", "    Vremea    "   // Index 5 - Nou
  };
byte b = 0; 
String info1;
char info2[20];

const String intro = {"    NTP clock with Meteo API v-9 AutoDST by niq_ro     "};

// Funcție nouă: Traducere coduri WMO (Open-Meteo) în text (fără diacritice pentru afișaj)
String getWeatherText(int code, byte lang) {
  if (lang % 2 == 0) { // Engleză
    switch(code) {
      case 0: return "  Clear Sky  ";
      case 1: return " Mainly Clear ";
      case 2: return "Partly Cloudy";
      case 3: return "   Overcast  ";
      case 45: case 48: return "     Fog     ";
      case 51: case 53: case 55: return "   Drizzle   ";
      case 56: case 57: return "Frz. Drizzle ";
      case 61: case 63: case 65: return "    Rain     ";
      case 66: case 67: return " Frz. Rain   ";
      case 71: case 73: case 75: return "    Snow     ";
      case 77: return " Snow Grains ";
      case 80: case 81: case 82: return "Rain Showers ";
      case 85: case 86: return "Snow Showers ";
      case 95: return " Thunderstorm ";
      case 96: case 99: return "T-Storm&Hail ";
      default: return "   Unknown   ";
    }
  } else { // Română
    switch(code) {
      case 0: return "  Cer Senin  ";
      case 1: return "Aproape Senin";
      case 2: return "Partial Noros";
      case 3: return " Total Noros ";
      case 45: case 48: return "    Ceata    ";
      case 51: case 53: case 55: return "   Burnita   ";
      case 56: case 57: return "Burnita ingh.";
      case 61: case 63: case 65: return "   Ploaie    ";
      case 66: case 67: return "Ploaie Ingh. ";
      case 71: case 73: case 75: return "   Zapada    ";
      case 77: return "Mazariche";
      case 80: case 81: case 82: return "Averse Ploaie";
      case 85: case 86: return "Averse Zapada";
      case 95: return "   Furtuna   ";
      case 96: case 99: return "Furtuna+Grind.";
      default: return "  Necunoscut  ";
    }
  }
}

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
      
      int currentSection = payload.indexOf("\"current\":{");
      
      if (currentSection != -1) {
        int tempIndex = payload.indexOf("\"temperature_2m\":", currentSection);
        if (tempIndex != -1) {
          int valStart = payload.indexOf(':', tempIndex) + 1;
          int valEnd = payload.indexOf(',', valStart);
          String tempStr = payload.substring(valStart, valEnd);
          temperature = (int)round(tempStr.toFloat());
        }

        int humIndex = payload.indexOf("\"relative_humidity_2m\":", currentSection);
        if (humIndex != -1) {
          int valStart = payload.indexOf(':', humIndex) + 1;
          int valEnd = payload.indexOf('}', valStart);
          String humStr = payload.substring(valStart, valEnd);
          humidity = (int)round(humStr.toFloat());
        }

        // Extragere cod vreme nou
        int wcIndex = payload.indexOf("\"weather_code\":", currentSection);
        if (wcIndex != -1) {
          int valStart = payload.indexOf(':', wcIndex) + 1;
          int valEnd = payload.indexOf(',', valStart);
          if (valEnd == -1) valEnd = payload.indexOf('}', valStart);
          String wcStr = payload.substring(valStart, valEnd);
          weatherCode = wcStr.toInt(); // Folosim toInt() pentru că e un număr întreg
        }
      }

      Serial.println("Meteo: Temp=" + String(temperature) + "C, Hum=" + String(humidity) + "%, Code=" + String(weatherCode));
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
    display.print(info2[j]); display.print(info2[j+1]); display.print(info2[j+2]); display.print(info2[j+3]);
    delay(pauzamedie); j++;
  }
  display.clear();

  String intrare = "    ";
  if (ora < 10) intrare = intrare + " "; else intrare = intrare + ora/10;
  intrare = intrare + ora%10 + minut/10 + minut%10;    
  char intrare_char[9];
  intrare.toCharArray(intrare_char,intrare.length()+1); 
  j = 0;
  while (j <= 4) {
    display.print(intrare_char[j]); display.print(intrare_char[j+1]); display.print(intrare_char[j+2]); display.print(intrare_char[j+3]);
    delay(pauzamica); j++;
  }
  display.clear();

  int s = 0;
  while (s < 20) {
    if (ora < 10) display.print(F(" "));
    display.print(ora);
    if (millis()/1000%2 == 0) display.print(F("."));
    if (minut < 10) display.print(F("0"));
    display.print(minut);  
    delay(500); s++;
  }
  display.clear();
  
  String iesire = "";
  if (ora < 10) iesire = iesire + " "; else iesire = iesire + ora/10;
  iesire = iesire + ora%10 + minut/10 + minut%10 + "    "; 
  char iesire_char[9];
  iesire.toCharArray(iesire_char,iesire.length()+1); 
  j = 0;
  while (j <= 4) {
    display.print(iesire_char[j]); display.print(iesire_char[j+1]); display.print(iesire_char[j+2]); display.print(iesire_char[j+3]);
    delay(pauzamica); j++;
  }
  display.clear();

  // --- WEEKDAY ---
  String weekDay = weekDays[zi][i%2];
  char numezi[20];
  weekDay.toCharArray(numezi,weekDay.length()); 
  j = 0;
  while (j <= weekDay.length()-4) {
    display.print(numezi[j]); display.print(numezi[j+1]); display.print(numezi[j+2]); display.print(numezi[j+3]);
    delay(pauzamedie); j++;
  }
  display.clear();

  // --- DATE ---
  b = 1; 
  info1 = informatii[b][i%2];
  info1.toCharArray(info2,info1.length()); 
  display.clear();   
  j = 0;
  while (j <= info1.length()-4) {
    display.print(info2[j]); display.print(info2[j+1]); display.print(info2[j+2]); display.print(info2[j+3]);
    delay(pauzamedie); j++;
  }
  display.clear();

  int day = timeinfo.tm_mday;
  int month = timeinfo.tm_mon + 1; 
  int year = timeinfo.tm_year + 1900; 

  String intrare3 = "    " + String(day/10) + String(day%10) + String(month/10) + String(month%10);    
  char intrare_char3[9];
  intrare3.toCharArray(intrare_char3,intrare3.length()+1); 
  j = 0;
  while (j <= 4) {
    display.print(intrare_char3[j]); display.print(intrare_char3[j+1]); display.print(intrare_char3[j+2]); display.print(intrare_char3[j+3]);
    delay(pauzamica); j++;
  }
  display.clear();

  if (day < 10) display.print(F("0"));
  display.print(day); display.print(F("."));
  if (month < 10) display.print(F("0"));
  display.print(month); display.print(F("."));
  delay(pauzamare); display.clear();

  display.print(year); display.print(F("."));
  delay(pauzamare); display.clear();

  String iesire4 = String(year) + "    "; 
  char iesire_char4[9];
  iesire4.toCharArray(iesire_char4,iesire4.length()+1); 
  j = 0;
  while (j <= 4) {
    display.print(iesire_char4[j]); display.print(iesire_char4[j+1]); display.print(iesire_char4[j+2]); display.print(iesire_char4[j+3]);
    delay(pauzamica); j++;
  }
  display.clear(); 

  // --- TEMPERATURE ---
  b = 3;  
  info1 = informatii[b][i%2];
  info1.toCharArray(info2,info1.length()); 
  display.clear();   
  j = 0;
  while (j <= info1.length()-4) {
    display.print(info2[j]); display.print(info2[j+1]); display.print(info2[j+2]); display.print(info2[j+3]);
    delay(pauzamedie); j++;
  }
  display.clear();

  String intrare2 = "    ";
  if (temperature >= 0) {
    temperature2 = temperature;
    if (temperature2/10 == 0) intrare2 = intrare2 + " "; else intrare2 = intrare2 + temperature2/10;
    intrare2 = intrare2 + temperature2%10 + "%" + "C"; 
  } else {
    temperature2 = -temperature;
    intrare2 = intrare2 + "-";
    if (temperature2 < 10) intrare2 = intrare2 + temperature2%10 + "%" + "C";
    else intrare2 = intrare2 + temperature2/10 + temperature2%10 + "C"; 
  }
  char intrare_char2[9];
  intrare2.toCharArray(intrare_char2,intrare2.length()+1); 
  j = 0;
  while (j <= 4) {
    display.print(intrare_char2[j]); display.print(intrare_char2[j+1]); display.print(intrare_char2[j+2]); display.print(intrare_char2[j+3]);
    delay(pauzamica); j++;
  }

  if (temperature >= 0) {
    temperature2 = temperature;
    if (temperature2 < 10) display.print(F(" "));
    display.print(temperature2); display.print("%C"); 
  } else {
    temperature2 = -temperature;
    display.print("-"); display.print(temperature2);
    if (temperature2 < 10) display.print("%C"); else display.print("C");  
  }
  delay(pauzamare); display.clear();

  String iesire2 = "";
  if (temperature >= 0) {
    temperature2 = temperature;
    if (temperature2/10 == 0) iesire2 = iesire2 + " "; else iesire2 = iesire2 + temperature2/10;
    iesire2 = iesire2 + temperature2%10 + "%" + "C"; 
  } else {
    temperature2 = -temperature;
    iesire2 = iesire2 + "-";
    if (temperature2 < 10) iesire2 = iesire2 + temperature2%10 + "%" + "C";
    else iesire2 = iesire2 + temperature2/10 + temperature2%10 + "C"; 
  }
  iesire2 = iesire2 + "    "; 
  char iesire_char2[9];
  iesire2.toCharArray(iesire_char2,iesire2.length()+1); 
  j = 0;
  while (j <= 4) {
    display.print(iesire_char2[j]); display.print(iesire_char2[j+1]); display.print(iesire_char2[j+2]); display.print(iesire_char2[j+3]);
    delay(pauzamica); j++;
  }
  display.clear();

  // --- HUMIDITY ---
  b = 4;  
  info1 = informatii[b][i%2];
  info1.toCharArray(info2,info1.length()); 
  display.clear();   
  j = 0;
  while (j <= info1.length()-4) {
    display.print(info2[j]); display.print(info2[j+1]); display.print(info2[j+2]); display.print(info2[j+3]);
    delay(pauzamedie); j++;
  }
  display.clear();

  if (humidity >= 100) humidity = 99;

  String intrare1 = "    ";
  if (humidity/10 == 0) intrare1 = intrare1 + " "; else intrare1 = intrare1 + humidity/10;
  intrare1 = intrare1 + humidity%10 + "%" + "o";    
  char intrare_char1[9];
  intrare1.toCharArray(intrare_char1,intrare1.length()+1); 
  j = 0;
  while (j <= 4) {
    display.print(intrare_char1[j]); display.print(intrare_char1[j+1]); display.print(intrare_char1[j+2]); display.print(intrare_char1[j+3]);
    delay(pauzamica); j++;
  }

  if (humidity < 10) display.print(F(" "));
  display.print(humidity); display.print("%o"); 
  delay(pauzamare); display.clear();

  String iesire1 = "";
  if (humidity/10 == 0) iesire1 = iesire1 + " "; else iesire1 = iesire1 + humidity/10;
  iesire1 = iesire1 + humidity%10 + "%" + "o" + "    "; 
  char iesire_char1[9];
  iesire1.toCharArray(iesire_char1,iesire1.length()+1); 
  j = 0;
  while (j <= 4) {
    display.print(iesire_char1[j]); display.print(iesire_char1[j+1]); display.print(iesire_char1[j+2]); display.print(iesire_char1[j+3]);
    delay(pauzamica); j++;
  }
  display.clear(); 

  // --- WEATHER (NOU) ---
  b = 5;  
  info1 = informatii[b][i%2];
  info1.toCharArray(info2,info1.length()); 
  display.clear();   
  j = 0;
  while (j <= info1.length()-4) {
    display.print(info2[j]); display.print(info2[j+1]); display.print(info2[j+2]); display.print(info2[j+3]);
    delay(pauzamedie); j++;
  }
  display.clear();

  // Obținem textul vremii tradus
  String weatherText = "  " + getWeatherText(weatherCode, i%2) + "  "; // Spații pentru scroll lin
  char weather_char[30];
  weatherText.toCharArray(weather_char, weatherText.length()+1); 

  // Deoarece textul e mai lung de 4 caractere, îl derulăm complet de la stânga la dreapta
  j = 0;
  while (j <= weatherText.length()-4) {
    display.print(weather_char[j]); 
    display.print(weather_char[j+1]); 
    display.print(weather_char[j+2]); 
    display.print(weather_char[j+3]);
    delay(pauzamedie); 
    j++;
  }
  display.clear(); 

  // --- SFARSIT SECȚIUNE WEATHER ---

  n++;
  if (n > 5) n=0; // Modificat de la 4 la 5 deoarece avem 6 secțiuni acum (0-5)
  i++;
  if (i>6) i=0;
}  // end main loop

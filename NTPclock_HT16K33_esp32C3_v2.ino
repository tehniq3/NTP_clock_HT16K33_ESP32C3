// Adaptat pentru ESP32-C3 Mini cu DST AUTOMAT (Romania)
// Original: https://www.hackster.io/alankrantas/esp8266-ntp-clock-on-ssd1306-oled-arduino-ide-35116e
// small changes by Nicu FLORICA (niq_ro)
// v.1 - changed by Z.AI from ESP8266 to ESP32-C3 Mini
// v.2 - removed manual DST

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <time.h>               
#include "NoiascaHt16k33.h"     // include the noiasca HT16K33 library - download from http://werner.rothschopf.net/

Noiasca_ht16k33_hw_14 display = Noiasca_ht16k33_hw_14();   // 14 segment - Present time

// Fusul orar de bază pentru România (fără DST) = UTC+2
const long timezoneOffset = 2 * 60 * 60; 

const char          *ntpServer  = "pool.ntp.org";
const unsigned long updateDelay = 900000;         // update time every 15 min
const unsigned long retryDelay  = 5000;           // retry 5 sec later if failed

const String weekDays1[7] = {
  "    Duminica    ", "    Luni    ", "    Marti    ", 
  "    Miercuri    ", "    Joi    ", "    Vineri    ", "    Sambata    "
};  // lb. romana

const String weekDays0[7] = {
  "    Saturday    ", "    Monday    ", "    Tuesday    ", 
  "    Wednesday    ", "    Thursday   ", "    Friday    ", "    Sunday    "
};  // english

int i = 0;
int j = 4;

unsigned long tpafisare;
unsigned long tpinfo = 60000;

unsigned long lastUpdatedTime = updateDelay * -1;
unsigned int  second_prev = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

bool currentDST = false;
byte a = 0;

// Funcție care calculează automat dacă este Ora de Vară (DST) pentru Europa
bool isDST_UTC(unsigned int day, unsigned int month, unsigned int hour, unsigned int dow) {
  if (month < 3 || month > 10) return false;
  if (month > 3 && month < 10) return true;

  // Calculăm ultima duminică din lună
  int dowOfLastDay = (dow + (31 - day)) % 7;
  int lastSunday = 31 - dowOfLastDay;

  if (month == 3) {
    if (day < lastSunday) return false;
    if (day > lastSunday) return true;
    return (hour >= 1); // Începe la 01:00 UTC în ultima duminică din Mart
  }
  
  if (month == 10) {
    if (day < lastSunday) return true;
    if (day > lastSunday) return false;
    return (hour < 1); // Se termină la 01:00 UTC în ultima duminică din Oct
  }
  
  return false;
}

void setup() {
  Serial.begin(9600);
  Wire.begin();                                  
  Wire.setClock(400000);                         
  display.begin(0x70, 1);                        
  display.setDigits(4);                          
  display.setBrightness(5);                      
  display.clear();
   
  delay(500);
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("WiFi connected!");
  delay(500);

  // Setăm fusul orar de bază (UTC+2). 
  timeClient.setTimeOffset(timezoneOffset);
  timeClient.begin();
}

void loop() {
  // Actualizare NTP
  if (WiFi.status() == WL_CONNECTED && millis() - lastUpdatedTime >= updateDelay) {
    if (timeClient.update()) {
      Serial.println("NTP time updated.");
      lastUpdatedTime = millis();
    } else {
      Serial.println("Failed to update time. Retrying...");
      lastUpdatedTime = millis() - updateDelay + retryDelay;
    }
  } else {
    if (WiFi.status() != WL_CONNECTED) Serial.println("WiFi disconnected!");
  }

  // --- LOGICĂ DST AUTOMAT (Fără getTimeOffset) ---
  time_t localEpoch = timeClient.getEpochTime();
  
  // Obținem timpul UTC scăzând offset-ul curent cunoscut de noi
  long currentOffset = timezoneOffset + (currentDST ? 3600 : 0);
  time_t utcEpoch = localEpoch - currentOffset;
  struct tm * utcTm = gmtime(&utcEpoch);
  
  unsigned int utcDay = utcTm->tm_mday;
  unsigned int utcMonth = utcTm->tm_mon + 1;
  unsigned int utcHour = utcTm->tm_hour;
  unsigned int utcDow = utcTm->tm_wday; // 0 = Duminică

  bool newDST = isDST_UTC(utcDay, utcMonth, utcHour, utcDow);
  
  // Dacă starea DST s-a schimbat, actualizăm offset-ul
  if (newDST != currentDST) {
    currentDST = newDST;
    long newOffset = timezoneOffset + (currentDST ? 3600 : 0);
    timeClient.setTimeOffset(newOffset);
    Serial.println(currentDST ? "-> Trecut la Ora de Vara (DST)" : "-> Trecut la Ora de Iarna");
  }
  // ---------------------------

  unsigned long t = millis();

  // Obținem timpul local pentru afișaj
  unsigned int year = getYear();
  unsigned int month = getMonth();
  unsigned int day = getDate();
  unsigned int hour = timeClient.getHours();
  unsigned int minute = timeClient.getMinutes();
  unsigned int second = timeClient.getSeconds();
  i = timeClient.getDay();

  if (second < second_prev) display.clear();

  // Afișare ceas
  if (hour < 10) display.print(F("0"));
  display.print(hour);
  if (millis()/1000%2 == 0) display.print(F("."));
  if (minute < 10) display.print(F("0"));
  display.print(minute);
  
  second_prev = second;

  int diff = millis() - t;
  delay(diff >= 0 ? (500 - (millis() - t)) : 0);

  // Afișare informații rulate (Zi / Dată / An)
  if (millis() - tpafisare > tpinfo) {
    display.clear();
    if (a % 2 == 0) {  
      String weekDay0 = weekDays0[i];
      char numezi0[20];
      weekDay0.toCharArray(numezi0, weekDay0.length()); 
      j = 0;
      while (j <= weekDay0.length() - 4) {
        display.print(numezi0[j]);
        display.print(numezi0[j+1]);
        display.print(numezi0[j+2]);
        display.print(numezi0[j+3]);
        delay(500);
        j++;
      }
    } else {
      String weekDay1 = weekDays1[i];
      char numezi1[20];
      weekDay1.toCharArray(numezi1, weekDay1.length()); 
      j = 0;
      while (j <= weekDay1.length() - 4) {
        display.print(numezi1[j]);
        display.print(numezi1[j+1]);
        display.print(numezi1[j+2]);
        display.print(numezi1[j+3]);
        delay(500);
        j++;
      }
    }
    
    // data
    display.clear();
    if (day < 10) display.print(F("0"));
    display.print(day);
    display.print(F("."));
    if (month < 10) display.print(F("0"));
    display.print(month); 
    display.print(F("."));
    delay(3000);
    
    // anul
    display.clear();
    display.print(year);
    display.print(F("."));
    delay(3000);
    
    tpafisare = millis();
    a++;
  }

  delay(10);
} 

// Funcții ajutătoare pentru extragerea datei din Epoch Time
unsigned int getYear() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  return ti->tm_year + 1900;
}

unsigned int getMonth() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  return ti->tm_mon + 1;
}

unsigned int getDate() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  return ti->tm_mday;
}

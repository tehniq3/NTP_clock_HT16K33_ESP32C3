// Adaptat pentru ESP32-C3 Mini
// =============================================================== 
// Original with ESP8266: https://www.hackster.io/alankrantas/esp8266-ntp-clock-on-ssd1306-oled-arduino-ide-35116e
// small changes by Nicu FLORICA (niq_ro)
// v.0 - changed date mode (day.mounth not month/day)
// v.1 - added hardware switch for DST (Daylight Saving Time)
// v.2 - added WiFiManager library 
// HT16K33_v.1 - changed to HT16K33 4-digit 14-segment display
// v.2 - added month, year and scrolling name day name
// ===============================================================
// v.1 for ESP32-C3 Mini (changed by Z.AI)
// 

#include <WiFi.h>               // Biblioteca WiFi standard pentru ESP32 (inlocuieste ESP8266WiFi.h)
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>        // https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <time.h>               // Necesar pentru ESP32 pentru a folosi localtime(), tm_year, etc.
#include "NoiascaHt16k33.h"     // include the noiasca HT16K33 library - download from http://werner.rothschopf.net/

Noiasca_ht16k33_hw_14 display = Noiasca_ht16k33_hw_14();   // 14 segment - Present time

// *** ATENTIE: GPIO14 nu exista pe ESP32-C3! ***
// Schimba in pinul la care ai cablat switch-ul (ex: GPIO0, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, etc.)
#define DSTpin 4 

const long timezoneOffset = 2 * 60 * 60; // ? hours * 60 * 60

const char          *ntpServer  = "pool.ntp.org"; // change it to local NTP server if needed
const unsigned long updateDelay = 900000;         // update time every 15 min
const unsigned long retryDelay  = 5000;           // retry 5 sec later if time query failed
const String   weekDays1[7] = {
  "    Duminica    ",
  "    Luni    ",
  "    Marti    ",
  "    Miercuri    ",
  "    Joi    ",
  "    Vineri    ",
  "    Sambata    "
  };  // lb. romana

const String   weekDays0[7] = {
  "    Saturday    ",
  "    Monday    ",
  "    Tuesday    ",
  "    Wednesday    ",
  "    Thursday   ",
  "    Friday    ",
  "    Sunday    "
  };  // english

int i = 0;
int j =4;

unsigned long tpafisare;
unsigned long tpinfo = 60000;  // 15000 for test in video

unsigned long lastUpdatedTime = updateDelay * -1;
unsigned int  second_prev = 0;
bool          colon_switch = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer);

byte DST = 0;
byte DST0;
bool updated;
byte a = 0;


void setup() {
  pinMode (DSTpin, INPUT);
  
  Serial.begin(9600);
  Wire.begin();                                  // start the I2C interface
  Wire.setClock(400000);                         // optional: activate I2C fast mode. If it is to fast for other I2C devices. deactivate this row.
  display.begin(0x70, 1);                        // I2C adress of the first display
  display.setDigits(4);                          // if your modules are not using all 8 digits, reduce the used digits
  display.setBrightness(5);                      // set brightness from 0 to 15
  display.clear();
   
  delay(500);
     //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    wifiManager.autoConnect("AutoConnectAP");
      
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    delay(500);

  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;
  timeClient.setTimeOffset(timezoneOffset + DST*3600);
  timeClient.begin();
  DST0 = DST;
}

void loop() {
  if (digitalRead(DSTpin) == LOW)
   DST = 0;
  else
   DST = 1;

  if (WiFi.status() == WL_CONNECTED && millis() - lastUpdatedTime >= updateDelay) {
    updated = timeClient.update();
    if (updated) {
      Serial.println("NTP time updated.");
      lastUpdatedTime = millis();
    } else {
      Serial.println("Failed to update time. Waiting for retry...");
      lastUpdatedTime = millis() - updateDelay + retryDelay;
    }
  } else {
    if (WiFi.status() != WL_CONNECTED) Serial.println("WiFi disconnected!");
  }

  unsigned long t = millis();

  unsigned int year = getYear();
  unsigned int month = getMonth();
  unsigned int day = getDate();
  unsigned int hour = timeClient.getHours();
  unsigned int minute = timeClient.getMinutes();
  unsigned int second = timeClient.getSeconds();
  i = timeClient.getDay();

  if (second < second_prev) display.clear();

  // display clock
  if (hour < 10) display.print(F("0"));
  display.print(hour);
  if (millis()/1000%2 == 0)
  display.print(F("."));
  if (minute < 10) display.print(F("0"));
  display.print(minute);
  
  second_prev = second;

  int diff = millis() - t;
  delay(diff >= 0 ? (500 - (millis() - t)) : 0);

  if (millis() - tpafisare > tpinfo)
  {
    display.clear();
    if (a%2 == 0)
    {  
      String weekDay0 = weekDays0[i];
      char numezi0[20];
      weekDay0.toCharArray(numezi0, weekDay0.length()); 
      Serial.println (numezi0);

      j = 0;
      while (j <= weekDay0.length()-4)
      {
        display.print(numezi0[j]);
        display.print(numezi0[j+1]);
        display.print(numezi0[j+2]);
        display.print(numezi0[j+3]);
        delay(500);
        j++;
      }
    }
    else
    {
      String weekDay1 = weekDays1[i];
      char numezi1[20];
      weekDay1.toCharArray(numezi1, weekDay1.length()); 
      Serial.println (numezi1);
      
      j = 0;
      while (j <= weekDay1.length()-4)
      {
        display.print(numezi1[j]);
        display.print(numezi1[j+1]);
        display.print(numezi1[j+2]);
        display.print(numezi1[j+3]);
        delay(500);
        j++;
      }
    }
    
    // date
    display.clear();
    if (day < 10) display.print(F("0"));
    display.print(day);
    display.print(F("."));
    if (month < 10) display.print(F("0"));
    display.print(month); 
    display.print(F("."));
    delay(3000);
    
    // year
    display.clear();
    display.print(year);
    display.print(F("."));
    delay(3000);
    
    tpafisare = millis();
    a++;
  }

  if (DST != DST0)
  {
    timeClient.setTimeOffset(timezoneOffset + DST*3600);
    timeClient.begin();
    updated = timeClient.update();
    if (updated) {
        Serial.println("NTP time updated.");
        lastUpdatedTime = millis();
      } else {
        Serial.println("Failed to update time. Waiting for retry...");
        lastUpdatedTime = millis() - updateDelay + retryDelay;
      }
    DST0 = DST;
  }

  delay(10);
} // end main loop

unsigned int getYear() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int year = ti->tm_year + 1900;
  return year;
}

unsigned int getMonth() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int month = ti->tm_mon + 1;
  return month;
}

unsigned int getDate() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int month = ti->tm_mday;
  return month;
}

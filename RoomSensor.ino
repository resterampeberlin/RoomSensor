//!
//! @file RoomSensor.ino
//! @author Markus Nickels
//! @version 1.0.0
//!

// This file is part of the Application "RoomSensor".
//
// RoomSensor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RoomSensor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RoomSensor.  If not, see <http://www.gnu.org/licenses/>.
 
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>   

#include "Confidential.h"

#define USE_DISPLAY                      //!< use OLED display as output

#ifdef USE_DISPLAY
  #include <SPI.h>
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>

//!
//! @name OLED wiring
//!
//!@{
  #define OLED_MOSI  13                   //!< data out on GPIO13   
  #define OLED_CLK   14                   //!< clock out on GPIO14    
  #define OLED_DC    4                    //!< data/command out on GPIO13     
  #define OLED_CS    12                   //!< chip select out on GPIO13 
  #define OLED_RESET 5                    //!< reset out on GPIO13 
//!@}

  #if (SSD1306_LCDHEIGHT != 64)
    #error("Height incorrect, please fix Adafruit_SSD1306.h!");
  #endif
  
  Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

  #define PRINTLN(s)    Serial.println(s); display.println(s); display.display()
  #define PRINT(s)      Serial.print(s); display.print(s); display.display()
#else
  #define PRINTLN(s)    Serial.println(s)
  #define PRINT(s)      Serial.print(s)
#endif 

#define SOL_PIN      0                    //!< sign of life LED on GPIO0
#define ONEWIRE_PIN  2                    //!< 1-wire connection pin on GPIO2

//!
//! @name Server specific definitions
//!
//! Defined in Confidential.h 
//!
//!@{
const char* ssid      = CONF_SSID;        //!< WLAN-Name
const char* password  = CONF_PASSWORD;    //!< WLAN Password
const char* host      = CONF_HOST;        //!< MySQL Server address
//!@}

OneWire oneWire(ONEWIRE_PIN);             //!< 1-wire connected to GPIO 2
DallasTemperature sensors(&oneWire);      //!< temperature sensors on 1-Wire

#define OneWireAddressSize 8              //!< A 1-Wire address consists of 8 bytes

//!
//! structure, where the sensor configuration is stored
//!

typedef struct {
  uint8_t sensorID;                       //!< Sensor ID which corresponds to the sensorID in the database
  boolean available;                      //!< Sensor available on bus
  uint8_t address[OneWireAddressSize];    //!< Unique HW-ID of sensor
} sensorInfo_t;

//!
//! configuration of my sensors
//! 
//! This structure has to be carefully pre-initialised with the real
//! HW-ID of the sensors. Look carefully to the output on the serial
//! console to check the configuration
//!

sensorInfo_t sensorInfo[] = {
  1, false, {0x28, 0xFF, 0x2A, 0x33, 0xA4, 0x15, 0x01, 0xE8},
  2, false, {0x28, 0xFF, 0xF4, 0x66, 0xA4, 0x15, 0x04, 0x72},
  3, false, {0x28, 0xFF, 0xFF, 0x54, 0xA4, 0x15, 0x03, 0xCE},
  4, false, {0x28, 0xFF, 0x96, 0x28, 0xA4, 0x15, 0x03, 0x96},
  5, false, {0x28, 0xFF, 0xCC, 0x67, 0xA4, 0x15, 0x04, 0x47} };

//!
//! ID of current capture
//!
//! This value must match the ID in the database, table "location" e.g.
//!
//! id | name
//! -- | --------
//! 1  | living room
//! 2  | kitchen
//!

int const locationId = 1;

//!
//! measurement counter
//!

uint16_t measurement = 0;

//!
//! Setup routine
//!

void setup() {
  //OneWire Sensor start working
  sensors.begin();

  #ifdef USE_DISPLAY
    display.begin(SSD1306_SWITCHCAPVCC);
    display.display();

    delay(2000);
    display.clearDisplay();
    
    display.setCursor(0,0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setTextWrap(true);
    display.setTextAutoScroll(true);
  #endif

  Serial.begin(115200);
  Serial.println("Starting setup");

  delay(10);

#ifdef SOL_PIN  
  // configure sign of life LED
  pinMode(SOL_PIN, OUTPUT); 
  digitalWrite(SOL_PIN, LOW);
#endif 

  // We start by connecting to a WiFi network

  PRINT("Connecting to ");
  PRINTLN(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    PRINT(".");
  }

  PRINTLN("WiFi connected");  
  PRINTLN("IP address: ");
  PRINTLN(WiFi.localIP());

  randomSeed(analogRead(0));

  // show available sensors
  Serial.println("Available Sensors");

  uint8_t sensorCount = 0;

  for (uint8_t i=0; i < sensors.getDeviceCount(); i++) {
    Serial.print(i);
    Serial.print(": ");

    // get address
    uint8_t address[8];

    sensors.getAddress(address, i);

    // print address
    for (int ii=0; ii < OneWireAddressSize; ii++) {
      Serial.print(address[ii], HEX);

      if (ii < OneWireAddressSize-1) {
        Serial.print("-");
      }  
    }

    uint8_t found = 0;

    // search address in predefined array
    for (int sensor=0; sensor < sizeof(sensorInfo)/sizeof(sensorInfo[0]); sensor++) {
      if (!memcmp(address, sensorInfo[sensor].address, sizeof(address))) {
        sensorInfo[sensor].available=1;
        found = sensorInfo[sensor].sensorID;
        sensorCount++;
        break;
      }
    }

    if (found) {
        PRINT("Sensor ");
        PRINT(found);
        PRINTLN(" found");
    } 
    else {
      Serial.println(" not found");
    }
  }

  PRINT(sensorCount);
  PRINTLN(" sensors found");
  PRINTLN("Setup completed");
}

//!
//! Main routine
//!

void loop() {
  float sensorValue = 0.0;

  PRINT("Measurement ");
  PRINTLN(measurement);

#ifdef SOL_PIN  
  // Blink sign of life LED
  digitalWrite(SOL_PIN, HIGH);
#endif 

#ifdef USE_DISPLAY
  display.dim(false);
#endif  

  for (int sensor=0; sensor < sizeof(sensorInfo)/sizeof(sensorInfo[0]); sensor++) {
    if (!sensorInfo[sensor].available) {
        break;
    }
    
    //Get One Wire Data
    sensors.requestTemperatures();
    sensorValue = sensors.getTempC(sensorInfo[sensor].address);

    PRINT("Sensor[");
    PRINT(sensorInfo[sensor].sensorID);
    PRINT("]=");
    PRINTLN(sensorValue);   
 
    // Start connection
    Serial.print("connecting to ");
    Serial.println(host);
    
    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    
    // We now create a URI for the request
    String url = "/Smarthome/addSensorData.php";
    url += "?locationId=";
    url += locationId;
    
    url += "&sensorId=";
    url += sensorInfo[sensor].sensorID;
    
    url += "&value=";
    url += sensorValue;
    
    Serial.print("Requesting URL: ");
    Serial.println(url);
   
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "Connection: close\r\n\r\n");
                 
    int timeout = millis() + 5000;
    while (client.available() == 0) {
      if (timeout - millis() < 0) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }

    delay(1000);
    
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    
    Serial.println();
    Serial.println("closing connection");
  }

  measurement++;

  Serial.println();
  Serial.println("waiting 1 minute");

#ifdef SOL_PIN  
  // Blink sign of life LED
  digitalWrite(SOL_PIN, LOW);
#endif 

#ifdef USE_DISPLAY
  display.dim(true);
  delay(1000 * 5);
  
  // setting the display dark saves 4mA current
  display.clearDisplay();
  display.setCursor(0,0);
  display.display();
  delay(1000 * 55);
#else
  delay(1000 * 60);
#endif  


}

//!
//! @mainpage Store and show sensor data recored from a remote sensor
//!
//! @tableofcontents
//!
//! @section s1 Purpose
//!
//! This arduino application runs on en ESP8266, captures data from the sensors the one wire bus 
//! and sends it to a MySQL server. This data can be used to be displayed in nice graphs
//! A typical application will be to capture temperature data in a room
//!
//! @section s2 Compatibility
//!
//! This is program has been tested with
//! - ESP8266
//! - Maxim DS18B20 digital temperature sensor
//! - (optional) Adafruit SSD1306 OLED display
//!
//! @section s3 Installation
//!
//! @subsection s3_1 Sensor
//!
//! -# Refer to sketch RoomSensor.sch for wiring (wire GPIO2 to 1-wire bus with 4,7K pullup)
//! -# Create file confidential.h in which you define host, WLAN name and password 
//!    (see ::ssid, ::password, ::host)
//! -# Define #locationId in Roomsensor.ino
//! -# Compile and load to ESP8266
//! -# Connect the sensor one by one (after a reset) and remember unique HW-Id using the 
//!    serial monitor
//! -# Modify #sensorInfo with the remembered values
//! -# Compile and load again. Now all sensor should be marked as "found"
//!
//! @subsubsection Inst1 Optional usage of "sign of life" LED
//!
//! -# Uncomment #SOL_PIN in source code
//! -# Connect a LED with a 330 resistor between GPIO0 and GND
//! -# for any other wiring change #SOL_PIN in source code
//! 
//! @subsubsection Inst2 Optional usage of OLED display
//!
//! If you want to use an OLED display as status display should should do the following:
//! -# Use an Adafruit HUZZAH ESP8266 board or similar (you need various GPIO pins)
//! -# Use an Adafruit SSD1306 display
//! -# use my forks of 
//!    <a href="https://github.com/resterampeberlin/Adafruit-GFX-Library">Adafruit_GFX.h</a> and 
//!    <a href="https://github.com/resterampeberlin/Adafruit_SSD1306">Adafruit_SSD1306.h</a>. 
//!    This is necessary to use the "AutoScroll" feature
//! -# Uncomment #USE_DISPLAY in source code
//! -# Connect the display to the MCU as follows 
//!    (for any other wiring change #OLED_MOSI, #OLED_CLK, #OLED_DC, #OLED_CS, #OLED_RESET in source code)
//!    display pin | MCU Pin
//!    ----------- | --------
//!    Data        | GPIO13
//!    CLK         | GPIO14
//!    DC          | GPIO4
//!    CS          | GPIO12
//!    RST         | GPIO5
//!    GND         | GND
//!    3v3         | 3,3V
//!    Vin         | not connected
//!
//! @subsection s3_2 Server
//!
//! -# Install MySQL/MariaDb and PHP on server
//! -# Install jpgraph library on server (see http://jpgraph.net)
//! -# Create datebase Smarthome (See SmartHome.sql)
//! -# modify tables sensor and location
//! -# Install server scripts 
//! -# Create file Confidential.php with user name and password for mysql server 
//!    (see Smarthome.php)
//!
//! @section vh Version History
//!
//! - Version 1.0.0
//!
//! @section s4 Known bugs
//!
//! - Last measurement displayed incorrectly (it is not the last measurement)
//! - no indication when selected date range does not contain any data
//!
//! @section s5 To Do
//!
//! - selection of location in index.php
//!
//! @section s6 Credits
//!
//! Currently nobody
//!


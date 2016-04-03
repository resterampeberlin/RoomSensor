//!
//! @file RoomSensor.ino
//! @author Markus Nickels
//! @version 1.0.0
//! @copyright GNU General Public License V3
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
//#define USE_SOL                          //!< use SOL led to show activity
#define USE_DS2438                       //!< use OLED display as output

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
  #define OLED_DC    12                    //!< data/command out on GPIO13     
  #define OLED_CS    15                   //!< chip select out on GPIO13 
  #define OLED_RESET 0                    //!< reset out on GPIO13 
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

#ifdef USE_DS2438
  #include "DS2438.h"
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

DallasTemperature ds18(&oneWire);         //!< temperature sensors on 1-Wire

#ifdef USE_DS2438
DS2438 ds2438(&oneWire);
#endif

#define OneWireAddressSize 8              //!< A 1-Wire address consists of 8 bytes

//!
//! structure, where the sensor configuration is stored
//!

typedef struct {
  boolean available;                      //!< Sensor available on bus
  uint8_t address[OneWireAddressSize];    //!< Unique HW-ID of sensor
} deviceInfo_t;

//!
//! configuration of my devices on the bus
//! 
//! This structure has to be carefully pre-initialised with the real
//! HW-ID of the devices. Look carefully to the output on the serial
//! console to check the configuration
//!

#define DS18B20 0x28
#define DS2438  0x26

deviceInfo_t deviceInfo[] = {
  false, {DS18B20, 0xFF, 0x2A, 0x33, 0xA4, 0x15, 0x01, 0xE8},
  false, {DS18B20, 0xFF, 0xF4, 0x66, 0xA4, 0x15, 0x04, 0x72},
  false, {DS18B20, 0xFF, 0xFF, 0x54, 0xA4, 0x15, 0x03, 0xCE},
  false, {DS18B20, 0xFF, 0x96, 0x28, 0xA4, 0x15, 0x03, 0x96},
  false, {DS18B20, 0xFF, 0xCC, 0x67, 0xA4, 0x15, 0x04, 0x47},
  false, {DS2438,  0x62, 0x7F, 0xB7, 0x01, 0x00, 0x00, 0x72} };

//!
//! sensor types as defined in database, table sensor_type
//!

typedef enum {
  temperature = 1,
  light       = 2,
  voltage     = 6
} sensorType_t;

//!
//! structure, where the mapping from devices to sensors is stored
//!
//! This mapping takes into account that many sensors could be connected to one 
//! device on the one wire bus, e.g. a 4 channel A/D converter
//!

typedef struct {
  uint8_t sensorId;                       //!< Sensor ID which corresponds to the sensorID in the database
  uint8_t device;                         //!< Index in array sensorInfo
  sensorType_t sensorType;                //!< Type of sensor
} sensorMapping_t;

//!
//! mapping of my sensors to devices
//! 

sensorMapping_t sensorMapping[] = {
  1, 0, temperature,
  2, 1, temperature,
  3, 2, temperature,
  4, 3, temperature,
  5, 4, temperature,
  6, 5, light,
  7, 5, voltage };

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
  ds18.begin();

#ifdef USE_DS2438
  ds2438.begin();
#endif

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

#ifdef USE_SOL  
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

  uint8_t deviceCount = 0;

  // get address
  uint8_t address[8];

  oneWire.reset_search();

  while (oneWire.search(address))   {
    Serial.print(deviceCount);
    Serial.print(": ");

    // print address
    for (int ii=0; ii < OneWireAddressSize; ii++) {
      Serial.print(address[ii], HEX);

      if (ii < OneWireAddressSize-1) {
        Serial.print("-");
      }  
    }

    int8_t found = -1;

    // search address in predefined array
    for (int i=0; i < sizeof(deviceInfo)/sizeof(deviceInfo[0]); i++) {
      if (!memcmp(address, deviceInfo[i].address, sizeof(address))) {
        deviceInfo[i].available=1;
        found = i;
        deviceCount++;
        break;
      }
    }

    if (found >= 0) {
        PRINT(" Device ");
        PRINT(found);
        PRINTLN(" found");
    } 
    else {
      Serial.println(" not found");
    }
  }

  PRINT(deviceCount);
  PRINTLN(" devices found");
  PRINTLN("Setup completed");
}

//!
//! read temperature
//!
//! @param aDevice  device number in array deviceInfo
//!

float readTemperature(uint8_t aDevice) {
  //Get One Wire Data
  ds18.requestTemperatures();
  return ds18.getTempC(deviceInfo[aDevice].address);
}

//!
//! read voltage
//!
//! The voltage is taken from chane A 
//!
//! @param aDevice  device number in array deviceInfo
//!

float readVoltage(uint8_t aDevice) {
  //Get One Wire Data
  ds2438.update(deviceInfo[aDevice].address);
  
  if (!ds2438.isError()) {
    return ds2438.getVoltage(DS2438_CHB);
  } 
  else {
    return 0.0;
  }
}

//!
//! read light
//!
//! The light intensity is taken from chanel B. 
//!
//! @param aDevice  device number in array deviceInfo
//!

float readLight(uint8_t aDevice) {
  //Get One Wire Data
  ds2438.update(deviceInfo[aDevice].address);

  if (!ds2438.isError()) {
    return ds2438.getVoltage(DS2438_CHA) / 3.3;
  } 
  else {
    return 0.0;
  }
}

//!
//! check if sensor is available
//!
//! @param aSensor  sensor number
//! @returns availabilty of sensor
//!

boolean sensorAvailable(uint8_t aSensor) {
  return deviceInfo[sensorMapping[aSensor].device].available;
}

//!
//! transmit sensor value to server
//!
//! @param aSensorId    sensor ID
//! @param aSensorValue the value which will be store in the database
//!

void sendSensorValue(uint8_t aSensorId, float aSensorValue) {

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
  url += aSensorId;
  
  url += "&value=";
  url += aSensorValue;
  
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

//!
//! Main routine
//!

void loop() {
  float sensorValue = 0.0;

  PRINT("Measurement ");
  PRINTLN(measurement);

#ifdef USE_SOL  
  // Blink sign of life LED
  digitalWrite(SOL_PIN, HIGH);
#endif 

#ifdef USE_DISPLAY
  display.dim(false);
#endif  

  for (int sensor=0; sensor < sizeof(sensorMapping)/sizeof(sensorMapping[0]); sensor++) {
    if (!sensorAvailable(sensor)) {
        break;
    }

    switch(sensorMapping[sensor].sensorType) {
      case temperature:
        sensorValue = readTemperature(sensorMapping[sensor].device);
        break;
        
      case voltage:
        sensorValue = readVoltage(sensorMapping[sensor].device);
        break;
        
      case light:
        sensorValue = readLight(sensorMapping[sensor].device);
        break;
    }

    PRINT("Sensor[");
    PRINT(sensorMapping[sensor].sensorId);
    PRINT("]=");
    PRINTLN(sensorValue);   

    sendSensorValue(sensorMapping[sensor].sensorId, sensorValue);
  }

  measurement++;

  Serial.println();
  Serial.println("waiting 1 minute");

#ifdef USE_SOL  
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
//! @section s3 Software Installation
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
//! -# Uncomment #USE_SOL in source code
//! -# Connect a LED with a 330 resistor as defined in @ref pinout. 
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
//! -# Connect the display to the MCU as defined in @ref pinout. 
//!    For any other wiring change #OLED_MOSI, #OLED_CLK, #OLED_DC, #OLED_CS, #OLED_RESET in source code)
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
//! @section pinout Hardware Installation / Pinout
//!
//!  | MCU Pin       | Direction | 1-Wire | Display (optional) | Sign of life LED (option) |
//!  | ------------- | --------- | ------ | ------------------ | ------------------------- |
//!  | GPIO0         | Out       |        |                    | LED In                    |
//!  | GPIO2         | In/Out    | Data   |                    |                           |
//!  | GPIO4         | Out       |        | DC                 |                           |
//!  | GPIO5         | Out       |        | RST                |                           |
//!  | GPIO12        | Out       |        | CS                 |                           |
//!  | GPIO13        | Out       |        | Data               |                           |
//!  | GPIO14        | Out       |        | Clk                |                           |
//!  | GPIO15        |           |        |                    |                           |
//!  | GPIO16        |           |        |                    |                           |
//!  | GND           |           | GND    | GND                | GND                       |
//!  | 3,3V          |           | VCC    |                    |                           |
//!  | not connected |           |        | Vin                |                           |
//!
//! @section vh Version History
//!
//! - Version 1.0.0
//!   Initial stable version
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


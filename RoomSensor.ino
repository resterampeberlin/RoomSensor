/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */
 
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>   

//#define SOL_PIN       2       // sign of life

const char* ssid     = "OpenWrt";
const char* password = "Kas!mir3efv";

const char* host = "192.168.0.4";

//OneWire Setup
OneWire oneWire(2);
DallasTemperature sensors(&oneWire);

#define OneWireAddressSize 8 

typedef struct {
  uint8_t sensorID;
  boolean available;
  uint8_t address[OneWireAddressSize];
} sensorInfo_t;

sensorInfo_t sensorInfo[] = {
  1, false, {0x28, 0xFF, 0x2A, 0x33, 0xA4, 0x15, 0x01, 0xE8},
  2, false, {0x28, 0xFF, 0xF4, 0x66, 0xA4, 0x15, 0x04, 0x72},
  3, false, {0x28, 0xFF, 0xFF, 0x54, 0xA4, 0x15, 0x03, 0xCE},
  4, false, {0x28, 0xFF, 0x96, 0x28, 0xA4, 0x15, 0x03, 0x96},
  5, false, {0x28, 0xFF, 0xCC, 0x67, 0xA4, 0x15, 0x04, 0x47} };

int numSensors = 0;

void setup() {
  //OneWire Sensor start working
  sensors.begin();

  Serial.begin(115200);
  Serial.println("Starting setup");

  delay(10);

  // configure I/O
  //pinMode(SOL_PIN, OUTPUT); 
  // digitalWrite(SOL_PIN, LOW);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  randomSeed(analogRead(0));

  // show available sensors
  Serial.println("Available Sensors");

  for (int i=0; i < sensors.getDeviceCount(); i++) {
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
        break;
      }
    }

    if (found) {
        Serial.print(" found as SensorID=");
        Serial.println(found);
    } 
    else {
      Serial.println(" not found");
    }
  }

  Serial.println("Setup completed");
  Serial.println("");
}

int const location = 1;

void loop() {
  float sensorValue = 0.0;
  
  for (int sensor=0; sensor < sizeof(sensorInfo)/sizeof(sensorInfo[0]); sensor++) {
    if (!sensorInfo[sensor].available) {
        break;
    }
    
    //Get One Wire Data
    sensors.requestTemperatures();
    sensorValue = sensors.getTempC(sensorInfo[sensor].address);

    Serial.print("TempSensor[");
    Serial.print(sensor);
    Serial.print("]=");
    Serial.println(sensorValue);
    
    // blink sign of life LED
    //digitalWrite(SOL_PIN, HIGH);

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
    String url = "/Smarthome/add_sensor_data.php";
    url += "?location=";
    url += location;
    
    url += "&sensor=";
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
      
      // blink sign of life LED
      //digitalWrite(SOL_PIN, LOW);
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

  Serial.println();
  Serial.println("waiting 1 minute");

  delay(1000 * 60);
}


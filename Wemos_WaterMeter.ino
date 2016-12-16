/*
Reads water meter rotations using a HMC5883L sensor
Reads temperature using DS18B20
Sends data to Domoticz controller

Greg McCarthy
greg@gjmccarthy.co.uk

*/

#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include "WifiClient.h"
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "XXXXXXXXX";
const char* password = "XXXXXXXXXXXX";
const char http_site[] = "XXXXXXXX";
const int http_port = 8080;

int rotation = 0;
int usage = 0;
boolean Trigger = false;

Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

WiFiClient client;

void setup() {
  Serial.begin(9600);

  sensors.begin();
  
  //Setup WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  if (!mag.begin())
  {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while (1);
  } else {
    Serial.println("HMC connected");
  }

  Serial.println("Setup complete");
}

void loop() {

  const unsigned long fiveMinutes = 5 * 60 * 1000UL;
  static unsigned long lastSampleTime = 0 - fiveMinutes;  

  /* Get a new sensor event */
  sensors_event_t event;
  mag.getEvent(&event);
  float z = event.magnetic.z;


  if ((z > 40) and (Trigger == false)) {
    //crossed 40 mark
    rotation++;
    Trigger = true;
  }

  if ((z < 0) and (Trigger == true)) {
    //crossed 0 mark
    Trigger = false;
  }

  unsigned long now = millis();

  // Every 5 mins send data to controller.home
  if (now - lastSampleTime >= fiveMinutes)
  {
    lastSampleTime += fiveMinutes;

    sensors.requestTemperatures(); // Send the command to get temperatures
    // We use the function ByIndex, and as an example get the temperature from the first sensor only.
    Serial.print("Temperature for the device 1 (index 0) is: ");
    float temp = (sensors.getTempCByIndex(0)); 
    //Serial.println(temp);

    // Work out water usage per minute
    usage = rotation; // 5;
    rotation = 0;   // Reset rotation value
   
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
      client.connect(http_site, http_port);
      Serial.println("Send data to controller");

      //Send water usage
      String url = "/json.htm?type=command&param=udevice&idx=33&nvalue=0&svalue=";
      client.print(String("GET ") + url + usage + " HTTP/1.1\r\n" +
               "Host: " + http_site + "\r\n" + 
               "Connection: close\r\n\r\n");

      //Send temp
      url = "/json.htm?type=command&param=udevice&idx=31&nvalue=0&svalue=";
      client.connect(http_site, http_port);
      client.print(String("GET ") + url + temp + " HTTP/1.1\r\n" +
               "Host: " + http_site + "\r\n" + 
               "Connection: close\r\n\r\n");
      } else {
        Serial.println("Error in WiFi connection");
      }
    }

  delay(250);

}


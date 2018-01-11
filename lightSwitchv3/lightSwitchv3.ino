#include <Wire.h>
#include "Adafruit_MCP23017.h"
#include "Adafruit_MPR121.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

const char* ssid = "xxx";
const char* password = "xxx";
const char* switchName = "manualswitch";

#define AIO_SERVER      "192.168.1.2"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "xxx"
#define AIO_KEY         "xxx"

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe statusfeed = Adafruit_MQTT_Subscribe(&mqtt, "manualswitch");
Adafruit_MQTT_Publish statussend = Adafruit_MQTT_Publish(&mqtt, "manualswitch");


Adafruit_MCP23017 mcp;
Adafruit_MPR121 cap = Adafruit_MPR121();


char currentState[10];

void loadingLEDs();
void processState();
void cls();

uint16_t lasttouched = 0;
uint16_t currtouched = 0;
int initialised = 0;
  
void setup() {  
  Wire.begin(D3,D4);
  mcp.begin();      // use default address 0  
  for (int i=0; i<16; i++) {
    mcp.pinMode(i, OUTPUT);
  }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");  


  Serial.begin(115200);
  WiFi.hostname(switchName);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { loadingLEDs(); }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  statusfeed.setCallback(statuscallback);

  
  // Setup MQTT subscription for time feed.
  mqtt.subscribe(&statusfeed);

}

void statuscallback(char *data, uint16_t len) {

  String dataString = String(data);

  if (dataString.length() == 8) {
    int leds = 0;
    for (int i=0; i<=dataString.length(); i++) {
      currentState[i]=dataString.charAt(i);
      if (dataString.charAt(i) == '0') {
        mcp.digitalWrite(leds, HIGH);
        mcp.digitalWrite(leds+1, LOW);
      } else if (dataString.charAt(i) == '1') {
        mcp.digitalWrite(leds, LOW);
        mcp.digitalWrite(leds+1, HIGH);
      }
      leds+=2;
    }
    initialised = 1;
  }
}


void loop() {

  MQTT_connect();
  mqtt.processPackets(5);
    
 // Get the currently touched pads
  currtouched = cap.touched();
  
  for (int i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      if (currentState[i] == '0') {
        String toSend = String(i) + ":on";
        char toSendChar[10];
        toSend.toCharArray(toSendChar,10);
        statussend.publish(toSendChar);  
      } else {
        String toSend = String(i) + ":off";
        char toSendChar[10];
        toSend.toCharArray(toSendChar,10);
        statussend.publish(toSendChar);  
      }
    }
  }

  // reset our state
  lasttouched = currtouched;

  if (initialised == 0) {
    statussend.publish("discover");
  }

}


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 20;
  cls();
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(ret);
       Serial.println("Retrying MQTT connection in 10 seconds...");
       mqtt.disconnect();
       delay(10000);  // wait 10 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
  statussend.publish("discover");
}



void loadingLEDs() {
  mcp.digitalWrite(0, HIGH);delay(10);mcp.digitalWrite(0, LOW);
  mcp.digitalWrite(8, HIGH);delay(10);mcp.digitalWrite(8, LOW);
  mcp.digitalWrite(1, HIGH);delay(10);mcp.digitalWrite(1, LOW);
  mcp.digitalWrite(9, HIGH);delay(10);mcp.digitalWrite(9, LOW);
  mcp.digitalWrite(2, HIGH);delay(10);mcp.digitalWrite(2, LOW);
  mcp.digitalWrite(10, HIGH);delay(10);mcp.digitalWrite(10, LOW);
  mcp.digitalWrite(3, HIGH);delay(10);mcp.digitalWrite(3, LOW);
  mcp.digitalWrite(11, HIGH);delay(10);mcp.digitalWrite(11, LOW);
  mcp.digitalWrite(4, HIGH);delay(10);mcp.digitalWrite(4, LOW);
  mcp.digitalWrite(12, HIGH);delay(10);mcp.digitalWrite(12, LOW);
  mcp.digitalWrite(5, HIGH);delay(10);mcp.digitalWrite(5, LOW);
  mcp.digitalWrite(13, HIGH);delay(10);mcp.digitalWrite(13, LOW);
  mcp.digitalWrite(6, HIGH);delay(10);mcp.digitalWrite(6, LOW);
  mcp.digitalWrite(14, HIGH);delay(10);mcp.digitalWrite(14, LOW);
  mcp.digitalWrite(7, HIGH);delay(10);mcp.digitalWrite(7, LOW);
  mcp.digitalWrite(15, HIGH);delay(10);mcp.digitalWrite(15, LOW);
}

void cls() {
  for (int i=0; i<16; i++) {
    mcp.digitalWrite(i, LOW);
    delay(1);
  }
}



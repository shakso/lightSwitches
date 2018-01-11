#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <EEPROM.h>
#include "EEPROMString.h"

#define AIO_SERVER      "192.168.1.2"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "xxx"
#define AIO_KEY         "xxx"
#define AIO_CID         "autoswitch"

static const char ICACHE_FLASH_ATTR homePage[] = "<!DOCTYPE html> <html> <head> <title>Auto switch</title> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"stylesheet\" href=\"http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css\" /><script src=\"http://code.jquery.com/jquery-1.8.3.min.js\"></script><script src=\"http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.js\"></script><style type=\"text/css\">h2 { margin-bottom:0px; margin-top:0px; }.ui-block-a, .ui-block-b { padding-left:10px; border:1px solid #ccc; width:48%!important; margin-left:2px; }.ui-checkbox { padding-bottom:5px; }div.ui-slider-switch { position: absolute !important; right: 0 !important; width: 44% !important; margin-top:5px!important;}div.ui-field-contain {padding:10px 0px 7px 0px;float:right;width:100%;}</style></head> <body> <div data-role=\"page\" id=\"settings\"><div data-role=\"header\"><h1>Autoswitch</h1><a href=\"#\" data-icon=\"check\" class=\"ui-btn-right\" id=\"saveButton\">Save</a></div><div data-role=\"content\"><div class=\"ui-grid-a\"><div class=\"ui-block-a\"><div data-role=\"fieldcontain\" id=\"leftSwitch\"><h2>Left</h2></div></div><div class=\"ui-block-b\"><div data-role=\"fieldcontain\" id=\"rightSwitch\"><h2>Right</h2></div></div></div></div> <div data-role=\"footer\" data-position=\"fixed\"> <div data-role=\"navbar\" data-iconpos=\"bottom\"> <ul> <li><a href=\"#switcher\" data-icon=\"grid\" data-role=\"tab\">Switches</a></li> <li><a href=\"#settings\" data-icon=\"gear\" data-role=\"tab\" class=\"ui-btn-active ui-state-persist\">Configure</a></li> </ul> </div> </div></div><div data-role=\"page\" id=\"switcher\"><div data-role=\"header\"><h1>Autoswitch</h1></div><div data-role=\"content\"><div class=\"ui-grid-a\"><div data-role=\"fieldcontain\" id=\"allSwitch\"></div></div></div> <div data-role=\"footer\" data-position=\"fixed\"> <div data-role=\"navbar\" data-iconpos=\"bottom\"> <ul> <li><a href=\"#switcher\" data-icon=\"grid\" data-role=\"tab\" class=\"ui-btn-active ui-state-persist\">Switches</a></li> <li><a href=\"#settings\" data-icon=\"gear\" data-role=\"tab\">Configure</a></li> </ul> </div> </div></div><script language=\"Javascript\">$(document).ready(function() {var ws = new WebSocket('ws://192.168.1.2:3000');ws.onopen = function () { ws.send(\"discover\"); };$(\"a[data-role=tab]\").each(function () { var anchor = $(this); anchor.bind(\"click\", function () { $.mobile.changePage(anchor.attr(\"href\"), { transition: \"none\", changeHash: false }); return false; });});count = 0;$.getJSON('http://192.168.1.2/?discover', function(data) {$.each(data.hosts, function(key, value) {$('div#leftSwitch').append('\ <fieldset data-role=\"controlgroup\" data-type=\"horizontal\">\ <input type=\"checkbox\" name=\"checkLeft' + count + '\" id=\"checkLeft' + count + '\" class=\"custom\" />\ <label for=\"checkLeft' + count + '\">' + value.name + '</label>\ </fieldset>'); $('div#rightSwitch').append('\ <fieldset data-role=\"controlgroup\" data-type=\"horizontal\">\ <input type=\"checkbox\" name=\"checkRight' + count + '\" id=\"checkRight' + count + '\" class=\"custom\" />\ <label for=\"checkRight' + count + '\">' + value.name + '</label>\ </fieldset>');$('div#allSwitch').append('<div data-role=\"fieldcontain\">\<label style=\"display:inline!important;float:left;\" for=\"' + value.name + '\">' + value.name + '</label>\<select class=\"slider\" name=\"' + value.name + '\" id=\"' + value.name + '\" data-role=\"slider\">\<option value=\"off\">Off</option>\<option value=\"on\">On</option>\</select>\</div>'); $('div#allSwitch').enhanceWithin(); count++;});$.get('/getSettings', function(settings) {var states = settings.split(\"|\");for (i=0; i<count; i++) {if (states[0].charAt(i) == \"1\") {$(\"input#checkLeft\" + i).prop(\"checked\", true);}if (states[1].charAt(i) == \"1\") {$(\"input#checkRight\" + i).prop(\"checked\", true);}}$('div#leftSwitch').enhanceWithin();$('div#rightSwitch').enhanceWithin();});});$('a#saveButton').on('click', function (e) {checkLeft=\"\";checkRight=\"\";for (i=0; i<count; i++) {if ($(\"input#checkLeft\" + i).attr('checked')) {checkLeft+=\"1\";} else{checkLeft+=\"0\"}if ($(\"input#checkRight\" + i).attr('checked')) {checkRight+=\"1\";} else{checkRight+=\"0\"}}$('#saveButton').css(\"backgroundColor\", \"#ffffcc\");$.get(\"/setSettings?data=\" + checkLeft + \"|\" + checkRight, function() {$('#saveButton').css(\"backgroundColor\", \"#ccffcc\");setTimeout(function() { $('#saveButton').css(\"backgroundColor\", \"#eeeeee\"); }, 2000);});});ws.onmessage = function (e) { var hostArray = JSON.parse(e.data).hosts; for(i=0; i<hostArray.length;i++) { if (hostArray[i].state) { $(\"select#\" + hostArray[i].name).val('on').slider('refresh'); } else { $(\"select#\" + hostArray[i].name).val('off').slider('refresh'); } }};$('select.slider').live('change', function(){ws.send($(this)[0].name + \":\" + $(this)[0].value);});});</script></body></html>";

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_CID, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe statusfeed = Adafruit_MQTT_Subscribe(&mqtt, "manualswitch");
Adafruit_MQTT_Publish statussend = Adafruit_MQTT_Publish(&mqtt, "manualswitch");

int leftSwitch = 0;
int rightSwitch = 0;
String previousDataString = "";
int ignoreSwitch = 0;

char currentState[10];

const char* ssid = "xxx";
const char* password = "xxx";

String allSettings = "";

ESP8266WebServer server(80);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

void handleRoot() {
  server.send_P(200, "text/html", homePage); 
  return;
}

void getSettings() {
  server.send(200,"text/plain", allSettings);
  return;
}

void setSettings() {
  allSettings = server.arg("data");
  Serial.println(allSettings);
  EEPROMWriteString(0, allSettings);
  EEPROM.commit();
  previousDataString = "";
  statussend.publish("discover");
  server.send(200,"text/plain", "OK");
  return;
}

void setup() {
  Wire.begin(D3,D4);
  EEPROM.begin(512);

  Serial.begin(57600);
  
  WiFi.hostname("autoswitch");
  WiFi.begin(ssid, password);
  Serial.println("");
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (MDNS.begin("esp8266")) {
  Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);
  server.on("/getSettings", getSettings);
  server.on("/setSettings", setSettings);
  
  server.begin();
  pwm.begin();

  pinMode(D6, OUTPUT);
  pinMode(D5, INPUT_PULLUP);
  pinMode(D7, INPUT_PULLUP);
  
  digitalWrite(D6, LOW);
  
  pwm.setPWMFreq(60);  

  allSettings = EEPROMReadString(0,35);
  
  Serial.print("*");
  Serial.print(allSettings);
  Serial.println("*");

  if (allSettings == "") {
    allSettings = "00000000|00000000";
  }
  
  statusfeed.setCallback(statuscallback);
  
  // Setup MQTT subscription for time feed.
  mqtt.subscribe(&statusfeed);
  // Get initial setting
  statussend.publish("discover"); 
  
}


void loop() {
    leftSwitch = digitalRead(D5);
    rightSwitch = digitalRead(D7);

    MQTT_connect();
    mqtt.processPackets(5);
    server.handleClient();

    if (ignoreSwitch == 0) {
      if (leftSwitch != digitalRead(D5)) {
        if (digitalRead(D5) == 1) {
          for (int i=0; i <= 7; i++) {
            if (allSettings.charAt(i) == '1') {
              char toSendChar[10];
              String toSend = String(i) + ":on";
              toSend.toCharArray(toSendChar,10);
              statussend.publish(toSendChar); 
            }
          }
        } else {
          for (int i=0; i <= 7; i++) {
            if (allSettings.charAt(i) == '1') {
              char toSendChar[10];
              String toSend = String(i) + ":off";
              toSend.toCharArray(toSendChar,10);
              statussend.publish(toSendChar); 
            }
          }
        } 
      }      
      if (rightSwitch != digitalRead(D7)) {
        if (digitalRead(D7) == 1) {
          for (int i=0; i <= 7; i++) {
            if (allSettings.charAt(i+9) == '1') {
              char toSendChar[10];
              String toSend = String(i) + ":on";
              toSend.toCharArray(toSendChar,10);
              statussend.publish(toSendChar); 
            }
          }
        } else {
          for (int i=0; i <= 7; i++) {
            if (allSettings.charAt(i+9) == '1') {
              char toSendChar[10];
              String toSend = String(i) + ":off";
              toSend.toCharArray(toSendChar,10);
              statussend.publish(toSendChar); 
            }
          }
        } 
      }      
    }

    if (ignoreSwitch > 0) {
      ignoreSwitch--;
      Serial.println(ignoreSwitch);
    }
        
}



void statuscallback(char *data, uint16_t len) {

  String dataString = String(data);

  if (previousDataString != dataString) {
    int leftOn = 0;
    int leftOff = 0;
    int leftTotal = 0;
    int leftDoSwitch = 0;
    
    for (int i=0; i<=7; i++) {
      if ((allSettings.charAt(i) == '1') && (dataString.charAt(i) == '1') && (leftSwitch == 0)) {
        leftOn++;
      }
      if ((allSettings.charAt(i) == '1') && (dataString.charAt(i) == '0') && (leftSwitch == 1)) {
        leftOff++;
      }
      if (allSettings.charAt(i) == '1') {
        leftTotal++;
      }
      if ((allSettings.charAt(i) == '1') && (dataString.charAt(i) != previousDataString.charAt(i))) {
        leftDoSwitch = 1;
      }
    }

    if (leftDoSwitch == 1) {
      if (leftOn == leftTotal) {
        // TURN ON
        pwm.setPWM(0,0,400);
        delay(250);
        pwm.setPWM(0,0,200);
      } 
      if (leftOff == leftTotal) {  
        // TURN OFF
        pwm.setPWM(3,0,90);
        delay(250);
        pwm.setPWM(3,0,380);
      }
    }

    int rightOn = 0;
    int rightOff = 0;
    int rightTotal = 0;
    int rightDoSwitch = 0;
    
    for (int i=0; i<=7; i++) {
      if ((allSettings.charAt(i+9) == '1') && (dataString.charAt(i) == '1') && (rightSwitch == 0)) {
        rightOn++;
      }
      if ((allSettings.charAt(i+9) == '1') && (dataString.charAt(i) == '0') && (rightSwitch == 1)) {
        rightOff++;
      }
      if (allSettings.charAt(i+9) == '1') {
        rightTotal++;
      }
      if ((allSettings.charAt(i+9) == '1') && (dataString.charAt(i) != previousDataString.charAt(i))) {
        rightDoSwitch = 1;
      }
    }

    if (rightDoSwitch == 1) {
      if (rightOn == rightTotal) {
        // TURN ON
        pwm.setPWM(1,0,200);
        delay(250);
        pwm.setPWM(1,0,400);
      } 
      if (rightOff == rightTotal) {  
        // TURN OFF
        pwm.setPWM(2,0,400);
        delay(250);
        pwm.setPWM(2,0,240);
      }
    }
    
    ignoreSwitch = 10;
  }
  
  previousDataString = dataString;
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 20;
  
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
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

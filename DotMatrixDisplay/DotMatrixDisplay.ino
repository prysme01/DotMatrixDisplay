// Configure is you want to use OLED display or DEBUG to serial
// Comment option to disable
#define USE_OLED_DISPLAY 
#define USE_DEBUG 1

#include <MD_Parola.h>          // https://github.com/MajicDesigns/MD_Parola
#include <MD_MAX72xx.h>         // https://github.com/MajicDesigns/MD_MAX72XX
#include <MD_MAX72xx_lib.h>
#include <SPI.h>
#include "utf8_font.h"
//#include "utf8_tools.c"

#ifdef USE_OLED_DISPLAY
  #include <Wire.h>
  #include <brzo_i2c.h>
  #include <OLEDDisplay.h>
  #include <OLEDDisplayFonts.h>
  #include <OLEDDisplayUi.h>
  #include <SSD1306.h>
  #include <SSD1306Spi.h>
  #include <SSD1306Brzo.h>
  #include <SSD1306Wire.h>
#endif

#include <DNSServer.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>          //WiFi Configuration Magic https://github.com/tzapu/WiFiManager 
#include "res.h"                  // Picture resources in the sketch directory
#include <EEPROM.h>               // Library for handle teh EEPROM

#include <TimeLib.h>
#include <NtpClientLib.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>

// DOT MATRIX PIN configuration
#define MAX_DEVICES 8
#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D6  // or SS

#define DS18B20BUS D4 // OneWire Dallas PIN
OneWire oneWire(DS18B20BUS);
DallasTemperature DS18B20(&oneWire);
char temperatureCString[6];

long temps;                                         // Use in the loop to store execution time

#define TIMEOUT_PORTAL 10
uint8_t parolaCurrentType = 0;
uint8_t frameDelay = 25; // default frame delay value
char parolaCurrentMessage[256] = "";
uint8_t pin_led = 16;

// MQTT CONFIGURATION
const char* MQTT_TOPIC_MESSAGE = "jeedom/message";  // MQTT topic to subscribe for message
const char* MQTT_CLIENT_ID ="ESP32Client";          // MQTT client id needed to get offline messages during reconnexion
const IPAddress MQTT_BROKER_IP(37,187,1,120);       // MQTT BROKER IP address
const int  MQTT_BROKER_PORT = 1883;                 // MQTT BROKER port

WiFiClient espClient;
PubSubClient mqttclient(espClient);

#ifdef USE_OLED_DISPLAY
  SSD1306Brzo  display(0x3c, D1, D2);
#endif 

MD_Parola Parola = MD_Parola(CS_PIN, MAX_DEVICES); // HARDWARE SPI for dot matrix display

void getTemperature() {
  float tempC;
  DS18B20.requestTemperatures(); 
  tempC = DS18B20.getTempCByIndex(0);
  dtostrf(tempC,2,2,temperatureCString);
}

uint8_t utf8Ascii(uint8_t ascii)
{
  static uint8_t cPrev;
  uint8_t c = '\0';

  if (ascii < 0x7f)   // Standard ASCII-set 0..0x7F, no conversion
  {
    cPrev = '\0';
    c = ascii;
  }
  else
  {
    switch (cPrev)  // Conversion depending on preceding UTF8-character
    {
    case 0xC2: c = ascii;  break;
    case 0xC3: c = ascii | 0xC0;  break;
    case 0x82: if (ascii==0xAC) c = 0x80; // Euro symbol special case
    }
    cPrev = ascii;   // save last char
  }

  PRINTX("\nConverted 0x", ascii);
  PRINTX(" to 0x", c);

  return(c);
}

void utf8Ascii(char* s)
{
  uint8_t c, k = 0;
  char *cp = s;

  PRINT("\nConverting: ", s);

  while (*s != '\0')
  {
    c = utf8Ascii(*s++);
    if (c != '\0')
      *cp++ = c;
  }
  *cp = '\0';   // terminate the new string
}


void configModeCallback (WiFiManager *myWiFiManager) {
  #if USE_DEBUG
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
  #endif
  
  #ifdef USE_OLED_DISPLAY
    display.drawString(0, 0, "Entering Config Mode" );
    display.drawString(0, 20, "SSID:"+myWiFiManager->getConfigPortalSSID() );
    display.drawString(0, 40,"IP:" + WiFi.softAPIP().toString() );
    display.display();
  #endif

  Parola.print("AP|MODE");
}

#ifdef USE_OLED_DISPLAY
/**
 * Display LOGO for 1 second (Logo is declared in res.h)
 * format XBM done with GIMP 128 pixels x 64 pixels Black and White
 */
void OLEDWelcome() {
  display.drawXbm(0 , 0, Logo_width, Logo_height, xbm_bits);
  display.display();
  delay(1000);
  display.clear();
}
#endif

/**
 * Setup code, run once a startup
 */
void setup() {
  
  #if USE_DEBUG
    Serial.begin(115200);
  #endif

  // Start DS18B20 measurement
  DS18B20.begin();

  //pinMode(A0,INPUT);
  
  //pinMode(LED_BUILTIN,OUTPUT);                        // configure the builtin led to OUTPUT mode
  //digitalWrite(LED_BUILTIN,!digitalRead(pin_led));  // switch the status (values can be LOW or HIGH)

  #ifdef USE_OLED_DISPLAY
    // OLED Init
    display.init();
    display.clear();
    //display.setContrast(55);
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    OLEDWelcome();
  #endif
  
  // Dot Matrix init
  Parola.begin(); 
  Parola.displayClear();
  Parola.displaySuspend(false);
  Parola.setIntensity(EEPROM.read(0));  // Restore last intensity value saved from EEPROM
  Parola.setFont(ExtASCII);
  Parola.setTextAlignment(PA_CENTER);
  
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  //wifiManager.setConfigPortalTimeout(TIMEOUT_PORTAL);
  Serial.println("Wifi.autoConnect start");
  int wifiStatus = wifiManager.autoConnect();
  if(!wifiStatus) {
    Parola.print("Wifi ko");
    //reset and try again, or maybe put it to deep sleep
    //ESP.reset();
    //delay(5000);
  } else {
    Parola.displayText ("WIFI ok", PA_CENTER, 15, 1000, PA_OPENING_CURSOR , PA_OPENING_CURSOR );
    NTP.begin("pool.ntp.org", 1, true);
    NTP.setInterval(3600);
    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
      if (error) {
        Serial.print("Time Sync error: ");
        if (error == noResponse)
          Serial.println("NTP server not reachable");
        else if (error == invalidAddress)
          Serial.println("Invalid NTP server address");
        }
      else {
        Serial.print("Got NTP time: ");
        Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
      }
    });
    #ifdef USE_OLED_DISPLAY
      display.drawString(0, 0, "Connected to " );
      display.drawString(0, 20, "SSID:"+WiFi.SSID() );
      display.drawString(0, 40,"IP:" + WiFi.localIP().toString() );
    #endif
  }
  
  // Start the webserver
  //server.begin();
  //server.on ( "/", handleRoot );
  //server.on ( "/command", handleCommand );
  
  // Print the IP address
  #if USE_DEBUG
    Serial.println(WiFi.localIP());
  #endif

  #ifdef USE_OLED_DISPLAY
    // Flush OLED display
    display.display();  
  #endif

  // Get first sync
  NTP.getTimeDateString();
  
  // init time
  temps = millis();

  mqttclient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
  mqttclient.setCallback(mqttCallback);
}
boolean reconnect() {
  if (mqttclient.connect(MQTT_CLIENT_ID)) {
    mqttclient.subscribe(MQTT_TOPIC_MESSAGE);
    Serial.println("mqtt connected.");
  }
  return mqttclient.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("mqttCallback start");
  // Allocate the correct amount of memory for the payload copy
  byte* payloadcopy = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(payloadcopy,payload,length);
  
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject((char*)payloadcopy);
  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  } else {
    String ltext=root["text"].as<String>();
    ltext.toCharArray(parolaCurrentMessage,ltext.length() + 1);
    Serial.print("priorite: ");
    Serial.println((const char*)root["priorite"]);
    Serial.print("type: ");
    parolaCurrentType = (int)root["type"];
    Serial.println((const char*)root["type"]);
    Serial.print("intensite: ");
    Serial.println((const char*)root["intensite"]);
    Serial.print("repetition: ");
    Serial.println((const char*)root["repetition"]);
  }
  // Free the memory
  free(payloadcopy);
  // Switch on the LED if an 1 was received as first character
  /*if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }*/
  
}

void sendTemptToMQQT() {
  int photocellReading = analogRead(A0);
          //Serial.println(analogRead(A0));

          if (photocellReading < 300) {
              //Serial.println(" - Noir");
              Parola.setIntensity(1);
            } else if (photocellReading < 600) {
              //Serial.println(" - Sombre");
              Parola.setIntensity(3);
            } else if (photocellReading < 800) {
              //Serial.println(" - Lumiere");
              Parola.setIntensity(5);
            } else if (photocellReading < 950) {
              //Serial.println(" - Lumineux");
              Parola.setIntensity(10);
            } else {
              //Serial.println(" - Tres lumineux");
              Parola.setIntensity(15);
            }
          
          // send json to mqtt
          StaticJsonBuffer<300> JSONbuffer;
          JsonObject& JSONencoder = JSONbuffer.createObject();
         
          JSONencoder["device"] = "ESP8266";
          JSONencoder["sensorType"] = "LightSensor";
          JSONencoder["values"] = photocellReading;
         
          char JSONmessageBuffer[100];
          JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
         
          if (mqttclient.publish("esp/test", JSONmessageBuffer) == true) {
            Serial.println(JSONmessageBuffer);  
          } else {
            Serial.println("Error sending message");
          }
}

void loop() {
  // Handle Web server client request
  //server.handleClient();
  //Serial.print("currentType : ");
  //Serial.println(parolaCurrentType);
  // start mqtt subscribe client
  

  // Handle Dot Matrix display
  if (Parola.displayAnimate()) // True if animation ended
   {

      if (!mqttclient.connected()) {
        Serial.println("not connected, reconnect");
        reconnect();
      } else {
        // Client connected
        mqttclient.loop();
      }
        
      if((millis() - temps) > 10000) {
          sendTemptToMQQT();
          temps = millis(); 
      }
      
    
    switch (parolaCurrentType) {
      if (parolaCurrentType >10){parolaCurrentType ==0;}

      // No message to display
      /*case 0: {
          String time = NTP.getTimeDateString().substring(0,5);
          char *cstr = new char[time.length()];
          strcpy(cstr, time.c_str());
          //Parola.displayText(cstr,  PA_CENTER, 20, 0,  PA_NO_EFFECT,  PA_NO_EFFECT);
          Parola.setTextAlignment(PA_CENTER);
          Parola.print(cstr);
          //Parola.displayText(cstr, PA_CENTER, 30, 10, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
          //delete [] cstr;
          delay(1000);
        
      }
      break;*/
      
      // One time message
      case 1:
         // Parola.displayText(currentMessage, PA_CENTER, 25, 1200, PA_OPENING_CURSOR  , PA_OPENING_CURSOR );
         Serial.print("message is ");
         Serial.println(parolaCurrentMessage);
          Parola.displayScroll(parolaCurrentMessage, PA_LEFT, PA_SCROLL_LEFT, frameDelay);
          parolaCurrentType=0;
      break;

      // Looping message
      case 2:
          // Parola.displayText(currentMessage, PA_CENTER, 25, 1200, PA_OPENING_CURSOR  , PA_OPENING_CURSOR );
          Serial.print("message is ");
          Serial.println(parolaCurrentMessage);
          Parola.displayScroll(parolaCurrentMessage, PA_LEFT, PA_SCROLL_LEFT, frameDelay);
          parolaCurrentType=2;
      break;
      // TESTING
      case 9:{
          
          parolaCurrentType=9;
      }
      break;
      case 10:
            getTemperature();
            Serial.print("temperature is ");
            Serial.println(temperatureCString);
            Parola.displayScroll(temperatureCString,  PA_LEFT, PA_SCROLL_LEFT,  frameDelay);
          parolaCurrentType=0;
      break;
    }
  }
  
}

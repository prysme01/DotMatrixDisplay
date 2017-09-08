// Configure is you want to use OLED display or DEBUG to serial
// Comment option to disable
#define USE_OLED_DISPLAY 
#define USE_LIGHT_SENSOR
#define USE_DEBUG 1

#include <MD_Parola.h>          // https://github.com/MajicDesigns/MD_parola
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
  #include "res.h"                  // Picture resources in the sketch directory
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
#include <EEPROM.h>               // Library for handle teh EEPROM

#include <TimeLib.h>
#include <NtpClientLib.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>

// ESP8266 PIN configuration
#define CLK_PIN   D5  // Matrix CLK
#define DATA_PIN  D7  // Matrix DIN
#define CS_PIN    D6  // Matrix DS
#define DS18B20BUS D4 // OneWire Dallas DS18B20 PIN

#define MAX_DEVICES 8 // Number of LED MATRIX module

// DS18B20 Temperature sensor
OneWire oneWire(DS18B20BUS);
DallasTemperature DS18B20(&oneWire);

long millis_temperature;                                                    // Use in the loop to store execution time
long millis_lightsensor;                                                    // Use in the loop to store execution time

// PAROLA 
uint8_t parolaCurrentType = 0;
uint8_t frameDelay = 25; // default frame delay value
//char parolaCurrentMessage[256] = "";
char parolaCurrentMessage[120];
textEffect_t parolaEffectIn;
textEffect_t parolaEffectOut;
textPosition_t parolaPosition;
int parolaAnimationSpeed;
int parolaPause;
bool parolaIntensityManagedBySensor = true;                                 // light sensor management on/off
bool newMessageAvailable = false;
int LIGHT_SENSOR_POLLING=60*1000;                                              // Wait 5 sec to adjust LED matrix intensity based on light sensor
MD_Parola parola = MD_Parola(CS_PIN, MAX_DEVICES);                          // HARDWARE SPI for dot matrix display
uint8_t pin_led = 16;

// MQTT CONFIGURATION
const char* MQTT_TOPIC_MESSAGE = "jeedom/message";                          // MQTT topic to subscribe for message
const char* MQTT_TOPIC_SENSOR  = "jeedom/ESP8266_LED_MATRIX_BOX/sensors";   // MQTT topic to publish temperature
const char* MQTT_TOPIC_SENSOR_DEVICE  = "ESP8266_LED_MATRIX_BOX";           // MQTT topic to publish temperature
const char* MQTT_CLIENT_ID ="ESP32Client";                                  // MQTT client id needed to get offline messages during reconnexion | not working mqtt lib doesnot support clean session
const IPAddress MQTT_BROKER_IP(37,187,1,120);                               // MQTT BROKER IP address
const int  MQTT_BROKER_PORT = 1883;                                         // MQTT BROKER port
const int MQTT_POLLING_SENSOR = 15*60*1000;                                      // MQTT polling delay between sending temperature to jeedom in millisec

WiFiClient wifiClient;
WiFiManager wifiManager;
PubSubClient mqttclient(wifiClient);

#ifdef USE_OLED_DISPLAY
  SSD1306Brzo  display(0x3c, D1, D2);
#endif 



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

  parola.print("AP|MODE");
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
  parola.begin(); 
  parola.displayClear();
  parola.displaySuspend(false);
  parola.setIntensity(5);  // Restore last intensity value saved from EEPROM
  parola.setFont(ExtASCII);
  parola.setTextAlignment(PA_CENTER);
  //parola.setCharSpacing(1);
  
  wifiManager.setAPCallback(configModeCallback);
  //wifiManager.setConfigPortalTimeout(TIMEOUT_PORTAL);
  Serial.println("Wifi.autoConnect start");
  if(!wifiManager.autoConnect()) {
    parola.displayText ("WIFI not working", PA_CENTER, 15, 1000, PA_OPENING_CURSOR , PA_OPENING_CURSOR );
    //reset and try again, or maybe put it to deep sleep
    //ESP.reset();
    //delay(5000);
  } else {
    parola.displayText ("WIFI Ready", PA_CENTER, 15, 1000, PA_OPENING_CURSOR , PA_OPENING_CURSOR );
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
  millis_temperature = millis();
  millis_lightsensor = millis();

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

textEffect_t parseAnimation(int anim) {
  // unknown default animation
  textEffect_t choosenAnimation = PA_SCROLL_RIGHT;
  switch (anim){
          case 0: 
            choosenAnimation = PA_SCROLL_RIGHT;
            break;    
          case 1: 
            choosenAnimation = PA_SCROLL_LEFT;
            break;
          case 2:
            choosenAnimation = PA_SCROLL_UP;
            break;
          case 3:
            choosenAnimation =  PA_SCROLL_DOWN;
            break;
          case 4:
            choosenAnimation = PA_OPENING_CURSOR;
            break;
          case 5:
            choosenAnimation = PA_CLOSING_CURSOR;
            break;
          case 6:
            choosenAnimation = PA_WIPE_CURSOR;
            break;
          case 7:
            choosenAnimation = PA_MESH;
            break;
          case 8:
            choosenAnimation = PA_FADE;
            break;
          case 9:
            choosenAnimation = PA_PRINT;
            break;
          case 10:
            choosenAnimation = PA_BLINDS;
            break;
          case 11:
            choosenAnimation = PA_SCAN_HORIZ0;
            break;    
          case 12:
            choosenAnimation = PA_SCAN_HORIZ1;
            break; 
          case 13:
            choosenAnimation = PA_SCAN_VERT0;
            break; 
          case 14:
            choosenAnimation = PA_SCAN_VERT1;
            break;                                                                                                                               
        }
    return choosenAnimation;
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
    // Dealing with TEXT
    newMessageAvailable = true;
    if (root.containsKey("text")) {
      if (root["text"] != "") {
        String text = root["text"].as<String>();
        text.toCharArray(parolaCurrentMessage, text.length()+1); 
        
      } else {
        strcpy (parolaCurrentMessage, "empty text");
      }
    } else {
      strcpy (parolaCurrentMessage, "miss text tag");
    }
    
    // Dealing with intensity
    parolaIntensityManagedBySensor = true;
    if (root.containsKey("lum")) {
      if ( root["lum"] != "") {
        // set specific intensity
        parola.setIntensity((int)root["lum"]);
        parolaIntensityManagedBySensor = false;
      }
    }
    // Dealing with text position
    // default CENTER
    parolaPosition = PA_CENTER;
    if (root.containsKey("pos")) {
      if ( root["pos"] != "") {
        int pos = (int)root["pos"];
        switch (pos) {
          case 0:parolaPosition=PA_LEFT;break;
          case 1:parolaPosition=PA_CENTER;break;
          case 2:parolaPosition=PA_RIGHT;break;
        }
      }
    }
    // dealing with animation IN
    parolaEffectIn = PA_SCROLL_RIGHT;
    // Dealing with intensity
    if (root.containsKey("eff_in")) {
      if ( root["eff_in"] != "") {
        int anim = (int)root["eff_in"];
        parolaEffectIn = parseAnimation(anim);
      }
    }

    // dealing with animation OUT
    parolaEffectOut = PA_SCROLL_RIGHT;
    // Dealing with intensity
    if (root.containsKey("eff_out")) {
      if ( root["eff_out"] != "") {
        int anim = (int)root["eff_out"];
        parolaEffectOut = parseAnimation(anim);
      }
    }
    
    // dealing with speed
    parolaAnimationSpeed = 15;
    if (root.containsKey("speed")) {
      if ( root["speed"] != "") {
        parolaAnimationSpeed = (int)root["speed"];
      }
    }
    
    // dealing with pause
    parolaPause = 1000;
    if (root.containsKey("pause")) {
      if ( root["pause"] != "") {
        parolaPause = (int)root["pause"];
      }
    }
  }
  
  // Free the memory
  free(payloadcopy);
}

/**
 * Adjust LED matrix luminosity according to the Light sensor
 */
void adjustLuminosity() {
  #ifdef USE_LIGHT_SENSOR
    int photocellReading = analogRead(A0);
    if (photocellReading < 600) {
      //Serial.println(" - Noir");
      parola.setIntensity(1);
    } else if (photocellReading < 700) {
      //Serial.println(" - Sombre");
      parola.setIntensity(3);
    } else if (photocellReading < 800) {
      //Serial.println(" - Lumiere");
      parola.setIntensity(5);
    } else if (photocellReading < 1000) {
      //Serial.println(" - Lumineux");
      parola.setIntensity(7);
    } else {
      //Serial.println(" - Tres lumineux");
      parola.setIntensity(15);
    }
    Serial.print("Adjusting luminosity to ");
    Serial.println(photocellReading);
  #endif
}

void sendTemperatureToMQTT() {

  DS18B20.requestTemperatures();
  
  // send json to mqtt
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  
  JSONencoder["device"] = MQTT_TOPIC_SENSOR_DEVICE;
  JSONencoder["sensorType"] = "TemperatureSensor";
  JSONencoder["temperature"] = DS18B20.getTempCByIndex(0);

  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
 
  if (mqttclient.publish(MQTT_TOPIC_SENSOR, JSONmessageBuffer) == true) {
    Serial.println(JSONmessageBuffer);  
  } else {
    Serial.println("Error sending message");
  }
}

void loop() {

  
  if (parola.displayAnimate())  {// True if animation ended

    if (newMessageAvailable) {
      char newMessage[120];
      strcpy(newMessage,parolaCurrentMessage);
      Serial.println("new msg available : ");
      Serial.print("text : ");
      Serial.println(newMessage);
      Serial.print("parolaPosition : ");
      Serial.println(parolaPosition);
      Serial.print("parolaAnimationSpeed : ");
      Serial.println(parolaAnimationSpeed);
      Serial.print("parolaPause : ");
      Serial.println(parolaPause);
      Serial.print("parolaEffectIn : ");
      Serial.println(parolaEffectIn);
      Serial.print("parolaEffectOut : ");
      Serial.println(parolaEffectOut);

      parola.displayText(newMessage, parolaPosition,parolaAnimationSpeed,parolaPause,parolaEffectIn,parolaEffectOut );
      //parola.displayScroll(newMessage, parolaPosition,parolaEffectIn,parolaAnimationSpeed );
      newMessageAvailable = false;
    }
    //parola.displayReset();
    
    if (parolaIntensityManagedBySensor && ((millis()-millis_lightsensor) > LIGHT_SENSOR_POLLING) ) {
      adjustLuminosity();
      millis_lightsensor = millis();
    }
    
    if (!mqttclient.connected()) {
      reconnect();
    } else {
      // Client connected
      mqttclient.loop();
    }

    // Every MQTT_POLLING_SENSOR
    if((millis() - millis_temperature) > MQTT_POLLING_SENSOR) {
        sendTemperatureToMQTT();
        millis_temperature = millis(); 
    }
  }
}

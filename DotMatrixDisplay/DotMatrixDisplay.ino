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
#include <ESP8266WebServer.h>
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

// DOT MATRIX PIN configuration
#define MAX_DEVICES 4
#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D6  // or SS



#define TIMEOUT_PORTAL 10
uint8_t messageType = 0;
uint8_t frameDelay = 25; // default frame delay value
char currentMessage[80] = "";
uint8_t pin_led = 16;

ESP8266WebServer server (80);
#ifdef USE_OLED_DISPLAY
  SSD1306Brzo  display(0x3c, D1, D2);
#endif 

//MD_Parola Parola = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES); // HARDWARE SPI for dot matrix display
MD_Parola Parola = MD_Parola(CS_PIN, MAX_DEVICES); // HARDWARE SPI for dot matrix display



uint8_t utf8Ascii(uint8_t ascii)
// Convert a single Character from UTF8 to Extended ASCII according to ISO 8859-1,
// also called ISO Latin-1. Codes 128-159 contain the Microsoft Windows Latin-1
// extended characters:
// - codes 0..127 are identical in ASCII and UTF-8
// - codes 160..191 in ISO-8859-1 and Windows-1252 are two-byte characters in UTF-8
//                 + 0xC2 then second byte identical to the extended ASCII code.
// - codes 192..255 in ISO-8859-1 and Windows-1252 are two-byte characters in UTF-8
//                 + 0xC3 then second byte differs only in the first two bits to extended ASCII code.
// - codes 128..159 in Windows-1252 are different, but usually only the €-symbol will be needed from this range.
//                 + The euro symbol is 0x80 in Windows-1252, 0xa4 in ISO-8859-15, and 0xe2 0x82 0xac in UTF-8.
//
// Modified from original code at http://playground.arduino.cc/Main/Utf8ascii
// Extended ASCII encoding should match the characters at http://www.ascii-code.com/
//
// Return "0" if a byte has to be ignored.
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
// In place conversion UTF-8 string to Extended ASCII
// The extended ASCII string is always shorter.
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

/**
 * Set the intensity of dot matrix.
 */
void setIntensity(String value ){
  long l = value.toInt();
  byte i = (byte) l;
  if ((i>=0) && (i<=15)){  
    // Write the new value to EEPROM
    EEPROM.write(0, i);       
    EEPROM.commit();
    Parola.setIntensity(i);   
  } 
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
 * Endpoint for URL http://IP/command
 */
void handleCommand() {

  String intensity;
  String command;
  String text;
  String type;
  String message = "Number of args received: ";
  if (server.args()==0) {
     server.send(200, "text/plain", "parameter : intensity (int), text= String to display");
     return;
  }
  message += server.args();
  for (int i = 0; i < server.args(); i++) {
    message += server.argName(i) + ": ";
    message += server.arg(i) + "\n";
    message += "\n";
    
   // Deal with the command
   intensity=server.arg("intensity");
   command = server.arg("command");
   text = server.arg("text");
   type = server.arg("type");
  } 
  
  if (intensity != "") {
    setIntensity(intensity);
  }
  if (text != "") {
    text.toCharArray(currentMessage,sizeof(currentMessage));
    
  }
  if (type!="") {
    Serial.println(type);
    messageType = atoi(type.c_str());
    Serial.println(messageType);
  } else {
    // no message type then defaut one time message
    messageType=1;
  }

  // convert UTF8 to ASCII in place
  utf8Ascii(currentMessage);
  
  // Send HTTP response and log
  server.send(200, "text/plain", message);       //Response to the HTTP request

  #if USE_DEBUG
    Serial.println(message);
    Serial.println(text.length());
  #endif
}

/**
 * Endpoint for URL http://ip/
 */
void handleRoot() {
  server.send ( 200, "text/plain", "this works as well" );
  Parola.displayText ("Hit Root", PA_CENTER, 25, 3000, PA_OPENING_CURSOR , PA_OPENING_CURSOR ); 
}

/**
 * Setup code, run once a startup
 */
void setup() {
  
  #if USE_DEBUG
    Serial.begin(115200);
  #endif
  
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
    NTP.setInterval(600);
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
  server.begin();
  server.on ( "/", handleRoot );
  server.on ( "/command", handleCommand );
  
  // Print the IP address
  #if USE_DEBUG
    Serial.println(WiFi.localIP());
  #endif

  #ifdef USE_OLED_DISPLAY
    // Flush OLED display
    display.display();  
  #endif
}



void loop() {
  // Handle Web server client request
  server.handleClient();

  // Handle Dot Matrix display
  if (Parola.displayAnimate()) // True if animation ended
   {
    switch (messageType) {
      if (messageType >10){messageType ==0;}

      // No message to display
      case 0: {
          String time = NTP.getTimeDateString();
          char *cstr = new char[time.length() + 1];
          strcpy(cstr, (const char *)time.c_str());
          Parola.displayText(cstr,  PA_CENTER, 50, 5000,  PA_SCROLL_LEFT,  PA_SCROLL_LEFT);
          //delete [] cstr;
      }
      break;
      
      // One time message
      case 1:
         // Parola.displayText(currentMessage, PA_CENTER, 25, 1200, PA_OPENING_CURSOR  , PA_OPENING_CURSOR );
          
          Parola.displayScroll(currentMessage, PA_LEFT, PA_SCROLL_LEFT, frameDelay);
          messageType=0;
      break;

      // Looping message
      case 2:
          // Parola.displayText(currentMessage, PA_CENTER, 25, 1200, PA_OPENING_CURSOR  , PA_OPENING_CURSOR );
          Serial.println("type 2 case");
          
          Parola.displayScroll(currentMessage, PA_LEFT, PA_SCROLL_LEFT, frameDelay);
          messageType=2;
      break;

      // TESTING
      case 9:
          //Parola.displayScroll('$', PA_LEFT, PA_SCROLL_LEFT, frameDelay);
          Parola.displayText("$",  PA_CENTER, 20, 500,  PA_SCROLL_LEFT,  PA_SCROLL_LEFT);
          messageType=0;
      break;
      // Looping message
      case 10:
          Parola.displayScroll("voila un é et un à", PA_LEFT, PA_SCROLL_LEFT, frameDelay);
          messageType=0;
      break;
    }
  }
}

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <MD_MAX72xx_lib.h>

#include <SPI.h>

#include <Wire.h>
#include <brzo_i2c.h>
#include <OLEDDisplay.h>
#include <OLEDDisplayFonts.h>
#include <OLEDDisplayUi.h>
#include <SH1106.h>
#include <SH1106Brzo.h>
#include <SH1106Spi.h>
#include <SH1106Wire.h>
#include <SSD1306.h>
#include <SSD1306Spi.h>
#include <SSD1306Brzo.h>
#include <SSD1306Wire.h>

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

#define USE_DISPLAY
#define TIMEOUT_PORTAL 10

#define MAX_DEVICES 4
#define CLK_PIN   D5  // or SCK
#define DATA_PIN  D7  // or MOSI
#define CS_PIN    D6  // or SS

#define USE_WIFI 1
uint8_t nStatus = 0;
uint8_t pin_led = 16;

ESP8266WebServer server ( 80 );

SSD1306  display(0x3c, D1, D2);
//MD_Parola Parola = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES); // HARDWARE SPI for dot matrix display
MD_Parola Parola = MD_Parola(CS_PIN, MAX_DEVICES); // HARDWARE SPI for dot matrix display

#if USE_WIFI
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());

  
  String wifiString="Wifi SSID";
  display.drawString(0, 0, "Entering Config Mode" );
  display.drawString(0, 20, "SSID:"+myWiFiManager->getConfigPortalSSID() );
  display.drawString(0, 40,"IP:" + WiFi.softAPIP().toString() );
  display.display();

  Parola.print("AP|MODE");
  
}
#endif

void OLEDWelcome() {
  // draw an xbm prysme logo at startup.
  display.drawXbm(0 , 0, Logo_width, Logo_height, xbm_bits);
  display.display();
  delay(1000);
  display.clear();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // configure the builtin led to OUTPUT mode
  pinMode(LED_BUILTIN,OUTPUT);
  // switch the status (values can be LOW or HIGH)
  //digitalWrite(LED_BUILTIN,!digitalRead(pin_led));
  // OLED Init
  display.init();
  display.clear();
  //display.setContrast(55);
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  OLEDWelcome();
  
  // Matrix init
  // Setup and start Dot matrix display
  Parola.begin(); 
  Parola.displayClear();
  Parola.displaySuspend(false);
  Parola.setIntensity(3);

  
  
  
  #if USE_WIFI
    // put your setup code here, to run once:
    WiFiManager wifiManager;
    //wifiManager.setConfigPortalTimeout(TIMEOUT_PORTAL);
    wifiManager.setAPCallback(configModeCallback);
    if(!wifiManager.autoConnect()) {
      Serial.println("failed to connect and hit timeout");
      Parola.print("Wififailed.");
      //reset and try again, or maybe put it to deep sleep
      //ESP.reset();
      //delay(5000);
    } else {
      Parola.displayText ("WIFI ok", PA_CENTER, 25, 3000, PA_OPENING_CURSOR , PA_OPENING_CURSOR ); 
      display.drawString(0, 0, "Connected to " );
      display.drawString(0, 20, "SSID:"+WiFi.SSID() );
      display.drawString(0, 40,"IP:" + WiFi.localIP().toString() );
    }
  #endif
  #if !USE_WIFI 
    //Parola.print("");
    Parola.displayText ("noWIFI", PA_CENTER, 25, 3000, PA_OPENING_CURSOR , PA_OPENING_CURSOR ); 
    display.drawString(0, 0, "Compiled with no WIFI." );
    
  #endif

  // Start the webserver
  server.begin();
  Serial.println("Server started");
  server.on ( "/", handleRoot );
  
  // Print the IP address
  Serial.println(WiFi.localIP());
 
 display.display();  
 
 /*Parola.displayText("Slice",PA_LEFT,25,1000,PA_SLICE,PA_SLICE);
 Parola.displayScroll("Fade",PA_LEFT,PA_FADE,25);*/
}



void handleRoot() {
  server.send ( 200, "text/plain", "this works as well" );
  //Parola.displayScroll ("Hit Root", PA_CENTER, 25, 3000, PA_OPENING_CURSOR , PA_OPENING_CURSOR ); 
}

void loop() {

//server.handleClient();
 
 /*if (Parola.displayAnimate()) {
  Parola.displayText("Slice",PA_LEFT,2,1000,PA_SLICE,PA_SLICE);
 }
 if (Parola.displayAnimate()) {
  Parola.displayScroll("Fade",PA_LEFT,PA_FADE,25);
 }*/

 if (Parola.displayAnimate()) {
  
  switch (nStatus) {
    
    if (nStatus>7) {nStatus=0;}
    case 0 : 
    Parola.setIntensity(rand()%15);
      Parola.displayText("Slice",PA_LEFT,2,1000,PA_SLICE,PA_SLICE);
    break;
    case 1 : 
    Parola.setIntensity(rand()%15);
      Parola.displayText("Fade",PA_LEFT,20,500,PA_FADE,PA_FADE);
      
    break;
    case 2 : 
    Parola.setIntensity(rand()%15);
      Parola.displayText("Wipe",PA_LEFT,10,500,PA_WIPE,PA_WIPE);
      
    break;
    case 3 : 
    Parola.setIntensity(rand()%15);
      Parola.displayText("Open",PA_CENTER,10,500,PA_OPENING,PA_OPENING);
      
    break;
    case 4 : 
    Parola.setIntensity(rand()%15);
      Parola.displayScroll("Non, moi j crois qu il faut que vous arretiez d essayer de dire des trucs. Ca vous fatigue, deja, pis pour les autres, vous vous rendez pas compte de c que c est. Moi quand vous faites ca, ca me fout une angoisse. J pourrais vous tuer, j crois. De chagrin, hein . J vous jure c est pas bien. Il faut plus que vous parliez avec des gens.",PA_LEFT,PA_SCROLL_LEFT,20); 
    break;
  }
  nStatus++; 
  }
  
 }
 
 /*Parola.displayScroll("Fade",PA_LEFT,PA_FADE,25);
 Parola.displayScroll("Wipe",PA_LEFT,PA_WIPE,25);
 Parola.displayScroll("Open",PA_LEFT,PA_OPENING,25);
 Parola.displayScroll("Scrolling 123456789",PA_LEFT,PA_SCROLL_LEFT,25);*/
 
 /*if (Parola.displayAnimate()) // True if animation ended
   {
    
    Parola.setIntensity(test);

    switch (nStatus) {
      if (nStatus >7){nStatus ==0;}
    case 0:
      //ConnectToServer();   // Get text from server
      Parola.displayText   ("Waiting for message", PA_CENTER, 25, 1200, PA_OPENING_CURSOR  , PA_OPENING_CURSOR );    
      break;
    
    nStatus ++;
    }
  }*/
  /*for (int i=0;i<=15;i++) {
    test(i);
  }*/
  
  


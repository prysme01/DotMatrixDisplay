# DotMatrixDisplay

Display messages to a DOT Matrix Display.
I use it to print usefull messages from my home automation solution (jeedom).

### Prerequisites

* ESP8266 (mine is Wemos D1 NodeMcu  ESP12F 
* DOT Matrix based on MAX7219
* OLED display (SSD1306 and SH1106 based 128x64 pixel) - not mandatory

### Features
* Display message by using simple http remote command (GET URL)
* Read a file to display text
* more to come

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

Thank you very much to [DIYDave](https://github.com/DIYDave/ScrollText-ESP8266) for his code and inspiration 

Libraries used, thanks to their respectives authors
* [MD_Parola](https://github.com/MajicDesigns/MD_Parola) DotMatrix management library
* [MAX72xx](https://github.com/MajicDesigns/MD_MAX72XX) Max7219 library
* [WifiManager](https://github.com/tzapu/WiFiManager) Wifi management
* [SSD1306](https://github.com/squix78/esp8266-oled-ssd1306) OLED library

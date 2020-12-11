/*
 Name:		BP.ino
 Created:	12/11/2020 11:05:16 AM
 Author:	Rob antonisse

 Sketch for EscapeWorld.nl The Missing Bride puzzle BomB


*/

//libraries
#include <EEPROM.h>
#include <FastLed.h>

//declarations
#define fl FastLED.show();

byte aantalpix = 24;
CRGB pix[24];

//temps
unsigned long tijd;
byte pixcount;


void setup() {
	FastLED.addLeds<NEOPIXEL, 8 >(pix, 24);
	FastLED.setBrightness(50);

}


void loop() {
	if (millis() - tijd > 1000) {
		tijd = millis();
		FL_exe();
  }
}
void FL_exe() {
	pixcount ++;
	FastLED.clearData();


	if (pixcount > aantalpix-1) pixcount = 0;
	pix[pixcount].r = random(0, 255);
	pix[pixcount].g = random(0, 255);
	pix[pixcount].b = random(0, 255);
	fl;
}
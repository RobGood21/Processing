/*
 Name:		Processing.ino
 Created:	11/1/2020 12:10:15 PM
 Author:	Rob Antonisse

 Sketch voor een interface tussen arduino en Processing
*/

//declaraties en libraries
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <splash.h>
#include <Adafruit_SSD1306.h>

//constructors
Adafruit_SSD1306 display(128, 64, &Wire, -1);


byte received;


void setup() {
	Serial.begin(9600);
	Serial.println("Hallo, even een berichtje van Art Duino.");

	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(2);
	display.setCursor(10, 10);
	display.print("hallo");
	display.drawRect(1, 1, 120, 60,WHITE);
	display.display();
}


void loop() {
	// reply only when you receive data:
	//if (Serial.available() > 0) {
		// read the incoming byte:
		//received = Serial.read();

		// say what you got:
		//Serial.print("I received: ");
		//Serial.print(char(received));
	//}
}

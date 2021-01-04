/*
 Name:		BrideExit.ino
 Created:	1/4/2021 2:16:15 PM
 Author:	Rob Antonisse

 Exit number puzzel. To entry the correct number wil release doorlock 
*/


//libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <splash.h>
#include <Adafruit_SSD1306.h>

//declaraties
Adafruit_SSD1306 display(128, 64, &Wire, -1);

byte kolom[4];
byte row; 
byte rowcount;
unsigned long SWcount;

//temp variables
unsigned long tijd;
byte ledcount;



void setup() {
	//display.begin
	Serial.begin(9600);
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);


	/*
	display.clearDisplay() – all pixels are off
		display.drawPixel(x, y, color) – plot a pixel in the x, y coordinates
		display.setTextSize(n) – set the font size, supports sizes from 1 to 8
		display.setCursor(x, y) – set the coordinates to start writing text
		display.print(“message”) – print the characters at location x, y
		display.display() – call this method for the changes to make effect
*/
	delay(500);
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(1);
	display.setCursor(10, 10);
	//display.clearDisplay();
	//display.setTextSize(1);
	//display.setCursor(5, 5);
	display.print("Hallo Davin");
	display.display();

	//ports//
	DDRB |= (7 << 0);
	DDRD |= (120 << 0); //pin7,6,5,4 as output
	PORTC |= (15 << 0); //pull ups to pins A0~A3
}


void loop() {
	if (millis() - SWcount > 10) {
		SWcount = millis();
		SW_exe();
	}


	/*

	if (millis()-tijd > 1000) {
		tijd = millis();	
		PORTB &= ~(15 << 0);
		PORTB |= (1 << ledcount);
			
		//for (byte i = 0; i < 3; i++) {
			//digitalWrite(8 + i, LOW);
		//}
		//PORTB |=(B00000111 << 0);
		//PINB |= (1 << ledcount);
		ledcount++;
		if (ledcount > 2)ledcount = 0;
	}

*/
}
void SW_exe() {	
	byte status = 0; byte changed = 0;
	PORTD &= ~(15 << 0);//clear row pins
	PORTD |= (1 << rowcount);
	status = PINC;



}
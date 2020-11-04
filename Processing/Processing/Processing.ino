/*
 Name:		Processing.ino
 Created:	11/1/2020 12:10:15 PM
 Author:	Rob Antonisse

 Sketch voor een interface tussen arduino en Processing
*/

//libraries
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <splash.h>
#include <Adafruit_SSD1306.h>

//constanten


//constructors
Adafruit_SSD1306 display(128, 64, &Wire, -1);

//variables
byte received[2];
byte receivedcount = 3;
unsigned long swTime;
byte swStatus = 0xFF;
byte SER_fase = 0;

byte inPar;
byte inValue;
byte inByte;
byte countRX;

void setup() {
	Serial.begin(9600);
	delay(500);
	PORTC |= (15 << 0); //pins A0~A3 pullup
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	DP_start();
	display.setTextSize(1);
	display.print("System ready....");
	display.drawRect(1, 1, 127, 63, WHITE);
	DP_end();
	SER_write(0xFF,0x00); //send wakeup call
	countRX = 0;
}

void SER_read() {
	while (Serial.available()>0) {
		inByte = Serial.read();
		if (inByte == 0xFF) {
			countRX = 1;
		}
		else {
			switch (countRX) {
			case 0: //doe niks start ontvangst 
				break;
			case 1: //ontvang parameter max 254
				inPar = inByte;
				countRX++;
					break;
			case 2: //ontvang value max 254
				inValue = inByte;
				countRX = 0;
				SER_rx();
				break;
			}
		}
	}	

}

void SER_rx() {
		DP_start();
		display.setCursor(10, 10);
		display.print("Control: "); display.print(inPar);
		display.setCursor(10, 30);
		display.print("Value: "); display.print(inValue);
		DP_end();
}


void SER_write(byte para,byte value) {
	Serial.write(0xFF); //Serial.write(state);
	Serial.write(para);
	Serial.write(value);
}


void DP_start() { //stelt display in
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(1);
	display.setCursor(10, 10);
}

void DP_end() { //updates display
	
	display.drawRect(1, 1, 127, 63, WHITE);
	display.display();
}


void SW_exe() {
	byte swNewStatus;
	byte changed;
	swNewStatus = PINC;
	changed = swNewStatus ^ swStatus;
	if (changed > 0) {
		for (byte i = 0; i < 4; i++) {
			if (changed & (1 << i)) {
				if (swNewStatus & (1 << i)) { //button released
					SW_output(i, false);
				}
				else { //button pressed
					SW_output(i, true);
				}
			}
		}
	}
	swStatus = swNewStatus;
}
void SW_output(byte sw, boolean state) {
	SER_write(sw,state);
}

void loop() {
	if (millis() - swTime > 20) {
		swTime = millis();
		SW_exe();
	SER_read();
	}
}

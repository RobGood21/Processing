/*
 Name:		BrideExit.ino
 Created:	1/4/2021 2:16:15 PM
 Author:	Rob Antonisse

 Exit number puzzel. To entry the correct number wil release doorlock
*/


//libraries

#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <splash.h>
#include <Adafruit_SSD1306.h>

#define dp GPIOR0 |=(1<<0); //request display.display

//declaraties
Adafruit_SSD1306 display(128, 64, &Wire, -1);

byte kolom[4];
byte rowcount;
unsigned long SWcount;

byte code[6]; //entry code can be changed 0=msb 5=lsb
byte defaultcode[6] = { 0,4,5,5,2,1 };
byte prgcode1[6]; //hard coded *6A731#  *=14  #=18 A=10 C=12
byte prgcode2[6]; //hard coded *657C2#
byte codeinput[6] = { 20,20,20,20,20,20}; //ingevoerde code
byte prgmode;

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
//delay(500);
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(1);
	display.setCursor(10, 10);
	display.print("Hallo Davin");
	dp;
	//display.display();

	//ports//
	DDRB |= (15 << 0);
	PORTB |= (1 << 3); //relais off
	DDRD |= (240 << 0); //pin7,6,5,4 as output


	MEM_read();


	//other initialise
	for (byte i = 0; i < 4; i++) {
		kolom[i] = 0xFF;
	}
	DP_exe();
}
void MEM_read() {
	for (byte i = 0; i < 6; i++) {
		code[i] = EEPROM.read(100 + i);
		if (code[i] > 16)code[i] = defaultcode[i];
	}
}
void loop() {
	if (millis() - SWcount > 10) {
		SWcount = millis();
		SW_exe();
		if (GPIOR0 &= (1 << 0)) {
			display.display();
			GPIOR0 &= ~(1 << 0);
		}
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
	//	byte a;
	byte status = 0; byte changed = 0;

	//Serial.print(status,BIN);
	status = PINC;
	status = status << 4;
	status = status >> 4;
	changed = status ^ kolom[rowcount];
	if (changed > 0) {
		//Serial.print("*");
		for (byte i = 0; i < 4; i++) {
			if (changed & (1 << i)) {
				if (status & (1 << i)) { //ingedrukt
					SW_on(i + (rowcount * 4));
				}
				else { //losgelaten
					//SW_off(i + (rowcount*4));
				}
			}
		}
	}

	kolom[rowcount] = status;
	rowcount++;
	if (rowcount > 3)rowcount = 0;

	PORTD &= ~(240 << 0);//clear row pins
	PORTD |= (1 << (7 - rowcount));

}
void SW_on(byte sw) {
	byte key;
	key = Keypress(sw);
	switch (key) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 10: //A
		ledoff();
		break;
	case 11:
		display.clearDisplay();
		display.display();
		break;
	case 14:
		PORTB |= (1 << 3);
		break;
	case 15:
		PORTB &= ~(1 << 3);
		break;
	}
	Serial.print("key: ");
	Serial.println(key);
}
byte Keypress(byte sw) {
	byte KP = 0;
	switch (sw) {
	case 0:
		KP = 13; //D
		break;
	case 1:
		KP = 15; //#
		break;
	case 2:
		KP = 0; //0
		break;
	case 3:
		KP = 14; //*
		break;
	case 4:
		KP = 12; //C
		break;
	case 5:
		KP = 9; //*
		break;
	case 6:
		KP = 8;
		break;
	case 7:
		KP = 7;

		break;
	case 8:
		KP = 11; //B
		break;
	case 9:
		KP = 6;
		break;
	case 10:
		KP = 5;
		break;
	case 11:
		KP = 4;
		break;
	case 12:
		KP = 10; //A
		break;
	case 13:
		KP = 3;
		break;
	case 14:
		KP = 2;
		break;
	case 15:
		KP = 1;
		break;
	}
	return KP;
}
void SW_off(byte sw) {
	Serial.print("Uit: ");
	Serial.println(sw);
}
void rood() {
	PORTB &= ~(6 << 0);
	PORTB |= (1 << 0);
}
void groen() {
	PORTB &= ~(5 << 0);
	PORTB |= (1 << 1);
}
void blauw() {
	PORTB &= ~(3 << 0);
	PORTB |= (1 << 2);
}
void ledoff() {
	PORTB &= ~(7 << 0);
}
void DP_exe() {
	DP_regeltop();
	display.print(F("Enter code to exit"));
	for (byte i = 0; i < 6; i++) {
		switch (codeinput[i]) {
		case 10:
			break;
		case 11:
			break;
		case 12:
			break;
		case 13:
			break;
		case 14:
			break;
		case 15:
			break;
		case 20: //no input
			display.drawLine(10 + (i * 20), 45, 20 + (i * 20), 45, WHITE);
			break;
		default: //0~9

			break;
		}
	}
	dp;
}
void DP_regeltop() {
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setCursor(8, 5);
	display.setTextSize(1);
}

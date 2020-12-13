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

byte MEM_reg;
byte shiftbyte[4]; //0=game2 1=game1 2=segments 3=digits
byte segmentcount;
byte bytecount;
byte bitcount;
byte hourtimer;
byte hourcurrent;
byte minutecurrent;
byte minutetimer;
byte secondcurrent;


//temps
unsigned long tijd;
byte pixcount;
byte bc;


void setup() {
	Serial.begin(9600);

	FastLED.addLeds<NEOPIXEL, 4 >(pix, 24);
	FastLED.setBrightness(50);
	//ports
	DDRB |= (7 << 0); //pin8;9;10 as output SRCLK RCLK SH/LD
	PORTB |= (1 << 2); //pin 10 high SH/LD
	DDRD |= (1 << 6); //pin 6 output Serial out
	DDRD &= ~(1 << 7); //pin7 as input Serial in
	PORTD |= (1 << 7); //build-in pullup to pin 7 

	MEM_read();
	TIME_init();
}
void loop() {
	SFT_exe();
	if (millis() - tijd > 999) { //1 seconde? calibratie maken in instellingen?
		tijd = millis();
		FL_exe();
		if (GPIOR0 & (1 << 0))TIME_clock();
	}
}

void MEM_read() {
	MEM_reg = EEPROM.read(100);
	if (MEM_reg & (1 << 0))GPIOR0 |= (1 << 0); //start timer on powerup
	hourtimer = EEPROM.read(110);
	if (hourtimer > 23)hourtimer = 1;
	minutetimer = EEPROM.read(111);
	if (minutetimer > 59) minutetimer = 0;
}
void TIME_init() {
	//sets time of the timer 
	hourcurrent = hourtimer;
	minutecurrent = minutetimer;
	secondcurrent = 0;
}
void TIME_clock() {
	if (secondcurrent == 0) {
		if (hourcurrent + minutecurrent + secondcurrent == 0) GPIOR0 &= ~(1 << 0); //stop timer
		secondcurrent = 60;
		if (minutecurrent == 0) {
			minutecurrent = 60;
			if (hourcurrent == 0) {
				hourcurrent = 24;
			}
			hourcurrent--;
		}
		minutecurrent--;
	}
	secondcurrent--;


	Serial.print(hourcurrent); Serial.print(":"); Serial.print(minutecurrent);
	Serial.print(":"); Serial.println(secondcurrent);
}
byte segments(boolean digit) {
	byte segment;
	byte value = 0; byte dgt1 = 0; byte dgt2 = 0;
	//calc value of the 6 seperate 7 segments
	//seconds
	value = secondcurrent;
	while (value > 9) {
		dgt2++;
		value = value - 10;
	}
	//dgt2 heeft tientallen, dgt1 rest
	if (digit == true) {
		segment = sgmnt(dgt2);
	}
	else {
		segment = sgmnt(dgt1);
	}
	return segment;
}
byte sgmnt(byte number) {
	byte result;
	switch (number) {
	case 0:
		//result=
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		break;
	case 8:
		break;
	case 9:
		break;
	}

	return result;
}
void FL_exe() {
	pixcount++;
	FastLED.clearData();


	if (pixcount > aantalpix - 1) pixcount = 0;
	pix[pixcount].r = random(0, 255);
	pix[pixcount].g = random(0, 255);
	pix[pixcount].b = random(0, 255);
	fl;

	shiftbyte[2] = B11110000;
	switch (bc) {
	case 0:
		shiftbyte[3] = B01111111;
		break;
	case 1:
		shiftbyte[3] = B10111111;
		break;
	case 2:
		shiftbyte[3] = B11011111;
		break;
	case 3:
		shiftbyte[3] = B11101111;
		break;
	case 4:
		shiftbyte[3] = B11110111;
		break;
	case 5:
		shiftbyte[3] = B11111011;
		break;
	}
	bc++; if (bc > 5)bc = 0;
}
void SFT_exe() {
	//shift out continue and reads switches and game 
	//port D6 = serial out, port B0 pin 8 = shiftpuls port B1 Pin9 = latch sipo
	//pin10= latch piso (high>low)
	PORTD &= ~(1 << 6); //clear serial pin
	if (shiftbyte[bytecount] & (1 << bitcount))PORTD |= (1 << 6); //set serial pin
	PORTB |= (1 << 0); PINB |= (1 << 0); //make shift puls
	//hier lezen shiftout bit
	bitcount++;
	if (bitcount > 7) {
		bitcount = 0;
		bytecount++;
		if (bytecount > 3) {
			bytecount = 0;
			PORTB |= (1 << 1); PINB |= (1 << 1); //make latch puls sipo
			//shiftbytes klaarmaken voor volgende run
			segmentcount++;
			if (segmentcount > 5)segmentcount = 0;
			switch (segmentcount) {
			case 0:
				shiftbyte[3] = B01111111;
				shiftbyte[2] = segments(false);
				break;
			case 1:
				shiftbyte[3] = B10111111;
				shiftbyte[2] = segments(true);
				break;
			case 2:
				shiftbyte[3] = B11011111;
				shiftbyte[2] = segments(false);
				break;
			case 3:
				shiftbyte[3] = B11101111;
				shiftbyte[2] = segments(true);
				break;
			case 4:
				shiftbyte[3] = B11110111;
				shiftbyte[2] = segments(false);
				break;
			case 5:
				shiftbyte[3] = B11111011;
				shiftbyte[2] = segments(true);
				break;
			}
		}
	}
}
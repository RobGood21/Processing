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
byte digit[6];
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

	//temps
	//shiftbyte[3] = B11111011;

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
	if (hourtimer > 23)hourtimer = 24;
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
			TIME_segments(2);
		}
		minutecurrent--;
		TIME_segments(1);
	}
	secondcurrent--;
	TIME_segments(0);

	Serial.print(hourcurrent); Serial.print(":"); Serial.print(minutecurrent);
	Serial.print(":"); Serial.println(secondcurrent);
}
void TIME_segments(byte ts) {
	byte value; byte tens = 0;
	switch (ts) {
	case 0: //seconds
		value = secondcurrent;
		break;
	case 1://minutes
		value = minutecurrent;
		break;
	case 2://hours
		value = hourcurrent;
		break;
	}
	while (value > 9) {
		tens++;
		value = value - 10;
	}
	digit[0 + ts*2] = segment(value);
	digit[1 + ts*2] = segment(tens);
}

byte segment(byte number) {
	byte result;
	switch (number) {
	case 0:
		result = B11111100;
		break;
	case 1:
		result = B01100000;
		break;
	case 2:
		result = B11011010;
		break;
	case 3:
		result = B11110010;
		break;
	case 4:
		result = B01100110;
		break;
	case 5:
		result = B10110110;
		break;
	case 6:
		result = B10111110;
		break;
	case 7:
		result = B11100000;
		break;
	case 8:
		result = B11111110;
		break;
	case 9:
		result = B11110110;
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
			//next digit
			segmentcount++;
			if (segmentcount > 5)segmentcount = 0;
			shiftbyte[2] = digit[segmentcount];
			shiftbyte[3] = 0xFF;
			shiftbyte[3] &= ~(1 << 7-segmentcount);
		}
	}
}
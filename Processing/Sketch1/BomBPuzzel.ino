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

struct connect
{
	byte first = 0;
	byte second = 0;
	boolean cnt = false;
}; connect con[8];

struct colors
{
	byte red;
	byte green;
	byte blue;
	boolean free = false;
}; colors color[8];


byte pixcolor[16]; //witch color assigned to pix
byte MEM_reg;
byte shiftbyte[4]; //0=game2 1=game1 2=segments 3=digits
byte gamebyte[2];
byte gamebytecount = 0x00;
byte gamebit = 0;
byte digit[6];
byte segmentcount = 0;
byte bytecount = 0;
byte bitcount = 0;
byte hourtimer;
byte hourcurrent;
byte minutecurrent;
byte minutetimer;
byte secondcurrent;
byte slow;
byte SW_status = 0xFF;
byte rndm[16];

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
	PORTC |= (63 << 0); //pull ups to pins A0~A5

	MEM_read();
	TIME_init();
	COLOR_set();
	gamebyte[0] = 0; gamebyte[1] = 0;

	//temps
	//shiftbyte[3] = B11111011;

}
void loop() {
	slow++;
	if (slow == 0) SW_exe();
	SHIFT_exe();

	if (millis() - tijd > 999) { //1 seconde? calibratie maken in instellingen?
		tijd = millis();
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
void COLOR_set() {
	//color 0 Rood
	color[0].red = 0xFF;
	color[0].green = 0x10;
	color[0].blue = 0x10;
	//color 1 Blauw
	color[1].red = 0x10;
	color[1].green = 0x10;
	color[1].blue = 0xFF;
	//color 2 Groen
	color[2].red = 0x10;
	color[2].green = 0xFF;
	color[2].blue = 0x10;
	//color 3 Oranje
	color[3].red = 0xFF;
	color[3].green = 0x60;
	color[3].blue = 0x00;
	//color 4 Paars
	color[4].red = 0xFF;
	color[4].green = 0x00;
	color[4].blue = 0xFF;
	//color 5 equal 
	color[5].red = 0x99;
	color[5].green = 0x30;
	color[5].blue = 0x40;
	//color 6 equal blue
	color[6].red = 0x30;
	color[6].green = 0x30;
	color[6].blue = 0xAA;
	//color 7 Yellow
	color[7].red = 0xAA;
	color[7].green = 0x99;
	color[7].blue = 0x00;

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

	//Serial.print(hourcurrent); Serial.print(":"); Serial.print(minutecurrent);
	//Serial.print(":"); Serial.println(secondcurrent);
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
	digit[0 + ts * 2] = segment(value);
	digit[1 + ts * 2] = segment(tens);
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
void SHIFT_exe() {
	//shift out continue and reads switches and game 
	//port D6 = serial out, port B0 pin 8 = shiftpuls port B1 Pin9 = latch sipo
	//pin10= latch piso (high>low)
	PORTD &= ~(1 << 6); //clear serial pin
	if (shiftbyte[bytecount] & (1 << bitcount))PORTD |= (1 << 6); //set serial pin


	//hier lezen shiftout bit
	switch (bytecount) {
	case 0:
		if (PIND & (1 << 7)) gamebyte[1] |= (1 << bitcount);
		break;
	case 1:
		if (PIND & (1 << 7)) gamebyte[0] |= (1 << bitcount);
		break;
	}

	PORTB |= (1 << 0); PINB |= (1 << 0); //make shift puls

	bitcount++;
	if (bitcount > 7) {
		bitcount = 0;
		bytecount++;


		switch (bytecount) {
		case 4: //alle bytes verzonden
			GAME_read();
			bytecount = 0;
			PORTB |= (1 << 1); PINB |= (1 << 1); //make latch puls sipo pin 9
			PORTB &= ~(1 << 2); PINB |= (1 << 2); //latch puls piso pin 10

			//next digit
			segmentcount++;
			if (segmentcount > 5)segmentcount = 0;
			shiftbyte[2] = digit[segmentcount];
			shiftbyte[3] = 0xFF;
			shiftbyte[3] &= ~(1 << 7 - segmentcount);

			//gamebytes			
			gamebit--;
			shiftbyte[gamebytecount] = shiftbyte[gamebytecount] << 1;
			if (shiftbyte[gamebytecount] == 0) {
				gamebytecount++;
				if (gamebytecount > 1) {
					gamebytecount = 0;
					gamebit = 16;
				}
				shiftbyte[gamebytecount] = 1;
			}
			break;
		}
	}
}
void GAME_read() {
	//Serial.print("gamebit: "); Serial.println(gamebit);
	//gamebytecount
	boolean nw = false;
	byte px1; byte px2;
	byte r[2]; byte d = 0;
	for (byte y = 0; y < 2; y++) {
		for (byte i = 0; i < 8; i++) {
			if (gamebyte[y] & (1 << i)) {
				r[d] = (7 - i) + (y * 8);
				d++;
			}
		}
	}
	if (d == 2) { //verbinding gevonden

		//Serial.print("gamebit: "); Serial.println(gamebit);
		if (r[0] == gamebit) {
			//Serial.println("yes, r[0]");
		}
		else if (r[1] == gamebit) {
			//Serial.println("no, r[1]");
		}
		else {
			//Serial.println("no match");
		}

		GPIOR0 |= (1 << 2);
		for (byte i = 0; i < 8; i++) { //check if connection exists met r[0]
			if (r[0] == con[i].first) {
				con[i].second = r[1];
				con[i].cnt = true;
				GPIOR0 &= ~(1 << 2);
				i = 10;
			}
		}

		if (GPIOR0 & (1 << 2)) { //new connection
			for (byte i = 0; i < 8; i++) {
				if (con[i].cnt == false) {
					con[i].first = r[0];
					con[i].second = r[1];
					con[i].cnt = true;
					i = 10;
				}
			}
		}
		//Serial.print("verbinding tussen: "); Serial.print(r[0]); Serial.print(" en: "); Serial.println(r[1]);
	}
	//Serial.print(gamebyte[0]); Serial.print("  "); Serial.println(gamebyte[1]);
	gamebyte[0] = 0x00; gamebyte[1] = 0x00;
	//execute and clear results
	//Serial.println(gamebit);
	if (gamebit == 16) {
		if (GPIOR0 & (1 << 3)) { //game setup
			GAME_start();
		}
		else { //game running
			FastLED.clearData();
			for (byte i = 0; i < 8; i++) {
				px1 = con[i].first; px2 = con[i].second;

				if (con[i].cnt == true) {
					pix[px1 + 8] = CRGB(color[pixcolor[px1]].red, color[pixcolor[px1]].green, color[pixcolor[px1]].blue);
					pix[px2 + 8] = CRGB(color[pixcolor[px2]].red, color[pixcolor[px2]].green, color[pixcolor[px2]].blue);
					//Serial.print("*");
				}
				con[i].first = 0;
				con[i].second = 0;
				con[i].cnt = false;
			}
			fl;
		}
	}
}

void GAME_start() {
	byte num1; byte num2; byte val;
	Serial.println("game start");
	//makes new game
	//clear all assigned colors
	for (byte i = 0; i < 16; i++) {
		pixcolor[i] = 0xFF;
	}
	//assign random colors to pix
	for (byte i = 0; i < 16; i++) {
		rndm[i] = i;
		Serial.print(rndm[i]); Serial.print("  ");
	}
	Serial.println("");
	for (byte i = 0; i < 100; i++) {
		num1 = random(0, 16);
		num2 = random(0, 16);
		val = rndm[num1];
		rndm[num1] = rndm[num2];
		rndm[num2] = val;
	}
	for (byte i = 0; i < 16; i++) {
		Serial.print(rndm[i]); Serial.print("  ");
	}
	Serial.println("");

	for (byte v = 0; v < 8; v++) {
		for (byte i = 0; i < 2; i++) {
			pixcolor[rndm[v + (i * 8)]] = v;
		}
	}




	for (byte i = 0; i < 16; i++) {
		Serial.print(pixcolor[i]); Serial.print(" ");


		pix[i + 8] = CRGB(color[pixcolor[i]].red, color[pixcolor[i]].green, color[pixcolor[i]].blue);
	}
	Serial.println("");

	fl;
	//delay(1000);
	GPIOR0 &= ~(1 << 3);
}
void SW_exe() {
	byte nss = PINC;
	byte changed;
	changed = nss ^ SW_status;
	if (changed > 0) {
		for (byte i = 0; i < 6; i++) {
			if (changed & (1 << i)) {
				if (nss & (1 << i)) {
					SW_off(i);
				}
				else {
					SW_on(i);
				}
			}
		}
	}
	SW_status = nss;
}
void SW_on(byte sw) {
	Serial.print("Aan: "); Serial.println(sw);
	switch (sw) {
	case 0:
		GPIOR0 |= (1 << 3); // start new game setup
		break;
	}
}
void SW_off(byte sw) {
	switch (sw) {
	case 4:
		GPIOR0 |= (1 << 3); // start new game setup
		break;
	}
}

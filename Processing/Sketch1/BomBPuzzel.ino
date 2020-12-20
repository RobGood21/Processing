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

unsigned int TIME_current;
unsigned int TIME_game;
unsigned int TIME_act;
unsigned int TIME_end;

byte code[6]; //code to exit escaperoom
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
byte SW_status = B11111111;
byte rndm[16];
unsigned long tijd; //timer clock
unsigned long ANIM_tijd; //timer animaties
byte ANIM_fase;
byte ANIM_count[6]; //tellers

byte PRG_mode;
byte PRG_memsw;

//temps

byte pixcount;
byte bc;


void setup() {
	Serial.begin(9600);

	FastLED.addLeds<NEOPIXEL, 4 >(pix, 24);
	FastLED.setBrightness(100);
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
	resetcounters();

	//auto gamestart, straks naar mem-read

	//GPIOR0 |= (24 << 0); //auto
	GPIOR0 |= (1 << 4); //man

	GPIOR1 |= (1 << 1); //enable clock show
	TIME_dp(); //display clock
	FastLED.clearData();
	fl;
}
void loop() {
	slow++;
	if (slow == 0) SW_exe();
	SHIFT_exe();
	if (millis() - tijd > 999) { //timer clock
		tijd = millis();
		if (GPIOR0 & (1 << 0))TIME_clock();
	}
}
void MEM_read() {
	byte hr; byte min;
	MEM_reg = EEPROM.read(100);
	if (MEM_reg & (1 << 0))GPIOR0 |= (1 << 0); //start timer on powerup	
	//instelbare tijden
	//game
	hourtimer = EEPROM.read(110);
	if (hourtimer > 12)hourtimer = 1;
	minutetimer = EEPROM.read(111);
	if (minutetimer > 59) minutetimer = 1;
	TIME_game = hourtimer * 360 + minutetimer * 60;  ///dit wordt denk ik niet gebruikt....?
	//activate
	min = EEPROM.read(111);
	if (min > 59)min = 10;
	TIME_act = min * 60;
	//endgame
	min = EEPROM.read(112);
	if (min > 59)min = 2;
	TIME_end = min * 60;
	//Serial.print("code: ");
	for (byte i = 0; i < 6; i++) {
		code[i] = EEPROM.read(i + 50);
		if (code[i] > 9)code[i] = i + 1;
		//Serial.print(code[i]);
	}
	//Serial.println(" ");

}
void COLOR_set() {
	//color 0 Rood
	color[0].red = 0xFF;
	color[0].green = 0x00;
	color[0].blue = 0x00;
	//color 1 Blauw
	color[1].red = 0x00;
	color[1].green = 0x00;
	color[1].blue = 0xFF;
	//color 2 Groen
	color[2].red = 0x00;
	color[2].green = 0xFF;
	color[2].blue = 0x00;
	//color 3 oranje
	color[3].red = 0xFF;
	color[3].green = 0x35;
	color[3].blue = 0x00;
	//color 4 geel
	color[4].red = 0xE0;
	color[4].green = 0xD0;
	color[4].blue = 0x00;
	//color 5 paars
	color[5].red = 0xFF;
	color[5].green = 0x00;
	color[5].blue = 0xA0;
	//color 6 
	color[6].red = 0x70;
	color[6].green = 0x70;
	color[6].blue = 0xAA;
	//color 7 mix1
	color[7].red = 0xF0;
	color[7].green = 0x25;
	color[7].blue = 0x30;

}
void TIME_init() {
	//sets time of the timer 
	hourcurrent = hourtimer;
	minutecurrent = minutetimer;
	secondcurrent = 0;
}
void TIME_clock() {

	if (secondcurrent == 0) {
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

	TIME_current = hourcurrent * 360 + minutecurrent * 60 + secondcurrent;
	if (TIME_end > TIME_current) GAME_end();  //21-12 dit checken


	if (TIME_current == 0)GAME_stop();

	//Serial.print(hourcurrent); Serial.print(":"); Serial.print(minutecurrent);
	//Serial.print(":"); Serial.println(secondcurrent);
}
void TIME_dp() { //displays the timer
	for (byte i = 0; i < 3; i++) {
		TIME_segments(i);
	}
}
void TIME_segments(byte ts) {
	byte value; byte tens = 0;
	if (GPIOR1 &(1 << 1)) {//show clock
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
	GPIOR1 ^= (1 << 0);
	if (GPIOR1 & (1 << 0)) {
		digit[2] |= (1 << 0);
		digit[4] &= ~(1 << 0);
	}
	else {
		digit[2] &= ~(1 << 0);
		digit[4] |= (1 << 0);
	}
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
	case 10: //b
		result = B00111110;
		break;
	case 11: //y
		result = B01110110;
		break;
	case 12: //p
		result = B11001110;
		break;
	case 13: //a
		result = B11111010;
		break;
	case 14://s
		result = B10110110;
		break;
	case 15: //r
		result = B00001010;
		break;
	case 16://e
		result = B10011110;
		break;
	case 17: //o
		result = B00111010;
		break;
	case 18://t
		result = B00011110;
		break;
	case 19: //G
		result = B10111110;
		break;
	case 20: //U
		result = B01111100;
		break;
	case 21: //C
		result = B10011100;
		break;
	case 22://d
		result = B01111010;
		break;
	case 23:
		result = B00000010;
		break;
	case 100:
		result = 0;
		break;


	}
	return result;
}
void TIME_txt(byte txt) {
	switch (txt) {
	case 0: //off
		for (byte i = 0; i < 6; i++) {
			digit[i] = 0x00;
		}
		break;
	case 1://bypass
		digit[0] = segment(14);
		digit[1] = segment(14);
		digit[2] = segment(13);
		digit[3] = segment(12);
		digit[4] = segment(11);
		digit[5] = segment(10);
		break;
	case 2: //reboot
		digit[0] = segment(18);
		digit[1] = segment(17);
		digit[2] = segment(17);
		digit[3] = segment(10);
		digit[4] = segment(16);
		digit[5] = segment(15);
		break;
	case 3: //Escape
		digit[0] = segment(16);
		digit[1] = segment(12);
		digit[2] = segment(13);
		digit[3] = segment(21);
		digit[4] = segment(14);
		digit[5] = segment(16);
		break;
	case 4://Code
		digit[0] = segment(100);
		digit[1] = segment(16);
		digit[2] = segment(22);
		digit[3] = segment(0);
		digit[4] = segment(21);
		digit[5] = segment(100);
		break;
	case 5: //exit code
		digit[0] = segment(code[5]);
		digit[1] = segment(code[4]);
		digit[2] = segment(code[3]);
		digit[3] = segment(code[2]);
		digit[4] = segment(code[1]);
		digit[5] = segment(code[0]);
		break;
	case 6: //start program mode, clear segments 
		for (byte i = 0; i < 6; i++) {
			digit[i] = segment(23);
		}
		break;
	}

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

			if (millis() - ANIM_tijd > 50) { //timer animaties x50ms
				ANIM_exe();
				ANIM_tijd = millis();
			}
			if (~GPIOR0 & (1 << 6)) GAME_read(); //game stops in end play

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
void GAME_read() { //leest de verbindingen, called shift_exe
	boolean nw = false;
	byte px1; byte px2;
	byte r[2]; byte d = 0; byte cc = 0;

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
		if (GPIOR0 & (1 << 3)) { //make new kleuren game
			GAME_start();
		}
		else {
			FastLED.clear();
			//count correct connections
			for (byte i = 0; i < 8; i++) {
				if (con[i].cnt) {
					if (pixcolor[con[i].first] == pixcolor[con[i].second]) {
						cc++;						
					}
				}
				//set pixels 0~7 rood
				pix[i] = CRGB(200, 3, 3);
			}
			if (GPIOR0 & (1 << 5)) {
				if (cc > 0) {
					GPIOR0 |= (1 << 3); //nieuw schema maken
				}
				else {
					GPIOR0 &= ~(1 << 5);
				}
			}

			for (byte c = 0; c < cc; c++) {
				pix[c] = CRGB(3, 200, 3);
			}
			//hier ergens de schakelfunctie voor programming
			if (PRG_mode > 0) {
				if (cc == 0)PRG_memsw = 0;
				if (cc == 1) PRG_sw();//Serial.println("1 verbinding");
			}


			for (byte i = 0; i < 8; i++) {
				px1 = con[i].first; px2 = con[i].second;

				if (con[i].cnt == true) {// &GPIOR0 & (1 << 4)) {
					pix[px1 + 8] = CRGB(color[pixcolor[px1]].red, color[pixcolor[px1]].green, color[pixcolor[px1]].blue);
					pix[px2 + 8] = CRGB(color[pixcolor[px2]].red, color[pixcolor[px2]].green, color[pixcolor[px2]].blue);
					//Serial.print("*");
				}
				con[i].first = 0;
				con[i].second = 0;
				con[i].cnt = false;
			}
			//Serial.println(cc);

			if (~GPIOR0 & (1 << 4)) {
				fl;
				if (cc == 8) GAME_end();
			}
		}
	}
}
void GAME_end() { //color puzzle solved, in 0pbouw knop2 on
	if (~GPIOR0 & (1 << 6)) { //
		//set time, no display
		hourcurrent = 0;
		minutecurrent = 2;// TIME_end / 60;
		secondcurrent = 30;
		GPIOR0 |= (1 << 4);
		GPIOR0 |= (1 << 6); //flag eindspel
		ANIM_fase = 10;
		resetcounters();
	}
}
void resetcounters() {
	for (byte i = 0; i < 6; i++) {
		ANIM_count[i] = 0;
	}
}
void GAME_start() {
	byte num1; byte num2; byte val;
	//Serial.println("game start");
	//makes new game
	//clear all assigned colors
	for (byte i = 0; i < 16; i++) {
		pixcolor[i] = 0xFF;
	}
	//assign random colors to pix
	for (byte i = 0; i < 16; i++) {
		rndm[i] = i;
		//Serial.print(rndm[i]); Serial.print("  ");
	}
	//Serial.println("");
	for (byte i = 0; i < 100; i++) {
		num1 = random(0, 16);
		num2 = random(0, 16);
		val = rndm[num1];
		rndm[num1] = rndm[num2];
		rndm[num2] = val;
	}
	for (byte i = 0; i < 16; i++) {
		//Serial.print(rndm[i]); Serial.print("  ");
	}
	//Serial.println("");

	for (byte v = 0; v < 8; v++) {
		for (byte i = 0; i < 2; i++) {
			pixcolor[rndm[v + (i * 8)]] = v;
		}
	}

	//toon kleuren
	//for (byte i = 0; i < 16; i++) {
		//Serial.print(pixcolor[i]); Serial.print(" ");
	//	pix[i + 8] = CRGB(color[pixcolor[i]].red, color[pixcolor[i]].green, color[pixcolor[i]].blue);
	//}

	//Serial.println("");
	GPIOR0 &= ~(1 << 3); //einde game start
	GPIOR0 |= (1 << 5); //check for connections

	// animatie starten
	ANIM_fase = 1;
	ANIM_count[0] = 0;
}
void GAME_stop() {
	//timer afgelopen op 0
	GPIOR0 &= ~(1 << 0);
	Serial.println("clock stop");
	ANIM_fase = 30;
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
		GPIOR0 |= (1 << 4);
		break;
	case 1:
		GPIOR0 |= (1 << 4);
		FastLED.clear();
		for (byte i = 0; i < 8; i++) {
			pix[i] = CRGB(color[i].red, color[i].green, color[i].blue);
		}
		fl;
		break;

	case 2:
		GAME_end();
		break;
	case 3:
		PRG_start();
		break;
	case 5:
		//PRG_start();
		break;
	}
}
void SW_off(byte sw) {
	Serial.print("Uit: "); Serial.println(sw);
	switch (sw) {
	case 1:
		FastLED.clear();
		fl;
		break;
	case 3:
		PRG_stop();
		break;
	case 4:
		//GPIOR0 |= (24 << 0); // start new game setup
		break;
	case 5:
		//PRG_stop();
		break;
	}
}
void ANIM_exe() {
	ANIM_count[0] ++; //1 = 20ms
	switch (ANIM_fase) {
	case 0:

		break;
	case 1:
		FastLED.clear();
		if (ANIM_count[0] > 4) { //timer 4x50ms
			ANIM_count[0] = 0;

			if (ANIM_count[1] > 7) {
				ANIM_count[1] = 0;
				ANIM_fase = 0;
				GPIOR0 &= ~(1 << 4); //start connection show colors				
			}
			else {
				//Serial.println(ANIM_count[1]);
				for (byte i = 0; i < 16; i++) {

					if (pixcolor[i] == ANIM_count[1]) {
						pix[i + 8] = CRGB(color[ANIM_count[1]].red, color[ANIM_count[1]].green, color[ANIM_count[1]].blue);
						fl;
					}
				}
				ANIM_count[1] ++;
			}
		}
		break;

	case 10:
		ANIM_count[0]++;
		if (ANIM_count[0] > 50) { // pauze na oplossen puzzel
			ANIM_count[0] = 0;
			FastLED.clear();
			fl;
			GPIOR1 &= ~(1 << 1); //disable clock
			TIME_txt(0); //off
			ANIM_fase = 12;
			ANIM_count[4] = 60; //blackout time
		}
		break;
	case 12:
		if (ANIM_count[0] > ANIM_count[4]) {
			ANIM_count[0] = 0;
			ANIM_count[1]++;
			TIME_txt(ANIM_count[1]); //1=bypass 2=reboot 3=getout 4=code 5=exitcode

			//eenmalig
			switch (ANIM_count[1]) { //tijden
			case 1:
				ANIM_count[4] = 40; //bypass
				break;
			case 2:
				ANIM_count[4] = 65; //reboot
				break;
			case 3:
				ANIM_count[4] = 80; //Escape
				break;
			case 4:
				ANIM_count[4] = 30; //Code
				FastLED.clear();
				for (byte i = 0; i < 25; i++) {
					pix[i] = CRGB(1, 5, 1);
				}
				fl;
				break;
			case 5:
				ANIM_count[4] = 60; //toon exit code
				break;
			case 6:
				ANIM_fase = 20;
				ANIM_count[0] = 0;
				ANIM_count[4] = 5;
				break;
			}
		}

		//iedere 50ms
		switch (ANIM_count[1]) { //leds animation
		case 2: //reboot
			FastLED.clear();
			pix[ANIM_count[3]] = CRGB::Red;
			ANIM_count[3]++;
			if (ANIM_count[3] > 24)ANIM_count[3] = 0;
			fl;
			break;

		case 3://EScape
			ANIM_count[3] ++;
			if (ANIM_count[3] > 3) {
				ANIM_count[3] = 0;
				GPIOR1 ^= (1 << 2);
				FastLED.clear();
				if (GPIOR1 & (1 << 2)) {
					for (byte i = 0; i < 25; i++) {
						pix[i] = CRGB(255, 0, 0);
					}
				}
				fl;
			}
			break;

		case 4: //code
			break;
		case 5://display code
			break;
		}
		break;

	case 20: //display clock
		if (ANIM_count[0] > ANIM_count[4]) {
			GPIOR1 |= (1 << 1); //enable display clock
			TIME_dp();
			ANIM_fase = 21;
			ANIM_count[4] = 100; //interval
			ANIM_count[0] = 0;
			ANIM_count[3]++;
			for (byte i = 0; i < 25; i++) {
				pix[i] = CRGB(200, 0, 0);
			}
			fl;
		}
		break;
	case 21: //display code
		if (ANIM_count[0] > ANIM_count[4]) {
			GPIOR1 &= ~(1 << 1); //disable display clock
			TIME_txt(5);
			ANIM_fase = 20;
			ANIM_count[4] = 15; //interval
			ANIM_count[0] = 0;
			for (byte i = 0; i < 25; i++) {
				pix[i] = CRGB(0, 30, 0);
			}
			fl;
		}
		break;
	case 30:
		if (ANIM_count[0] > 220) {
			ANIM_fase = 31;
			for (byte i = 0; i < 25; i++) {
				pix[i] = CRGB(0, 30, 0);
			}
			fl;
			TIME_txt(5);
		}

		break;
	case 31:
		break;
	}

}
void PRG_sw() {
	byte sw;
	//berekend de gemaakte connectie tbv program
	for (byte i = 0; i < 8; i++) {
		if (con[i].cnt) {
			if (con[i].first == 0) {
				sw = con[i].second;
			}
			else if (con[i].second == 0) {
				sw = con[i].first;
			}
		}
	}
	if (sw != PRG_memsw) {
		Serial.print("connect met "); 
		Serial.println(sw);
	}
	PRG_memsw = sw;
}
void PRG_start() {
	//enters program mode
	GPIOR0 &= ~(1 << 0); //disable clock
	TIME_txt(6); //clear display
	FastLED.clear();
	fl;
	PRG_mode = 1;
}
void PRG_stop() {
	//stops program mode 
	GPIOR0 |= (1 << 0); //enable clock
	PRG_mode = 0;
}

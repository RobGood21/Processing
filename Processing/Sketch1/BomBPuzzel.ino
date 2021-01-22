/*
 Name:		BP.ino
 Created:	12/11/2020 11:05:16 AM
 Author:	Rob antonisse

 Sketch for EscapeWorld.nl The Missing Bride puzzle BomB

 15jan2021
 Smart leds op uiteindelijk puzzel zitten in omgekeerde volgorde, daarom bij iedere redefinitie van pixels "23- " toegevoegd.

 22jan2021
 Groot probleem met statische ontladingen op de banana entrees. 
 Opgelost door hardware koppelen met optocouplers.
 Gebleken dat de TLP281-4 te langzaam uitdoofde daarom het lezen van de gamebits, de banana entrees dus, niet in iedere shiftactie te doen, maar om en om. 
 Dit is de GPIOR2 bit4. Schijnt de oplossing te zijn. Verder weerstanden veranderd naar arrays voor meer ruimte.


*/

//libraries
#include <EEPROM.h>
#include <FastLed.h>

//declarations
#define fl GPIOR1 |=(1<<7); //request Fastled
#define aantalpix 25
#define aantalled 1

CRGB pix[aantalpix];
CRGB led[aantalled];

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
byte TIME_gamehr;
byte TIME_gamemin;

//times in seconds
unsigned int TIME_game;
unsigned int TIME_act;
unsigned int TIME_end;

//byte BEEP_mode = 2; //tikking mode
unsigned long BEEP_stop;
unsigned int BEEP_length;
byte ledstatus = 0; //bit 0=groen; bit 1= oranje bit 2=rood
unsigned long PIEP_time;
int PIEP_count[4];
int PIEP_periode;
byte PIEP_vol;
byte TUP; //aantal keren dat tijd kan worden verhoogd
byte TUP_count;
byte code[6]; //code to exit escaperoom
byte defaultCD[6];
byte pixcolor[16]; //witch color assigned to pix
byte MEM_reg;
byte shiftbyte[4]; //0=game2 1=game1 2=segments 3=digits
byte gamebyte[2];
byte gamebytecount = 0x00;
byte gamebit = 0;
byte grs; ///GAME READ SPEED************************************************
byte digit[6];
byte segmentcount = 0;
byte bytecount = 0;
byte bitcount = 0;
byte hourtimer;
byte hourcurrent;
byte minutecurrent;
byte minutetimer;
byte secondcurrent;
byte ANIM_speed;
unsigned long slowtime;
byte SW_status;
byte rndm[16];
unsigned long tijd; //timer clock
unsigned long ANIM_tijd; //timer animaties
byte ANIM_fase;
byte ANIM_count[6]; //tellers
byte LED_count[3];
byte LED_fase;
byte PRG_mode;
byte PRG_memsw;

//temps
int tempcount;
byte pixcount;
byte bc;


void setup() {
	Serial.begin(9600);
	FastLED.addLeds<WS2811, 4, RGB >(pix, aantalpix);
	FastLED.addLeds<NEOPIXEL, 5 >(led, aantalled);

	FastLED.setBrightness(200);
	//ports
	delay(200);
	DDRB |= (15 << 0); //pin8;9;10,11 as output SRCLK RCLK SH/LD
	PORTB |= (1 << 3);//pin11, BEEP high. 

	PORTB |= (1 << 2); //pin 10 high SH/LD
	DDRD |= (1 << 6); //pin 6 output Serial out
	DDRD &= ~(1 << 7); //pin7 as input Serial in
	PORTD |= (1 << 7); //build-in pullup to pin 7 
	PORTC |= (63 << 0); //pull ups to pins A0~A5

	//set interrupt for BEEP
	TCCR2A = 0x00; // B01000010;
	TCCR2B = 0x00;
	TCCR2B |= (1 << 2);//prescaler	
	//TIMSK1 |= (3 << 0);	*****DEZE NIET AANZETTEN***
	GPIOR0 = 0;//reset general purpose registers
	GPIOR1 = 0;
	GPIOR2 = 0;
	SW_status = 0xFF;
	MEM_read();
	TIME_init();
	COLOR_set();

	//other initialisations
	//LED_fase = 1;

	PRG_mode = 0;
	TUP_count = 0;
	PIEP_periode = 500;
	gamebyte[0] = 0; gamebyte[1] = 0;
	resetcounters();
	GPIOR0 |= (1 << 4); //disable color game, switch operation
	GPIOR1 |= (1 << 1); //enable clock show
	GPIOR2 |= (1 << 2); //disable fastled 

	TIME_dp(); //display clock
	//Serial.println("setup");
	//ANIM_fase = 00; //
	ledstatus = 7;
	//LED_fase = 10; //control lights
	FastLED.clear();
	fl;
}


void loop() {
	SHIFT_exe();
	//****
	if (millis() - slowtime > 10) {
		slowtime = millis();
		SW_exe();
		if (GPIOR1 & (1 << 7)) {
			if (GPIOR2 & (1 << 2))FastLED.clear();

			//led[0] = CRGB(0x0);
			if (ledstatus & (1 << 0))led[0].r = 255; //oranje
			if (ledstatus & (1 << 1))led[0].g = 255; //rood
			if (ledstatus & (1 << 2))led[0].b = 255; //groen

			FastLED.show();


			GPIOR1 &= ~(1 << 7);
		}
	}
	//***

	if (GPIOR0 & (1 << 0)) {
		if (MEM_reg & (1 << 2) && GPIOR1 & (1 << 5)) {
			if (millis() - PIEP_time > 10) { //timer 11 ms
				PIEP_time = millis();
				PIEP_on();
			}
		}


		if (millis() - tijd > 999) { //timer clock
			tijd = millis();
			TIME_clock();
		}
	}

	//****
	if (GPIOR1 & (1 << 3))TIK_off();

}
void FACTORY() {
	//resets EEPROM
	for (byte i = 0; i < 255; i++) {
		EEPROM.update(i, 0xFF);
	}
	delay(200);
	setup();
}
void MEM_read() {
	byte hr; byte min; byte sec;
	MEM_reg = EEPROM.read(100);
	if (MEM_reg & (1 << 0))GPIOR0 |= (1 << 0); //start timer on powerup	

	hourtimer = EEPROM.read(110);
	if (hourtimer > 9)hourtimer = 1; //default 1 hour
	TIME_gamehr = hourtimer;
	minutetimer = EEPROM.read(111);
	if (minutetimer > 59) minutetimer = 0;
	TIME_gamemin = minutetimer;

	TIME_game = hourtimer * 360 + minutetimer * 60;  ///dit wordt denk ik niet gebruikt....?

	//activate
	min = EEPROM.read(112);
	if (min > 59)min = 10; //default 10minutes
	TIME_act = min * 60;
	//endgame
	min = EEPROM.read(113);
	sec = EEPROM.read(114);
	if (min > 59)min = 2; //default 2 minutes
	if (sec > 59)sec = 0;
	TIME_end = (min * 60) + sec;
	//animatie snelheid 
	ANIM_speed = EEPROM.read(115);
	if (ANIM_speed > 120)ANIM_speed = 60;

	defaultCD[0] = 0;
	defaultCD[1] = 4;
	defaultCD[2] = 5;
	defaultCD[3] = 5;
	defaultCD[4] = 2;
	defaultCD[5] = 1;

	for (byte i = 0; i < 6; i++) {
		code[i] = EEPROM.read(i + 50);
		if (code[i] > 9) code[i] = defaultCD[i];
		//Serial.print(code[i]);
	}
	TUP = EEPROM.read(116);
	if (TUP > 20)TUP = 10;

	//Serial.println(" ");
}

void MEM_write() {
	EEPROM.update(100, MEM_reg);

	int  minutes = 0;; int seconds = 0;
	//tijden
	EEPROM.update(110, TIME_gamehr);
	EEPROM.update(111, TIME_gamemin);
	EEPROM.update(112, TIME_act / 60);
	seconds = TIME_end;
	while (seconds > 59) {
		minutes++;
		seconds = seconds - 60;
	}
	EEPROM.update(113, minutes);
	EEPROM.update(114, seconds);
	//deurcode opslaan
	for (byte i = 0; i < 6; i++) {
		EEPROM.update(50 + i, code[i]);
	}
	//animatie snelheid
	EEPROM.update(115, ANIM_speed);
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
	color[7].green = 0x35;
	color[7].blue = 0x30;

}
void TIME_init() {
	//sets time of the timer 
	hourcurrent = hourtimer;
	minutecurrent = minutetimer;
	secondcurrent = 0;
}
void TIME_clock() {

	if (~GPIOR1 & (1 << 6)) { //alleen als buzzer vrij is
		if (MEM_reg & (1 << 1)) TIK_on(); //Hier kan nog een instelling komen
	}
	ledstatus ^= (1 << 2); //knipper green led 

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

	if (TIME_end == TIME_current) GAME_end();  //21-12 dit checken, pas op voor dubbel callen als puzzel te laat wordt opgelost??
	if (TIME_act == TIME_current + 4) ACT_exe(false);

	ledstatus &= ~(1 << 0);
	ledstatus &= ~(1 << 1);
	if (hourcurrent == 0) {
		if (TIME_act > TIME_current)ledstatus |= (1 << 0); //set oranje controlled
		if (TIME_end + 60 > TIME_current)ledstatus |= (1 << 1); //set red control led
	}
	if (TIME_current == 0)GAME_stop();
	if (ANIM_fase == 0) fl; //*******************
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
	case 23://-
		result = B00000010;
		break;
	case 100:
		result = 0;
		break;


	}
	return result;
}
void TIME_txt(byte txt) {
	byte tens = 0; int units = 0; byte seconds = 0;
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
			digit[i] = 0;
		}
		break;
	case 7: //program mode 1 (1t) looptijd spel, main timer bij power up
		digit[5] = segment(1);
		digit[4] = segment(18);
		digit[3] = segment(0);
		digit[2] = segment(TIME_gamehr);
		units = TIME_gamemin; // 60;
		while (units > 9) {
			units = units - 10;
			tens++;
		}
		digit[1] = segment(tens);
		digit[0] = segment(units);
		break;
	case 8: //program mode 2 (2t) activatie tijd, alleen minuten instelbaar
		digit[5] = segment(2);
		digit[4] = segment(18);
		digit[3] = segment(23);
		digit[2] = segment(23);
		units = TIME_act / 60;
		while (units > 9) {
			units = units - 10;
			tens++;
		}
		digit[1] = segment(tens);
		digit[0] = segment(units);

		break;
	case 9: //program mode 3 (3t) eindspeltijd minuten en seconden instelbaar
		digit[5] = segment(3);
		digit[4] = segment(18);
		units = TIME_end;
		while (units > 59) {
			tens++;
			units = units - 60;
		}
		//units aantal seconden; tens=aantal minuten
		seconds = units;
		units = tens;
		tens = 0;
		while (units > 9) {
			tens++;
			units = units - 10;
		}
		digit[3] = segment(tens);
		digit[2] = segment(units);
		units = 0;
		tens = 0;
		while (seconds > 9) {
			tens++;
			seconds = seconds - 10;
		}
		digit[1] = segment(tens);
		digit[0] = segment(seconds);
		break;
	case 10: //deurcode
		for (byte i = 0; i < 6; i++) {
			digit[i] = segment(code[5 - i]);
		}
		break;
	case 11: //Animatie speed Spd
		digit[5] = segment(14);
		digit[4] = segment(12);
		digit[3] = segment(22);
		digit[2] = segment(23);
		seconds = 13 - (ANIM_speed / 10);
		tens = 0;
		while (seconds > 9) {
			tens++;
			seconds = seconds - 10;
		}
		digit[1] = segment(tens);
		digit[0] = segment(seconds);
		break;
	case 12: //tictoc
		digit[0] = segment(21);
		digit[1] = segment(0);
		digit[2] = segment(18);
		digit[3] = segment(21);
		digit[4] = segment(1);
		digit[5] = segment(18);
		break;
	case 13: //BEEP
		digit[0] = 0;
		digit[1] = segment(12);
		digit[2] = segment(16);
		digit[3] = segment(16);
		digit[4] = segment(10);
		digit[5] = 0;

		break;
	case 14: //AC-Tijd Schakelt bij activatie timer naar activatie tijd
		digit[0] = segment(22);
		digit[1] = segment(11);
		digit[2] = segment(18);
		digit[3] = segment(23);
		digit[4] = segment(21);
		digit[5] = segment(13);
		break;
	case 15://TUP time up how many times during 1 game
		units = TUP;
		while (units > 9) {
			tens++;
			units = units - 10;
		}
		digit[0] = segment(units);
		digit[1] = segment(tens);

		digit[2] = segment(23);
		digit[3] = segment(12);
		digit[4] = segment(20);
		digit[5] = segment(18);
		break;
	case 16: //Reset (factory)
		digit[0] = segment(18);
		digit[1] = segment(16);
		digit[2] = segment(14);
		digit[3] = segment(16);
		digit[4] = segment(15);
		digit[5] = 0;
		break;
	}
}
void SHIFT_exe() {
	//PORTB &= ~(1 << 2); PINB |= (1 << 2);
	//tempcount++;
	//if (tempcount == 0)Serial.print("+");
	//shift out continue and reads switches and game 
	//port D6 = serial out, port B0 pin 8 = shiftpuls port B1 Pin9 = latch sipo
	//pin10= latch piso (high>low)
	PORTD &= ~(1 << 6); //clear serial pin
	if (shiftbyte[bytecount] & (1 << bitcount))PORTD |= (1 << 6); //set serial pin


	//hier lezen shiftout bit naar game bytes

	if (GPIOR2 & (1 << 4)) {
		switch (bytecount) {
		case 0:
			if (PIND & (1 << 7)) gamebyte[1] |= (1 << bitcount);
			break;
		case 1:
			if (PIND & (1 << 7)) gamebyte[0] |= (1 << bitcount);
			break;
		}
	}

	PORTB |= (1 << 0); PINB |= (1 << 0); //make shift puls

	bitcount++;
	if (bitcount > 7) {
		bitcount = 0;
		bytecount++;

		if (bytecount > 3) { //alle bytes

			GPIOR2 ^= (1 << 4); //lezen game 1x per 2 doorlopen ///************


			if (millis() - ANIM_tijd > ANIM_speed) { //timer animaties
				ANIM_exe();
				ANIM_tijd = millis();
			}

			if (GPIOR2 & (1 << 4)) {
				if (~GPIOR0 & (1 << 6)) GAME_read(); //game stops in end play
			}
			//****
			//hier byte 0 en byte 1 0 maken?


			bytecount = 0;
			PORTB |= (1 << 1); PINB |= (1 << 1); //make latch puls sipo pin 9

			//hier tijd tussen creeren? naar onderen verplaatst
			//denkbaar dat in inlatchen van piso op een flank van de sipo plaats vindt????
			//20jan2021 defect piso1 veroorzaakt sluiting in de VCC shiftproces gestopt.
			//Zeer ernstige complicatie niet bekend waarom.
			//PORTB &= ~(1 << 2);PINB |= (1 << 2); //latch puls piso pin 10

			//next digit
			segmentcount++;
			if (segmentcount > 5)segmentcount = 0;
			shiftbyte[2] = digit[segmentcount];
			shiftbyte[3] = 0xFF;
			shiftbyte[3] &= ~(1 << 7 - segmentcount);


			if (GPIOR2 & (1 << 4)) {

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

				PORTB &= ~(1 << 2); PINB |= (1 << 2); //latch puls piso pin 10 verplaatst naar boven
			}

		} //bytecount>3
	} //bitcount >>7
}

void clearbits() {


}

void GAME_read() { //leest de verbindingen, called shift_exe
	//grs++;
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
			Serial.println("Gameread");
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
				if (PRG_mode == 0) {
					pix[23 - i] = CRGB(200, 10, 2); ///***************
					//Serial.println("|");
				}


			}
			if (GPIOR0 & (1 << 5)) {
				if (cc > 0) {
					GPIOR0 |= (1 << 3); //nieuw schema maken
				}
				else {
					GPIOR0 &= ~(1 << 5);
				}
			}
			//Serial.print("*");

			//hier ergens de schakelfunctie voor programming
			if (PRG_mode > 0) {
				if (cc == 0)PRG_memsw = 0;
				if (cc == 1) PRG_sw();//Serial.println("1 verbinding");
			}
			//else {

			if (PRG_mode == 0) { //**************************************
				for (byte c = 0; c < cc; c++) {
					pix[23 - c] = CRGB(3, 200, 3);
				}
			}


			for (byte i = 0; i < 8; i++) {
				px1 = con[i].first; px2 = con[i].second;

				if (con[i].cnt == true & PRG_mode == 0) {// & GPIOR0 & (1 << 4)) {
					pix[23 - (px1 + 8)] = CRGB(color[pixcolor[px1]].red, color[pixcolor[px1]].green, color[pixcolor[px1]].blue);
					pix[23 - (px2 + 8)] = CRGB(color[pixcolor[px2]].red, color[pixcolor[px2]].green, color[pixcolor[px2]].blue);
					//Serial.print("*");
				}
				con[i].first = 0;
				con[i].second = 0;
				con[i].cnt = false;
			}
			//}

			//Serial.println(cc);
			if (~GPIOR0 & (1 << 4)) { //aonly if color game is enabled				
				fl;
				if (cc == 8) {
					GPIOR0 |= (1 << 6); GAME_end();
				}
			}
		}
	}
}
void GAME_end() { //color puzzle solved, in 0pbouw knop2 on
	GPIOR2 &= ~(1 << 2); //enable pixels
	int time; int min = 0;
	if (GPIOR0 & (1 << 6)) { //	puzzle solved	
  //set time, no display
		hourcurrent = 0;
		time = TIME_end;
		while (time > 59) {
			min++;
			time = time - 60;
		}
		minutecurrent = min;// TIME_end / 60;
		secondcurrent = time;
		ANIM_fase = 10;
		resetcounters();
	}
	else { //puzzle NOT solved
		//direct to animation ESCAPE!
		ANIM_fase = 10;
		//ANIM_count[0] = 0;
		//ANIM_count[1] = 10;
		//ANIM_count[4] = 100;
		//GPIOR0 |= (1 << 6); //disable game
		resetcounters();
	}
	GPIOR0 |= (1 << 4);//disable color game
}
void resetcounters() {
	for (byte i = 0; i < 6; i++) {
		ANIM_count[i] = 0;
	}
}
void GAME_start() {
	//if (~GPIOR2 & (1 << 1)) { //one shot


	GPIOR2 |= (1 << 1); //flag kleurenpuzzel is gestart
	byte num1 = 0; byte num2 = 0; byte val = 0;
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
	resetcounters();
	//}
}
void GAME_stop() {
	//timer afgelopen op 0
	GPIOR0 &= ~(1 << 0);
	//Serial.println("clock stop");
	ANIM_fase = 40; // 30;
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
	//Plaatsing op Shield is verwarrend 
	//Switch optie 3 en 4 zijn verwisseld op de shield.
	//Serial.print("Aan: "); Serial.println(sw);
	switch (sw) {
	case 0://activatie switch van GM of Booby trap 
		ACT_exe(true);
		break;
	case 1: //verhoog speeltijd met 1 minuut
		if (TUP > TUP_count) {
			TUP_count++;
			minutecurrent++;
			if (minutecurrent > 59) {
				minutecurrent = 0;
				if (hourcurrent < 9) hourcurrent++;
			}
			TIME_dp();
		}
		break;
	case 2:
		GPIOR0 |= (1 << 6); // set flag game solved
		GAME_end();
		break;
	case 3:
		GPIOR0 ^= (1 << 7);
		if (GPIOR0 & (1 << 7)) {
			PRG_start();
		}
		else {
			PRG_stop();
		}
		break;
	case 4:
		//PRG_start();
		break;
	}
}
void SW_off(byte sw) {
	//Serial.print("Uit: "); Serial.println(sw);
	switch (sw) {
	case 1:
		break;
	case 2:
		break;
	case 3:
		//PRG_stop();
		break;
	case 4:
		//GPIOR0 |= (24 << 0); // start new game setup
		if (~GPIOR2 & (1 << 3)) {
			Serial.println("Switch 4 off");
			GPIOR0 |= (1 << 3); //start gamestart
			GPIOR0 |= (1 << 4); //disable game
			GPIOR2 |= (1 << 3);
		}

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
		GPIOR2 &= ~(1 << 2);// enable fastled show 

		GPIOR0 |= (1 << 6); //disable //******************
		FastLED.clear();
		if (ANIM_count[0] > 10) { //timer 10x20ms
			ANIM_count[0] = 0;
			//Serial.print(ANIM_count[1]);

			if (ANIM_count[1] > 7) {
				ANIM_count[1] = 0;
				ANIM_fase = 0;
				GPIOR0 &= ~(1 << 4); //enable color game	
				//*****************
				GPIOR0 &= ~(1 << 6);//enable gameread
			}
			else {
				//Serial.println(ANIM_count[1]);
				for (byte i = 0; i < 16; i++) {

					if (pixcolor[i] == ANIM_count[1]) {
						pix[23 - (i + 8)] = CRGB(color[ANIM_count[1]].red, color[ANIM_count[1]].green, color[ANIM_count[1]].blue);
						fl;
					}
				}
				ANIM_count[1] ++;
			}
		}
		break;

	case 10:
		ANIM_count[0]++;
		if (ANIM_count[0] > 80) { // pauze na oplossen puzzel
			ANIM_count[0] = 0;
			FastLED.clear();
			fl;
			GPIOR1 &= ~(1 << 1); //disable clock
			GPIOR0 &= ~(1 << 0); //stop clock
			TIME_txt(0); //off
			ANIM_fase = 12;
			ANIM_count[4] = 60; //blackout time
		}
		break;
	case 12:
		if (ANIM_count[0] > ANIM_count[4]) {
			ANIM_count[0] = 0;
			ANIM_count[1]++;
			if (~GPIOR0 & (1 << 6)) { //puzzle not solved
				GPIOR0 |= (1 << 6);
				ANIM_count[1] = 3;
			}
			TIME_txt(ANIM_count[1]); //1=bypass 2=reboot 3=ESCAPE 4=code 5=exitcode
			//eenmalig
			switch (ANIM_count[1]) { //tijden
			case 1:
				ANIM_count[4] = 40; //bypass
				PIEP_periode = 300;
				BEEP();
				break;
			case 2:
				ANIM_count[4] = 65; //reboot
				break;
			case 3:
				GPIOR0 |= (1 << 0); //start clock
				ANIM_count[4] = 80; //Escape
				break;
			case 4:
				ANIM_count[4] = 30; //Code
				FastLED.clear();
				for (byte i = 0; i < 25; i++) {
					pix[23 - i] = CRGB(1, 5, 1);
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
			pix[23 - (ANIM_count[3])] = CRGB::Red;
			ANIM_count[3]++;
			if (ANIM_count[3] > aantalpix)ANIM_count[3] = 0;
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
						pix[23 - i] = CRGB(255, 0, 0);
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
				pix[23 - i] = CRGB(200, 0, 0);
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
				pix[23 - i] = CRGB(0, 30, 0);
			}
			fl;
		}
		break;
	case 30:
		if (ANIM_count[0] > 220) {
			ANIM_fase = 31;
			for (byte i = 0; i < 25; i++) {
				pix[23 - i] = CRGB(0, 30, 0);
			}
			fl;
			TIME_txt(5);
		}
		break;

	case 40: //blow
		resetcounters();
		ANIM_fase = 41;
		break;
	case 41:
		for (byte i; i < aantalpix; i++) {
			pix[23 - i] = CRGB(0xFFFFFF);
		}
		fl;
		//FastLED.show();
		ANIM_fase = 42;
		break;
	case 42:
		if (ANIM_count[0] > 5) {
			ANIM_count[0] = 0;
			ANIM_fase = 30;
			FastLED.clear();
			//FastLED.show();
			fl;
		}
		break;

	default:
		//do nothing
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
		//hier nog iets maken voor contact dender....
		//Serial.print("connect met ");
		//Serial.println(sw);
		PRG_exe(sw);
		PRG_memsw = sw;
	}
}
void PRG_exe(byte sw) {
	byte minutes = 0;
	switch (sw) {
	case 1:
		FastLED.clear();
		//fl;
		PRG_mode++;
		if (PRG_mode > 10)PRG_mode = 1;

		break;
	case 4: //2e rij 1e links
		switch (PRG_mode) {
		case 5: //deurcode digit 2
			code[2]++;
			if (code[2] > 9)code[2] = 0;
			break;
		}
		break;

	case 5: //2e rij 2e links
		switch (PRG_mode) {
		case 1: //1t minutes tens
			TIME_gamehr = TIME_gamehr + 1;
			if (TIME_gamehr > 9) TIME_gamehr = 0;
			break;
		case 2: //2t
			break;
		case 3: //3t
			TIME_end = TIME_end + 60;
			if (TIME_end >= TIME_act)TIME_end = 10; //minimum time
			break;
		case 5: //deurcode digit 3
			code[3]++;
			if (code[3] > 9)code[3] = 0;
			break;

		}

		break;
	case 6: //2e rij derde links (2e rechts)
		switch (PRG_mode) {
		case 1: //1t minutes tens
			TIME_gamemin = TIME_gamemin + 10;
			if (TIME_gamemin > 59) TIME_gamemin = TIME_gamemin - 60;
			break;
		case 2: //2t
			minutes = TIME_act / 60;
			minutes = minutes + 10;
			if (minutes > 59)minutes = minutes - 60;
			TIME_act = minutes * 60;
			break;
		case 3: //3t
			TIME_end = TIME_end + 10;
			if (TIME_end >= TIME_act)TIME_end = 10; //minimum time
			break;
		case 5: //deurcode digit 4
			code[4]++;
			if (code[4] > 9)code[4] = 0;
			break;

		}
		break;

	case 7: //2e rij 1e rechts
		switch (PRG_mode) {
		case 1: //1t minutes units
			TIME_gamemin = TIME_gamemin + 1;
			if (TIME_gamemin > 59)TIME_gamemin = 0;
			break;
		case 2: //2t
			minutes = TIME_act / 60;
			minutes = minutes + 1;
			if (minutes > 59)minutes = 1;
			TIME_act = minutes * 60;
			break;
		case 3: //3t
			TIME_end++;
			if (TIME_end >= TIME_act)TIME_end = 10; //minimum time
			break;
		case 4: //animatie speed
			ANIM_speed = ANIM_speed - 10;
			if (ANIM_speed < 10) ANIM_speed = 120;
			break;
		case 5: //deurcode digit 6
			code[5]++;
			if (code[5] > 9)code[5] = 0;
			break;
		case 6: //TicToc on/off
			MEM_reg ^= (1 << 1);
			break;
		case 7: //beep on/off
			MEM_reg ^= (1 << 2);
			break;
		case 8: //activatie schakeld tijd naar activatietijd
			MEM_reg ^= (1 << 3);
			break;
		case 9: //TUP aantal keren tijd te verhogen
			TUP++;
			if (TUP > 20)TUP = 1;
			break;
		}
		break;
	case 8: //3e rij rechts
		switch (PRG_mode) {
		case 5: //deurcode digit 1
			code[1]++;
			if (code[1] > 9)code[1] = 0;
			break;
		}
		break;
	case 9:// 3e rij 2e van rechts
		switch (PRG_mode) {
		case 5: //deurcode digit 0
			code[0]++;
			if (code[0] > 9)code[0] = 0;
			break;
		}
		break;
	case 15:
		if (PRG_mode == 10) {
			//factory reset
			FACTORY();
		}
		break;
	}
	PRG_display();
}
void PRG_start() {
	//enters program mode
	GPIOR0 &= ~(1 << 0); //disable clock
	GPIOR0 |= (1 << 4); //disable color puzzle
	GPIOR2 &= ~(1 << 2);

	TIME_txt(6); //clear display
	FastLED.clear();
	FastLED.setBrightness(50);
	fl;
	PRG_mode = 1;
	PRG_display();
}
void PRG_stop() {
	//stops program mode 
	//GPIOR0 |= (1 << 0); //enable clock
	//PRG_mode = 0;
	MEM_write();
	setup();
}
void BAR(byte clr) {
	switch (clr) {
	case 0: //red
		for (byte i = 0; i < 8; i++) {
			pix[23 - i] = CRGB::Red;
		}
		break;
	case 1: //green
		for (byte i = 0; i < 8; i++) {
			pix[23 - i] = CRGB::Green;
		}
		break;
	case 10:
		FastLED.clear();
		break;
	}
	FastLED.show(); //HIER NIET FL;
}
void PRG_display() {
	BAR(10);
	switch (PRG_mode) {
	case 1://t1 totale speeltijd, starttijd timer bij powerup of na reset	
		TIME_txt(7);
		break;
	case 2: //t2
		TIME_txt(8);
		break;
	case 3: //t3
		TIME_txt(9);
		break;
	case 4: //animatie speed
		TIME_txt(11);
		break;
	case 5: //deurcode
		TIME_txt(10);
		break;
	case 6://tixtoc on/off
		if (MEM_reg & (1 << 1)) {
			BAR(1);
		}
		else {
			BAR(0);
		}
		TIME_txt(12);
		break;
	case 7: //beep on/off
		if (MEM_reg & (1 << 2)) {
			BAR(1);
		}
		else {
			BAR(0);
		}
		TIME_txt(13);
		break;
	case 8: //activatie tijd aanpassen (uit(true) = default)
		if (MEM_reg & (1 << 3)) {
			BAR(0);
		}
		else {
			BAR(1);
		}
		TIME_txt(14);
		break;
	case 9: // TUP aantal mogelijke ophogingen speeltijd met 1 minuut
		TIME_txt(15);
		break;
	case 10: //reset (factory)
		TIME_txt(16);
		break;
	}
}
void TIK_on() {
	//TIMSK2 |= (6 << 0);
	//TCNT1H = 0; TCNT1L = 0;
	TCNT2 = 0;
	TCCR2B |= (1 << 3);
	TCCR2A = B01000010;
	GPIOR1 |= (1 << 3);
	BEEP_stop = millis();
	GPIOR2 ^= (1 << 0);
	if (GPIOR2 & (1 << 0)) {
		OCR2A = 100;
	}
	else {
		OCR2A = 75;
	}
	//OCR2B = 248;
	BEEP_length = 1;

}
void TIK_off() {
	if (millis() - BEEP_stop > BEEP_length) {
		GPIOR1 &= ~(1 << 3);
		GPIOR1 &= ~(1 << 6); //buzzer vrij geven
		TCCR2B &= ~(1 << 3);
		TCCR2A = 0;
		//TIMSK2 &= ~(6 << 0);
		PORTB |= (1 << 3); //port hoog zetten

	}
}
void PIEP_on() {
	PIEP_count[1]++;
	if (PIEP_periode > 30)PIEP_count[3]++; //seconde teller, totdat piepperiode <=50=500ms
	if (PIEP_count[3] > 100) {
		PIEP_count[3] = 0;
		PIEP_periode--;
	}

	PIEP_count[0]++;
	if (PIEP_count[0] > PIEP_periode & ~(GPIOR1 & (1 << 6))) { //iedere piep periode*10ms

		//Serial.println(PIEP_periode);
		PIEP_count[0] = 0;
		BEEP_set(40);

	}
}
void BEEP_set(int bl) {
	TCNT2 = 0;
	TCCR2B |= (1 << 3);//start interupt timer
	TCCR2A = B01000010;
	GPIOR1 |= (1 << 3);
	BEEP_stop = millis();
	BEEP_length = bl; //hoelang beept de beep in ms
	OCR2A = 30; //klank
	//OCR2B = 1; // -PIEP_vol; //verschil is volume
	GPIOR1 |= (1 << 6); //buzzer bezet
}
void BEEP() { //lange beep
	BEEP_set(500);
}

void ACT_exe(boolean time) { //activatie time true is met schakelaar 

	if (~GPIOR1 & (1 << 5)) {
		GPIOR1 |= (1 << 5); //latching, only 1x use in a game
		if (time == true) {
			if (~MEM_reg & (1 << 3)) {
				hourcurrent = 0;
				minutecurrent = TIME_act / 60;
				//Serial.println(minutecurrent);
				secondcurrent = 0;
				TIME_dp();
			}
		}

		PIEP_vol = 0;
		PIEP_periode = 400;
		PIEP_time = millis();
		for (byte i = 0; i < 4; i++) {
			PIEP_count[i] = 0;
		}
		//Als bij bereiken activatie de puzzel niet is gestart..
		if (~GPIOR2 & (1 << 1)) { //puzzel niet gestart

			if (PINC & (1 << 4)) { //ontsteker is niet verbonden met massa				
				GPIOR0 |= (24 << 0); //start puzzel
			}
		}
	}
}



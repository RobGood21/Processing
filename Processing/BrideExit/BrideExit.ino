/*
 Name:		BrideExit.ino
 Created:	1/4/2021 2:16:15 PM
 Author:	Rob Antonisse

 Exit number puzzel. To entry the correct number wil release doorlock

 V1.01 8januari2021

 */

//libraries
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <splash.h>
#include <Adafruit_SSD1306.h>
#define open 30 ///tijd dat de deur open blijft

//declaraties
Adafruit_SSD1306 display(128, 64, &Wire, -1);

byte kolom[4];
byte rowcount;
unsigned long SWcount;
unsigned long opentime;
byte opencount;

byte code[6]; //entry code can be changed 0=msb 5=lsb
byte codedefault[6] = { 1,2,5,5,4,0 }; //bit0 = MSB
byte codeprg[6] = { 5,3,11,1,8,10 }; //hard coded A81B35  *=14  #=18 A=10 C=12
byte codeprg2[6] = { 5,0,13,6,2,12 }; //hard coded *C26D05#
byte codeinput[6];//ingevoerde code
byte codenew[6];  //nieuwe code bij programmeren
byte codecount = 0;
byte prgfase = 0;

void setup() {
	Serial.begin(9600);
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
	//ports//
	DDRB |= (15 << 0);
	PORTB |= (1 << 3); //relais off
	DDRD |= (240 << 0); //pin7,6,5,4 as output
	MEM_read();
	//other initialise
	for (byte i = 0; i < 4; i++) {
		kolom[i] = 0xFF;
	}
	ClearInput();
	opencount = open+1;
	DP_exe();
}
void MEM_read() {
	for (byte i = 0; i < 6; i++) {
		code[i] = EEPROM.read(100 + i);
		if (code[i] > 16)code[i] = codedefault[i];
	}
}
void loop() {
	if (GPIOR0 & (1 << 2)) {
		if (millis() - opentime > 1000) {
			opentime = millis();
			DoorOpen();
		}
	}

	if (millis() - SWcount > 10) {
		SWcount = millis();
		SW_exe();
	}
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
	case 14: //clear code
		ClearInput();
		break;
	case 15: //#
		Program();
		break;
	default:
		for (byte i = 5; i > 0; i--) {
			codeinput[i] = codeinput[i - 1];
		}
		codeinput[0] = key;
		break;
	}
	//Serial.print("key: ");
	//Serial.println(key);
	if (~GPIOR0 & (1 << 2)) DP_exe(); //not if game is solved
}
void Program() {
	byte cd;
	switch (prgfase) {
	case 0:
		if (GPIOR0 & (1 << 3)) {
			if (GPIOR0 & (1 << 4)) { //2e code
				cd = 0;
				for (byte i = 0; i < 6; i++) {
					cd = cd + codeinput[i] ^ codeprg2[i];
				}
				if (cd == 0) {
					blauw();
					//Serial.println("ok, tweede code goed");
					for (byte i = 0; i < 6; i++) {
						codeinput[i] = 21;
					}
					prgfase = 1;
				}
				else { //code niet goed 
					ClearInput();
				}
			}
			else { //1e code
				cd = 0;
				for (byte i = 0; i < 6; i++) {
					cd = cd + codeinput[i] ^ codeprg[i];
				}
				if (cd == 0) {
					//Serial.println("eerste code ok");
					paars();
					GPIOR0 |= (1 << 4); //set voor tweede code
				}
				else { //invoer niet ok, reset program flags
					ClearInput();
				}
			}
		}
		else {
			GPIOR0 |= (1 << 3);
		}
		break;
	case 1:
		break;
	case 2: //eerste code ingevoerd
		for (byte i = 0; i < 6; i++) {
			codenew[i] = codeinput[i];
			codeinput[i] = 21;
		}
		prgfase = 3;
		oranje();
		break;
	case 4: //tweede keer nieuwe code ingevoerd
		cd = 0;
		for (byte i = 0; i < 6; i++) {
			cd = cd + codenew[i] ^ codeinput[i];
		}

		if (cd == 0) { //2x nieuwe code goed ingevoerd
			for (byte i = 0; i < 6; i++) {
				code[i] = codenew[i];
				EEPROM.update(100 + i, code[i]);
			}
			prgfase = 0;
			//code nu gelijk aan input code dus puzzle solved...
			//Serial.println("2x code goed");
		}
		else {//als cd niet 0 dan puzzle niet solved
			//Serial.println("nee, 2e invoer niet juist");
			ClearInput();
		}
		break;
	}
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
void paars() {
	PORTB |= (5 << 0);
	PORTB &= ~(1 << 1);
}
void oranje() {
	PORTB |= (3 << 0);
	PORTB &= ~(1 << 2);
}
void DP_exe() {
	GPIOR0 |= (1 << 1);
	byte a;
	DP_regeltop();
	switch (prgfase) {
	case 0:
		display.print(F("Enter code to exit"));
		break;
	case 1:
		display.print("New code + # ");
		break;
	case 3:
		display.print("New code again + # ");
		break;
	}
	for (byte i = 0; i < 6; i++) {
		a = codeinput[i];
		switch (a) {
		case 10:
			teken(i);
			display.print("A");
			break;
		case 11:
			teken(i);
			display.print("B");
			break;
		case 12:
			teken(i);
			display.print("C");
			break;
		case 13:
			teken(i);
			display.print("D");
			break;
		case 20: //no input
			display.drawLine(10 + ((5 - i) * 20), 55, 20 + ((5 - i) * 20), 55, WHITE);
			GPIOR0 &= ~(1 << 1);
			break;
		case 21: //input program mode
			display.setCursor(5 + ((5 - i) * 20), 55);
			display.setTextSize(1);
			display.print("*");
			GPIOR0 &= ~(1 << 1);
			break;
		default: //0~9
			teken(i);
			display.print(a);
			break;
		}
	}
	display.display();
	if (GPIOR0 & (1 << 1)) { //6 digits ingevoerd, check invoer
		switch (prgfase) {
		case 0:
			a = 0;
			for (byte i = 0; i < 6; i++) {
				a = a + (codeinput[i] ^ code[i]);
			}
			if (a == 0)Solved();
			break;
		case 1:
			prgfase = 2;
			break;
		case 3:
			prgfase = 4;
			break;
		}
	}
}
void teken(byte t) {
	display.setCursor(5 + ((5 - t) * 20), 30);
	display.setTextSize(3);
}
void DP_regeltop() {
	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setCursor(10, 5);
	display.setTextSize(1);
}
void Solved() { //juiste code ingevoerd
	//Serial.println("opgelost");
	//volgorde belangrijk!
	ClearInput();
	groen();
	GPIOR0 |= (1 << 2); //start timer, disable inputs
	opencount = open+1;
}
void DoorOpen() {
	opencount--;
	if (opencount <= open ) {
		display.clearDisplay();
		DP_regeltop();
		display.print("Door open");
		display.setCursor(10, 20);
		display.print("Door close after ");
		display.setCursor(10, 40);
		display.setTextSize(3);
		display.print(opencount);
		display.setCursor(53, 47);
		display.setTextSize(2);
		display.print("sec");
		display.display();
		PORTB &=~(1 << 3); //relais on
	}
	else {
		ClearInput();		
	}
}
void ClearInput() {
	for (byte i = 0; i < 6; i++) {
		codeinput[i] = 20;
	}
	GPIOR0 &= ~(7 << 3); //clear bit 3&4&5
	rood();
	prgfase = 0;
	GPIOR0 &= ~(1 << 2); //
	PORTB |= (1 << 3); //relais off
	DP_exe();
}


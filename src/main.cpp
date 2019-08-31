/*
 * Galaga Lite
 * Do what you want with the code.
 * Created: 5/22/19 2:43:02 PM
 * Author : Liam
 */ 

#include <avr/eeprom.h>
#include <avr/io.h>
#include "io.c"
#include "SNES.c"
#include "Timer.c"
#include <stdlib.h>
#include <time.h>

using namespace std;
unsigned char debug = '0';
//#include "program.c"
//#include "CustomCharID.h"

unsigned char ship[8] = {
	0b01000,
	0b11000,
	0b11100,
	0b11111,
	0b11111,
	0b11100,
	0b11000,
	0b01000
};

unsigned char typeOne[8] = {
	0b00000,
	0b00100,
	0b01010,
	0b01011,
	0b01011,
	0b01010,
	0b00100,
	0b00000
};

unsigned char typeTwo[8] = {
	0b00000,
	0b01000,
	0b00110,
	0b00101,
	0b00101,
	0b00110,
	0b01000,
	0b00000
};
unsigned char typeThree[8] = {
	0b00000,
	0b00000,
	0b00100,
	0b00010,
	0b00111,
	0b00010,
	0b00100,
	0b00000
};

unsigned char las[8] = {
	0b00000,
	0b00000,
	0b00000,
	0b01111,
	0b01111,
	0b00000,
	0b00000,
	0b00000
};
unsigned char power[8] = {
	0b00000,
	0b00000,
	0b00000,
	0b00100,
	0b01110,
	0b00100,
	0b00000,
	0b00000
};

unsigned char waveBoss[8] = {
		0b00010,
		0b00110,
		0b01101,
		0b10110,
		0b10110,
		0b01101,
		0b00110,
		0b00010
};

typedef struct task {
	int state;                  // Task's current state
	unsigned long period;       // Task period
	unsigned long elapsedTime;  // Time elapsed since last task tick
	int (*TickFct)(int);        // Task tick function
} task;

class Ship;


//Forward Declarations
int FilterInput(int state); //SM One

//Helper functions
void main_init(); 
void menu();
void SysTick();
void draw();
void PewPew(Ship play);
void Kill();
void CreateHostiles();
void refreshHostiles();
void LevelUp();
void GameOver();
void LCD_CustomWrite(char* str, int num);
unsigned char AllHostilesDead();
unsigned char AllLsrDead();
void cpy(unsigned char* src, unsigned char* dest);
void initSymbol(unsigned char pos, unsigned char *str);

class Ship {
	public:
	unsigned char pos;
	unsigned short disp;
	unsigned char killed;
	Ship() {
		pos = 17;
		disp = 1;
		killed = 0;
	}
	void draw();
	virtual void updatePos(unsigned char input);
	
};

void Ship::draw() {
	if(!killed){
		LCD_Cursor(pos);
		LCD_Cursor(pos);
		LCD_WriteData(disp);
		
	}
}

void Ship::updatePos(unsigned char input) {
	unsigned char L = (input & 0x04);
	unsigned char R = (input & 0x02);
	if(L) pos = 1;
	else if (R) pos = 17;
}

class Hostile : public Ship {
	public:
	int type;
	Hostile() {
		type = rand()%4;
		pos = 16;
		killed = 0;
		switch (type) {
			case 0:
			disp = 3;
			break;
			case 1:
			disp = 4;
			break;
			case 3:
			disp = 5;
			break;
			default:
			disp = 3;
			break;
		}
	}
	void updatePos();
};

void Hostile::updatePos() {
	if(pos == 17) pos = 0;
	else if(pos > 0 && pos <= 32) {
		--pos;
	} else killed = 1;
}

class laser : public Ship {
	public:
	char u;
	laser() {
		disp = 2;
		pos = 1;
		u = 2;
	}
	laser(Ship user) {

		disp = 2;
		pos = user.pos + 1;
		u = 1;
	}
	laser(Hostile enemy){
		disp = 2;
		pos = enemy.pos - 1;
		u = 2;
	}
	
	void updatePos();
};

void laser::updatePos() {
	if (u == 2) {
		if(pos == 17) pos = 0;
		else if(pos > 0 && pos <= 32) {
			--pos;
		} else {
			killed = 1;
		}
	} else {
		if(pos == 16) pos = 0;
		else if(pos > 0 && pos <= 32) {
			++pos;
		} else {
			killed = 1;
		}
	}
}

//State enumerations
enum OneState{oneOn, oneOff};
enum TwoState{twoOn, twoOn1, twoOn2};
enum ThreeState{threeOn};
enum FourState{fourOn};
enum FiveState{fiveOn};
enum SixState{sixOn};

//Global variables
task tasks[6];

unsigned char out;
unsigned char numTasks = 6;
unsigned short sysPeriod = 1;
unsigned char lvl = 2;
unsigned char shot = 0;

unsigned char cap = 20;
Ship player;
Hostile ships[21];
laser ulsr[11];
laser elsr[11];
unsigned char lcap = 10;
unsigned char l = 0;
unsigned char el = 0;
unsigned char sh = 0;
unsigned char GO = 0;
unsigned char LU = 0;
uint16_t score = 0;



////////////////////////////////////////////////////////////MAIN
int main(void)
{
	DDRA = 0x03; PORTA = 0x00; 
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	main_init();
	LCD_init();
	
	TimerSet(sysPeriod);
	TimerOn();
	SNES_init();
	initSymbol(1, ship);
	initSymbol(2, las);
	initSymbol(3, typeOne);
	initSymbol(4, typeTwo);
	initSymbol(5, typeThree);
	initSymbol(6, power);
	initSymbol(7, waveBoss);
	menu();
	srand(time(0));
	//LCD_WriteData(1);
	//LCD_WriteData(2);
	//LCD_WriteData(3);
	//LCD_WriteData(4);
	//LCD_WriteData(5);
	//eeprom_write_word((uint16_t*) 0,0);
    while (1) {
		if(!(LU ||GO))SysTick();
		if (LU) {
			++lvl;
			(LevelUp());
		}
		if (GO) {
			(GameOver());
			
		}
		while(!TimerFlag);
		//Ship poo;
		//initSymbol(1, ship);
		//LCD_WriteData(1);
		PORTB = out;
		TimerFlag = 0;
		LU = 0;
		
	}
}
//////////////////////////////////////////////////////////////

int FilterInput(int state){
	unsigned short input;
	out = 0x00;
	input = SNES_Read();
	if(input){
		//if(input & 16) out = 0x01;
		//if(input & 32) out = 0x02;
		//if(input & 64) out = 0x04;
		//if(input & 128) out = 0x08;
		if(input & 256) out |= 0x02; //R
		if(input & 512) out |= 0x04; //L
		//if(input & 1024) out = 0x40;
		//if(input & 2048) out = 0x80;
		if(input & 4096) out |= 0x08; //Start
		if(input & 8192) out |= 0x10; //Select
		//if(input & 16384) out = 0x01;
		if(input & 32768) out |= 0x01; //B
	} else out = 0x00;
	
	return oneOn;
}


int GameTick(int state) {
	if(player.killed) GO = 1;
	unsigned char B = out & 0x01;
	unsigned char L = out & 0x04;
	unsigned char R = out & 0x02;
	unsigned char start = out & 0x08;
	
	if (B) shot = 1;
	if((L && player.pos != 1) || (R && player.pos != 17)){
		player.updatePos(out);

		draw();
		LCD_Cursor(36);
	}
	Kill();
	return twoOn;
	
}
int GameDisplay(int state) {
	//draw();
	//LCD_Cursor(36);
	return threeOn;
}

int HostileMove(int state) {
	score = score + 1*lvl;
	for (int i = 0; i < sh; ++i) {
		ships[i].updatePos();
	} 

	if (sh <= cap) CreateHostiles();
	else if (AllHostilesDead()) {
		refreshHostiles();
		LU = 1;
	}
	draw();
	LCD_Cursor(36);
	return fourOn;
}

int LaserCreate(int state) {
	if (shot) PewPew(player);
	shot = 0;
	return fiveOn;
}

int LaserMove(int state) {
	for (int i = 0; i < l; ++i) ulsr[i].updatePos();
	for(int i = 0; i < el; ++i) elsr[i].updatePos();
	draw();
	LCD_Cursor(36);
	return sixOn;
}


void draw() {
	LCD_ClearScreen();
	player.draw();
	for (int i = 0; i < sh; ++i) {
		ships[i].draw();
	}
	for (int i = 0; i < l; ++i) {
		ulsr[i].draw();
	}
	for (int i = 0; i < el; ++i) {
		elsr[i].draw();
	}
}

void Kill() {
	for (int i = 0; i < l; ++i) {
		for(int j = 0; j < sh; ++j) {
			if(!(ulsr[i].killed || ships[j].killed) && (ulsr[i].pos == ships[j].pos) && (ships[j].disp != 6)) {
				ships[j].killed = 1;
				ulsr[i].killed = 1;
			}
		}
	}

	for(int j = 0; j < sh; ++j) {
		if(!(player.killed || ships[j].killed)&& (player.pos == ships[j].pos)) {
			if((ships[j].disp == 6)) {
				ships[j].killed = 1;
				l = 0;
			}
			else {
				ships[j].killed = 1;
				player.killed = 1;
			}
		}
	}
	
	for(int k = 0; k < el; ++k) {
		if(!(elsr[k].killed || player.killed) && (elsr[k].pos == player.pos)) {
			player.killed = 1;
			elsr[k].killed = 1;
		}
	}

}

void CreateHostiles() {
	
	int num = rand() % 100;
	if (num < 20 + lvl*5) {
		Hostile temp;
		num = rand() % 2;
		if (num == 0) temp.pos = 32;
		if (sh == cap) temp.disp = 7;
		ships[sh] = temp;
		++sh;
	} else if (num > 95) {
		Hostile temp;
		temp.disp = 6;
		if (num == 0) temp.pos = 32;
		ships[sh] = temp;
		++sh;
	}
	for(int i = 0; i < sh; ++i) {
		if(ships[i].disp != 6 && !ships[i].killed && el < lcap) {
			int ran = rand() % 100;
			if (ran < 3) {
				laser temp(ships[i]);
				elsr[el] = temp;
				el++;
			}
		} 
	}
}

void refreshHostiles() {
	sh = 0;
	el = 0;
}

unsigned char AllHostilesDead(){
	for (int i = 0; i < sh; ++i) {
		if (ships[i].killed == 0) return 0;
	}
	return 1;
}

unsigned char AllLsrDead(){
	for (int i = 0; i < el; ++i) {
		if (elsr[i].killed == 0) return 0;
	}
	return 1;
}

void PewPew(Ship play) {
	if (l < lcap) {
		laser temp(play);
		ulsr[l] = temp;
		l++;
	}

}

void LevelUp(){}



void GameOver(){
	unsigned char newHigh = 0;
	if((short)eeprom_read_word(0x00) < score) {
		newHigh = 1;
		 eeprom_write_word((uint16_t*) 0,score);
	}
	char str[5];
	itoa((int)eeprom_read_word(0x00),str,10);
	LCD_DisplayString(4, "Game Over!    Hi Score:");
	LCD_Cursor(1); LCD_WriteData(7);
	LCD_Cursor(4); LCD_WriteData('G');
	LCD_Cursor(28); LCD_CustomWrite(str, 5);
	if(newHigh) {
		LCD_Cursor(17); LCD_WriteData('N');
		LCD_WriteData('H'); LCD_WriteData('!');
	}
	LCD_Cursor(36);
	while(1){
		FilterInput(oneOn);
		if(out & 0x08) {
			player.killed = 0;
			sh = 0;
			el = 0;
			l = 0;
			lvl = 2;
			GO = 0;
			break;
		}
		;
		}

}

void LCD_CustomWrite(char* str, int num) {
	for(int i = 0; i < num; ++i) {
		if(str[i] < '0' || str[i] > '9') break;
		LCD_WriteData(str[i]);
	}
}


void main_init(){
	//LCD_init();

	unsigned char i = 0;
	tasks[i].state = oneOn;
	tasks[i].period = 1;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &FilterInput;
	++i;
	
	tasks[i].state = twoOn;
	tasks[i].period = 10;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &GameTick;
	++i;
	
	tasks[i].state = threeOn;
	tasks[i].period = 200;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &GameDisplay;
	++i;
	
	tasks[i].state = fourOn;
	tasks[i].period = 1000/lvl;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &HostileMove;
	++i;
	
	tasks[i].state = fiveOn;
	tasks[i].period = 204;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &LaserCreate;
	++i;
	
	tasks[i].state = sixOn;
	tasks[i].period = 204;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &LaserMove;
	++i;
}

void SysTick() {
	for (unsigned char i=0; i < numTasks; i++) {
		if (tasks[i].elapsedTime >= tasks[i].period){
			// Task is ready to tick, so call its tick function
			tasks[i].state = tasks[i].TickFct(tasks[i].state);
			tasks[i].elapsedTime = 0; // Reset the elapsed time
		}
		tasks[i].elapsedTime += sysPeriod;
	}
}

void menu() {
	

	LCD_DisplayString(3,"Galaga Mobile!   Press Start");
	LCD_Cursor(1); LCD_WriteData(1);
	LCD_Cursor(3);LCD_WriteData('G');
	//LCD_Cursor(17); LCD_WriteData(1);
	LCD_Cursor(20);
	LCD_Cursor(36);
	while(out != 0x08) {
		FilterInput(oneOn);
		continue;
	}
	LCD_ClearScreen();
}



void initSymbol(unsigned char pos, unsigned char *str){
	LCD_WriteCommand (0x40 + (pos*8));
	for(int i = 0; i < 8; i++) {
		LCD_WriteData(str[i]);
	}
	LCD_WriteCommand(0x80);
}

void cpy(unsigned char* src, unsigned char* dest) {
	for (int i = 0; i < 8; ++i) {
		dest[i] = src[i]; 
	}
}



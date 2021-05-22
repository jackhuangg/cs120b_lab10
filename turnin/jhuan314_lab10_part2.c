/*	Author: Jack Huang
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #10  Exercise #2
 *	Exercise Description: [optional - include for your own benefit]
 *	https://drive.google.com/drive/folders/1JBIqqJb-m900203LVLXI8yLaMciH493w?usp=sharing
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include "bit.h"
#include "keypad.h"
#include "timer.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

typedef struct _task{
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime; 
	int (*TickFct)(int); 
}task;

unsigned long int findGCD (unsigned long int a, unsigned long int b) {
	unsigned long int c;
	while(1){
		c = a%b;
		if(c == 0){
			return b;
		}
		a = b;
		b = c;
	}
	return 0;
}

enum keypadstates {checkinput,waitnext,unlock};
char array[] = {'#','1','2','3','4','5'};
unsigned char temp = 0;

int keypad(int state){
	unsigned char a = GetKeypadKey();
	switch(state){
		case checkinput:
			if(a == array[temp]){
				if(temp != 5){
					temp++;
					state = waitnext;
				}
				else{
					state = unlock;
				}
			}
			else{
				if(a != '\0'){
					temp = 0;
				}
				state = checkinput;
			}
			break;
		case waitnext:
			a = GetKeypadKey();
			if(a == '\0'){
				state = checkinput;
			}
			else{
				if(a == array[temp-1]){
					state = waitnext;
				}
				else{
					state = checkinput;
					temp = 0;
				}
			}
			break;
		case unlock:
			state = checkinput;
			break;
		default:
			state = checkinput;
			break;
	}
	switch(state){
		case checkinput:
			break;
		case waitnext:
			break;
		case unlock:
			PORTB = 0x01;
			temp = 0;
			break;
	}
	return state;
}

enum lockstates{lock,waitlock};
int lockdoor(int state){
	switch(state){
		case waitlock:
			if((~PINB & 0x80) == 0x80){
				state = lock;
			}
			else{
				state = waitlock;
			}
			break;
		case lock:
			if((~PINB & 0x80) == 0x80){
				state = lock;
			}
			else{
				state = waitlock;
			}
			break;
		default:
			state = waitlock;
			break;
	}
	switch(state){
		case waitlock:
			break;
		case lock:
		PORTB = 0x00;
			break;
	}
	return state;
}


int main(void){
	DDRC = 0xF0; PORTC = 0x0F;
	DDRB = 0x7F; PORTB = 0x00;

	static task task1, task2;
	task *tasks[] = {&task1,&task2};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	const char start = -1;
	

	task1.state = start;
	task1.period = 10;
	task1.elapsedTime = task1.period;
	task1.TickFct = &keypad;

	task2.state = start;
	task2.period = 10;
	task2.elapsedTime = task2.period;
	task2.TickFct = &lockdoor;

	unsigned long GCD = tasks[0]->period;
	for (int i = 0; i < numTasks; i++){
		GCD = findGCD(GCD,tasks[i]->period);
	}

	TimerSet(GCD);
	TimerOn();

    	while (1) {
		for (int i = 0; i < numTasks; i++){ 
			if(tasks[i]->elapsedTime >= tasks[i]->period){ 
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state); 
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += GCD;
		}
		while(!TimerFlag);
		TimerFlag = 0;
    	}
	return 0;
}

























/*	Author: Jack Huang
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #10  Exercise #1
 *	Exercise Description: [optional - include for your own benefit]
 *	https://drive.google.com/drive/folders/1JBIqqJb-m900203LVLXI8yLaMciH493w?usp=sharing
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include "bit.h"
#include "keypad.h"
#include "timer.h"
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

typedef struct _task {
    signed char state;
    unsigned long int period;
    unsigned long int elapsedTime;
    int (*TickFct)(int);
} task;

unsigned long int findGCD (unsigned long int a, unsigned long int b) {
    unsigned long int c;
    while(1){
	c = a%b;
	if(c==0){return b;}
	a = b;
	b = c;
    }
    return 0;
}

enum states1 {start,checkinput,waitinput,unlock};

unsigned char array[6] = {'#','1','2','3','4','5'};
unsigned char temp = 0;


int tick(int state){
	unsigned char x = GetKeypadKey();
	switch(state){
		case start:
			state = checkinput;
			break;
		case checkinput:
			if(x == '\0'){
				state = checkinput;
			}
			//else if(x == '#' && temp != 0){
			//	temp = 0;
			//	state1 = checkinput;
			//	PORTB = 0x00;
			//}
			else if(x == array[temp]){
				if(temp == 5){
					state = unlock;
				}
				else{
					state = waitinput;
					temp++;
				}
			}
			break;
		case waitinput:
			if(x == '\0'){
				state = checkinput;
			}
			else{
				state = waitinput;
			}
			break;
		case unlock:
			state = checkinput;
			break;
	}
	switch(state){
		case start:
			break;
		case checkinput:
			PORTB = 0x02;
			break;
		case waitinput:
			break;
		case unlock:
			temp = 0;
			PORTB = 0x01;
			break;
	}
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRB = 0x7F; PORTB = 0x00;
    DDRC = 0xF0; PORTC = 0x0F;

    static task task1;
    task *tasks[] = {&task1,};
    const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
    const char start = -1;

    task1.state = start;
    task1.period = 10;
    task1.elapsedTime = task1.period;
    task1.TickFct = &tick;

    unsigned short i;
    unsigned long GCD = tasks[0]->period;
    for(i = 1; i < numTasks; i++){
    	GCD = findGCD(GCD,tasks[i]->period);
    }
    TimerSet(GCD);
    TimerOn();
    /* Insert your solution below */
    while (1) {
	for(i = 0; i < numTasks; i++){
		if(tasks[i]->elapsedTime == tasks[i]->period){
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

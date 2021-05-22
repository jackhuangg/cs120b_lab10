/*	Author: Jack Huang
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #10  Exercise #3
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

void set_PWM(double frequency) {
    static double current_frequency;

    if (frequency != current_frequency) {
        if(!frequency)
            TCCR3B &= 0x08;
        else
            TCCR3B |= 0x03;

        if(frequency < 0.954)
            OCR3A = 0xFFFF;
        else if (frequency > 31250)
            OCR3A = 0x0000;
        else
            OCR3A = (short) (8000000 / (128 * frequency)) - 1;
        
        TCNT3 = 0;
        current_frequency = frequency;
    }
}

void PWM_on() {
    TCCR3A = (1 << COM3A0);
    TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
    set_PWM(0);
}

void PWM_off() {
    TCCR3A = 0x00;
    TCCR3B = 0x00;
}

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

enum songstates {smstart,initial,start1,wait};

double freq[] = {261.63,0,261.63,0,293.66,293.66,0,261.63,261.63,0,349.23,349.23,0,329.63,329.63,329.63,0,261.63,0,261.63,0,293.66,293.66,0,261.63,261.63,0,392,392,0,349.23,349.23,349.23,0,261.63,0,261.63,0,524,524,0,440,440,0,349.23,349.23,0,329.63,329.63,0,293.66,293.66,0,466.16,0,466.16,0,440,440,0,349.23,349.23,0,392,392,0,349.23,349.23,349.23,349.23,0};
unsigned char i=0;

int play(int state){
	switch(state){
		case smstart:
			state = initial;
			break;
		case initial:
			if((~PINA & 0x80) == 0x80){
				state = start1;
			}
			else{
				state = initial;
			}
			break;
		case start1:
			if(i<20){
				state = start1;
			}
			else{
				state = wait;
			}
			break;
		case wait:
			if((~PINA & 0x80) == 0x80){
				state = wait;
			}
			else{
				state = initial;
			}
			break;
		default:
			state = smstart;
			break;
	}
	switch(state){
		case smstart:
			break;
		case initial:
			set_PWM(0);
			i=0;
			break;
		case start1:
			set_PWM(freq[i]);
			i++;
			break;
		case wait:
			set_PWM(0);
			break;
	}
	return state;
}

int main(void){
	DDRC = 0xF0; PORTC = 0x0F;
	DDRB = 0x7F; PORTB = 0x00;
	DDRA = 0x00; PORTA = 0xFF;

	static task task1, task2, task3;
	task *tasks[] = {&task1,&task2,&task3};
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

	task3.state = start;
	task3.period = 200;
	task3.elapsedTime = task3.period;
	task3.TickFct = &play;

	unsigned long GCD = tasks[0]->period;
	for (int i = 0; i < numTasks; i++){
		GCD = findGCD(GCD,tasks[i]->period);
	}

	//set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	
	unsigned short i; //scheduler for loop iterator */
	PWM_on();
    	while (1) {
		for (i = 0; i < numTasks; i++){ //scheduler code
			if(tasks[i]->elapsedTime >= tasks[i]->period){ //task is ready to tick
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state); //set next state
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += GCD;
		}
		while(!TimerFlag);
		TimerFlag = 0;
    	}
	PWM_off();
	return 0;
}

























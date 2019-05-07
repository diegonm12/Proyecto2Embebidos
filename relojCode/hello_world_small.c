#include <stdio.h>

#include "system.h"
#include "altera_avalon_timer_regs.h"
#include "sys/alt_irq.h"
#include "altera_avalon_uart_regs.h"

volatile char* timer_status_ptr  = (char *)(TIMER_BASE);
volatile char* timer_control_ptr = (char *)(TIMER_BASE + 4);

volatile int  edge_capture;
volatile char *keys_ptr               = (volatile char *)(KEYS_BASE);
volatile char *keys_direction_ptr = (volatile char *)(KEYS_BASE + 4);
volatile char *keys_mask_ptr      = (volatile char *)(KEYS_BASE + 8);
volatile char *keys_edge_ptr      = (volatile char *)(KEYS_BASE + 12);

volatile char* uart_ptr  = (char *)(UART_BASE);
volatile char* uart_status_ptr  = (char *)(UART_BASE + 8);
volatile char* uart_ctrl_ptr  = (char *)(UART_BASE + 12);


volatile int*  leds_ptr = (int *) (LEDS_BASE);
volatile int*  hex0_ptr = (int *) (HEX0_BASE);
volatile int*  hex1_ptr = (int *) (HEX1_BASE);
volatile int*  hex2_ptr = (int *) (HEX2_BASE);
volatile int*  hex3_ptr = (int *) (HEX3_BASE);
volatile int*  hex4_ptr = (int *) (HEX4_BASE);
volatile int*  hex5_ptr = (int *) (HEX5_BASE);
volatile int*  buzzer_ptr = (int *) (BUZZER_BASE);

int counter = 0;

int sec = 0;
int min = 0;
int hour = 0;

int alarmCounter = 0;

int alarmSec = 0;
int alarmMin = 0;
int alarmHour = 12;

int state = 1; // 1 = muestra tiempo, 2 = set alarma, 3 = alarmado

//-----------------------------------------------------------------------
// Prototype functions
//-----------------------------------------------------------------------
static void timer_irs(void * context){
	//Borra bandera por interrupci�n de timer
	*timer_status_ptr = 0;

	counter++;
	if(counter >= 1000) {
		counter = 0;
		sec++;
		if (sec >= 60) {
			sec = 0;
			min++;
			if (min >= 60) {
				min = 0;
				hour++;
				if (hour >=24) {
					hour = 0;
				}
			}
		}
	}

	if (sec == alarmSec && min == alarmMin && hour == alarmHour && state != 3) {
		state = 3;
	}

	if(state == 3) {
		alarmCounter++;

		if (*buzzer_ptr == 0) {
			*buzzer_ptr = 1;
		}

		else {
			*buzzer_ptr = 0;
		}


		if (alarmCounter > 5000) {
			alarmCounter = 0 ;
			state = 1;
			*buzzer_ptr = 0;
			*leds_ptr = 0;
		}
	}
}

void incrementSeconds() {
	if (state == 1) {
		sec++;
		if( sec >= 60) {
			sec = 0;
		}
	}

	else if (state == 2){
		alarmSec++;
		if( alarmSec >= 60) {
			alarmSec = 0;
		}
	}
}

void incrementMinutes() {
	if (state == 1) {
		min++;
		if( min >= 60) {
			min = 0;
		}
	}

	else if (state == 2){
		alarmMin++;
		if( alarmMin >= 60) {
			alarmMin = 0;
		}
	}
}

void incrementHours() {
	if(state == 1) {
		hour++;
		if(hour >= 24) {
			hour = 0;
		}
	}

	else if (state == 2) {
		alarmHour++;
		if(alarmHour >= 24) {
			alarmHour = 0;
		}
	}
}

void setAlarmState() {

	if(state == 1) {
		state =  2;
	}

	else {
	    state = 1;
	}
}


static void keys_irs(void * context){
	printf("inside inteerurpt\n");
	volatile int * edge_capture_ptr = (volatile int*) context;
	*edge_capture_ptr = *(volatile int *)(KEYS_BASE + 12);
	*keys_mask_ptr = 0xf;
	*keys_edge_ptr = *edge_capture_ptr;

	int key = *keys_ptr;

		if(key == 14) { //if key 0
			incrementSeconds();
		}

		else if(key == 13) { //if key 1
			incrementMinutes();
		}

		else if (key == 11) { //if key 2
			incrementHours();
		}

		else if (key == 7) { //if key 3
			setAlarmState();
		}
}

static void uart_irs(void * context){
	int com = (int)*uart_ptr;
	printf("%d\n", com);
	*uart_status_ptr = 0x00;
	*uart_ctrl_ptr = 0x80;

	if(com == 97){
		printf("Alarm loop\n");
		setAlarmState();
		 //delay(1);
	 }
			 if(com == 115){
				 printf("Horas loop\n");
				 incrementHours();
				 //delay(1);
			 }
			 if(com == 100){
				 printf("Minutos loop\n");
				 incrementMinutes();
				 //delay(1);
			 }
			 if(com == 102){
				 printf("SEg loop\n");
				 incrementSeconds();
				 //delay(1);
			 }
}

void printTime(int* hex_ptr, int number) {

	int hex;

	if (number == 0){ hex = 0x40; }

	else if(number == 1) { hex = 0x79; }

	else if(number == 2) { hex = 0x24; }

	else if(number == 3) { hex = 0x30; }

	else if(number == 4) { hex = 0x19; }

	else if(number == 5) { hex = 0x12; }

	else if(number == 6) { hex = 0x02; }

	else if(number == 7) { hex = 0x78; }

	else if(number == 8) { hex = 0x00; }

	else if(number == 9) { hex = 0x18; }

	else {	hex = 0x7F;	}

	*hex_ptr = hex;

}

void alarmClock() {

	if( state == 1 ) {
		printTime(hex0_ptr, sec%10);
		printTime(hex1_ptr, sec/10);
		printTime(hex2_ptr, min%10);
		printTime(hex3_ptr, min/10);
		printTime(hex4_ptr, hour%10);
		printTime(hex5_ptr, hour/10);
	}

	else if (state == 2){

		if(counter < 600) {
			printTime(hex0_ptr, alarmSec%10);
			printTime(hex1_ptr, alarmSec/10);
			printTime(hex2_ptr, alarmMin%10);
			printTime(hex3_ptr, alarmMin/10);
			printTime(hex4_ptr, alarmHour%10);
			printTime(hex5_ptr, alarmHour/10);
		}

		else {
			printTime(hex0_ptr, -1);
			printTime(hex1_ptr, -1);
			printTime(hex2_ptr, -1);
			printTime(hex3_ptr, -1);
			printTime(hex4_ptr, -1);
			printTime(hex5_ptr, -1);
		}
	}

	else if(state == 3) {

		if(counter < 500) {
			printTime(hex0_ptr, 8);
			printTime(hex1_ptr, 8);
			printTime(hex2_ptr, 8);
			printTime(hex3_ptr, 8);
			printTime(hex4_ptr, 8);
			printTime(hex5_ptr, 8);
			*leds_ptr = 767;
		}

		else {
			printTime(hex0_ptr, -1);
			printTime(hex1_ptr, -1);
			printTime(hex2_ptr, -1);
			printTime(hex3_ptr, -1);
			printTime(hex4_ptr, -1);
			printTime(hex5_ptr, -1);
			*leds_ptr = 0;
		}


	}

}

//-----------------------------------------------------------------------
// main functions
//-----------------------------------------------------------------------
int main()
{


  printf("Hello from Nios II!\n");

  *buzzer_ptr = 0;

  //timer interruption
  alt_ic_isr_register (TIMER_IRQ_INTERRUPT_CONTROLLER_ID, TIMER_IRQ, timer_irs, 0x00, 0x00);
  *timer_control_ptr = 7; //	initialize timer
  *timer_status_ptr = 0;  //    reset status flag

  //button interruption
  void* edge_capture_ptr = (void*) &edge_capture;
  *keys_mask_ptr = 0xF;
  *keys_edge_ptr = 0xF;
  *keys_direction_ptr = 0;

  alt_ic_isr_register( KEYS_IRQ_INTERRUPT_CONTROLLER_ID, KEYS_IRQ, keys_irs, edge_capture_ptr, 0);

  *uart_ctrl_ptr = 0x80;
  alt_ic_isr_register( UART_IRQ_INTERRUPT_CONTROLLER_ID, UART_IRQ, uart_irs, 0x00, 0x00);


  printf("Entering loop\n");

  while(1) {
	 alarmClock();
  }

  return 0;

}

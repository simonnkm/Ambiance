/*
 * Name FIFO.c
 * Brief: provides a library to create a First in first out queue
 * Author: Caitlin Bonesio
 * Created: 4/15/25
 * Modified: 4/15/25
 */

//----------------------------------------Private Includes---------------------------------------
#include "FIFO.h"
#include <stdlib.h>

//----------------------------------------Private Defines----------------------------------------


//----------------------------------------Private Typedefs---------------------------------------


//----------------------------------------Private Variables--------------------------------------


//----------------------------------------Private Functions--------------------------------------


//----------------------------------------Public Functions---------------------------------------
FIFO FIFO_Create(void){
	FIFO new = malloc(sizeof(struct _FIFO));
	if(new == NULL){
		return NULL;
	}
	new->first = NULL;
	new->last = NULL;
	new->size = 0;
	return new;
}

uint8_t FIFO_Destroy(FIFO doomed){
	if(doomed == NULL){
		return 0;// operation failled
	}
	node* cur = doomed->first;
	while(cur != NULL){
		node* last = cur;
		cur = cur->next;
		free(last);
	}
	free(doomed);
	return 1;
}

uint8_t FIFO_Enqueue(FIFO list, Event_t event){
	if(list == NULL){
		return 0;
	}
	node* new = malloc(sizeof(node));
	if(new == NULL){
		return 0;
	}
	new->event = event;
	new->next = NULL;
	if(list->last != NULL){//predefined list
		list->last->next = new;
	}else{//new list
		list->first = new;
	}
	list->last = new;
	list->size++;
	return 1;
}

Event_t FIFO_Dequeue(FIFO list){
	if(list == NULL){
		return (Event_t){EVENT_ERROR, 0};
	}
	if(list->first == NULL){
		list->last = NULL;// It is preferable to have a memory leak, rather than crash the system
		return (Event_t){EVENT_NONE, 0};
	}
	node* doomed = list->first;
	list->first = doomed->next;
	if(list->first == NULL){
		list->last = NULL;
	}
	Event_t out = doomed->event;
	free(doomed);
	list->size--;
	return out;

}

bool FIFO_GetSize(FIFO list){
	return list->size;
}

//----------------------------------------Private Test Harness-----------------------------------
//#define FIFOTESTHARNESS

#ifdef FIFOTESTHARNESS
#include "BOARD.h"
#include "UART.h"
#include <stdio.h>
#include <string.h>

int main(){
	HAL_Init();
	BOARD_Init();
	BSP_LED_Init(LED_BLUE);
	BSP_LED_Init(LED_RED);
	UART_Init();
	//BEHOLD! discount printf;
	char hellowords[200] = "Hello World!\n";
	for(int i = 0; i < strlen(hellowords); i++){
		USART_WriteTx(hellowords[i]);
	}
	FIFO list1 = FIFO_Create();
	FIFO_Enqueue(list1, (Event_t){EVENT_I2C, 32});
	Event_t test = FIFO_Dequeue(list1);
	FIFO_Enqueue(list1, (Event_t){EVENT_I2C, 32});
	FIFO_Enqueue(list1, (Event_t){EVENT_BUTTONS, 57});

	while (1);


}
#endif


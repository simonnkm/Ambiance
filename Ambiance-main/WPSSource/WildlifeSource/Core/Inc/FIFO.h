/*
 * Name FIFO.h
 * Brief: provides a library to initialize and access a free running timer
 * Author: Caitlin Bonesio
 * Created: 4/15/25
 * Modified: 4/15/25
 */

#ifndef FIFO_H//FIFO HEADER GUARDS
#define FIFO_H//FIFO HEADER GUARDS

#include "Events.h"
#include "stdbool.h"
#include <stdint.h>

//----------------------------------------Public Typedefs----------------------------------------
typedef struct node{
	Event_t event;
	struct node* next;
}node;

struct _FIFO{
	 node* first;
	 node* last;
	 uint16_t size;
};

typedef struct _FIFO* FIFO;

//----------------------------------------Public Functions---------------------------------------
FIFO FIFO_Create(void);

uint8_t FIFO_Destroy(FIFO doomed);

uint8_t FIFO_Enqueue(FIFO list, Event_t event);

Event_t FIFO_Dequeue(FIFO list);

bool FIFO_GetSize(FIFO list);
#endif //FIFO HEADER GUARDS

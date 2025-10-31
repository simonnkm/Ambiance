/*
 * Name Events.h
 * Brief: Provides
 * Author: Caitlin Bonesio
 * Created: 4/15/25
 * Modified: 4/21/25
 */

#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>



typedef enum Event_Type{
	EVENT_ERROR = -1,
	EVENT_NONE = 0,
	EVENT_INIT,
	EVENT_ENTRY,
	EVENT_EXIT,
	EVENT_BUTTONS,
	EVENT_I2C,
	EVENT_USART,
	EVENT_USART_READY,
	EVENT_LPUART,
	EVENT_TIMEOUT,
	EVENT_SETTINGS,
	EVENT_PLAY,
	EVENT_BLE,
}Event_Type;

typedef struct EventReturntype{
	Event_Type status;
	uint16_t data;
}Event_t;
#endif /* EVENTS_H */

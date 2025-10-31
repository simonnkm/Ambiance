/*
 * Name COMM.c
 * Brief: a module to listen for signals from the GUI interface
 * Author: Caitlin Bonesio
 * Created: 4/15/25
 * Modified: 5/16/25
 */

//----------------------------------------Private Includes---------------------------------------
#include "COMM.h"

#include "FIFO.h"
#include "UART.h"
#include "DiscountIO.h"
#include "FLASH.h"
#include "BLUETOOTH.h"
#include "I2C.h"

#include <stdbool.h>

//----------------------------------------Private Defines----------------------------------------
#define VOLUMECONTROL   0x00
#define FOLDERCONTROL   0x01
#define LOGSREQUEST     0x02
#define LOGSDONE        0x03
#define DCCONTROL       0x04
#define SCHEDULECONTROL 0x05
#define SCHEDULEEND     0x0D
#define DEBUGPRINT   	0x0E
#define SETTIME			0x0F
#define DEBUGPRINTEND  	'\n'

#define MAXSCHEDULEEVENTS 180

#define PLACEHOLDER    (10*6)//ten events

//----------------------------------------Private Typedefs---------------------------------------
 typedef enum COMMSTATES{
	 init = 0,
	 idle,
	 volumecontrol,
	 foldercontrol,
	 folderselected,
	 logsrequest,
	 logsdata,
	 dccontrol,
	 schedulecontrol,
	 schedulemonth,
	 scheduledaystart,
	 schedulestart,
	 scheduledaystop,
	 schedulestop,
	 schedulefolder,
	 scheduletrack,
	 scheduleend,
	 timeminute,
	 timehour,
	 timeday,
	 timemonth,
 }COMMSTATES_t;


//----------------------------------------Private Variables--------------------------------------
 COMMSTATES_t commSM;
 FIFO COMMqueue;
 static uint8_t sendinglogs;
//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: EVENT_COMM_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t COMM_Event_Init(FIFO Queue){
	commSM = init;
	COMMqueue = Queue;
	UARTs_Init();
	FLASH_Init();
	COMM_Event_Post((Event_t){EVENT_INIT, 0});
	return INIT_OK;
}

void COMM_Event_Post(Event_t event){
	FIFO_Enqueue(COMMqueue, event);
}

/*
 * @Function: EVENT_COMM_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the 
 * @param: none
 * @return: An event 
 */
Event_t COMM_Event_Updater(void){
	uint8_t event = 0;
    Event_t out = (Event_t){EVENT_NONE, 0};
    char input = USART_ReadRx();
    if(input != UARTFAILED){
    	event = 1;
    	out.status = EVENT_USART;
    	out.data = (uint16_t)input;
    }
    if(sendinglogs == 1){
    	if(USART_TxEmpty() || BLUETOOTH_BufferEmpty() == 1){
    		FIFO_Enqueue(COMMqueue, (Event_t){EVENT_USART_READY, 0});
    	}
    }
	if(event){FIFO_Enqueue(COMMqueue, out);}
    return out;
}

/*
 * @Function: EVENT_COMM_Event_Handler
 * @Brief: 
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t COMM_Event_Handler(Event_t event){
	bool transition = 0;

	static uint8_t folder;
	COMMSTATES_t next = commSM;
	//char text[50];
	switch(commSM){
	case init:
		if(event.status == EVENT_INIT){
			next = idle;
			transition = true;
		}
		break;
	case idle:
		if(event.status == EVENT_ENTRY){
			sendinglogs = 0;
		}
		if(event.status == EVENT_USART){
			switch (event.data){
			case VOLUMECONTROL:
				//discountprintf("received volume control");
				next = volumecontrol;
				transition = true;
				break;
			case FOLDERCONTROL:
				//discountprintf("received folder control");
				next = foldercontrol;
				transition = true;
				break;
			case LOGSREQUEST:
				//discountprintf("received logs request");
				next = logsrequest;
				transition = true;
				break;
			case DCCONTROL:
				//discountprintf("received DC control");
				next = dccontrol;
				transition = true;
				break;
			case SCHEDULECONTROL:
				//discountprintf("received schedule control");
				next = schedulecontrol;
				transition = true;
				break;
			case SETTIME:
				//discountprintf("received set time control");
				next = timeminute;
				transition = true;
				break;
			default:
				break;
			}
		}
		break;
	case volumecontrol:
		if(event.status == EVENT_USART){
			if(FLASH_SetDCVol((uint8_t)event.data, FLASH_GetDutyCycle())==0){
				discountprintf("failed to set volume");
			}
			//sprintf(text, "Storing volume %d", FLASH_GetVolume());
			//discountprintf(text);
			next = idle;
			transition = true;
		}
		break;
	case foldercontrol:
		if(event.status == EVENT_USART){
			folder = event.data;
			//discountprintf("Storing folder selector");
			next = folderselected;
			transition = true;
		}
		break;
	case folderselected:
		if(event.status == EVENT_USART){
			//post to mp3 controller with the new data
			Event_t play = (Event_t){EVENT_PLAY, (folder<<8) + (event.data)};
			MP3_Event_Post(play);
			//discountprintf("Sending track selector");
			next = idle;
			transition = true;
		}
		break;
	case logsrequest:
		static uint32_t sent;
		if(event.status == EVENT_ENTRY){
			sendinglogs = 1;
			sent = 0;
			uint16_t size = FLASH_GetLogsSize();
			//uint16_t size = 32;
			USART_WriteTx((uint8_t)(size>>8));
			USART_WriteTx((uint8_t)(size));
			next = logsdata;
			transition = true;
		}
		break;
	case logsdata:
		if(event.status == EVENT_USART_READY){
			//get logs size
			uint16_t size  = 32;
			//uint16_t size = FLASH_GetLogsSize();
			uint16_t sendsize = size - sent;
			if(size - sent > (uint16_t)(USARTBUFFERSIZE/6)){
				sendsize = (uint16_t)(USARTBUFFERSIZE/6);
			}
			for(int i = 0; i < sendsize; i++){
				//scheduleEvent levent = FLASH_ReadLogs(sent);
				scheduleEvent levent = {1, 1, 0b1001001, 0b1001010, 1, 1};
				USART_WriteTx(levent.month);
				USART_WriteTx(levent.daystart);
				USART_WriteTx(levent.start);
				USART_WriteTx(levent.daystop);
				USART_WriteTx(levent.stop);
				USART_WriteTx(levent.folder);
				USART_WriteTx(levent.track);
				sent++;
			}
			if(size - sent <= (uint16_t)(USARTBUFFERSIZE/6)){
				USART_WriteTx(LOGSDONE);
				if(size >= 255){
					discountprintf("logs buffer overflowed, most recent data has been lost");
				}
				//send last of logs here
				sent = 0;
				sendinglogs = 0;
				next = idle;
				transition = true;
			}
		}
		break;
	case dccontrol:
		if(event.status == EVENT_USART){
			if(FLASH_SetDCVol(FLASH_GetVolume(), (uint8_t)event.data) == 0){
				discountprintf("failed to set duty cycle");
			}
			//sprintf(text, "Storing volume %d", FLASH_GetDutyCycle());
			//discountprintf(text);

			next = idle;
			transition = true;
		}
		break;
	case schedulecontrol:
		static uint8_t numevents;
		static scheduleEvent sevent;
		if(event.status == EVENT_ENTRY){
			//discountprintf("receiving schedule");
			sevent = (scheduleEvent){0,0,0,0,0,0};
			if(FLASH_ClearSchedule()== 0){
				discountprintf("failed to clear schedule");
			}
			next = schedulemonth;
			transition = true;
			numevents = 0;
		}
		break;
	case schedulemonth:
		if(event.status == EVENT_USART){
			if(event.data == SCHEDULEEND){
				discountprintf("schedule complete");
				next = idle;
				transition = true;
				FLASH_AppendSchedule(sevent);
			} else if(numevents > MAXSCHEDULEEVENTS){
				discountprintf("schedule overflow, forced to complete");
				FLASH_AppendSchedule(sevent);
				next = scheduleend;
				transition = true;
				USART_WriteTx(SCHEDULEEND);//please stop sending me the schedule
			}else{
				if(numevents != 0){
					FLASH_AppendSchedule(sevent);
				}
				sevent.month = event.data;
				//sprintf(text, "Month: %d", event.data);
				//discountprintf(text);
				//store month here
				next = scheduledaystart;
				transition = true;
			}
		}
		break;
	case scheduledaystart:
		if(event.status == EVENT_USART){
			sevent.daystart = event.data;
//			sprintf(text, "Day: %d", event.data);
//			discountprintf(text);
			next = schedulestart;
			transition = true;
		}
		break;
	case schedulestart:
		if(event.status == EVENT_USART){
			sevent.start =  event.data;
//			sprintf(text, "start time: %d:%d", (event.data&0b11111000)>>3, (event.data & 0b011)*15);
//			discountprintf(text);
			next = scheduledaystop;
			transition = true;
		}
		break;
	case scheduledaystop:
		if(event.status == EVENT_USART){
			sevent.daystop = event.data;
//			sprintf(text, "Day: %d", event.data);
//			discountprintf(text);
			next = schedulestop;
			transition = true;
		}
		break;
	case schedulestop:
		if(event.status == EVENT_USART){
			sevent.stop = event.data;
//			sprintf(text, "end time: %d:%d", (event.data&0b11111000)>>3, (event.data & 0b011)*15);
//			discountprintf(text);
			next = schedulefolder;
			transition = true;
		}
		break;
	case schedulefolder:
		if(event.status == EVENT_USART){
			sevent.folder = event.data;
//			sprintf(text, "folder#: %d", event.data);
//			discountprintf(text);
			next = scheduletrack;
			transition = true;
		}
		break;
	case scheduletrack:
		if(event.status == EVENT_USART){
			sevent.track = event.data;
//			sprintf(text, "track#: %d", event.data);
//			discountprintf(text);
			//record end time here
			next = schedulemonth;
			transition = true;
			numevents++;
		}
		break;
	case scheduleend:
		if(event.status == EVENT_USART){
			if(event.data == SCHEDULEEND){
				next = idle;
				transition = true;
			}
		}
		break;
	case timeminute:
		if(event.status == EVENT_USART){
			if(event.data < 60){
				I2C_Transmit(RTCADDRESS, RTCMINADDR, ((event.data/10)<<4) | event.data%10);
				next = timehour;
				transition = true;
			}else{

				discountprintf("failed set time: minute");
				next = idle;
				transition = true;
			}
		}

		break;
	case timehour:
		if(event.status == EVENT_USART){
			if(event.data < 24){
				I2C_Transmit(RTCADDRESS, RTCHOURADDR, ((event.data/10)<<4) | event.data%10);
				next = timeday;
				transition = true;
			}else{
				discountprintf("failed set time: hour");
				next = idle;
				transition = true;
			}
		}

		break;
	case timeday:
		if(event.status == EVENT_USART){
			if(event.data <= 31){
				I2C_Transmit(RTCADDRESS, RTCDAYADDR, ((event.data/10)<<4) | event.data%10);
				next = timemonth;
				transition = true;
			}else{
				discountprintf("failed set time: day");
				next = idle;
				transition = true;
			}
		}
		break;
	case timemonth:
		if(event.status == EVENT_USART){
			if(event.data <= 12){
				I2C_Transmit(RTCADDRESS, RTCMNTHADDR, ((event.data/10)<<4) | event.data%10);
				next = idle;
				transition = true;
			}else{
				discountprintf("failed set time: month");
				next = idle;
				transition = true;
			}
		}

		break;
	default:
		break;
	}
	if(transition){
		COMM_Event_Handler((Event_t){EVENT_EXIT});
		commSM = next;
		COMM_Event_Handler((Event_t){EVENT_ENTRY});
	}
	return 1;
}


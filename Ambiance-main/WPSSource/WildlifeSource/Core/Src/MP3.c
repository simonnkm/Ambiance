/*
 * Name MP3.c
 * Brief: a MP3 to use when creating event/service modules
 * Author: Caitlin Bonesio
 * Created: 4/19/25
 * Modified: 4/23/25
 */


//----------------------------------------Private Includes---------------------------------------
#include "CONFIG.h"
#include "MP3.h"
#include "TIMERS.h"
#include "FLASH.h"
#include "UART.h"
#include "Scheduler.h"
#include "discountIO.h"
#include "FIFO.h"

#include <stdio.h>
#include <string.h>
//----------------------------------------Private Defines----------------------------------------
#define CYCLELENGTH (10/*minutes*/*60000/*milliseconds/minute*/)//duty cycle time in milliseconds
#define RESETTIMER 2000
#define FAILEDTRACKRECOVERY (10/*minutes*/*60000/*milliseconds/minute*/)


//----------------------------------------Private Typedefs---------------------------------------
typedef enum DFPacketStates{
	Start,
	Version,
	Length,
	Command,
	Ack,
	Param1,
	Param2,
	Checksum1,
	Checksum2,
}DFPacketState_t;

typedef struct DFPacket{
	uint8_t command;
	uint8_t ack;
	uint8_t Param1;
	uint8_t Param2;
}DFPacket_t;

typedef enum DFinitSM{
	setup,
	scanning,
}DFinitSM_t;


//----------------------------------------Private Variables--------------------------------------

extern RNG_HandleTypeDef hrng;
FIFO MP3queue;

static uint8_t pause;//should the mp3 player be playing. 0x01 for pause in DC, 0x02 for stop until next play event
static uint8_t DC;//how long until the next cycle of the duty cycle
static uint8_t volume;

static uint32_t starttime;
static uint32_t endtime;

static uint32_t inittime;
static uint8_t initialized;
static DFinitSM_t initSM;

static DFPacketState_t PacketSM;
static DFPacket_t Packet;

static uint8_t track;
static uint8_t folder;
static uint8_t firsttrack;
static uint8_t nexttrack;
static uint8_t lastplayed;
static uint8_t* folders;
static uint8_t numfolders;

static char lastsent[4];
//----------------------------------------Private Functions--------------------------------------
void MP3_SendData(char string[4]){
	uint16_t accumulation = 0;
	LPUART_WriteTx(0x7E);//start flag
	accumulation += 0xFF;
	LPUART_WriteTx(0xFF);//Version?
	accumulation += 0x06;
	LPUART_WriteTx(0x06);//length

	for(int i = 0; i < 4; i++){
		accumulation += string[i];
		lastsent[i] = string[i];
		LPUART_WriteTx(string[i]);//command and data
	}
	accumulation = -accumulation;//??required for it to work
	LPUART_WriteTx(accumulation>>8);//checksum
	LPUART_WriteTx(accumulation&0xFF);//checksum
	LPUART_WriteTx(0xEF);//start flag

}

uint8_t parsePacket(char rx){
	if(rx == UARTFAILED){
		return 0;
	}
	if(rx == 0x7E){
		PacketSM = Start;
	}
	switch (PacketSM){
	case Start:
		if(rx == 0xFF){
			PacketSM = Version;
		}
		break;
	case Version:
		if(rx == 0x06){
			PacketSM = Length;
		} else {
			PacketSM = Start;
		}
		break;
	case Length:
		PacketSM = Command;
		Packet.command = rx;
		break;
	case Command:
		if(rx == 0x01 || rx == 0x00){
			PacketSM = Ack;
		} else {
			PacketSM = Start;
		}
		break;
	case Ack:
		PacketSM = Param1;
		Packet.Param1 = rx;

		break;
	case Param1:
		PacketSM = Param2;
		Packet.Param2 = rx;
		break;
	case Param2:
		uint16_t checkval = -(0x105+Packet.command+Packet.ack+Packet.Param1 +Packet.Param2);
		if(rx == checkval>>8){
			PacketSM = Checksum1;
		}else {
			PacketSM = Start;
			char send[4] = {0x40, 0x00, 0x00, 0x06};
			MP3_SendData(send);
		}
		break;
	case Checksum1:
		checkval = -(0x105+Packet.command+Packet.ack+Packet.Param1 +Packet.Param2);
		if(rx == (checkval&0xFF)){
			PacketSM = Checksum2;
		}else {
			PacketSM = Start;
			char send[4] = {0x40, 0x00, 0x00, 0x06};
			MP3_SendData(send);
		}
		break;
	case Checksum2:
		if(rx == 0xEF){
			if(Packet.command == 0x40){
				BSP_LED_On(LED_RED);
				discountprintf("%X %X",Packet.Param1, Packet.Param2);
				if(Packet.Param2 == 0x04){
//					if(lastsent[0]==0x03){
//						uint32_t rand = 0;
//						HAL_RNG_GenerateRandomNumber(&hrng, &rand);
//						rand &= 0xFF;//convert it to one byte of random data
//						rand = (rand*(folders[folder-1])-1)/0xFF;//convert the one bye to the range of 0-max tracks-1
//						char send[4] = {0x03, 0x00, 0x00, firsttrack+rand};
//						MP3_SendData(send);
//					}else {
						MP3_SendData(lastsent);
//					}

					PacketSM = Start;
					return 0;
				}
			}
			PacketSM = Start;
			return 1;
		}
		PacketSM = Start;
		break;
	}
	return 0;
}
//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: MP3_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t MP3_Event_Init(FIFO Queue){
	//somehow need to be sent 3 seconds after the mp3 turns on
    MP3queue = Queue;
    TIMERS_Init();
    pause = 0x02;
    DC = FLASH_GetDutyCycle();
    volume = FLASH_GetVolume();
    starttime = TIMERS_GetMilliSeconds();
    inittime = TIMERS_GetMilliSeconds();
    initSM = setup;
    initialized = 0;
	{
	char send[4] = {0x0C, 0x00, 0x00, 0x00};//reset module
	MP3_SendData(send);
	}

    return INIT_OK;
}
/*
 * @Function: MP3_Event_Init
 * @Brief: Provides the ability for state machines to interact
 * @param: event to be posted
 * @return: none
 */
void MP3_Event_Post(Event_t event){
    FIFO_Enqueue(MP3queue, event);
}


/*
 * @Function: MP3_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the
 * @param: none
 * @return: An event
 */
Event_t MP3_Event_Updater(void){
    Event_t event = (Event_t){EVENT_NONE, 0};
    uint32_t timer = TIMERS_GetMilliSeconds();
    char rx = LPUART_ReadRx();
    //Check for LPUART events
	if(rx != UARTFAILED){
		if(parsePacket(rx)){
			event.status = EVENT_LPUART;
			event.data = rx;

			if(!initialized){
				event.status = EVENT_INIT;
				event.data = 1;//indicate a packet is ready
			}

			MP3_Event_Post(event);
		}
	}
	//checks a timer to un-pause the speaker to adhere to the duty cycle
	if(pause == 1 && initialized){
		uint32_t waittime = (endtime-starttime)*((100.0/((double)DC))-1);//Pause time = Active time * (1/DC-1)
		uint32_t curtime = TIMERS_GetMilliSeconds();
		if(curtime >= endtime+waittime){
			event.status = EVENT_TIMEOUT;
			MP3_Event_Post(event);

		}

	}
	//error recover, the MP3 Player should have finished the track by now
	if(pause == 0 && initialized){
		if(TIMERS_GetMilliSeconds() > FAILEDTRACKRECOVERY){
			Packet.command = 0x3D;//hijack the system to pretend like the track ended
			Packet.Param2 = track;
			starttime = TIMERS_GetMilliSeconds();
			event.status = EVENT_LPUART;
			MP3_Event_Post(event);
		}
	}

	//check for init timer events
    if(((timer-inittime) >= 3000) && !initialized){//wait for the speaker to be ready for use n initialization
		event.status = EVENT_INIT;
		event.data = 0;//indicate a timer ran out
		MP3_Event_Post(event);
	}

    //check for settings update events
    if((volume != FLASH_GetVolume() || DC != FLASH_GetDutyCycle())&&initialized){
    	event.status = EVENT_SETTINGS;
    	event.data = (volume == FLASH_GetVolume());
		MP3_Event_Post(event);
    }
    return event;
}

/*
 * @Function: MP3_Event_Handler
 * @Brief:
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t MP3_Event_Handler(Event_t event){
	if(event.status == EVENT_INIT){
//		static FIFO tempFolders;
//		switch (initSM){
//		case setup:
//			if(event.data == 0){
//				{
//				char send[4] = {0x0E, 0x00, 0x00, 0x00};//pause
//				MP3_SendData(send);
//				}
//				numfolders = 1;
//				tempFolders = FIFO_Create();
//				inittime = TIMERS_GetMilliSeconds();
//
//				{
//				char send[4] = {0x4E, 0x00, 0x00, numfolders};//query the number of files on the micrSD
//				MP3_SendData(send);
//				}
//				initSM = scanning;
//			}
//			break;
//		case scanning:
//			if(event.data == 0){//timeout
//				{
//				char send[4] = {0x4E, 0x00, 0x00, numfolders};//query the number of files on the micrSD
//				MP3_SendData(send);
//				}
//				inittime = TIMERS_GetMilliSeconds();
//			}else if(event.data == 1){//packet complete
//
//				if(Packet.command==0x4E){//MP3 returned the query with then number of files in the folder
//					numfolders++;
//					FIFO_Enqueue(tempFolders, (Event_t){EVENT_NONE, Packet.Param2});
//
//					{
//					char send[4] = {0x4E, 0x00, 0x00, numfolders};//query the number of files on the micrSD
//					MP3_SendData(send);
//					}
//					inittime = TIMERS_GetMilliSeconds();
//				}else if(Packet.command == 0x40){//The MP3 return an error for no folder found, meaning we ran out of folders to scan
//
//					numfolders--;
//					folders = malloc(sizeof(uint8_t)*numfolders);
//					for(int i = 0; i < numfolders; i++){
//						folders[i] = FIFO_Dequeue(tempFolders).data;
//					}
//					FIFO_Destroy(tempFolders);
//					initialized = 1;
//					char send[4] = {0x06, 0x00, 0x00, (uint8_t)(((uint16_t)volume)*30/100)};
//					MP3_SendData(send);
//				}//^scanning state
//			}
//			break;
//		default:
//
//			break;
//		}
		{
		char send[4] = {0x0E, 0x00, 0x00, 0x00};//pause
		MP3_SendData(send);
		}
		uint8_t scanning = 1;
		numfolders = 1;
		FIFO tempFolders = FIFO_Create();
		while(scanning){//^setup state
			{
			char send[4] = {0x4E, 0x00, 0x00, numfolders};//query the number of files on the micrSD
			MP3_SendData(send);
			}
			uint32_t time = TIMERS_GetMilliSeconds();
			while(!parsePacket(LPUART_ReadRx()) && (time+1000) > TIMERS_GetMilliSeconds());
			if((time+1000) < TIMERS_GetMilliSeconds()){
				continue;// reattempt communication
			}
			if(Packet.command==0x4E){
				numfolders++;
				FIFO_Enqueue(tempFolders, (Event_t){EVENT_NONE, Packet.Param2});
			}else if(Packet.command == 0x40){
				scanning = 0;
			}//^scanning state
		}
		numfolders--;
		folders = malloc(sizeof(uint8_t)*numfolders);
		for(int i = 0; i < numfolders; i++){
			folders[i] = FIFO_Dequeue(tempFolders).data;
		}
		FIFO_Destroy(tempFolders);
		initialized = 1;
		char send[4] = {0x06, 0x00, 0x00, (uint8_t)(((uint16_t)volume)*30/100)};
		MP3_SendData(send);
	}
	if(event.status == EVENT_TIMEOUT){
		//restart the MP3 player
		pause = 0;
		starttime = TIMERS_GetMilliSeconds();
		track = nexttrack;
		lastplayed = 0;//removes the devices memory of the last played track to allow repeat songs
		char send[4] = {0x03, 0x00, 0x00, firsttrack+nexttrack-1};
		MP3_SendData(send);

	}if(event.status == EVENT_PLAY){
		char text[30];
		sprintf(text, "Play event: %d, %d", event.data>>8, (event.data&0xFF));
		discountprintf(text);
		Scheduler_Event_Post(event);
		starttime = TIMERS_GetMilliSeconds();
		//if 0 < folder <= max folders and 0 < track <= max tracks in given folder
		if(event.data>>8 != 0 && (event.data>>8) <= numfolders){
			if((event.data&0xFF)!= 0 && (event.data&0xFF) <= folders[(event.data>>8)-1]){
				pause = 0;
				//store the desired track and folder
				folder = event.data>>8;
				track = event.data&0xFF;
				//calculate the absolute position of the desired track
				firsttrack = 1;
				for(int i = 0; i < folder-1; i ++){
					firsttrack+= folders[i];
				}
				//update the desired track to the MP3
				{
				char send2[4] = {0x03, 0x00, 0x00, firsttrack+track-1};
				MP3_SendData(send2);
				HAL_Delay(100);
				}
				//send a play signal
				{
				char send[4] = {0x0D, 0x00, 0x00, 0x00};
				MP3_SendData(send);
				}
				lastplayed = 0;//allow the event play to play the same track that was played before
			}
		}else {

			pause = 0x02;
			{
			char send[4] = {0x0E, 0x00, 0x00, 0x00};//pause
			MP3_SendData(send);
			}
			//set folder and track to zero, this is so that when other modules query/track, a zero can indicate paused
			folder = 0;
			track = 0;
		}


	}if(event.status == EVENT_SETTINGS){
		DC = FLASH_GetDutyCycle();
		volume = FLASH_GetVolume();
//		char text[30];
//		sprintf(text, "Settings event: %d, %d", event.data>>8, (event.data&0xFF));
//		discountprintf(text);
		if(!event.data){//volume == FLASH_GetVolume()
			//send new volume to the mp3 player
			char send[4] = {0x06, 0x00, 0x00, (uint8_t)(((uint16_t)volume)*30/100)};
			MP3_SendData(send);
		}
	}if (event.status == EVENT_LPUART){
		if(Packet.command == 0x3D){//song complete
			if(Packet.Param2 != lastplayed){//prevent the mp3 play from sending the same command twice
				lastplayed = Packet.Param2;
				endtime = TIMERS_GetMilliSeconds();
				if(folder){
					uint32_t rand = 0;
					do{//provide a simplistic method of out of bounds detection (should be unused)
						HAL_RNG_GenerateRandomNumber(&hrng, &rand);
						rand &= 0xFF;//convert it to one byte of random data
						rand = (rand*(folders[folder-1])-1)/0xFF;//convert the one bye to the range of 0-max tracks-1
						nexttrack = rand+1;
					}while(nexttrack > folders[folder-1]);//prevent repeat tracks to avoid confusion the same command twice code above
					track = 0;
					pause = 1;//entering duty cycle pause
					char send[4] = {0x0E, 0x00, 0x00, 0x00};//pause
					MP3_SendData(send);
				}
			}

		}

	}
    return 1;
}

/*
 * @Function: MP3_GetCurrentFile
 * @Brief: gets the current track being played on the mp3 player
 * @param:
 * @return: two bytes, the upper 8bits contain the folder, the lower 8 bits hold the tracck number in the folder
 */
uint16_t MP3_GetCurrentFile(){
	return ((uint16_t)folder<<8)+track;
}

//----------------------------------------Test Harness-------------------------------------------
//----------------------------------------Private Defines----------------------------------------
//#define MP3TESTHARNESS

#ifdef MP3TESTHARNESS
#include "BOARD.h"
#include "discountIO.h"

int main(){
	BOARD_Init();
	BSP_LED_Init(LED_BLUE);
	BSP_LED_Init(LED_RED);
	UARTs_Init();
	TIMERS_Init();
	{
	char send[4] = {0x0C, 0x00, 0x00, 0x00};//reset module
	MP3_SendData(send);
	HAL_Delay(1000);
	}


	{
	char send[4] = {0x0E, 0x00, 0x00, 0x00};//pause
	MP3_SendData(send);
	}
	HAL_Delay(100);
//	{
//	char send[4] = {0x08, 0x00, 0x00, 0x01};
//	MP3_SendData(send);
//	HAL_Delay(100);
//	}
	{
	char send[4] = {0x06, 0x00, 0x00, 0x10};
	MP3_SendData(send);
	HAL_Delay(100);
	}
	uint8_t scanning = 1;
		static uint8_t numfolders = 1;
		FIFO tempFolders = FIFO_Create();
		while(scanning){
			{
			char send[4] = {0x4E, 0x00, 0x00, numfolders};//query the number of files on the micrSD
			MP3_SendData(send);
			}
			while(!parsePacket(LPUART_ReadRx()));
			if(Packet.command==0x4E){
				numfolders++;
				FIFO_Enqueue(tempFolders, (Event_t){EVENT_NONE, Packet.Param2});
			}else if(Packet.command == 0x40){
				scanning = 0;
			}
		}
		numfolders--;
		uint8_t folders[numfolders];
		for(int i = 0; i < numfolders; i++){
			folders[i] = FIFO_Dequeue(tempFolders).data;
		}
		FIFO_Destroy(tempFolders);

//	{
//	char send[4] = {0x11, 0x00, 0x00, 0x01};
//	MP3_SendData(send);
//	HAL_Delay(1000);
//	}

//	char send1[4] = {0x0B, 0x00, 0x00, 0x00};//exit low power mode?
//	MP3_SendData(send1);
	//7E FF 06 0F 00 FOLDER#1 TRACK#1 xx EF
//	HAL_Delay(1000);


	HAL_Delay(100);
//	{
//	char send[4] = {0x17, 0x00, 0x00, folder};
//	MP3_SendData(send);
//	HAL_Delay(100);
//	}

	folder = 1;
	track = 1;
	firsttrack = 1;
	for(int i = 0; i < folder-1; i ++){
		firsttrack+= folders[i];
	}
	{
	char send2[4] = {0x03, 0x00, 0x00, firsttrack+track-1};
	MP3_SendData(send2);
	HAL_Delay(100);
	}
	{
	char send[4] = {0x0D, 0x00, 0x00, 0x00};
	MP3_SendData(send);
	}

	while (1){//print response to console
//		if(TIMERS_GetMilliSeconds()%1000){
//			if(TIMERS_GetMilliSeconds()%2000){
//				BSP_LED_On(LED_RED);
//			}else {
//				BSP_LED_Off(LED_RED);
//			}
//		}
		char rx = LPUART_ReadRx();
		if(rx != UARTFAILED){
			if(parsePacket(rx)){
				if(Packet.command == 0x3D){
					if(Packet.Param2 != lastplayed){//prevent the mp3 play from sending double
						lastplayed = Packet.Param2;
						track++;
						HAL_Delay(100);
	//							char send[4] = {0x0D, 0x00, 0x00, 0x00};
	//							MP3_SendData(send);
						if(track > folders[folder-1]){
							char send2[4] = {0x03, 0x00, 0x00, firsttrack};
							MP3_SendData(send2);
							BSP_LED_On(LED_RED);
							track = 1;
						} else {

							char send[4] = {0x01, 0x00, 0x00, 0x00};
							MP3_SendData(send);
							BSP_LED_Off(LED_RED);
						}
					}

				}
			}
		}

//		if(rx == 0x3D){
//			{
//			char send[4] = {0x01, 0x00, 0x00, 0x00};
//			MP3_SendData(send);
//			}
//		}
		HAL_Delay(1);

	}

}
#endif

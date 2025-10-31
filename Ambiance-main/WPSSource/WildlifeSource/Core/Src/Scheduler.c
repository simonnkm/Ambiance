/*
 * Name Scheduler.c
 * Brief: the moudle that reads the schedule from the flash memory and the time from the rtc clock
 * 		  to control what to play and when.
 * 		  Also creates system logs
 * Author: Caitlin Bonesio
 * Created: 4/21/25
 * Modified: 4/23/25
 */


//----------------------------------------Private Includes---------------------------------------
#include "CONFIG.h"
#include "Scheduler.h"
#include "I2C.h"
#include "FLASH.h"
#include "TIMERS.h"
#include "discountIO.h"
//----------------------------------------Private Defines----------------------------------------
#define REFRESHRATE (.5/*minutes*/*60000/*milliseconds/minute*/)
#define NULLDATE    0xFF
//----------------------------------------Private Variables--------------------------------------
FIFO Schedulerqueue;
static uint32_t starttime;//timer

static uint8_t month;//I2C data
static uint8_t day;
static uint8_t hour;
static uint8_t minute;

static uint8_t logging;//logging
static uint8_t playdata;

static int16_t curschedule;//contains the current schedule being followed, -1 if no schedule active
//----------------------------------------Private Functions--------------------------------------
void CompareTime(){
	if(logging){
		scheduleEvent event;

		event.month = month;
		if(playdata){
			event.start = ((hour & 0b11111) << 3) & ((uint8_t)(minute/15));
			event.daystart = day;
			event.stop = 0;
			event.daystop = 0;
			event.folder = (playdata>>8)&0xFF;
			event.track = playdata&0xFF;
		} else {
			scheduleEvent prevevent = FLASH_ReadLogs(FLASH_GetLogsSize()-1);
			event.start = 0;
			event.daystart = 0;
			event.stop = ((hour & 0b11111) << 3) & ((uint8_t)(minute/15));
			event.daystop = day;
			event.folder = prevevent.folder;
			event.track = prevevent.track;
		}
		FLASH_AppendLogs(event);
		logging  = 0;
	} else {
		scheduleEvent event;
		Event_t play = (Event_t){EVENT_PLAY, 0};

		//allows for a double check to allow one schedule to end the same minute a second schedule starts
		//this is done with the manipulation of reset
		for(int reset = 0; reset < 1; reset++){
			if(curschedule == -1){//no active schedule
				//this section is designed to allow for the device to reboot inside of a scheduled time and pick up seamlessly
				for(uint16_t i = 0; i < FLASH_GetScheduleSize(); i++){
					event = FLASH_ReadSchedule(i);
					uint8_t secondmonth = event.month;
					//unpack the stored start and stop hour/minute
					uint8_t Shour = (event.start&0b11111000)>>3;
					uint8_t Ehour = (event.stop &0b11111000)>>3;
					uint8_t Smin  = (event.start&0b00000011)*15;
					uint8_t Emin  = (event.stop &0b00000011)*15;
					//if the stop day is before the start day, the schedule will run into the nest month
					//If the event runs over to the next month, set up second month so the time in the next month is handled
					if(event.daystart > event.daystop){
						secondmonth = event.month+1;
						//loop from December to January
						if(secondmonth == 13){
							secondmonth = 1;
						}
					}
					//tests if the current time is inside the period this schedule event says to play in
					if((
							event.month == month || secondmonth == month || event.month == 0
					   )&& //if within the month period
					   (
							( (event.daystart <= day && event.daystop >= day) && event.daystart <= event.daystop)||//within the period in single month mode
							( (event.daystart <= day || event.daystop >= day) && event.daystart >  event.daystop)  //if within the day period in multi-month mode
					   )&&
					   (
							(Shour < hour && Ehour> hour)||   //fully within the period
							(
								(
								(Shour == hour&& Smin <= minute)|| //In the start hour, after or on the start minute OR
								(Ehour == hour&& Emin > minute)   //In the end hour, before the stop minute
								) && (Shour != Ehour)
							)||
							(
								(
								(Shour == hour&& Smin <= minute)&& //In the start hour, after or on the start minute AND
								(Ehour == hour&& Emin > minute)   //In the end hour, before the stop minute
								) && (Shour == Ehour)
							)
					   )
					  ){

						play.data = (event.folder<<8) + event.track;//Update the MP3
						MP3_Event_Post(play);
						curschedule = i;
						break;
					}
				}
			}
			if(curschedule != -1){//active schedule
				event = FLASH_ReadSchedule(curschedule);
				uint8_t secondmonth = event.month;
				//unpack the stored start and stop hour/minute
				uint8_t Shour = (event.start&0b11111000)>>3;
				uint8_t Ehour = (event.stop &0b11111000)>>3;
				uint8_t Smin  = (event.start&0b00000011)*15;
				uint8_t Emin  = (event.stop &0b00000011)*15;
				//if the stop day is before the start day, the schedule will run into the nest month
				//If the event runs over to the next month, set up second month so the time in the next month is handled
				if(event.daystart > event.daystop){
					secondmonth = event.month+1;
					//loop from December to January
					if(secondmonth == 13){
						secondmonth = 1;
					}
				}
				//only check the hour and minute as this should happen at the end of every day's schedule to allow for overlapping schedules
				if(!(
					   (
							event.month == month || secondmonth == month || event.month == 0
					   )&& //if within the month period
					   (
							( (event.daystart <= day && event.daystop >= day) && event.daystart <= event.daystop)||//within the period in single month mode
							( (event.daystart <= day || event.daystop >= day) && event.daystart >  event.daystop)  //if within the day period in multi-month mode
					   )&&
					   (
							(Shour < hour && Ehour> hour)||   //fully within the period
							(
								(
								(Shour == hour&& Smin <= minute)|| //In the start hour, after or on the start minute OR
								(Ehour == hour&& Emin > minute)   //In the end hour, before the stop minute
								)&& (Shour != Ehour)
							)||
							(
								(
								(Shour == hour&& Smin <= minute)&& //In the start hour, after or on the start minute AND
								(Ehour == hour&& Emin > minute)   //In the end hour, before the stop minute
								)&& (Shour == Ehour)
							)
					    )
				    )
				  ){
					play.data = 0;//indicate a pause to the MP3 module
					MP3_Event_Post(play);
					curschedule = -1;//no active schedule
					reset = 0;// run the schedule start code again
							  //this section should not be reentered unless the second schedule event starts and ends on the same minute as the first ends
				}
			}
		}
	}
}
uint8_t Scheduler_GetMonth(){
	I2C_Recieve(RTCADDRESS, RTCMNTHADDR, 1);
	return month;
}
uint8_t Scheduler_GetDay(){
	I2C_Recieve(RTCADDRESS, RTCDAYADDR, 1);
	return day;
}
uint8_t Scheduler_GetHour(){
	I2C_Recieve(RTCADDRESS, RTCHOURADDR, 1);
	return hour;
}
uint8_t Scheduler_GetMinute(){
	I2C_Recieve(RTCADDRESS, RTCMINADDR, 1);
	return minute;
}

//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: Scheduler_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t Scheduler_Event_Init(FIFO Queue){
    Schedulerqueue = Queue;
    I2C_Init();
    TIMERS_Init();
    I2C_Transmit(RTCADDRESS, RTCSECADDR, 0x80);//enable the clock
	I2C_Transmit(RTCADDRESS, RTCSTATADDR, 0x08);//enables the use of backup battery
    starttime = 0;
    Event_t event = (Event_t){EVENT_NONE, 0};
	event.status = EVENT_TIMEOUT;
	event.data = 0;
	starttime = TIMERS_GetMilliSeconds();//force check time on wake-up
	Scheduler_Event_Post(event);
    curschedule = -1;//no active schedule
    return INIT_OK;
}
/*
 * @Function: Scheduler_Event_Init
 * @Brief: Provides the ability for state machines to interact
 * @param: event to be posted
 * @return: none
 */
void Scheduler_Event_Post(Event_t event){
    FIFO_Enqueue(Schedulerqueue, event);
}


/*
 * @Function: Scheduler_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the
 * @param: none
 * @return: An event
 */
Event_t Scheduler_Event_Updater(void){
    Event_t event = (Event_t){EVENT_NONE, 0};
    uint32_t timer = TIMERS_GetMilliSeconds();
	if((timer-starttime) >= REFRESHRATE){
		event.status = EVENT_TIMEOUT;
		event.data = 0;
		starttime = timer;
		Scheduler_Event_Post(event);
	}
    return event;
}

/*
 * @Function: Scheduler_Event_Handler
 * @Brief:
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t Scheduler_Event_Handler(Event_t event){
	if(event.status == EVENT_PLAY){
		I2C_Recieve(RTCADDRESS, RTCMNTHADDR, 1);
		I2C_Recieve(RTCADDRESS, RTCDAYADDR, 1);
		I2C_Recieve(RTCADDRESS, RTCHOURADDR, 1);
		I2C_Recieve(RTCADDRESS, RTCMINADDR, 1);
		logging = 1;
		playdata = event.data;
	}
	if(event.status == EVENT_TIMEOUT){
		I2C_Recieve(RTCADDRESS, RTCMNTHADDR, 1);
		I2C_Recieve(RTCADDRESS, RTCDAYADDR, 1);
		I2C_Recieve(RTCADDRESS, RTCHOURADDR, 1);
		I2C_Recieve(RTCADDRESS, RTCMINADDR, 1);
		logging = 0;
	}
	if(event.status == EVENT_I2C){
		switch (event.data>>8){
		case RTCMNTHADDR:
			month = ((event.data & 0x10)>>4)*10 + (event.data & 0x0F);
			break;
		case RTCDAYADDR:
			day = ((event.data & 0x30)>>4)*10 + (event.data & 0x0F);
			break;
		case RTCHOURADDR:
			if(event.data & 0x40){//AM/PM
				hour = 12*((event.data &0x20)>>5)+ 10*((event.data &0x10)>>4) + ((event.data &0x0F));
			}else {//24Hr
				hour = 10*((event.data &0x30)>>4) + ((event.data &0x0F));
			}
			break;
		case RTCMINADDR:
			//MINTEN2 MINTEN1 MINTEN0 MINONE3 MINONE2 MINONE1 MINONE0
			minute = 10*((event.data & 0x70)>>4) +((event.data &0x0F));
			CompareTime();

			break;
		}
	}
    return 1;
}


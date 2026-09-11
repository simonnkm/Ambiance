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
#include "MP3.h"
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
static uint16_t playdata;

#define LOGBUCKETHOURS 2//log one broadcast/no-broadcast summary per this many hours

static uint8_t lastLoggedDay = NULLDATE;//day the current log bucket started on, NULLDATE = not yet seen a valid day
static uint8_t lastLoggedMonth;
static uint8_t lastLoggedBucket;//which LOGBUCKETHOURS-wide bucket of the day (0..(24/LOGBUCKETHOURS)-1) is currently open
static uint8_t playedInBucket;//nonzero if at least one play command fired since the current bucket opened
static uint8_t silentInBucket;//nonzero if MP3 reported PROGRAMMED_SILENCE (alive, idle by design) at least once since the current bucket opened
static uint8_t lastFolder;//most recent folder/track played in the current bucket, for reference in the summary
static uint8_t lastTrack;

static int16_t curschedule;//contains the current schedule being followed, -1 if no schedule active

static uint8_t pendingBootResetCause = 0;//RESETCAUSE_* bits from main(), logged once RTC time is valid
static uint8_t bootLogged = 0;//has this boot's marker been written to the log yet
//----------------------------------------Private Functions--------------------------------------
void CompareTime(){
	//Once-per-bucket summary: did the speaker broadcast at all during the bucket that just ended, or was it dead?
	uint8_t currentBucket = hour / LOGBUCKETHOURS;

	if(day == 0 || month == 0){
		//The RTC's own power-on-reset default reads as 0, not the NULLDATE
		//(0xFF) sentinel below - so before the very first real time sync
		//after a reset, day/month briefly read as 0 rather than "not yet
		//valid". That used to get latched as the first valid day, producing
		//bogus "Month 00 Day 00" log entries until the next sync corrected
		//things. Wait for an actual date instead.
		return;
	}

	if(lastLoggedDay == NULLDATE){
		//first valid date seen since boot; nothing to summarize yet
		lastLoggedDay    = day;
		lastLoggedMonth  = month;
		lastLoggedBucket = currentBucket;

		if(!bootLogged){
			//Record that a boot happened, when (now that RTC time is finally
			//valid), and why (RESETCAUSE_* bits latched at the very start of
			//main(), before anything else could touch RCC->CSR). Reuses the
			//scheduleEvent wire format like the bucket summaries below, but
			//tagged with stop=2 (bucket summaries only ever use 0 or 1) so a
			//log reader can tell a boot marker apart from a broadcast
			//summary; folder carries the reset-cause bitmask instead of a
			//folder number, and start carries the boot time-of-day packed
			//the same way schedule start/stop times already are elsewhere
			//in this codebase (hour<<3 | quarter-hour).
			scheduleEvent bootEvent;
			bootEvent.month    = month;
			bootEvent.daystart = day;
			bootEvent.start    = (uint8_t)((hour<<3) | (minute/15));
			bootEvent.daystop  = 0;
			bootEvent.stop     = 2;//boot-marker sentinel
			bootEvent.folder   = pendingBootResetCause;
			bootEvent.track    = 0;
			FLASH_AppendLogs(bootEvent);
			bootLogged = 1;
		}
	} else if(day != lastLoggedDay || currentBucket != lastLoggedBucket){
		scheduleEvent summary;
		summary.month    = lastLoggedMonth;
		summary.daystart = lastLoggedDay;
		summary.start    = lastLoggedBucket;//which bucket this entry covers
		//1 = broadcast at least once; 3 = never broadcast but MP3 confirmed
		//alive and idle by design (duty-cycle pause / outside a scheduled
		//window) at least once - "programmed silence", not a failure; 0 =
		//neither was ever observed the whole bucket, i.e. MP3 never
		//reported anything but MP3_STATUS_DEAD - genuinely unresponsive.
		//(3, not 2: stop==2 is already the boot-marker sentinel above.)
		summary.stop     = playedInBucket ? 1 : (silentInBucket ? 3 : 0);
		summary.daystop  = 0;
		summary.folder   = lastFolder;
		summary.track    = lastTrack;
		FLASH_AppendLogs(summary);

		lastLoggedDay    = day;
		lastLoggedMonth  = month;
		lastLoggedBucket = currentBucket;
		playedInBucket   = 0;
		silentInBucket   = 0;
		lastFolder       = 0;
		lastTrack        = 0;
	}

	if(logging){
		if(playdata){
			playedInBucket = 1;
			lastFolder = (playdata>>8)&0xFF;
			lastTrack  = playdata&0xFF;
		}
		logging  = 0;
	} else {
		scheduleEvent event;
		Event_t play = (Event_t){EVENT_PLAY, 0};

		//Confirm ongoing playback each poll (every REFRESHRATE, ~30s), not just
		//the bucket where the schedule first started. MP3.c only calls
		//Scheduler_Event_Post() (which sets playedInBucket via the "logging"
		//branch above) on the *initial* EVENT_PLAY - its own randomized-
		//continuation plays after each track finishes (the 0x3D "song
		//complete" handler) never notify Scheduler. Without this, any
		//broadcast window longer than one LOGBUCKETHOURS-wide bucket got
		//logged as "No broadcast" for every bucket after the first one, even
		//while the speaker was still actively playing - this queries MP3's
		//own status (already exposed for the GUI's status request) instead
		//of relying solely on that one-time push notification.
		{
			uint8_t mp3status = MP3_GetStatus();
			if(mp3status == MP3_STATUS_BROADCASTING){
				playedInBucket = 1;
				uint16_t curfile = MP3_GetCurrentFile();
				lastFolder = (uint8_t)(curfile>>8);
				lastTrack  = (uint8_t)(curfile&0xFF);
			} else if(mp3status == MP3_STATUS_PROGRAMMED_SILENCE){
				//MP3 is alive and correctly idle (mid duty-cycle pause, or no
				//schedule is active right now) - record that this bucket saw
				//a confirmed-alive device, not a dead/unresponsive one, even
				//if it never actually played a track.
				silentInBucket = 1;
			}
		}

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
void Scheduler_RecordBootResetCause(uint8_t cause){
	pendingBootResetCause = cause;
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


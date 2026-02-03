/*
 * Name ButtonsMenuSM.c
 * Brief: a module that allows the user to change setting through the pcb's push buttons
 * Author: Caitlin Bonesio
 * Created: 4/23/25
 * Modified: 4/23/25
 */


//----------------------------------------Private Includes---------------------------------------
#include "CONFIG.h"
#include "ButtonsMenuSM.h"
#include "MP3.h"
#include "GPIO.h"
#include "I2C.h"
#include "TIMERS.h"
#include "FLASH.h"
#include "UART.h"
#include "discountIO.h"
#include "Oled.h"
#include "Scheduler.h"
#include <string.h>
//----------------------------------------Private Defines----------------------------------------
#define DEBOUNCETIME 50//milliseconds
#define SLEEPTIMER   30000
//----------------------------------------Private Typedefs---------------------------------------
typedef enum ButtonsMenuStates{
	main,
	folderControl,
	scheduleTime,
//	scheduleDay,
//	scheduleStart,
//	scheduleStop,
	scheduleFolder,
	setTime,
	options,
	viewschedule,
}ButtonsMenuStates_t;
//----------------------------------------Private Variables--------------------------------------
FIFO ButtonsMenuSMqueue;

static ButtonsMenuStates_t BMState;
static uint8_t displayoff;

static uint8_t cursorpos;//display cursors
static uint8_t folderselect;
static uint8_t trackselect;


static int8_t minute;
static int8_t hour;
static int8_t day;
static int8_t month;
//----------------------------------------Private Functions--------------------------------------

void DrawMain(uint8_t month, uint8_t day, uint8_t hour, uint8_t minute){
	char maintext[176];
	sprintf(maintext, "Main menu %.2d/%.2d %.2d:%.2d\n"
					  "Back:play track\n"
			   	   	  "Sel:append schedule\n"
	   	   	  	  	  "%c:Set Date/time\n"
					  "%c:more options\n"
					  "Volume: %c%.3d%c\n"
					  "Current song %d-%d",
					  month, day, hour, minute,
					  DOWN_ARROW_OFF, UP_ARROW_OFF, LEFT_ARROW_ON, FLASH_GetVolume(), RIGHT_ARROW_ON, MP3_GetCurrentFile()>>8, MP3_GetCurrentFile()&0xFF);
	OledClear(OLED_COLOR_BLACK);
	OledDrawString(maintext);
	OledUpdate();
}

void DrawFolder(uint8_t cursor, uint8_t folder, uint8_t track){

	char folderselectstring[88];
	sprintf(folderselectstring, "Select Folder\n"
								"        %c        %c\n"
			                    "Folder: %d Track: %d\n"
			                    "        %c        %c\n",
								!cursor?UP_ARROW_ON:UP_ARROW_OFF, cursor?UP_ARROW_ON:UP_ARROW_OFF,
								folder, track,
								!cursor?DOWN_ARROW_ON:DOWN_ARROW_OFF, cursor?DOWN_ARROW_ON:DOWN_ARROW_OFF);
	OledClear(OLED_COLOR_BLACK);
	OledDrawString(folderselectstring);
	OledUpdate();
}

void DrawOptions(uint8_t cursor, uint8_t dutycycle){
	char folderselectstring[88];
	sprintf(folderselectstring, "Options\n"
								" Duty Cycle: %c%.2d%c\n"
								" Song:%c%c\n"
								"%cView Schedule%c\n"
								"%cClear Schedule%c\n"
								"%cClear Logs%c\n",
								0==cursor?LEFT_ARROW_ON:LEFT_ARROW_OFF, dutycycle,0==cursor?RIGHT_ARROW_ON:RIGHT_ARROW_OFF,
								1==cursor?LEFT_ARROW_ON:LEFT_ARROW_OFF,           1==cursor?RIGHT_ARROW_ON:RIGHT_ARROW_OFF,
								2==cursor?RIGHT_ARROW_ON:RIGHT_ARROW_OFF, 		  2==cursor?LEFT_ARROW_ON:LEFT_ARROW_OFF,
								3==cursor?RIGHT_ARROW_ON:RIGHT_ARROW_OFF, 		  3==cursor?LEFT_ARROW_ON:LEFT_ARROW_OFF,
								4==cursor?RIGHT_ARROW_ON:RIGHT_ARROW_OFF, 		  4==cursor?LEFT_ARROW_ON:LEFT_ARROW_OFF);
	OledClear(OLED_COLOR_BLACK);
	OledDrawString(folderselectstring);
	OledUpdate();

}

void DrawSetTime(uint8_t cursor, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute){
	char folderselectstring[88];
#define TOPCHAR cursor?UP_ARROW_ON:UP_ARROW_OFF
#define BOTCHAR cursor?DOWN_ARROW_ON:DOWN_ARROW_OFF
	sprintf(folderselectstring, "Set Date/Time\n"
								" %c  %c  %c  %c\n"
								"%.2d/%.2d %.2d:%.2d\n"
								" %c  %c  %c  %c\n",
								0==TOPCHAR, 1 ==TOPCHAR, 2 ==TOPCHAR, 3==TOPCHAR,
								month, day, hour, minute,
								0==BOTCHAR, 1 ==BOTCHAR, 2 ==BOTCHAR, 3==BOTCHAR);
	OledClear(OLED_COLOR_BLACK);
	OledDrawString(folderselectstring);
	OledUpdate();

}

void DrawSchedule(uint8_t cursor, uint8_t month, uint8_t daystart, uint8_t dayend, uint8_t start, uint8_t end){
	char folderselectstring[151];
	uint8_t hour1 = (start&0b11111000)>>3;
	uint8_t minute1 = ((start & 0b011)*15);
	uint8_t hour2 = (end&0b11111000)>>3;
	uint8_t minute2 = ((end & 0b011)*15);
	uint8_t month2 = month;
	if(dayend < daystart){
		month2 = month+1;
		if(month2 == 13){
			month2 = 1;
		}
	}
	sprintf(folderselectstring, "Schedule time to play\n"
								" %c  %c        %c\n"
								"%.2d/%.2d to %.2d/%.2d ...\n"
								" %c  %c        %c\n"
								"   %c  %c    %c  %c\n"
								"S:%.2d:%.2d E:%.2d:%.2d\n"
								"   %c  %c    %c  %c\n"
								"month=1 - repeat",
								0==TOPCHAR, 1==TOPCHAR, 2==TOPCHAR,
								month, daystart, month2, dayend,
								0==BOTCHAR, 1==BOTCHAR, 2==BOTCHAR,
								3==TOPCHAR, 4==TOPCHAR, 5==TOPCHAR, 6==TOPCHAR,
								hour1, minute1, hour2, minute2,
								3==BOTCHAR, 4==BOTCHAR, 5==BOTCHAR, 6==BOTCHAR);
	OledClear(OLED_COLOR_BLACK);
	OledDrawString(folderselectstring);
	OledUpdate();

}
void DrawViewSchedule(int16_t index){
	char viewschedulestring[151];
	if(FLASH_GetScheduleSize() == 0){
		sprintf(viewschedulestring,
				"the schedule is empty\n"
				"press back to leave");
	}else if(index < FLASH_GetScheduleSize() && index >= 0){
		scheduleEvent cur = FLASH_ReadSchedule(index);
		sprintf(viewschedulestring,
				"%c                   %c\n"
				"schedule #%d\n"
				"month: %d\n"
				"days: %d-%d\n"
				"time %.2d:%.2d-%.2d:%.2d\n"
				"folder: %d, track: %d",
				(index<=0)?LEFT_ARROW_OFF:LEFT_ARROW_ON, (index>=(FLASH_GetScheduleSize()-1))?RIGHT_ARROW_OFF:RIGHT_ARROW_ON,//disable keys when at end of schedule
				index,
				cur.month,
				cur.daystart, cur.daystop,
				(cur.start &0b11111000)>>3, (cur.start&0b00000011)*15,(cur.stop &0b11111000)>>3, (cur.stop&0b00000011)*15,
				cur.folder, cur.track);
	}else{
		sprintf(viewschedulestring,
				"an error occurred\n"
				"press back to recover");
	}
	OledClear(OLED_COLOR_BLACK);
	OledDrawString(viewschedulestring);
	OledUpdate();
}

//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: ButtonsMenuSM_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t ButtonsMenuSM_Event_Init(FIFO Queue){
    ButtonsMenuSMqueue = Queue;
    BMState = main;
    displayoff = 1;
    GPIO_Init();
    I2C_Init();
	TIMERS_Init();
	FLASH_Init();
	OledInit();
	BSP_LED_Init(LED_BLUE);
	BSP_LED_Init(LED_RED);

//	uint8_t monthselect = 9;
//	uint8_t Sdayselect = 6;
//	uint8_t Edayselect = 7;
//	uint8_t Stimeselect = 10;
//	uint8_t Etimeselect = 13;
//	uint8_t trackselect = 2;
//	uint8_t folderselect = 1;
//	scheduleEvent sevent;
//	sevent.month = monthselect;
//	sevent.daystart = Sdayselect+1;
//	sevent.daystop = Edayselect+1;
//	sevent.start = Stimeselect;
//	sevent.stop = Etimeselect;
//	sevent.track = trackselect;
//	sevent.folder = folderselect;
//	FLASH_AppendSchedule(sevent);
    return INIT_OK;
}
/*
 * @Function: ButtonsMenuSM_Event_Init
 * @Brief: Provides the ability for state machines to interact
 * @param: event to be posted
 * @return: none
 */
void ButtonsMenuSM_Event_Post(Event_t event){
    FIFO_Enqueue(ButtonsMenuSMqueue, event);
}



/*
 *sample every type of button Event

if(event.status == EVENT_BUTTONS){
	if(event.data & B1XORMASK && event.data & ~B1MASK){
		nextstate = main;
		transition = 1;
	} else
	if(event.data & B2XORMASK && event.data & ~B2MASK){

	} else
	if(event.data & B3XORMASK && event.data & ~B3MASK){

	} else
	if(event.data & B4XORMASK && event.data & ~B4MASK){
	} else
	if(event.data & B5XORMASK && event.data & ~B5MASK){
	} else
	if(event.data & B6XORMASK && event.data & ~B6MASK){
	}
}
 */

/*
 * @Function: ButtonsMenuSM_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the
 * @param: none
 * @return: An event
 */
Event_t ButtonsMenuSM_Event_Updater(void){

    static uint32_t debounce = 0;
    static uint8_t lastbuttons = 0xFF;
    static uint32_t timer = 0;
    static uint8_t timeractive = 0;

    Event_t event = (Event_t){EVENT_NONE, 0};

    uint8_t currbuttons = GPIO_ReadButtons();

    if(currbuttons != lastbuttons && (debounce + DEBOUNCETIME < TIMERS_GetMilliSeconds())){
    	event.status = EVENT_BUTTONS;
    	event.data = (((uint16_t)currbuttons ^ (uint16_t)lastbuttons) << 8) + (uint16_t)currbuttons;
    	lastbuttons = currbuttons;
    	debounce = TIMERS_GetMilliSeconds();
    	timer = TIMERS_GetMilliSeconds();//delay the sleep timer
    	timeractive = 1;
    	ButtonsMenuSM_Event_Post(event);
    }
    if(timeractive & (timer+SLEEPTIMER < TIMERS_GetMilliSeconds())){
    	event.status = EVENT_TIMEOUT;
    	event.data = 0;
    	timeractive = 0;// disable sleep timer until next button press
    	ButtonsMenuSM_Event_Post(event);

    }
    if(!displayoff && BMState == main)
		if(TIMERS_GetMilliSeconds() % 1000 == 0){//updates the date and time on the main menu
			minute = Scheduler_GetMinute();
			hour = Scheduler_GetHour();
			day = Scheduler_GetDay();
			month = Scheduler_GetMonth();
			event.status = EVENT_ENTRY;
	    	ButtonsMenuSM_Event_Post(event);
		}
    return event;
}

/*
 * @Function: ButtonsMenuSM_Event_Handler
 * @Brief:
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t ButtonsMenuSM_Event_Handler(Event_t event){
	uint8_t transition = 0;
	ButtonsMenuStates_t nextstate = BMState;
	if(event.status == EVENT_TIMEOUT && event.data == 0){
		displayoff = 1;
		OledOff();
	}
	if(displayoff){
		if(event.status == EVENT_BUTTONS){
			displayoff = 0;
			nextstate = main;
			transition = 1;
			OledOn();

			minute = Scheduler_GetMinute();
			hour = Scheduler_GetHour();
			day = Scheduler_GetDay();
			month = Scheduler_GetMonth();
		}
	} else {

		switch (BMState){
		case main:
			if(event.status == EVENT_ENTRY){
				DrawMain(month, day, hour, minute);
			}
			if(event.status == EVENT_BUTTONS){
				if(event.data & B1XORMASK && !(event.data & B1MASK)){
					//discountprintf("moving to folder control");
					nextstate = folderControl;
					transition = 1;
					folderselect = 0;
					trackselect = 0;
				} else
				if(event.data & B2XORMASK && !(event.data & B2MASK)){
					//discountprintf("moving to schedule day");
					nextstate = scheduleTime;
					transition = 1;
					folderselect = 0;
					trackselect = 0;

				}else
				if(event.data & B3XORMASK && !(event.data & B3MASK)){
					//discountprintf("incrementing volume");
					nextstate = setTime;
					transition = 1;

				} else
				if(event.data & B4XORMASK && !(event.data & B4MASK)){
					//discountprintf("moving to options");
					nextstate = options;
					transition = 1;
				} else
				if(event.data & B5XORMASK && !(event.data & B5MASK)){
					//discountprintf("decrementing volume");
					uint8_t vol = FLASH_GetVolume()-5;
					if(vol >= 0){
						FLASH_SetDCVol(vol, FLASH_GetDutyCycle());
					}
					DrawMain(month, day, hour, minute);

				} else
				if(event.data & B6XORMASK && !(event.data & B6MASK)){
					//discountprintf("moving to set Time");
					uint8_t vol = FLASH_GetVolume()+5;
					if(vol <= 100){
						FLASH_SetDCVol(vol, FLASH_GetDutyCycle());
					}
					DrawMain(month, day, hour, minute);

				}

			}
			break;
		case folderControl:
			if(event.status == EVENT_ENTRY){
				cursorpos = 0;
				DrawFolder(cursorpos, folderselect, trackselect);
			}
			if(event.status == EVENT_BUTTONS){
				if(event.data & B1XORMASK && !(event.data & B1MASK)){
					//discountprintf("moving to main, back");
					nextstate = main;
					transition = 1;
				} else
				if(event.data & B2XORMASK && !(event.data & B2MASK)){
					MP3_Event_Post(((Event_t){EVENT_PLAY, ((folderselect)<<8) + (trackselect)}));
					//discountprintf("moving to main, select");
					nextstate = main;
					transition = 1;

				} else
				if(event.data & B3XORMASK && !(event.data & B3MASK)){
					if(!cursorpos){
						folderselect--;
						if(folderselect==255){
							folderselect = 100;
						}
					} else {
						trackselect--;
						trackselect %= 256;
					}
					DrawFolder(cursorpos, folderselect, trackselect);

				} else
				if(event.data & B4XORMASK && !(event.data & B4MASK)){
					if(!cursorpos){
						folderselect++;
						folderselect %= 100;
					} else {
						trackselect++;
						trackselect %= 256;
					}
					DrawFolder(cursorpos, folderselect, trackselect);
					break;
				} else
				if(event.data & B5XORMASK && !(event.data & B5MASK)){
					cursorpos = !cursorpos;
					DrawFolder(cursorpos, folderselect, trackselect);
				} else
				if(event.data & B6XORMASK && !(event.data & B6MASK)){
					cursorpos = !cursorpos;
					DrawFolder(cursorpos, folderselect, trackselect);
				}
			}
			break;
		case scheduleTime:
			static uint8_t monthselect;
			static uint8_t Sdayselect;
			static uint8_t Edayselect;
			static uint8_t Stimeselect;
			static uint8_t Etimeselect;
			if(event.status == EVENT_ENTRY){
				cursorpos = 0;
				monthselect=0;
				Sdayselect=0;
				Edayselect=0;
				Stimeselect=0;
				Etimeselect=0;
				DrawSchedule(cursorpos, monthselect, Sdayselect+1, Edayselect+1, Stimeselect, Etimeselect);
			}
			if(event.status == EVENT_BUTTONS){
				if(event.data & B1XORMASK && !(event.data & B1MASK)){
					nextstate = main;
					transition = 1;
				} else
				if(event.data & B2XORMASK && !(event.data & B2MASK)){
					nextstate = scheduleFolder;
					transition = 1;
				} else
				if(event.data & B3XORMASK && !(event.data & B3MASK)){
					switch(cursorpos){
					case 0:
						monthselect--;
						if (monthselect == 255){
							monthselect = 12;
						}
						break;
					case 1:
					Sdayselect--;
						if (Sdayselect == 255){
							Sdayselect = 30;
						}
						break;
					case 2:
						Edayselect--;
						if (Edayselect == 255){
							Edayselect = 30;
						}
						break;
					case 3:
						if((Stimeselect&0b11111000) == 0){
							Stimeselect = 0b10111000 | (Stimeselect&0b11);//23:XX
						} else {
							Stimeselect -= 0b00001000;
						}
						break;
					case 4:
						if(Stimeselect == 0){
							Stimeselect = 0b10111011;//23:45
						} else {
							Stimeselect--;
							Stimeselect &= 0b11111011;
						}
						break;
					case 5:
						if((Etimeselect&0b11111000) == 0){
							Etimeselect = 0b10111000 | (Etimeselect&0b11);//23:XX
						} else {
							Etimeselect -= 0b00001000;
						}break;
					case 6:
						if(Etimeselect == 0){
							Etimeselect = 0b10111011;//23:45
						} else {
							Etimeselect--;
							Etimeselect &= 0b11111011;
						}
						break;
					default:
						break;
					}
					DrawSchedule(cursorpos, monthselect, Sdayselect+1, Edayselect+1, Stimeselect, Etimeselect);
				} else
				if(event.data & B4XORMASK && !(event.data & B4MASK)){
					switch(cursorpos){
					case 0:
						monthselect++;
						monthselect %= 13;
						break;
					case 1:
						Sdayselect++;
						Sdayselect%=31;
						break;
					case 2:
						Edayselect++;
						Edayselect%=31;
						break;
					case 3:
						if((Stimeselect&0b11111000) == 0b10111000){
							Stimeselect &= 0b00000011;
						} else {
							Stimeselect += 0b00001000;
						}
						break;
					case 4:
						if(Stimeselect == 0b10111011){//23:45
							Stimeselect = 0;//0:00
						} else {
							Stimeselect++;
							if(Stimeselect & 0b00000100){
								Stimeselect += 0b00000100;
							}
						}
						break;
					case 5:
						if((Etimeselect&0b11111000) == 0b10111000){
							Etimeselect &= 0b00000011;
						} else {
							Etimeselect += 0b00001000;
						}
						break;
					case 6:
						if(Etimeselect == 0b10111011){//23:45
							Etimeselect = 0;//0:00
						} else {
							Etimeselect++;
							if(Etimeselect & 0b00000100){
								Etimeselect += 0b00000100;
							}
						}
						break;
					default:
						break;
					}
					DrawSchedule(cursorpos, monthselect, Sdayselect+1, Edayselect+1, Stimeselect, Etimeselect);
				} else
				if(event.data & B5XORMASK && !(event.data & B5MASK)){
					cursorpos--;
					if(cursorpos == 255){
						cursorpos = 6;
					}
					DrawSchedule(cursorpos, monthselect, Sdayselect+1, Edayselect+1, Stimeselect, Etimeselect);
				} else
				if(event.data & B6XORMASK && !(event.data & B6MASK)){
					cursorpos++;
					cursorpos %= 7;
					DrawSchedule(cursorpos, monthselect, Sdayselect+1, Edayselect+1, Stimeselect, Etimeselect);
				}
			}
			break;
		case scheduleFolder:
			if(event.status == EVENT_ENTRY){
				cursorpos = 0;
				DrawFolder(cursorpos, folderselect, trackselect);
			}
			if(event.status == EVENT_BUTTONS){
				if(event.data & B1XORMASK && !(event.data & B1MASK)){
					//discountprintf("moving to schedule stop");
					nextstate = scheduleTime;
					transition = 1;
				} else
				if(event.data & B2XORMASK && !(event.data & B2MASK)){
					//discountprintf("moving to main");
					scheduleEvent sevent;
					sevent.month = monthselect;
					sevent.daystart = Sdayselect+1;
					sevent.daystop = Edayselect+1;
					sevent.start = Stimeselect;
					sevent.stop = Etimeselect;
					sevent.track = trackselect;
					sevent.folder = folderselect;
					FLASH_AppendSchedule(sevent);
					nextstate = main;
					transition = 1;

				} else
				if(event.data & B3XORMASK && !(event.data & B3MASK)){
					if(!cursorpos){
						folderselect--;
						if(folderselect==255){
							folderselect = 100;
						}
					} else {
						trackselect--;
					}
					DrawFolder(cursorpos, folderselect, trackselect);

				} else
				if(event.data & B4XORMASK && !(event.data & B4MASK)){
					if(!cursorpos){
						folderselect++;
						folderselect %= 100;
					} else {
						trackselect++;
						trackselect %= 256;
					}
					DrawFolder(cursorpos, folderselect, trackselect);
				} else
				if(event.data & B5XORMASK && !(event.data & B5MASK)){
					cursorpos = !cursorpos;
					DrawFolder(cursorpos, folderselect, trackselect);
				} else
				if(event.data & B6XORMASK && !(event.data & B6MASK)){
					cursorpos = !cursorpos;
					DrawFolder(cursorpos, folderselect, trackselect);
				}
			}
			break;
		case setTime:
			if(event.status == EVENT_ENTRY){
				cursorpos = 0;
				minute = Scheduler_GetMinute();
				hour = Scheduler_GetHour();
				day = Scheduler_GetDay();
				month = Scheduler_GetMonth();
				DrawSetTime(cursorpos, month, day, hour, minute);
			}
			if(event.status == EVENT_BUTTONS){
				if(event.data & B1XORMASK && !(event.data & B1MASK)){
					//discountprintf("moving to main, back");
					nextstate = main;
					transition = 1;
				} else
				if(event.data & B2XORMASK && !(event.data & B2MASK)){
					//discountprintf("moving to main, select");
					I2C_Transmit(RTCADDRESS, RTCSECADDR, 0b10000000);
					I2C_Transmit(RTCADDRESS, RTCMINADDR, ((((minute/10)<<4))&0b01110000)|minute%10);
					I2C_Transmit(RTCADDRESS, RTCHOURADDR, ((hour/10)<<4)|hour%10);
					I2C_Transmit(RTCADDRESS, RTCDAYADDR, (((day)/10)<<4)|(day)%10);
					I2C_Transmit(RTCADDRESS, RTCMNTHADDR, (((month)/10)<<4)|(month)%10);

					minute = Scheduler_GetMinute();
					hour = Scheduler_GetHour();
					day = Scheduler_GetDay();
					month = Scheduler_GetMonth();
					nextstate = main;
					transition = 1;

				} else
				if(event.data & B3XORMASK && !(event.data & B3MASK)){
					switch(cursorpos){
					case 0:
						month--;
						if(month < 1){
							month = 12;
						}
						break;
					case 1:
						day--;
						if(day < 1){
							day = 31;
						}
						break;
					case 2:
						hour--;
						if(hour < 0){
							hour = 23;
						}
						break;
					case 3:
						minute--;
						if(minute < 0){
							minute = 59;
							hour--;
							if(hour < 0){
								hour = 23;
							}
						}
						break;
					}
					DrawSetTime(cursorpos, month, day, hour, minute);
					break;
				} else
				if(event.data & B4XORMASK && !(event.data & B4MASK)){
					switch(cursorpos){
					case 0:
						month++;
						if(month > 12){
							month = 1;
						}
						break;
					case 1:
						day++;
						if(day > 31){
							day = 1;
						}
						break;
					case 2:
						hour++;
						hour %= 24;
						break;
					case 3:
						minute++;
						minute%=60;
						if(minute == 0){
							hour++;
							hour %= 24;
						}
					}
					DrawSetTime(cursorpos, month, day, hour, minute);
				} else
				if(event.data & B5XORMASK && !(event.data & B5MASK)){
					cursorpos--;
					if(cursorpos == 255){
						cursorpos = 3;
					}
					DrawSetTime(cursorpos, month, day, hour, minute);
				} else
				if(event.data & B6XORMASK && !(event.data & B6MASK)){
					cursorpos++;
					cursorpos %= 4;
					DrawSetTime(cursorpos, month, day , hour, minute);
				}
			}
			break;
		case options:
			if(event.status == EVENT_ENTRY){
				cursorpos = 0;

				DrawOptions(cursorpos, FLASH_GetDutyCycle());
			}
			if(event.status == EVENT_BUTTONS){
				if(event.data & B1XORMASK && !(event.data & B1MASK)){
					//discountprintf("moving to main");
					nextstate = main;
					transition = 1;
				} else
				if(event.data & B2XORMASK && !(event.data & B2MASK)){
					//discountprintf("moving to main");
					switch(cursorpos){
					case 0:
						break;
					case 1:
						break;
					case 2:
						nextstate = viewschedule;
						transition = 1;
						break;
					case 3:
						FLASH_ClearSchedule();
						nextstate = main;
						transition = 1;
						break;
					case 4:
						FLASH_ClearLogs();
						nextstate = main;
						transition = 1;
						break;
					default:
						break;
					}
				} else
				if(event.data & B3XORMASK && !(event.data & B3MASK)){
					cursorpos++;
					cursorpos %= 5;
					DrawOptions(cursorpos, FLASH_GetDutyCycle());
				} else
				if(event.data & B4XORMASK && !(event.data & B4MASK)){
					cursorpos--;
					if(cursorpos == 255){
						cursorpos = 4;
					}
					DrawOptions(cursorpos, FLASH_GetDutyCycle());
					break;
				} else
				if(event.data & B5XORMASK && !(event.data & B5MASK)){
					switch(cursorpos){
					case 0:
						int8_t dc = FLASH_GetDutyCycle()-5;
						if(dc < 0){
							dc = 0;
							FLASH_SetDCVol(FLASH_GetVolume(), dc);
							DrawOptions(cursorpos, dc);
						} else {
							FLASH_SetDCVol(FLASH_GetVolume(), dc);
							DrawOptions(cursorpos, dc);
						}
						break;
					case 1:
						MP3_Event_Post(((Event_t){EVENT_PLAY, MP3_GetCurrentFile()-1}));
						break;
					default:
						break;
					}
				} else
				if(event.data & B6XORMASK && !(event.data & B6MASK)){
					switch(cursorpos){
					case 0:
						uint8_t dc = FLASH_GetDutyCycle()+5;
						if(dc > 100){
							dc = 100;
							FLASH_SetDCVol(FLASH_GetVolume(), dc);
							DrawOptions(cursorpos, dc);
						} else {
							FLASH_SetDCVol(FLASH_GetVolume(), dc);
							DrawOptions(cursorpos, dc);
						}
						break;
					case 1:
						MP3_Event_Post(((Event_t){EVENT_PLAY, MP3_GetCurrentFile()+1}));
						break;
					default:
						break;
					}
				}
			}

			break;
		case viewschedule:
			static int16_t index = 0;
			if(event.status == EVENT_ENTRY){
				index = 0;
				DrawViewSchedule(index);
			}
			if(event.status == EVENT_BUTTONS){
				if(event.data & B1XORMASK && !(event.data & B1MASK)){//back button
					nextstate = options;
					transition = 1;
					index = 0;
				} else
				if(event.data & B2XORMASK && !(event.data & B2MASK)){//select
					nextstate = options;
					transition = 1;
					index = 0;

				}else
				if(event.data & B3XORMASK && !(event.data & B3MASK)){//down arrow
				} else
				if(event.data & B4XORMASK && !(event.data & B4MASK)){//up arrow
				} else
				if(event.data & B5XORMASK && !(event.data & B5MASK)){//left arrow
					if(index > 0){
						index--;
						DrawViewSchedule(index);
					}

				} else
				if(event.data & B6XORMASK && !(event.data & B6MASK)){//right arrow
					if(index < FLASH_GetScheduleSize()-1){
						index++;
						DrawViewSchedule(index);
					}

				}

			}
			break;
		default:
			break;
		}
	}
	if(transition){
		ButtonsMenuSM_Event_Handler((Event_t){EVENT_EXIT});
		BMState = nextstate;
		ButtonsMenuSM_Event_Handler((Event_t){EVENT_ENTRY});
	}
    return 1;
}

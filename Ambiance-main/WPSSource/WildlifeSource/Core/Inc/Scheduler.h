/*
 * Name Scheduler.h
 * Brief: a Scheduler to use when creating event/service modules
 * Author: Caitlin Bonesio
 * Created: 4/21/25
 * Modified: 4/21/25
 */

 #ifdef __cplusplus//allows code to be ran in a c++ program
 extern "C" {
 #endif

 #ifndef SCHEDULER_H//SCHEDULER_H Header Guards
 #define SCHEDULER_H//SCHEDULER_H Header Guards
//----------------------------------------Public Includes----------------------------------------
#include "CONFIG.h"
#include "FIFO.h"

//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: Scheduler_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t Scheduler_Event_Init(FIFO Queue);

/*
 * @Function: Scheduler_Event_Init
 * @Brief: Provides the ability for state machines to interact
 * @param: event to be posted
 * @return: none
 */
void Scheduler_Event_Post(Event_t event);

/*
 * @Function: Scheduler_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the
 * @param: none
 * @return: An event
 */
Event_t Scheduler_Event_Updater(void);

/*
 * @Function: Scheduler_Event_Handler
 * @Brief:
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t Scheduler_Event_Handler(Event_t event);

uint8_t Scheduler_GetMonth();
uint8_t Scheduler_GetDay();
uint8_t Scheduler_GetHour();
uint8_t Scheduler_GetMinute();

 #ifdef __cplusplus//allows code to be ran in a c++ program
  }
 #endif
 #endif//SCHEDULER_H Header Guards


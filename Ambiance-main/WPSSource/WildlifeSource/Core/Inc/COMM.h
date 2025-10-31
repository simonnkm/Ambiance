/*
 * Name Event_Comm.h
 * Brief: a Event_Comm to use when creating event/service modules
 * Author: Caitlin Bonesio
 * Created: 4/15/25
 * Modified: 4/15/25
 */

 #ifdef __cplusplus//allows code to be ran in a c++ program
 extern "C" {
 #endif
 
 #ifndef EVENT_COMM_H//EVENT_COMM_H Header Guards
 #define EVENT_COMM_H//EVENT_COMM_H Header Guards
//----------------------------------------Public Includes----------------------------------------
#include "CONFIG.h"
#include "FIFO.h"

//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: EVENT_COMM_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t COMM_Event_Init(FIFO Queue);


/*
 * @Function: EVENT_COMM_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
void COMM_Event_Post(Event_t event);

/*
 * @Function: EVENT_COMM_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the 
 * @param: none
 * @return: An event 
 */
Event_t COMM_Event_Updater(void);

/*
 * @Function: EVENT_COMM_Event_Handler
 * @Brief: 
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t COMM_Event_Handler(Event_t event);

 #ifdef __cplusplus//allows code to be ran in a c++ program
  }
 #endif
 #endif//EVENT_COMM_H Header Guards

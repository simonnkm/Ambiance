/*
 * Name ButtonsMenuSM.h
 * Brief: a module that allows the user to change setting through the pcb's push buttons
 * Author: Caitlin Bonesio
 * Created: 4/23/25
 * Modified: 4/23/25
 */

 #ifdef __cplusplus//allows code to be ran in a c++ program
 extern "C" {
 #endif

 #ifndef BUTTONSMENUSM_H//BUTTONSMENUSM_H Header Guards
 #define BUTTONSMENUSM_H//BUTTONSMENUSM_H Header Guards
//----------------------------------------Public Includes----------------------------------------
#include "CONFIG.h"
#include "FIFO.h"

//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: ButtonsMenuSM_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t ButtonsMenuSM_Event_Init(FIFO Queue);

/*
 * @Function: ButtonsMenuSM_Event_Init
 * @Brief: Provides the ability for state machines to interact
 * @param: event to be posted
 * @return: none
 */
void ButtonsMenuSM_Event_Post(Event_t event);

/*
 * @Function: ButtonsMenuSM_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the
 * @param: none
 * @return: An event
 */
Event_t ButtonsMenuSM_Event_Updater(void);

/*
 * @Function: ButtonsMenuSM_Event_Handler
 * @Brief:
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t ButtonsMenuSM_Event_Handler(Event_t event);

 #ifdef __cplusplus//allows code to be ran in a c++ program
  }
 #endif
 #endif//BUTTONSMENUSM_H Header Guards

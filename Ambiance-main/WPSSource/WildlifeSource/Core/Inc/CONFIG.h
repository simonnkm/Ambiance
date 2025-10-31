/*
 * Name CONFIG.h
 * Brief: provides a way to easily configure the main loop and add more modules
 * Author: Caitlin Bonesio
 * Created: 2/7/25
 * Modified: 3/10/25
 */

#ifndef CONFIG_H
#define CONFIG_H


#ifdef __cplusplus//allows code to be ran in a c++ program
extern "C" {
#endif

//----------------------------------------Public Includes----------------------------------------
#include "BOARD.h"
#include "Events.h"
#include "FIFO.h"
//----------------------------------------Public Typedefs----------------------------------------


//----------------------------------------Public Defines-----------------------------------------


//Include event module header files here
#include "GPIO.h"
#include "COMM.h"
#include "MP3.h"
#include "Scheduler.h"
#include "ButtonsMenuSM.h"

//Add the initialization, Updater, and handler functions to these lists
#define EVENTLISTSIZE 4

#define EVENT_INITLIST {COMM_Event_Init, MP3_Event_Init, Scheduler_Event_Init, ButtonsMenuSM_Event_Init}
#define EVENT_UPDATELIST {COMM_Event_Updater, MP3_Event_Updater, Scheduler_Event_Updater, ButtonsMenuSM_Event_Updater}
#define EVENT_HANDLERLIST {COMM_Event_Handler,  MP3_Event_Handler, Scheduler_Event_Handler, ButtonsMenuSM_Event_Handler}


#ifdef __cplusplus//allows code to be ran in a c++ program
}
#endif


#endif /* CONFIG_H */

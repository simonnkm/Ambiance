/*
 * Name MP3.h
 * Brief: a module to send and receive commands from the external mp3 player
 * Author: Caitlin Bonesio
 * Created: 4/19/25
 * Modified: 4/19/25
 */

 #ifdef __cplusplus//allows code to be ran in a c++ program
 extern "C" {
 #endif

 #ifndef MP3_H//MP3_H Header Guards
 #define MP3_H//MP3_H Header Guards
//----------------------------------------Public Includes----------------------------------------
#include "CONFIG.h"
#include "FIFO.h"
//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: MP3_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t MP3_Event_Init(FIFO Queue);


/*
 * @Function: MP3_Event_Init
 * @Brief: Provides the ability for state machines to interact
 * @param: event to be posted
 * @return: none
 */
void MP3_Event_Post(Event_t event);

/*
 * @Function: MP3_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the
 * @param: none
 * @return: An event
 */
Event_t MP3_Event_Updater(void);

/*
 * @Function: MP3_Event_Handler
 * @Brief:
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t MP3_Event_Handler(Event_t event);

/*
 * @Function: MP3_GetCurrentFile
 * @Brief: gets the current track being played on the mp3 player
 * @param:
 * @return: two bytes, the upper 8bits contain the folder, the lower 8 bits hold the tracck number in the folder
 */
uint16_t MP3_GetCurrentFile();

 #ifdef __cplusplus//allows code to be ran in a c++ program
  }
 #endif
 #endif//MP3_H Header Guards

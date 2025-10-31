/*
 * Name GPIO.h
 * Brief: provides a library to initialize and read from the external buttons.
 * 		  Also provides the functions to implement the buttons into the events and services main
 * Author: Caitlin Bonesio
 * Created: 2/5/25
 * Modified: 3/7/25
 */

#ifdef __cplusplus//allows code to be ran in a c++ program
extern "C" {
#endif

#ifndef GPIO_H//GPIO_H Header Guards
#define GPIO_H//GPIO_H Header Guards


//----------------------------------------Public Includes----------------------------------------
#include "CONFIG.h"
#include "FIFO.h"

//----------------------------------------Public Defines-----------------------------------------
#define B1MASK 		0x01
#define B2MASK 		0x02
#define B3MASK 		0x04
#define B4MASK 		0x08
#define B5MASK 		0x10
#define B6MASK 		0x20
#define B1XORMASK 	0x0100
#define B2XORMASK 	0x0200
#define B3XORMASK 	0x0400
#define B4XORMASK 	0x0800
#define B5XORMASK 	0x1000
#define B6XORMASK 	0x2000
//----------------------------------------Public Functions---------------------------------------
/*
 * @function: GPIO_Init()
 * @brief: initialized GPIO pins to receive 6 external buttons
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
int GPIO_Init(void);

/*
 * @Function: GPIO_ReadButtons
 * @Brief: Allows an easy way to read the external button states
 * @param: none
 * @return: An 8 bit integer where the LSB is button 0, and the 6th bit is button 5
 */
uint8_t GPIO_ReadButtons(void);

///*
// * @Function: GPIO_Event_Init
// * @Brief: Provides the initialization function for the events and serviced routine
// * @param: none
// * @return: An 8 bit integer flag reflecting The initialization status
// */
//uint8_t GPIO_Event_Init(FIFO Queue);
//
///*
// * @Function: GPIO_Event_Updater
// * @Brief: Provides the event checker that checks and posts the changes in the buttons and the current buttons states
// * @param: none
// * @return: An event where the 16bit data is formatted at the 8 most significant bits representing the current states
// * 		 	and the least significant 8 bits representing the changes in the buttons states
// */
//Event_t GPIO_Event_Updater(void);
//
///*
// * @Function: GPIO_Event_Handler
// * @Brief: Currently changes the LEDs on the devboard based on the updates
// * @param: Event_t event, incoming event for the handler to handle
// * @return: An 8 byte integer success flag, returns 0 if the program should crash
// */
//uint8_t GPIO_Event_Handler(Event_t event);

#ifdef __cplusplus
}
#endif

#endif//GPIO_H Header Guards

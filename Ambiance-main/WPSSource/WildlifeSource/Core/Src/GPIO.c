/*
 * Name GPIO.c
 * Brief: provides a library to initialize and read from the external buttons.
 * 		  Also provides the functions to implement the buttons into the events and services main
 * Author: Caitlin Bonesio
 * Created: 2/5/25
 * Modified: 3/7/25
 */


//----------------------------------------Private Includes---------------------------------------
#include "GPIO.h"
#include "FIFO.h"
//----------------------------------------Private Defines----------------------------------------
//#define USEDEBUG

#define BUTTON_1_PIN GPIO_PIN_8
#define BUTTON_2_PIN GPIO_PIN_2
#define BUTTON_3_PIN GPIO_PIN_3
#define BUTTON_4_PIN GPIO_PIN_4
#define BUTTON_5_PIN GPIO_PIN_9
#define BUTTON_6_PIN GPIO_PIN_11

#define BUTTON_1 GPIOA, BUTTON_1_PIN
#define BUTTON_2 GPIOA, BUTTON_2_PIN
#define BUTTON_3 GPIOA, BUTTON_3_PIN
#define BUTTON_4 GPIOB, BUTTON_4_PIN
#define BUTTON_5 GPIOA, BUTTON_5_PIN
#define BUTTON_6 GPIOA, BUTTON_6_PIN

//----------------------------------------Private Variable---------------------------------------
//static uint8_t buttons = 0;
static uint8_t initialized = 0;
//static FIFO GPIOqueue;//the queue of all events generated for the GPIO

//----------------------------------------Public Functions---------------------------------------

/*
 * @function: GPIO_Init()
 * @brief: initialized GPIO pins to receive 6 external buttons
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
int GPIO_Init(){
	if(initialized){return INIT_OK;}
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	//GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();


	//Configure GPIO pins on port A
	GPIO_InitStruct.Pin = BUTTON_1_PIN | BUTTON_5_PIN | BUTTON_6_PIN ;
#ifndef USEDEBUG
//	GPIO_InitStruct.Pin |= BUTTON_2_PIN | BUTTON_3_PIN;//on a separate line to easily disable to allow the debugger to run
#endif
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	//Configure GPIO pins : PB15 PB13
	GPIO_InitStruct.Pin = BUTTON_4_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	initialized = 1;
	return INIT_OK;
}


/*
 * @Function: GPIO_ReadButtons
 * @Brief: Allows an easy way to read the external button states
 * @param: none
 * @return: An 8 bit integer where the LSB is button 0, and the 6th bit is button 5
 */
uint8_t GPIO_ReadButtons(void){
	return ((HAL_GPIO_ReadPin(BUTTON_6) << 5) |
			(HAL_GPIO_ReadPin(BUTTON_5) << 4) |
			(HAL_GPIO_ReadPin(BUTTON_4) << 3) |
#ifndef USEDEBUG
			(HAL_GPIO_ReadPin(BUTTON_3) << 2) |
			(HAL_GPIO_ReadPin(BUTTON_2) << 1) |
#else
			(1<<2)|
			(1<<1)|
#endif
			(HAL_GPIO_ReadPin(BUTTON_1) << 0));
}

//For use before receiving buttons
/*
 * @Function: GPIO_ReadButtons
 * @Brief: Allows an easy way to read the internal button states
 * @param: none
 * @return: An 8 bit integer where the LSB is button 0, and the 3rd bit is button 3
 */
//uint8_t GPIO_ReadButtons(void){
//	if(!initialized){return 0;}
//	return ((!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) << 2) |
//			(!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) << 1) |
//			(!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14) << 0));
//}

///*
// * @Function: GPIO_Event_Init
// * @Brief: Provides the initialization function for the events and serviced routine
// * @param: none
// * @return: An 8 bit integer flag reflecting The initialization status
// */
//uint8_t GPIO_Event_Init(FIFO Queue){
//	buttons = 0;
//	//Initialize dev board leds, for debug purposes
//	BSP_LED_Init(LED_BLUE);
//	BSP_LED_Init(LED_GREEN);
//	BSP_LED_Init(LED_RED);
//
//	/* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
//	BSP_PB_Init(B1, BUTTON_MODE_GPIO);
//	BSP_PB_Init(B2, BUTTON_MODE_GPIO);
//	BSP_PB_Init(B3, BUTTON_MODE_GPIO);
//	GPIOqueue = Queue;
//	//GPIO_Init()
//	return HAL_OK;
//
//}
//
///*
// * @Function: GPIO_Event_Updater
// * @Brief: Provides the event checker that checks and posts the changes in the buttons and the current buttons states
// * @param: none
// * @return: An event where the 16bit data is formatted at the 8 most significant bits representing the current states
// * 		 	and the least significant 8 bits representing the changes in the buttons states
// */
//Event_t GPIO_Event_Updater(void){
//	uint8_t event = 0;
//	Event_t update = {0, 0};//Initialize to no event
//	uint8_t newButtons = GPIO_ReadButtons();
//	if(newButtons != buttons){
//		update.data = (uint16_t)(buttons ^ newButtons) | (((uint16_t)newButtons) << 8);
//		update.status = EVENT_BUTTONS;
//		buttons = newButtons;
//		event = 1;
//	}
//	if(event){FIFO_Enqueue(GPIOqueue, update);}
//	return update;
//}
//
///*
// * @Function: GPIO_Event_Handler
// * @Brief: Currently changes the LEDs on the devboard based on the updates
// * @param: Event_t event, incoming event for the handler to handle
// * @return: An 8 byte integer success flag, returns 0 if the program should crash
// */
//uint8_t GPIO_Event_Handler(Event_t event){
//	if(event.status == EVENT_BUTTONS){
//		if((event.data>>0)&0x01){
//			BSP_LED_Toggle(LED_BLUE);
//		}
//		if((event.data>>1)&0x01){
//			BSP_LED_Toggle(LED_GREEN);
//		}
//		if((event.data>>2)&0x01){
//			BSP_LED_Toggle(LED_RED);
//		}
//	}
//	return 1;
//}

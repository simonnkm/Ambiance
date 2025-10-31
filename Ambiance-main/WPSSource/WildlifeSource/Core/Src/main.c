/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


// USER CODE END Header
// Includes ------------------------------------------------------------------
//#include "BOARD.h"
//
#include "BOARD.h"
#include "CONFIG.h"
#include "UART.h"
#include "GPIO.h"
#include "FIFO.h"
#include "BLUETOOTH.h"

// Private includes ----------------------------------------------------------

// Private typedef -----------------------------------------------------------

// Private define ------------------------------------------------------------
//#define TESTHARNESSACTIVE// Define this when you are using a test harness for a module, disables main
// Private macro -------------------------------------------------------------

// Private variables ---------------------------------------------------------
//Provides a list of all initialization, updater, and handler functions for each modules, functions are defined in CONFIG.h
uint8_t (*InitList[EVENTLISTSIZE])(FIFO Queue) = EVENT_INITLIST;
Event_t (*UpdateList[EVENTLISTSIZE])(void) = EVENT_UPDATELIST;
uint8_t (*HandlerList[EVENTLISTSIZE])(Event_t update) = EVENT_HANDLERLIST;

FIFO EventQueues[EVENTLISTSIZE];
//I2C_HandleTypeDef hi2c1;

PKA_HandleTypeDef hpka;

RNG_HandleTypeDef hrng;

//#define TESTHARNESSACTIVE
#ifndef TESTHARNESSACTIVE
// Private function prototypes -----------------------------------------------

static void MX_RADIO_Init(void);
static void MX_RADIO_TIMER_Init(void);
static void MX_RNG_Init(void);
static void MX_PKA_Init(void);
/**
 *
 * @brief  The application entry point. Implements a simple events and services routine. the event checkers are called updaters
 * `	   the services are called handlers
 * @author Caitlin Bonesio
 *
 */
#include "FLASH.h"
int main(void)
{

	//MCU Configuration--------------------------------------------------------
	if( BOARD_Init() != INIT_OK){
		BOARD_CrashHandler();
	}
    BSP_LED_Init(LED_BLUE);
    BSP_LED_Init(LED_RED);

	MX_RADIO_Init();
	MX_RADIO_TIMER_Init();
	MX_RNG_Init();
	MX_PKA_Init();
	//Initialize BLE middleware
	MX_APPE_Init(NULL);


	BLUETOOTH_BufferInit();
	// Initialize all configured peripherals
	UARTs_Init();
	//Initialize all modules
	for(int i = 0; i < EVENTLISTSIZE; i++){
		EventQueues[i] = FIFO_Create();
		if ((*InitList[i])(EventQueues[i]) == EVENT_ERROR){
			return 0;//We've crashed
		}
	}
	while(1){
		//run all module event checkers
		for(int i = 0; i < EVENTLISTSIZE; i++){
			//collect updates from the updaters
			if (((*UpdateList[i])()).status == EVENT_ERROR){
				BOARD_CrashHandler();//We've crashed
			}
		}
		uint8_t done = 0;
		while(!done){
			//run all module event handlers
			done = 1;//assume we're done
			for(int i = 0; i < EVENTLISTSIZE; i++){
				Event_t event = FIFO_Dequeue(EventQueues[i]);
				if(FIFO_GetSize(EventQueues[i]) != 0){
					done = 0;//if any queue is not empty, we are not done
				}
				if(event.status != EVENT_NONE){//If there is an update
					if ((*HandlerList[i])(event) == EVENT_ERROR){//pass the updates to the handlers
						BOARD_CrashHandler();//We've crashed
					}
				}
			}
		}
		//Run Bluetooth Middleware
	    MX_APPE_Process();

	}
}
#endif




static void MX_PKA_Init(void)
{

  /* USER CODE BEGIN PKA_Init 0 */

  /* USER CODE END PKA_Init 0 */

  /* USER CODE BEGIN PKA_Init 1 */

  /* USER CODE END PKA_Init 1 */
  hpka.Instance = PKA;
  if (HAL_PKA_Init(&hpka) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN PKA_Init 2 */

  /* USER CODE END PKA_Init 2 */

}

/**
  * @brief RADIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_RADIO_Init(void)
{

  /* USER CODE BEGIN RADIO_Init 0 */
//
  /* USER CODE END RADIO_Init 0 */

  RADIO_HandleTypeDef hradio = {0};

  /* USER CODE BEGIN RADIO_Init 1 */
//
  /* USER CODE END RADIO_Init 1 */

  if (__HAL_RCC_RADIO_IS_CLK_DISABLED())
  {
    /* Radio Peripheral reset */
    __HAL_RCC_RADIO_FORCE_RESET();
    __HAL_RCC_RADIO_RELEASE_RESET();

    /* Enable Radio peripheral clock */
    __HAL_RCC_RADIO_CLK_ENABLE();
  }
  hradio.Instance = RADIO;
  HAL_RADIO_Init(&hradio);
  /* USER CODE BEGIN RADIO_Init 2 */
//
  /* USER CODE END RADIO_Init 2 */

}

/**
  * @brief RADIO_TIMER Initialization Function
  * @param None
  * @retval None
  */
static void MX_RADIO_TIMER_Init(void)
{

  /* USER CODE BEGIN RADIO_TIMER_Init 0 */
//
  /* USER CODE END RADIO_TIMER_Init 0 */

  RADIO_TIMER_InitTypeDef RADIO_TIMER_InitStruct = {0};

  /* USER CODE BEGIN RADIO_TIMER_Init 1 */
//
  /* USER CODE END RADIO_TIMER_Init 1 */

  if (__HAL_RCC_RADIO_IS_CLK_DISABLED())
  {
    /* Radio Peripheral reset */
    __HAL_RCC_RADIO_FORCE_RESET();
    __HAL_RCC_RADIO_RELEASE_RESET();

    /* Enable Radio peripheral clock */
    __HAL_RCC_RADIO_CLK_ENABLE();
  }
  /* Wait to be sure that the Radio Timer is active */
  while(LL_RADIO_TIMER_GetAbsoluteTime(WAKEUP) < 0x10);
  RADIO_TIMER_InitStruct.XTAL_StartupTime = 320;
  RADIO_TIMER_InitStruct.enableInitialCalibration = TRUE;
  RADIO_TIMER_InitStruct.periodicCalibrationInterval = 10000;
  HAL_RADIO_TIMER_Init(&RADIO_TIMER_InitStruct);
  /* USER CODE BEGIN RADIO_TIMER_Init 2 */
//
  /* USER CODE END RADIO_TIMER_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}


/* USER CODE BEGIN 4 */

/* USER CODE END 4 */



#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

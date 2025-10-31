/*
 * Name TIMERS.c
 * Brief: provides a library to initialize and access a free running timer
 * Author: Caitlin Bonesio
 * Created: 3/4/25
 * Modified: 3/7/25
 */

//----------------------------------------Private Includes---------------------------------------
#include "BOARD.h"
#include "stm32wb0x_hal_tim.h"

//----------------------------------------Private Variables--------------------------------------

TIM_HandleTypeDef htim2;

static uint32_t milliSeconds = 0;
static uint8_t initialized = 0;

//----------------------------------------Public Functions---------------------------------------

/*
 * @function: TIMERS_Init()
 * @brief: initialize
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
uint32_t TIMERS_Init(){
	if(initialized){return INIT_OK;}
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 32;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 999;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
    HAL_TIM_Base_Start_IT(&htim2); // start interrupt
    initialized = 1;
	return INIT_OK;
}

/*
 * @function: TIMERS_GetMilliSeconds()
 * @brief: accesses the free running timer's milliseconds
 * @param: none
 * @return: uint32_t FRT's milliseconds
 */
uint32_t TIMERS_GetMilliSeconds(){
	if(!initialized){return 0;}
	return milliSeconds;
}

/*
 * @function: TIMERS_GetMicroSeconds()
 * @brief: accesses the free running timer's microseconds
 * @param: none
 * @return: uint32_t FRT's microseconds
 */
uint64_t TIMERS_GetMircoSeconds(){
	if(!initialized){return 0;}
	uint16_t micro = __HAL_TIM_GET_COUNTER(&htim2);
	return ((milliSeconds*1000)+(micro%1000));
}

//----------------------------------------Private Interrupt--------------------------------------

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{

	milliSeconds++;
	HAL_TIM_IRQHandler(&htim2);
}

//----------------------------------------Private Test Harness-----------------------------------
//#define TIMERSTESTHARNESS

#ifdef TIMERSTESTHARNESS
int main(){
	HAL_Init();
	//Initialize dev board leds, for debug purposes
	BSP_LED_Init(LED_BLUE);
	BSP_LED_Init(LED_GREEN);
	BSP_LED_Init(LED_RED);

	/* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
	BSP_PB_Init(B1, BUTTON_MODE_GPIO);
	BSP_PB_Init(B2, BUTTON_MODE_GPIO);
	BSP_PB_Init(B3, BUTTON_MODE_GPIO);


	GPIO_InitTypeDef GPIO_Init = {0};
	GPIO_Init.Pin   = GPIO_PIN_1;
	GPIO_Init.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_Init.Pull  = GPIO_PULLUP;
	GPIO_Init.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_Init);//initialize CN3 Pin 37 for output (Testing where the I2C ports lead)
	BOARD_Init();
	TIMERS_Init();
	while(1){
		if(TIMERS_GetMilliSeconds()%500 == 1){
			HAL_Delay(1);
			BSP_LED_Toggle(LED_RED);
			HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
		}
	}
}


#endif


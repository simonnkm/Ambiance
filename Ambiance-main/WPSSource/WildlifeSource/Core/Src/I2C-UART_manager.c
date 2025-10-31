/*
 * Name I2C-UART_manager.h
 * Brief: A library tha provide arbitration between the two peripherals that share port A pin 1
 * Author: Caitlin Bonesio
 * Created: 5/23/25
 * Modified: 5/23/25
 */

//----------------------------------------Private Includes----------------------------------------
#include "I2C-UART_Manager.h"
#include "TIMERS.h"
#include "I2C.h"
#include "UART.h"


//----------------------------------------Private variables-----------------------------------------
static GPIO_InitTypeDef PORTAPIN1 = {GPIO_PIN_1, GPIO_MODE_AF_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0x01};
//----------------------------------------Public Functions---------------------------------------
/*
 * @function: I2CUARTtoI2C()
 * @brief: if control is with USART, swaps control from the USART to I2C
 * @brief: If the module does swap the pin, a delay is added to insure the pin is initialized before leaving
 * @param: delay - a boolean value that indicates if the function should delay because the pin is used immediately after
 * @return: Init Status, whether the operation failed or succeeded
 */
void I2CUARTtoI2C(uint8_t delay){
	if(PORTAPIN1.Alternate != GPIO_AF0_I2C1){
		 hi2c1.Lock = HAL_UNLOCKED;
		 hi2c1.State = HAL_I2C_STATE_READY;
		husart1.Instance->CR1 &= ~0x00000008;//disable the USART Transmit
//		BSP_LED_Off(LED_BLUE);
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);

		PORTAPIN1.Mode = GPIO_MODE_AF_OD;
		PORTAPIN1.Alternate = GPIO_AF0_I2C1;
		HAL_GPIO_Init(GPIOA, &PORTAPIN1);

		HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_0);

		HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_0);
		if(delay){
			uint32_t start = TIMERS_GetMilliSeconds();
			while(start+20 > TIMERS_GetMilliSeconds());
		}

		I2C_Flushbuffer();
	}
}

/*
 * @function: I2CUARTtoUSART()
 * @brief: if control is with I2C, swaps control from the I2C to the USART
 * @brief: If the module does swap the pin, a delay is added to insure the pin is initialized before leaving
 * @param: delay - a boolean value that indicates if the function should delay because the pin is used immediately after
 * @return: Init Status, whether the operation failed or succeeded
 */
void I2CUARTtoUSART(uint8_t delay){

	if(PORTAPIN1.Alternate != GPIO_AF2_USART1){
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);
		 hi2c1.Lock = HAL_LOCKED;
		 hi2c1.State = HAL_I2C_STATE_RESET;
		 husart1.Instance->CR1 |= 0x00000008;//enable the USART Transmit
		 husart1.Instance->CR1 |= 0x00000040;//enable the USART Transmit complete interrupt

//		BSP_LED_On(LED_BLUE);
		PORTAPIN1.Mode = GPIO_MODE_AF_PP;
		PORTAPIN1.Alternate = GPIO_AF2_USART1;
		HAL_GPIO_Init(GPIOA, &PORTAPIN1);
		if(delay){
			uint32_t start = TIMERS_GetMilliSeconds();
			while(start+12 > TIMERS_GetMilliSeconds());
		}
	}

}


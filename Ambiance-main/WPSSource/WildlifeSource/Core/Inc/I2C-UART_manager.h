/*
 * Name I2C-UART_manager.h
 * Brief: A library tha provide arbitration between the two peripherals that share port A pin 1
 * Author: Caitlin Bonesio
 * Created: 5/23/25
 * Modified: 5/23/25
 */



#ifndef I2CUART_MANAGER_H
#define I2CUART_MANAGER_H


#ifdef __cplusplus//allows code to be ran in a c++ program
extern "C" {
#endif


//----------------------------------------Public Includes----------------------------------------
#include "CONFIG.h"


//----------------------------------------Public Defines-----------------------------------------

//----------------------------------------Public Functions---------------------------------------
/*
 * @function: I2CUARTtoI2C()
 * @brief: if control is with USART, swaps control from the USART to I2C
 * @brief: If the module does swap the pin, a delay is added to insure the pin is initialized before leaving
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
void I2CUARTtoI2C(uint8_t delay);

/*
 * @function: I2CUARTtoUSART()
 * @brief: if control is with I2C, swaps control from the I2C to the USART
 * @brief: If the module does swap the pin, a delay is added to insure the pin is initialized before leaving
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
void I2CUARTtoUSART(uint8_t delay);

#ifdef __cplusplus//allows code to be ran in a c++ program
}
#endif

#endif /* I2CUART_MANAGER_H_ */

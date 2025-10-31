/*
 * Name BLUETOOTH.h
 * Brief: Provides a TX buffer for the virtual USART held over the bluetooth connection
 * Author: Caitlin Bonesio
 * Created: 4/12/25
 * Modified: 5/16/25
 */

 #ifdef __cplusplus//allows code to be ran in a c++ program
 extern "C" {
 #endif

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "BOARD.h"

 /*
  * @function: BLUETOOTH_BufferInit()
  * @brief: initialize the the bluetooth tx buffer for virtual USART communication
  * @param: none
  * @return: none
  */
void BLUETOOTH_BufferInit();

/*
 * @function: BLUETOOTH_WriteBuffer(uint8_t input)
 * @brief: places one character into the buffer for the external PC to read
 * @param: input: character to be placed in buffer
 * @return: none
 */
uint8_t BLUETOOTH_WriteBuffer(uint8_t input);

/*
 * @function: BLUETOOTH_ReadBuffer(uint8_t input)
 * @brief: returns a character from the buffer and removes it from the buffer
 * @param: none
 * @return: -1 if buffer empty, uint8_t character data if the buffer has data to return
 */
int16_t BLUETOOTH_ReadBuffer();

/*
 * @function: BLUETOOTH_BufferEmpty()
 * @brief: returns if the buffer is empty or if the Bluetooth is disconnected
 * @param: none
 * @return: 0 if disconnected, 1 if connected and empty, 2 if connected and not empty
 */
uint8_t BLUETOOTH_BufferEmpty();

#endif /* INC_BLUETOOTH_H_ */
#ifdef __cplusplus//allows code to be ran in a c++ program
 }
#endif

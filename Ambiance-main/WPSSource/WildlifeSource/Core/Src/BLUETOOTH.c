/*
 * Name BLUETOOTH.c
 * Brief: Provides a TX buffer for the virtual USART held over the bluetooth connection
 * Author: Caitlin Bonesio
 * Created: 4/12/25
 * Modified: 5/16/25
 */

#include "BLUETOOTH.h"
#include "BOARD.h"
#include "UART.h"

#define BTBUFFERSIZE USARTBUFFERSIZE

typedef struct BTBuff {
	uint8_t data[BTBUFFERSIZE];
	uint8_t head;
	uint8_t tail;
	uint8_t full;
}BTBuff_t;

static BTBuff_t buffer;


/*
 * @function: BLUETOOTH_BufferInit()
 * @brief: initialize the the bluetooth tx buffer for virtual USART communication
 * @param: none
 * @return: none
 */
void BLUETOOTH_BufferInit(){
	buffer.head = 0;
	buffer.tail = 0;
	buffer.full = 0;
}

/*
 * @function: BLUETOOTH_WriteBuffer(uint8_t input)
 * @brief: places one character into the buffer for the external PC to read
 * @param: input: character to be placed in buffer
 * @return: none
 */
uint8_t BLUETOOTH_WriteBuffer(uint8_t input){
	if(!buffer.full){
		buffer.data[buffer.head] = input;
		buffer.head++;
		buffer.head %= BTBUFFERSIZE;
		buffer.full = ((buffer.head+1)%BTBUFFERSIZE == buffer.tail);

		return 1;
	} else {
		return 0;
	}
}

/*
 * @function: BLUETOOTH_ReadBuffer(uint8_t input)
 * @brief: returns a character from the buffer and removes it from the buffer
 * @param: none
 * @return: -1 if buffer empty, uint8_t character data if the buffer has data to return
 */
int16_t BLUETOOTH_ReadBuffer(){
	int16_t ret = -1;
	if(buffer.head != buffer.tail || buffer.full){

		ret = buffer.data[buffer.tail];
		buffer.tail++;
		buffer.tail %= BTBUFFERSIZE;
		buffer.full = ((buffer.head+1)%BTBUFFERSIZE == buffer.tail);
	}
	return ret;
}

/*
 * @function: BLUETOOTH_BufferEmpty()
 * @brief: returns if the buffer is empty or if the Bluetooth is disconnected
 * @param: none
 * @return: 0 if disconnected, 1 if connected and empty, 2 if connected and not empty
 */
uint8_t BLUETOOTH_BufferEmpty(){
	return (buffer.head == buffer.tail && !buffer.full);
}

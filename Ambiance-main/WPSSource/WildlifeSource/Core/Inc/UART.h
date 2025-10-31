/*
 * Name UART.h
 * Brief: provides a library to initialize and access a free running timer
 * Author: Caitlin Bonesio
 * Created: 2/5/25
 * Modified: 4/14/25
 */
#ifndef UART_H
#define UART_H

#ifdef __cplusplus//allows code to be ran in a c++ program
extern "C" {
#endif

#include "BOARD.h"

#define UARTSUCCESS 0x00
#define UARTFAILED 0x25
#define USARTBUFFERSIZE 512

#define USEUSART 0

extern USART_HandleTypeDef husart1;


/*
 * @function: UART_Init()
 * @brief: initialize the lpuart module
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
int UARTs_Init(void);

/*
 * @function: LPUART_ReadRx()
 * @brief: reads one received byte from the lpuart
 * @param: none
 * @return: the character received, is 0x25 (NAK) if no character to read
 */
char LPUART_ReadRx(void);

/*
 * @function: LPUART_WriteTx()
 * @brief: sends one byte through the lpuart
 * @param: none
 * @return: status, 0x00 if success,  0x25 (NAK) if failed,
 */
char LPUART_WriteTx(char input);

/*
 * @function: USART_ReadRx()
 * @brief: reads one received byte from the usart
 * @param: none
 * @return: the character received, 0x25 (NAK) if no character to read
 */
char USART_ReadRx(void);

/*
 * @function: USART_WriteTx()
 * @brief: sends one byte through the usart
 * @param: none
 * @return: status, 0x00 if success,  0x25 (NAK) if failed,
 */
char USART_WriteTx(char input);
/*
 * @function: USART_TxEmpty()
 * @brief: returns if the Tx  buffer is empty
 * @param: none
 * @return: status, 0x00 if not empty,  0x01 if empty,
 */
uint8_t USART_TxEmpty(void);

#ifdef __cplusplus//allows code to be ran in a c++ program
}
#endif

#endif

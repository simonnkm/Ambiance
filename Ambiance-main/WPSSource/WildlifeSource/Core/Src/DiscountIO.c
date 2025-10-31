/*
 * DiscountIO.c
 *
 *  Created on: Apr 17, 2025
 *      Author: stipi
 */
#include <stdio.h>
#include <string.h>
#include "UART.h"
#include "DiscountIO.h"

#define DEBUGPRINT   	0x0E
#define DEBUGPRINTEND  	'\n'

int	discountprintf (const char * text){
	int length = strlen(text);
	USART_WriteTx(DEBUGPRINT);
	for(int i = 0; i < length; i++){
		USART_WriteTx(text[i]);
	}
	USART_WriteTx(DEBUGPRINTEND);
	return 1;
}



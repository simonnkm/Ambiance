/*
 * Name BOARD.h
 * Brief: provides a library to initialize the board's clocks
 * Author: Caitlin Bonesio
 * Created: 2/4/25
 * Modified: 3/10/25
 */


#ifndef BOARD_H
#define BOARD_H

#ifdef __cplusplus//allows code to be ran in a c++ program
extern "C" {
#endif


#include "stm32wb0x_hal.h"
#include "app_entry.h"
#include "app_common.h"
#include "app_debug.h"
#include "compiler.h"

#include "stm32wb0x_nucleo.h"

#define INIT_ERROR 0
#define INIT_OK 1

/*
 * @function: BOARD_Init()
 * @brief: initialize the board's timers
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
int BOARD_Init(void);

/*
 * @function: BOARD_CrashHandler()
 * @brief: disables all interrupts and enters an infinite loop
 * @param: none
 * @return: none
 */
void BOARD_CrashHandler(void);

/*
 * @function: Error_Handler()
 * @brief: disables all interrupts and enters an infinite loop
 * @param: none
 * @return: none
 */
void Error_Handler(void);

#ifdef __cplusplus//allows code to be ran in a c++ program
}
#endif

#endif /* BOARD_H */

/*
 * Name TIMERS.h
 * Brief: provides a library to initialize and access a free running timer
 * Author: Caitlin Bonesio
 * Created: 3/4/25
 * Modified: 3/7/25
 */

#ifndef TIMERS_H
#define TIMERS_H

#ifdef __cplusplus//allows code to be ran in a c++ program
extern "C" {
#endif
/*
 * @function: TIMERS_Init()
 * @brief: initialize the free running timer
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
int TIMERS_Init();

/*
 * @function: TIMERS_GetMilliSeconds()
 * @brief: accesses the free running timer's milliseconds
 * @param: none
 * @return: uint32_t FRT's milliseconds
 */
uint32_t TIMERS_GetMilliSeconds();

/*
 * @function: TIMERS_GetMicroSeconds()
 * @brief: accesses the free running timer's microseconds
 * @param: none
 * @return: uint32_t FRT's microseconds
 */
uint64_t TIMERS_GetMircoSeconds();

#ifdef __cplusplus//allows code to be ran in a c++ program
}
#endif
#endif /* INC_TIMERS_H_ */

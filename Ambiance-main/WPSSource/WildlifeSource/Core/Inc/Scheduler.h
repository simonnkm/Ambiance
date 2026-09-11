/*
 * Name Scheduler.h
 * Brief: a Scheduler to use when creating event/service modules
 * Author: Caitlin Bonesio
 * Created: 4/21/25
 * Modified: 4/21/25
 */

 #ifdef __cplusplus//allows code to be ran in a c++ program
 extern "C" {
 #endif

 #ifndef SCHEDULER_H//SCHEDULER_H Header Guards
 #define SCHEDULER_H//SCHEDULER_H Header Guards
//----------------------------------------Public Includes----------------------------------------
#include "CONFIG.h"
#include "FIFO.h"

//----------------------------------------Public Defines------------------------------------------
//Bitmask values for Scheduler_RecordBootResetCause()'s "cause" byte, read from
//the STM32's RCC->CSR reset-flag register (see RCC_FLAG_* in stm32wb0x_hal_rcc.h).
//More than one bit can be set if multiple reset causes were latched since the
//flags were last cleared. Logged once per boot alongside the RTC timestamp of
//the first valid time read, so a downloaded log can show exactly how many
//times the device has reset and why - including whether it was ever a
//power-on/brown-out reset (bit RESETCAUSE_POR_BOR), which is the signature of
//a power supply / charging-circuit brownout rather than a firmware fault.
#define RESETCAUSE_POR_BOR   (1<<0)//power-on or brown-out reset (VDD dropped out)
#define RESETCAUSE_PAD       (1<<1)//external NRSTn pin/pad reset
#define RESETCAUSE_SOFT      (1<<2)//software-triggered reset
#define RESETCAUSE_WATCHDOG  (1<<3)//watchdog reset
#define RESETCAUSE_LOCKUP    (1<<4)//CPU lockup reset (firmware fault, not power)

//----------------------------------------Public Functions---------------------------------------
/*
 * @Function: Scheduler_Event_Init
 * @Brief: Provides the initialization function for the events and serviced routine
 * @param: none
 * @return: An 8 bit integer flag reflecting The initialization status
 */
uint8_t Scheduler_Event_Init(FIFO Queue);

/*
 * @Function: Scheduler_RecordBootResetCause
 * @Brief: Stashes the RCC reset-cause bitmask (RESETCAUSE_* bits, read and
 *         cleared once at the very top of main() before anything else can
 *         touch RCC->CSR) so it can be written into the log the first time
 *         the RTC gives a valid timestamp after this boot. Must be called
 *         before that first valid RTC read - i.e. as early in main() as
 *         possible, and in particular before Scheduler_Event_Init().
 * @param: cause - bitwise OR of RESETCAUSE_* flags describing why this boot happened
 * @return: none
 */
void Scheduler_RecordBootResetCause(uint8_t cause);

/*
 * @Function: Scheduler_Event_Init
 * @Brief: Provides the ability for state machines to interact
 * @param: event to be posted
 * @return: none
 */
void Scheduler_Event_Post(Event_t event);

/*
 * @Function: Scheduler_Event_Updater
 * @Brief: Provides the event checker that checks and posts the changes in the
 * @param: none
 * @return: An event
 */
Event_t Scheduler_Event_Updater(void);

/*
 * @Function: Scheduler_Event_Handler
 * @Brief:
 * @param: Event_t event, incoming event for the handler to handle
 * @return: An 8 byte integer success flag, returns 0 if the program should crash
 */
uint8_t Scheduler_Event_Handler(Event_t event);

uint8_t Scheduler_GetMonth();
uint8_t Scheduler_GetDay();
uint8_t Scheduler_GetHour();
uint8_t Scheduler_GetMinute();

 #ifdef __cplusplus//allows code to be ran in a c++ program
  }
 #endif
 #endif//SCHEDULER_H Header Guards


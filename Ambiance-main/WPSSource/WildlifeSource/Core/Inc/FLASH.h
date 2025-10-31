/*
 * Name FLASH.h
 * Brief: provides a library to initialize and access the flash memory
 * Author: Caitlin Bonesio
 * Created: 4/11/25
 * Modified: 4/23/25
 */

#ifndef FLASH_H
#define FLASH_H
#include "CONFIG.h"


typedef struct scheduleEvent {
	uint8_t month;
	uint8_t daystart;
	uint8_t start;
	uint8_t stop;
	uint8_t daystop;
	uint8_t folder;
	uint8_t track;
}scheduleEvent;
//note the number of bytes in this struct is hard coded into COMM.c during the logs data state
/*
 * @function: FLASH_Init()
 * @brief: initialize the flash access library
 * @param: none
 * @return: Init Status, whether the operation failed or succeeded
 */
uint8_t FLASH_Init();

/*
 * @function: FLASH_SetDCVol();
 * @brief: edits the volume and duty cycle stored in memory
 * @param: none
 * @return: success status
 */
uint8_t FLASH_SetDCVol(uint8_t volume, uint8_t DC);

/*
 * @function: FLASH_GetVolume();
 * @brief: read the volume stored in memory
 * @param: none
 * @return: volume
 */
uint8_t FLASH_GetVolume();



/*
 * @function: FLASH_GetDutyCycle();
 * @brief: read the volume stored in memory
 * @param: none
 * @return: Duty Cycle
 */
uint8_t FLASH_GetDutyCycle();

/*
 * @function: FLASH_AppendLogs();
 * @brief: Adds a new event entry to the logs
 * @param: event - new event to be added, must have an empty start xor stop time
 * @return: success status
 */
uint8_t FLASH_AppendLogs(scheduleEvent event);

/*
 * @function: FLASH_GetLogsSize();
 * @brief: returns the size of the logs
 * @param: none
 * @return: Logs size
 */
uint16_t FLASH_GetLogsSize();

/*
 * @function: FLASH_ReadLogs();
 * @brief: allows reading the logs that have been stored in flash memory
 * @param: index to read at
 * @return: an schedule event struct at index
 */
scheduleEvent FLASH_ReadLogs(uint16_t index);

/*
 * @function: FLASH_ClearLogs();
 * @brief: Clears all log entries
 * @param: none
 * @return: success status
 */
uint8_t FLASH_ClearLogs();


/*
 * @function: FLASH_AppendSchedule();
 * @brief: Adds a new event entry to the schedule
 * @param: none
 * @return: success status
 */
uint8_t FLASH_AppendSchedule(scheduleEvent event);

/*
 * @function: FLASH_GetScheduleSize();
 * @brief: returns the size of the schedule
 * @param: none
 * @return: Schedule size
 */
uint16_t FLASH_GetScheduleSize();

/*
 * @function: FLASH_ReadSchedule();
 * @brief: allows reading the schedule that have been stored in flash memory
 * @param: index to read at
 * @return: an schedule event struct at index
 */
scheduleEvent FLASH_ReadSchedule(uint16_t index);

/*
 * @function: FLASH_ClearSchedule();
 * @brief: Clears all Schedule entries
 * @param: none
 * @return: success status
 */
uint8_t FLASH_ClearSchedule();

#endif /* INC_FLASH_H_ */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    USART_app.c
  * @author  MCD Application Team
  * @brief   USART_app application definition.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_common.h"
#include "app_ble.h"
#include "ble.h"
#include "usart_app.h"
#include "usart.h"
#include "stm32_seq.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "COMM.h"
#include "events.h"
#include "BLUETOOTH.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

typedef enum
{
  /* USER CODE BEGIN Service1_APP_SendInformation_t */

  /* USER CODE END Service1_APP_SendInformation_t */
  USART_APP_SENDINFORMATION_LAST
} USART_APP_SendInformation_t;

typedef struct
{
  /* USER CODE BEGIN Service1_APP_Context_t */

  /* USER CODE END Service1_APP_Context_t */
  uint16_t              ConnectionHandle;
} USART_APP_Context_t;

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private macros ------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static USART_APP_Context_t USART_APP_Context;

uint8_t a_USART_UpdateCharData[247];

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
void USART_Notification(USART_NotificationEvt_t *p_Notification)
{
  /* USER CODE BEGIN Service1_Notification_1 */

  /* USER CODE END Service1_Notification_1 */
  switch(p_Notification->EvtOpcode)
  {
    /* USER CODE BEGIN Service1_Notification_Service1_EvtOpcode */

    /* USER CODE END Service1_Notification_Service1_EvtOpcode */

    case USART_TX_READ_EVT:
      /* USER CODE BEGIN Service1Char1_READ_EVT */
      /* USER CODE END Service1Char1_READ_EVT */
      break;

    case USART_RX_WRITE_NO_RESP_EVT:
      /* USER CODE BEGIN Service1Char2_WRITE_NO_RESP_EVT */
      Event_t recieved = {EVENT_USART, p_Notification->DataTransfered.p_Payload[0]};
      COMM_Event_Post(recieved);
      /* USER CODE END Service1Char2_WRITE_NO_RESP_EVT */
      break;

    case USART_REQ_TX_READ_EVT:
      /* USER CODE BEGIN Service1Char3_READ_EVT */

      /* USER CODE END Service1Char3_READ_EVT */
      break;

    case USART_REQ_TX_WRITE_EVT:
      /* USER CODE BEGIN Service1Char3_WRITE_EVT */
        int16_t data = BLUETOOTH_ReadBuffer();
        if(data != -1){
        	uint8_t data1 = (uint8_t)data;
			USART_Data_t value1 = {&data1, 1};
			USART_UpdateValue(USART_TX, &value1);
			uint8_t data2 = 0;
			USART_Data_t value2 = {&data2, 1};
			USART_UpdateValue(USART_REQ_TX, &value2);
        } else {

			uint8_t data2 = 2;
			USART_Data_t value2 = {&data2, 1};
			USART_UpdateValue(USART_REQ_TX, &value2);
        }

      /* USER CODE END Service1Char3_WRITE_EVT */
      break;

    default:
      /* USER CODE BEGIN Service1_Notification_default */

      /* USER CODE END Service1_Notification_default */
      break;
  }
  /* USER CODE BEGIN Service1_Notification_2 */

  /* USER CODE END Service1_Notification_2 */
  return;
}

void USART_APP_EvtRx(USART_APP_ConnHandleNotEvt_t *p_Notification)
{
  /* USER CODE BEGIN Service1_APP_EvtRx_1 */

  /* USER CODE END Service1_APP_EvtRx_1 */

  switch(p_Notification->EvtOpcode)
  {
    /* USER CODE BEGIN Service1_APP_EvtRx_Service1_EvtOpcode */

    /* USER CODE END Service1_APP_EvtRx_Service1_EvtOpcode */
    case USART_CONN_HANDLE_EVT :
      USART_APP_Context.ConnectionHandle = p_Notification->ConnectionHandle;
      /* USER CODE BEGIN Service1_APP_CENTR_CONN_HANDLE_EVT */

      /* USER CODE END Service1_APP_CENTR_CONN_HANDLE_EVT */
      break;
    case USART_DISCON_HANDLE_EVT :
      USART_APP_Context.ConnectionHandle = 0xFFFF;
      /* USER CODE BEGIN Service1_APP_DISCON_HANDLE_EVT */
      /* Start to Advertise to accept a new connection */
            APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);
      /* USER CODE END Service1_APP_DISCON_HANDLE_EVT */
      break;
    default:
      /* USER CODE BEGIN Service1_APP_EvtRx_default */

      /* USER CODE END Service1_APP_EvtRx_default */
      break;
  }

  /* USER CODE BEGIN Service1_APP_EvtRx_2 */

  /* USER CODE END Service1_APP_EvtRx_2 */

  return;
}

void USART_APP_Init(void)
{
  USART_APP_Context.ConnectionHandle = 0xFFFF;
  USART_Init();

  /* USER CODE BEGIN Service1_APP_Init */

  /* USER CODE END Service1_APP_Init */
  return;
}

/* USER CODE BEGIN FD */

/* USER CODE END FD */

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS*/

/* USER CODE END FD_LOCAL_FUNCTIONS*/

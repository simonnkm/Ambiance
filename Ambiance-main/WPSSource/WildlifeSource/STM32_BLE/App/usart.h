/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    USART.h
  * @author  MCD Application Team
  * @brief   Header for USART.c
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef USART_H
#define USART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "ble_status.h"
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported defines ----------------------------------------------------------*/
/* USER CODE BEGIN ED */

/* USER CODE END ED */

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  USART_TX,
  USART_RX,
  USART_REQ_TX,

  /* USER CODE BEGIN Service1_CharOpcode_t */

  /* USER CODE END Service1_CharOpcode_t */

  USART_CHAROPCODE_LAST
} USART_CharOpcode_t;

typedef enum
{
  USART_TX_READ_EVT,
  USART_RX_WRITE_NO_RESP_EVT,
  USART_REQ_TX_READ_EVT,
  USART_REQ_TX_WRITE_EVT,

  /* USER CODE BEGIN Service1_OpcodeEvt_t */

  /* USER CODE END Service1_OpcodeEvt_t */

  USART_BOOT_REQUEST_EVT
} USART_OpcodeEvt_t;

typedef struct
{
  uint8_t *p_Payload;
  uint8_t Length;

  /* USER CODE BEGIN Service1_Data_t */

  /* USER CODE END Service1_Data_t */

} USART_Data_t;

typedef struct
{
  USART_OpcodeEvt_t       EvtOpcode;
  USART_Data_t             DataTransfered;
  uint16_t                ConnectionHandle;
  uint16_t                AttributeHandle;
  uint8_t                 ServiceInstance;

  /* USER CODE BEGIN Service1_NotificationEvt_t */

  /* USER CODE END Service1_NotificationEvt_t */

} USART_NotificationEvt_t;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macros -----------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions ------------------------------------------------------- */
void USART_Init(void);
void USART_Notification(USART_NotificationEvt_t *p_Notification);
tBleStatus USART_UpdateValue(USART_CharOpcode_t CharOpcode, USART_Data_t *pData);
tBleStatus USART_NotifyValue(USART_CharOpcode_t CharOpcode, USART_Data_t *pData, uint16_t ConnectionHandle);
/* USER CODE BEGIN EF */

/* USER CODE END EF */

#ifdef __cplusplus
}
#endif

#endif /*USART_H */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    USART.c
  * @author  MCD Application Team
  * @brief   USART definition.
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
#include <app_common.h>
#include "ble.h"
#include "usart.h"
#include "usart_app.h"
#include "ble_evt.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

typedef struct{
  uint16_t  UsartSvcHdle;				/**< Usart Service Handle */
  uint16_t  TxCharHdle;			/**< TX Characteristic Handle */
  uint16_t  RxCharHdle;			/**< RX Characteristic Handle */
  uint16_t  Req_TxCharHdle;			/**< REQ_TX Characteristic Handle */
/* USER CODE BEGIN Context */
  /* Place holder for Characteristic Descriptors Handle*/

/* USER CODE END Context */
}USART_Context_t;

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private macros ------------------------------------------------------------*/
#define CHARACTERISTIC_DESCRIPTOR_ATTRIBUTE_OFFSET        2
#define CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET             1
#define TX_SIZE        1	/* VIRTUAL USART TX Characteristic size */
#define RX_SIZE        1	/* VIRTUAL USART RX Characteristic size */
#define REQ_TX_SIZE        1	/* VIRTUAL USART REQ TX Characteristic size */
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

static USART_Context_t USART_Context;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/
/* USER CODE BEGIN PFD */

/* USER CODE END PFD */

/* Private functions ----------------------------------------------------------*/

/*
 * UUIDs for VIRTUAL USART service
 */
#define USART_UUID			0x66,0x9a,0x0c,0x20,0x00,0x08,0xa7,0x90,0xf0,0x11,0x7a,0x2b,0xd0,0x8b,0xde,0xd2
#define TX_UUID				0x66,0x9a,0x0c,0x20,0x00,0x08,0xa7,0x90,0xf0,0x11,0x7a,0x2b,0xd1,0x8b,0xde,0xd2
#define RX_UUID				0x66,0x9a,0x0c,0x20,0x00,0x08,0xa7,0x90,0xf0,0x11,0x7a,0x2b,0xd2,0x8b,0xde,0xd2
#define REQ_TX_UUID			0x66,0x9a,0x0c,0x20,0x00,0x08,0xa7,0x90,0xf0,0x11,0x7a,0x2b,0xd3,0x8b,0xde,0xd2

/* USER CODE BEGIN DESCRIPTORS DECLARATION */

/* USER CODE END DESCRIPTORS DECLARATION */

uint8_t tx_val_buffer[TX_SIZE];

static ble_gatt_val_buffer_def_t tx_val_buffer_def = {
  .op_flags = BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG,
  .val_len = TX_SIZE,
  .buffer_len = sizeof(tx_val_buffer),
  .buffer_p = tx_val_buffer
};

uint8_t rx_val_buffer[RX_SIZE];

static ble_gatt_val_buffer_def_t rx_val_buffer_def = {
  .op_flags = BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG,
  .val_len = RX_SIZE,
  .buffer_len = sizeof(rx_val_buffer),
  .buffer_p = rx_val_buffer
};

uint8_t req_tx_val_buffer[REQ_TX_SIZE];

static ble_gatt_val_buffer_def_t req_tx_val_buffer_def = {
  .op_flags = BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG,
  .val_len = REQ_TX_SIZE,
  .buffer_len = sizeof(req_tx_val_buffer),
  .buffer_p = req_tx_val_buffer
};

/* VIRTUAL USART service RX (write without response), characteristics definition */
static const ble_gatt_chr_def_t usart_chars[] = {
	{
        .properties = BLE_GATT_SRV_CHAR_PROP_READ,
        .permissions = BLE_GATT_SRV_PERM_NONE,
        .min_key_size = 0x10,
        .uuid = BLE_UUID_INIT_128(TX_UUID),
        .val_buffer_p = &tx_val_buffer_def
    },
	{
        .properties = BLE_GATT_SRV_CHAR_PROP_WRITE_NO_RESP,
        .permissions = BLE_GATT_SRV_PERM_NONE,
        .min_key_size = 0x10,
        .uuid = BLE_UUID_INIT_128(RX_UUID),
        .val_buffer_p = &rx_val_buffer_def
    },
	{
        .properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_WRITE,
        .permissions = BLE_GATT_SRV_PERM_NONE,
        .min_key_size = 0x10,
        .uuid = BLE_UUID_INIT_128(REQ_TX_UUID),
        .val_buffer_p = &req_tx_val_buffer_def
    },
};

/* VIRTUAL USART service definition */
static const ble_gatt_srv_def_t usart_service = {
   .type = BLE_GATT_SRV_PRIMARY_SRV_TYPE,
   .uuid = BLE_UUID_INIT_128(USART_UUID),
   .chrs = {
       .chrs_p = (ble_gatt_chr_def_t *)usart_chars,
       .chr_count = 3U,
   },
};

/* USER CODE BEGIN PF */

/* USER CODE END PF */

/**
 * @brief  Event handler
 * @param  p_Event: Address of the buffer holding the p_Event
 * @retval Ack: Return whether the p_Event has been managed or not
 */
static BLEEVT_EvtAckStatus_t USART_EventHandler(aci_blecore_event *p_evt)
{
  BLEEVT_EvtAckStatus_t return_value = BLEEVT_NoAck;
  aci_gatt_srv_attribute_modified_event_rp0 *p_attribute_modified;
  aci_gatt_srv_write_event_rp0   *p_write;
  aci_gatt_srv_read_event_rp0    *p_read;
  USART_NotificationEvt_t notification;
  /* USER CODE BEGIN Service1_EventHandler_1 */

  /* USER CODE END Service1_EventHandler_1 */

  switch(p_evt->ecode)
  {
    case ACI_GATT_SRV_ATTRIBUTE_MODIFIED_VSEVT_CODE:
    {
      /* USER CODE BEGIN EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_BEGIN */

      /* USER CODE END EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_BEGIN */
      p_attribute_modified = (aci_gatt_srv_attribute_modified_event_rp0*)p_evt->data;
      notification.ConnectionHandle         = p_attribute_modified->Connection_Handle;
      notification.AttributeHandle          = p_attribute_modified->Attr_Handle;
      notification.DataTransfered.Length    = p_attribute_modified->Attr_Data_Length;
      notification.DataTransfered.p_Payload = p_attribute_modified->Attr_Data;
      if(p_attribute_modified->Attr_Handle == (USART_Context.RxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))
      {
        return_value = BLEEVT_Ack;

        notification.EvtOpcode = USART_RX_WRITE_NO_RESP_EVT;
        /* USER CODE BEGIN Service1_Char_2_ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE */

        /* USER CODE END Service1_Char_2_ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE */
        USART_Notification(&notification);
      } /* if(p_attribute_modified->Attr_Handle == (USART_Context.RxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))*/
      else if(p_attribute_modified->Attr_Handle == (USART_Context.Req_TxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))
      {
        return_value = BLEEVT_Ack;

        notification.EvtOpcode = USART_REQ_TX_WRITE_EVT;
        /* USER CODE BEGIN Service1_Char_3_ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE */

        /* USER CODE END Service1_Char_3_ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE */
        USART_Notification(&notification);
      } /* if(p_attribute_modified->Attr_Handle == (USART_Context.Req_TxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))*/

      /* USER CODE BEGIN EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_END */

      /* USER CODE END EVT_BLUE_GATT_ATTRIBUTE_MODIFIED_END */
      break;/* ACI_GATT_SRV_ATTRIBUTE_MODIFIED_VSEVT_CODE */
    }
    case ACI_GATT_SRV_READ_VSEVT_CODE :
    {
      /* USER CODE BEGIN EVT_BLUE_GATT_SRV_READ_BEGIN */

      /* USER CODE END EVT_BLUE_GATT_SRV_READ_BEGIN */
      p_read = (aci_gatt_srv_read_event_rp0*)p_evt->data;
	  if(p_read->Attribute_Handle == (USART_Context.TxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))
	  {
		return_value = BLEEVT_Ack;
		/*USER CODE BEGIN Service1_Char_1_ACI_GATT_SRV_READ_VSEVT_CODE_1 */
		notification.EvtOpcode = USART_TX_READ_EVT;
		USART_Notification(&notification);
		/*USER CODE END Service1_Char_1_ACI_GATT_SRV_READ_VSEVT_CODE_1 */

		/*USER CODE BEGIN Service1_Char_1_ACI_GATT_SRV_READ_VSEVT_CODE_2 */
		  /*USER CODE END Service1_Char_1_ACI_GATT_SRV_READ_VSEVT_CODE_2 */
	  } /* if(p_read->Attribute_Handle == (USART_Context.TxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))*/
	  else if(p_read->Attribute_Handle == (USART_Context.Req_TxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))
	  {
		return_value = BLEEVT_Ack;
		/*USER CODE BEGIN Service1_Char_3_ACI_GATT_SRV_READ_VSEVT_CODE_1 */
		/*USER CODE END Service1_Char_3_ACI_GATT_SRV_READ_VSEVT_CODE_1 */

		/*USER CODE BEGIN Service1_Char_3_ACI_GATT_SRV_READ_VSEVT_CODE_2 */

		  /*USER CODE END Service1_Char_3_ACI_GATT_SRV_READ_VSEVT_CODE_2 */
	  } /* if(p_read->Attribute_Handle == (USART_Context.Req_TxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))*/

      /* USER CODE BEGIN EVT_BLUE_GATT_SRV_READ_END */

      /* USER CODE END EVT_EVT_BLUE_GATT_SRV_READ_END */
      break;/* ACI_GATT_SRV_READ_VSEVT_CODE */
    }
    case ACI_GATT_SRV_WRITE_VSEVT_CODE:
    {
      /* USER CODE BEGIN EVT_BLUE_SRV_GATT_BEGIN */

      /* USER CODE END EVT_BLUE_SRV_GATT_BEGIN */
      p_write = (aci_gatt_srv_write_event_rp0*)p_evt->data;
      if(p_write->Attribute_Handle == (USART_Context.RxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))
      {
        return_value = BLEEVT_Ack;
        /*USER CODE BEGIN Service1_Char_2_ACI_GATT_SRV_WRITE_VSEVT_CODE */
        /*USER CODE END Service1_Char_2_ACI_GATT_SRV_WRITE_VSEVT_CODE*/
      } /*if(p_write->Attribute_Handle == (USART_Context.RxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))*/

      else if(p_write->Attribute_Handle == (USART_Context.Req_TxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))
      {
        return_value = BLEEVT_Ack;
        /*USER CODE BEGIN Service1_Char_3_ACI_GATT_SRV_WRITE_VSEVT_CODE */
        /*USER CODE END Service1_Char_3_ACI_GATT_SRV_WRITE_VSEVT_CODE*/
      } /*if(p_write->Attribute_Handle == (USART_Context.Req_TxCharHdle + CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET))*/

      /* USER CODE BEGIN EVT_BLUE_GATT_SRV_WRITE_END */

      /* USER CODE END EVT_BLUE_GATT_SRV_WRITE_END */
      break;/* ACI_GATT_SRV_WRITE_VSEVT_CODE */
    }
    case ACI_GATT_TX_POOL_AVAILABLE_VSEVT_CODE:
    {
      aci_gatt_tx_pool_available_event_rp0 *p_tx_pool_available_event;
      p_tx_pool_available_event = (aci_gatt_tx_pool_available_event_rp0 *) p_evt->data;
      UNUSED(p_tx_pool_available_event);

      /* USER CODE BEGIN ACI_GATT_TX_POOL_AVAILABLE_VSEVT_CODE */

      /* USER CODE END ACI_GATT_TX_POOL_AVAILABLE_VSEVT_CODE */
      break;/* ACI_GATT_TX_POOL_AVAILABLE_VSEVT_CODE*/
    }
    case ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE:
    {
      aci_att_exchange_mtu_resp_event_rp0 *p_exchange_mtu;
      p_exchange_mtu = (aci_att_exchange_mtu_resp_event_rp0 *)  p_evt->data;
      UNUSED(p_exchange_mtu);

      /* USER CODE BEGIN ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE */

      /* USER CODE END ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE */
      break;/* ACI_ATT_EXCHANGE_MTU_RESP_VSEVT_CODE */
    }
    /* USER CODE BEGIN BLECORE_EVT */

    /* USER CODE END BLECORE_EVT */
  default:
    /* USER CODE BEGIN EVT_DEFAULT */

    /* USER CODE END EVT_DEFAULT */
    break;
  }

  /* USER CODE BEGIN Service1_EventHandler_2 */

  /* USER CODE END Service1_EventHandler_2 */

  return(return_value);
}/* end USART_EventHandler */

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void USART_Init(void)
{
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  UNUSED(USART_Context);

  /* USER CODE BEGIN InitService1Svc_1 */

  /* USER CODE END InitService1Svc_1 */

  /**
   *  Register the event handler to the BLE controller
   */
  BLEEVT_RegisterGattEvtHandler(USART_EventHandler);

  ret = aci_gatt_srv_add_service((ble_gatt_srv_def_t *)&usart_service);

  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail   : aci_gatt_srv_add_service command: USART, error code: 0x%x \n", ret);
  }
  else
  {
    APP_DBG_MSG("  Success: aci_gatt_srv_add_service command: USART \n");
  }

  USART_Context.UsartSvcHdle = aci_gatt_srv_get_service_handle((ble_gatt_srv_def_t *) &usart_service);
  USART_Context.TxCharHdle = aci_gatt_srv_get_char_decl_handle((ble_gatt_chr_def_t *)&usart_chars[0]);
  USART_Context.RxCharHdle = aci_gatt_srv_get_char_decl_handle((ble_gatt_chr_def_t *)&usart_chars[1]);
  USART_Context.Req_TxCharHdle = aci_gatt_srv_get_char_decl_handle((ble_gatt_chr_def_t *)&usart_chars[2]);

  /* USER CODE BEGIN InitService1Svc_2 */

  /* USER CODE END InitService1Svc_2 */

  if (ret != BLE_STATUS_SUCCESS)
  {
    APP_DBG_MSG("  Fail registering USART handlers\n");
  }

  return;
}

/**
 * @brief  Characteristic update
 * @param  CharOpcode: Characteristic identifier
 * @param  pData: pointer to the new data to be written in the characteristic
 *
 */
tBleStatus USART_UpdateValue(USART_CharOpcode_t CharOpcode, USART_Data_t *pData)
{
  tBleStatus ret = BLE_STATUS_SUCCESS;

  /* USER CODE BEGIN Service1_App_Update_Char_1 */

  /* USER CODE END Service1_App_Update_Char_1 */

  switch(CharOpcode)
  {
    case USART_TX:
      memcpy(tx_val_buffer, pData->p_Payload, MIN(pData->Length, sizeof(tx_val_buffer)));
      /* USER CODE BEGIN Service1_Char_Value_1*/

      /* USER CODE END Service1_Char_Value_1*/
      break;

    case USART_RX:
      memcpy(rx_val_buffer, pData->p_Payload, MIN(pData->Length, sizeof(rx_val_buffer)));
      /* USER CODE BEGIN Service1_Char_Value_2*/

      /* USER CODE END Service1_Char_Value_2*/
      break;

    case USART_REQ_TX:
      memcpy(req_tx_val_buffer, pData->p_Payload, MIN(pData->Length, sizeof(req_tx_val_buffer)));
      /* USER CODE BEGIN Service1_Char_Value_3*/

      /* USER CODE END Service1_Char_Value_3*/
      break;

    default:
      break;
  }

  /* USER CODE BEGIN Service1_App_Update_Char_2 */

  /* USER CODE END Service1_App_Update_Char_2 */

  return ret;
}

/**
 * @brief  Characteristic notification
 * @param  CharOpcode: Characteristic identifier
 * @param  pData: pointer to the data to be notified to the client
 * @param  ConnectionHandle: connection handle identifying the client to be notified.
 *
 */
tBleStatus USART_NotifyValue(USART_CharOpcode_t CharOpcode, USART_Data_t *pData, uint16_t ConnectionHandle)
{
  tBleStatus ret = BLE_STATUS_INVALID_PARAMS;
  /* USER CODE BEGIN Service1_App_Notify_Char_1 */

  /* USER CODE END Service1_App_Notify_Char_1 */

  switch(CharOpcode)
  {

    default:
      break;
  }

  /* USER CODE BEGIN Service1_App_Notify_Char_2 */

  /* USER CODE END Service1_App_Notify_Char_2 */

  return ret;
}

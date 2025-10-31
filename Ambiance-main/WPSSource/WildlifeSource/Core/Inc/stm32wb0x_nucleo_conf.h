
/**
  ******************************************************************************
  * @file    stm32wb0x_nucleo_conf.h
  * @author  MCD Application Team
  * @brief   STM32WB0x_Nucleo board configuration file.
  *          This file should be copied to the application folder and renamed
  *          to stm32wb0x_nucleo_conf.h
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32WB0X_NUCLEO_CONF_H
#define STM32WB0X_NUCLEO_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wb0x_hal.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32WB0X_NUCLEO
  * @{
  */

/** @defgroup STM32WB0X_NUCLEO_CONFIG Config
  * @{
  */

/** @defgroup STM32WB0X_NUCLEO_CONFIG_Exported_Constants Exported Constants
  * @{
  */
/* Nucleo pin and part number defines */
#define NUCLEO_WB05KZ

/* COM define */
#define USE_COM_LOG                         0U
#define USE_BSP_COM_FEATURE                 1U

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32WB0XXX_NUCLEO_CONF_H */

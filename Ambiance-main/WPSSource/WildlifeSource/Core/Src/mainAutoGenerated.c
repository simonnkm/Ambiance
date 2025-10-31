///* USER CODE BEGIN Header */
////**
////  ******************************************************************************
////  * @file           : main.c
////  * @brief          : Main program body
////  ******************************************************************************
////  * @attention
////  *
////  * Copyright (c) 2025 STMicroelectronics.
////  * All rights reserved.
////  *
////  * This software is licensed under terms that can be found in the LICENSE file
////  * in the root directory of this software component.
////  * If no LICENSE file comes with this software, it is provided AS-IS.
////  *
////  ******************************************************************************
////  */
///* USER CODE END Header */
///* Includes ------------------------------------------------------------------*/
//#include "main.h"
//
///* Private includes ----------------------------------------------------------*/
///* USER CODE BEGIN Includes */
////
///* USER CODE END Includes */
//
///* Private typedef -----------------------------------------------------------*/
///* USER CODE BEGIN PTD */
////////
///* USER CODE END PTD */
//
///* Private define ------------------------------------------------------------*/
///* USER CODE BEGIN PD */
////////
///* USER CODE END PD */
//
///* Private macro -------------------------------------------------------------*/
///* USER CODE BEGIN PM */
////////
///* USER CODE END PM */
//
///* Private variables ---------------------------------------------------------*/
//
//static uint32_t delay = 250;
//
//I2C_HandleTypeDef hi2c1;
//
//UART_HandleTypeDef hlpuart1;
//
//PKA_HandleTypeDef hpka;
//
//RNG_HandleTypeDef hrng;
//
//TIM_HandleTypeDef htim2;
//
///* USER CODE BEGIN PV */
////////
///* USER CODE END PV */
//
///* Private function prototypes -----------------------------------------------*/
//void SystemClock_Config(void);
//void PeriphCommonClock_Config(void);
//static void MX_GPIO_Init(void);
//static void MX_LPUART1_UART_Init(void);
//static void MX_RADIO_Init(void);
//static void MX_RADIO_TIMER_Init(void);
//static void MX_TIM2_Init(void);
//static void MX_RNG_Init(void);
//static void MX_PKA_Init(void);
//static void MX_I2C1_Init(void);
///* USER CODE BEGIN PFP */
////////
///* USER CODE END PFP */
//
///* Private user code ---------------------------------------------------------*/
///* USER CODE BEGIN 0 */
////////
///* USER CODE END 0 */
//
///**
//  * @brief  The application entry point.
//  * @retval int
//  */
//int main(void)
//{
//
//  /* USER CODE BEGIN 1 */
////////
//  /* USER CODE END 1 */
//
//  /* MCU Configuration--------------------------------------------------------*/
//
//  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//  HAL_Init();
//
//  /* USER CODE BEGIN Init */
////////
//  /* USER CODE END Init */
//
//  /* Configure the system clock */
//  SystemClock_Config();
//
//  /* Configure the peripherals common clocks */
//  PeriphCommonClock_Config();
//
//  /* USER CODE BEGIN SysInit */
////////
//  /* USER CODE END SysInit */
//
//  /* Initialize all configured peripherals */
//  MX_GPIO_Init();
//  MX_LPUART1_UART_Init();
//  MX_RADIO_Init();
//  MX_RADIO_TIMER_Init();
//  MX_TIM2_Init();
//  MX_RNG_Init();
//  MX_PKA_Init();
//  MX_I2C1_Init();
//  /* USER CODE BEGIN 2 */
////////
//  /* USER CODE END 2 */
//
//  /* Initialize leds */
//  BSP_LED_Init(LED_BLUE);
//  BSP_LED_Init(LED_GREEN);
//  BSP_LED_Init(LED_RED);
//
//  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
//  BSP_PB_Init(B1, BUTTON_MODE_EXTI);
//  BSP_PB_Init(B2, BUTTON_MODE_EXTI);
//  BSP_PB_Init(B3, BUTTON_MODE_EXTI);
//
//  /* USER CODE BEGIN BSP */
////
//  /* USER CODE END BSP */
//
//  /* Init code for STM32_BLE */
//  MX_APPE_Init(NULL);
//
//  /* Infinite loop */
//  /* USER CODE BEGIN WHILE */
////  while (1)
////  {
////
////    /* -- Sample board code for User push-button in interrupt mode ---- */
////    BSP_LED_Toggle(LED_BLUE);
////    HAL_Delay(delay);
////
////    BSP_LED_Toggle(LED_GREEN);
////    HAL_Delay(delay);
////
////    BSP_LED_Toggle(LED_RED);
////    HAL_Delay(delay);
////
//    /* USER CODE END WHILE */
//    MX_APPE_Process();
//
//    /* USER CODE BEGIN 3 */
////////  }
//  /* USER CODE END 3 */
//}
//
///**
//  * @brief System Clock Configuration
//  * @retval None
//  */
//void SystemClock_Config(void)
//{
//  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
//  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
//
//  /** Initializes the RCC Oscillators according to the specified parameters
//  * in the RCC_OscInitTypeDef structure.
//  */
//  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
//  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
//  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
//  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Configure the SYSCLKSource and SYSCLKDivider
//  */
//  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_RC64MPLL;
//  RCC_ClkInitStruct.SYSCLKDivider = RCC_RC64MPLL_DIV2;
//
//  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_WAIT_STATES_0) != HAL_OK)
//  {
//    Error_Handler();
//  }
//}
//
///**
//  * @brief Peripherals Common Clock Configuration
//  * @retval None
//  */
//void PeriphCommonClock_Config(void)
//{
//  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
//
//  /** Initializes the peripherals clock
//  */
//  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
//  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLK_DIV4;
//
//  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
//  {
//    Error_Handler();
//  }
//}
//
///**
//  * @brief I2C1 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_I2C1_Init(void)
//{
//
//  /* USER CODE BEGIN I2C1_Init 0 */
//
//  /* USER CODE END I2C1_Init 0 */
//
//  /* USER CODE BEGIN I2C1_Init 1 */
//
//  /* USER CODE END I2C1_Init 1 */
//  hi2c1.Instance = I2C1;
//  hi2c1.Init.Timing = 0x00303D5B;
//  hi2c1.Init.OwnAddress1 = 0;
//  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
//  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
//  hi2c1.Init.OwnAddress2 = 0;
//  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
//  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
//  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
//  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Configure Analogue filter
//  */
//  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
//  {
//    Error_Handler();
//  }
//
//  /** Configure Digital filter
//  */
//  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN I2C1_Init 2 */
//
//  /* USER CODE END I2C1_Init 2 */
//
//}
//
///**
//  * @brief LPUART1 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_LPUART1_UART_Init(void)
//{
//
//  /* USER CODE BEGIN LPUART1_Init 0 */
////////
//  /* USER CODE END LPUART1_Init 0 */
//
//  /* USER CODE BEGIN LPUART1_Init 1 */
////////
//  /* USER CODE END LPUART1_Init 1 */
//  hlpuart1.Instance = LPUART1;
//  hlpuart1.Init.BaudRate = 209700;
//  hlpuart1.Init.WordLength = UART_WORDLENGTH_7B;
//  hlpuart1.Init.StopBits = UART_STOPBITS_1;
//  hlpuart1.Init.Parity = UART_PARITY_NONE;
//  hlpuart1.Init.Mode = UART_MODE_TX_RX;
//  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
//  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
//  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
//  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
//  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN LPUART1_Init 2 */
////////
//  /* USER CODE END LPUART1_Init 2 */
//
//}
//
///**
//  * @brief PKA Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_PKA_Init(void)
//{
//
//  /* USER CODE BEGIN PKA_Init 0 */
//////
//  /* USER CODE END PKA_Init 0 */
//
//  /* USER CODE BEGIN PKA_Init 1 */
//////
//  /* USER CODE END PKA_Init 1 */
//  hpka.Instance = PKA;
//  if (HAL_PKA_Init(&hpka) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN PKA_Init 2 */
//////
//  /* USER CODE END PKA_Init 2 */
//
//}
//
///**
//  * @brief RADIO Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_RADIO_Init(void)
//{
//
//  /* USER CODE BEGIN RADIO_Init 0 */
////////
//  /* USER CODE END RADIO_Init 0 */
//
//  RADIO_HandleTypeDef hradio = {0};
//
//  /* USER CODE BEGIN RADIO_Init 1 */
////////
//  /* USER CODE END RADIO_Init 1 */
//
//  if (__HAL_RCC_RADIO_IS_CLK_DISABLED())
//  {
//    /* Radio Peripheral reset */
//    __HAL_RCC_RADIO_FORCE_RESET();
//    __HAL_RCC_RADIO_RELEASE_RESET();
//
//    /* Enable Radio peripheral clock */
//    __HAL_RCC_RADIO_CLK_ENABLE();
//  }
//  hradio.Instance = RADIO;
//  HAL_RADIO_Init(&hradio);
//  /* USER CODE BEGIN RADIO_Init 2 */
////////
//  /* USER CODE END RADIO_Init 2 */
//
//}
//
///**
//  * @brief RADIO_TIMER Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_RADIO_TIMER_Init(void)
//{
//
//  /* USER CODE BEGIN RADIO_TIMER_Init 0 */
////////
//  /* USER CODE END RADIO_TIMER_Init 0 */
//
//  RADIO_TIMER_InitTypeDef RADIO_TIMER_InitStruct = {0};
//
//  /* USER CODE BEGIN RADIO_TIMER_Init 1 */
////////
//  /* USER CODE END RADIO_TIMER_Init 1 */
//
//  if (__HAL_RCC_RADIO_IS_CLK_DISABLED())
//  {
//    /* Radio Peripheral reset */
//    __HAL_RCC_RADIO_FORCE_RESET();
//    __HAL_RCC_RADIO_RELEASE_RESET();
//
//    /* Enable Radio peripheral clock */
//    __HAL_RCC_RADIO_CLK_ENABLE();
//  }
//  /* Wait to be sure that the Radio Timer is active */
//  while(LL_RADIO_TIMER_GetAbsoluteTime(WAKEUP) < 0x10);
//  RADIO_TIMER_InitStruct.XTAL_StartupTime = 320;
//  RADIO_TIMER_InitStruct.enableInitialCalibration = TRUE;
//  RADIO_TIMER_InitStruct.periodicCalibrationInterval = 10000;
//  HAL_RADIO_TIMER_Init(&RADIO_TIMER_InitStruct);
//  /* USER CODE BEGIN RADIO_TIMER_Init 2 */
////////
//  /* USER CODE END RADIO_TIMER_Init 2 */
//
//}
//
///**
//  * @brief RNG Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_RNG_Init(void)
//{
//
//  /* USER CODE BEGIN RNG_Init 0 */
//////
//  /* USER CODE END RNG_Init 0 */
//
//  /* USER CODE BEGIN RNG_Init 1 */
//////
//  /* USER CODE END RNG_Init 1 */
//  hrng.Instance = RNG;
//  if (HAL_RNG_Init(&hrng) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN RNG_Init 2 */
//////
//  /* USER CODE END RNG_Init 2 */
//
//}
//
///**
//  * @brief TIM2 Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_TIM2_Init(void)
//{
//
//  /* USER CODE BEGIN TIM2_Init 0 */
////////
//  /* USER CODE END TIM2_Init 0 */
//
//  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
//
//  /* USER CODE BEGIN TIM2_Init 1 */
////////
//  /* USER CODE END TIM2_Init 1 */
//  htim2.Instance = TIM2;
//  htim2.Init.Prescaler = 32;
//  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
//  htim2.Init.Period = 1000;
//  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
//  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
//  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
//  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
//  {
//    Error_Handler();
//  }
//  /* USER CODE BEGIN TIM2_Init 2 */
////////
//  /* USER CODE END TIM2_Init 2 */
//
//}
//
///**
//  * @brief GPIO Initialization Function
//  * @param None
//  * @retval None
//  */
//static void MX_GPIO_Init(void)
//{
//  GPIO_InitTypeDef GPIO_InitStruct = {0};
//  /* USER CODE BEGIN MX_GPIO_Init_1 */
//  /* USER CODE END MX_GPIO_Init_1 */
//
//  /* GPIO Ports Clock Enable */
//  __HAL_RCC_GPIOB_CLK_ENABLE();
//  __HAL_RCC_GPIOA_CLK_ENABLE();
//
//  /*Configure GPIO pins : PA3 PA2 PA10 PA11 */
//  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_10|GPIO_PIN_11;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//
//  /*Configure GPIO pins : PB15 PB13 */
//  GPIO_InitStruct.Pin = GPIO_PIN_15|GPIO_PIN_13;
//  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
//  GPIO_InitStruct.Pull = GPIO_NOPULL;
//  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//  /**/
//  HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_3|PWR_GPIO_BIT_2|PWR_GPIO_BIT_10|PWR_GPIO_BIT_11);
//
//  /**/
//  HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_15|PWR_GPIO_BIT_13);
//
//  /**/
//  HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_3|PWR_GPIO_BIT_2|PWR_GPIO_BIT_10|PWR_GPIO_BIT_11);
//
//  /**/
//  HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_15|PWR_GPIO_BIT_13);
//
//  /*RT DEBUG GPIO_Init */
//  RT_DEBUG_GPIO_Init();
//
//  /* USER CODE BEGIN MX_GPIO_Init_2 */
//  /* USER CODE END MX_GPIO_Init_2 */
//}
//
///* USER CODE BEGIN 4 */
////////
///* USER CODE END 4 */
//
///**
//  * @brief EXTI line detection callback.
//  * @param GPIO_Pin: Specifies the pins connected EXTI line
//  * @retval None
//  */
//void HAL_GPIO_EXTI_Callback(GPIO_TypeDef* GPIOx,uint16_t GPIO_Pin)
//{
//  switch(GPIO_Pin)
//  {
//    case B1_PIN:
//      /* Change the period to 100 ms */
//      delay = 100;
//      break;
//    case B2_PIN:
//      /* Change the period to 500 ms */
//      delay = 500;
//      break;
//    case B3_PIN:
//      /* Change the period to 1000 ms */
//      delay = 1000;
//      break;
//    default:
//      break;
//  }
//}
//
///**
//  * @brief  This function is executed in case of error occurrence.
//  * @retval None
//  */
//void Error_Handler(void)
//{
//  /* USER CODE BEGIN Error_Handler_Debug */
////////  /* User can add his own implementation to report the HAL error return state */
////////  __disable_irq();
////////  while (1)
////////  {
////////  }
//  /* USER CODE END Error_Handler_Debug */
//}
//
//#ifdef  USE_FULL_ASSERT
///**
//  * @brief  Reports the name of the source file and the source line number
//  *         where the assert_param error has occurred.
//  * @param  file: pointer to the source file name
//  * @param  line: assert_param error line source number
//  * @retval None
//  */
//void assert_failed(uint8_t *file, uint32_t line)
//{
//  /* USER CODE BEGIN 6 */
////////  /* User can add his own implementation to report the file name and line number,
////////     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//  /* USER CODE END 6 */
//}
//#endif /* USE_FULL_ASSERT */

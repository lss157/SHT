/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
/* 温湿度传感器需要的定时器 */
TIM_HandleTypeDef htim16;


static void MX_GPIO_Init(void);
//static void MX_TIM2_Init(void);
static void MX_TIM16_Init(void);
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */
/* 外设初始化 */
void Board_Init(void)
{
    /* 一组LED引脚和一个用于中断的按键引脚 */
    MX_GPIO_Init();
    /* 用于温湿度传感器的定时器 */
//    MX_TIM16_Init();
}
/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
    设置一组LED引脚和一个用于中断的按键引脚
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED3_Pin|LED2_Pin|LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PBPin PBPin PBPin */
  GPIO_InitStruct.Pin = LED3_Pin|LED2_Pin|LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
//  GPIO_InitStruct.Pin = BUT1_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
//  GPIO_InitStruct.Pull = GPIO_PULLUP;
//  HAL_GPIO_Init(BUT1_GPIO_Port, &GPIO_InitStruct);

//  /* EXTI interrupt init*/
//  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
//  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}

/* 用于温湿度传感器的定时器 */
static void MX_TIM16_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 39;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 19;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim16, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}  

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

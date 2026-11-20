/**
  ******************************************************************************
  * @file    py32f0xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "py32f0xx_it.h"

/* 说明：本空工程的 UART / I2C 都是阻塞式轮询，不开任何外设中断，
 *       因此这里只有 Cortex-M0+ 的异常与 SysTick 处理。 */

/******************************************************************************/
/*           Cortex-M0+ Processor Interruption and Exception Handlers          */
/******************************************************************************/
void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void SVC_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* PY32F0xx Peripheral Interrupt Handlers                                     */
/* 用到外设中断时在这里补对应 Handler，名字参考 startup_py32f002ax5.s        */
/******************************************************************************/

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/

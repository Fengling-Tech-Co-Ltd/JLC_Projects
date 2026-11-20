/**
  ******************************************************************************
  * @file    Bsp_Clock.h
  * @brief   系统时钟：HSI 24MHz 直出（PY32F002A 无 PLL、无 LSE）
  ******************************************************************************
  */
#ifndef __BSP_CLOCK_H
#define __BSP_CLOCK_H

#include "main.h"

#define BSP_SYSCLK_HZ    24000000u

void Bsp_Clock_Init(void);

#endif /* __BSP_CLOCK_H */

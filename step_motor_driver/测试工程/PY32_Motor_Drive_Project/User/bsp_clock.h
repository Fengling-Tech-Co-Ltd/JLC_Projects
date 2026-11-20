/**
  ******************************************************************************
  * @file    bsp_clock.h
  * @brief   系统时钟配置接口
  * @note    板上 PF0/PF1 用作 USART1，这两脚同时是 OSC_IN/OSC_OUT，
  *          因此没有外部晶振可用，只能 HSI 24MHz 经 PLL x2 到 48MHz。
  ******************************************************************************
  */
#ifndef __BSP_CLOCK_H
#define __BSP_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "py32f0xx_hal.h"

#define BSP_SYSCLK_HZ     48000000U

void bsp_clock_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_CLOCK_H */

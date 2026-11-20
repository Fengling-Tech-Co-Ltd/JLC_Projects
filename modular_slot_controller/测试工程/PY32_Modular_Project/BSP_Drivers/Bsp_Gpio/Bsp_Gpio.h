/**
  ******************************************************************************
  * @file    Bsp_Gpio.h
  * @brief   GPIO 安全态：悬空脚配模拟输入，不给任何脚上意外电平
  ******************************************************************************
  */
#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "main.h"

/* 上电第一件事：把不用的脚（PA4、PF0）配成模拟输入，避免对外输出 */
void Bsp_Gpio_SafeInit(void);

/* 三路开关量输入的初始化（BIT0/BIT1/液位），无上下拉，
 * 电平由大板 R-2R 和 MCP6541 推挽输出驱动 */
void Bsp_Gpio_InputInit(void);

#endif /* __BSP_GPIO_H */

/**
  ******************************************************************************
  * @file    bsp_clock.c
  * @brief   HSI 24MHz -> PLL x2 -> SYSCLK 48MHz，AHB/APB1 均不分频
  * @note    1. 不能用 HSE：PF0/PF1 已接 USART1，即芯片的 OSC_IN/OSC_OUT。
  *          2. PY32F030 的 PLL 倍频固定为 2，HAL 里没有倍频字段，
  *             只要求输入源在 16~24MHz 之间，HSI 配 24MHz 正好满足。
  *          3. APB1 不分频，TIM1 / TIM3 / TIM16 的计数时钟都是 48MHz，
  *             VREF 与 STEP 的换算直接按 48MHz 算。
  ******************************************************************************
  */
#include "bsp_clock.h"
#include "main.h"

void bsp_clock_init(void)
{
  RCC_OscInitTypeDef osc_init = {0};
  RCC_ClkInitTypeDef clk_init = {0};

  /* 振荡器：只开 HSI，标定到 24MHz */
  osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  osc_init.HSIState = RCC_HSI_ON;
  osc_init.HSIDiv = RCC_HSI_DIV1;
  osc_init.HSICalibrationValue = RCC_HSICALIBRATION_24MHz;
  osc_init.HSEState = RCC_HSE_OFF;
  osc_init.LSIState = RCC_LSI_OFF;
  osc_init.LSEState = RCC_LSE_OFF;

  osc_init.PLL.PLLState = RCC_PLL_ON;
  osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSI;

  if (HAL_RCC_OscConfig(&osc_init) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  /* 时钟树：PLL 作 SYSCLK，AHB/APB1 不分频 */
  clk_init.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk_init.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_1) != HAL_OK)
  {
    APP_ErrorHandler();
  }
}

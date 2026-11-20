/**
  ******************************************************************************
  * @file    Bsp_Clock.c
  * @brief   HSI 24MHz 直出，无 PLL
  * @note    PY32F002A 没有 PLL 也没有 LSE，HSI 只有 8/24MHz 两档。
  *          "HSE + PLL → 48MHz" 的模板配置在这颗芯片上既编不过也起不了振。
  ******************************************************************************
  */
#include "Bsp_Clock.h"

void Bsp_Clock_Init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSIDiv              = RCC_HSI_DIV1;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_24MHz;
    osc.HSEState            = RCC_HSE_OFF;
    osc.LSIState            = RCC_LSI_OFF;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        APP_ErrorHandler();
    }

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK)
    {
        APP_ErrorHandler();
    }
}

/**
  ******************************************************************************
  * @file    Bsp_Adc.c
  * @brief   ADC1 轮询采样
  ******************************************************************************
  */
#include "Bsp_Adc.h"
#include "Bsp_PinMap/Bsp_PinMap.h"

static ADC_HandleTypeDef s_hadc;

void Bsp_Adc_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    ADC_ChannelConfTypeDef ch = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC_CLK_ENABLE();

    /* PA5 模拟输入 */
    gpio.Pin  = PIN_BN_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(PIN_BN_PORT, &gpio);

    s_hadc.Instance                   = ADC1;
    s_hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;   /* 24MHz/4 = 6MHz */
    s_hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    s_hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    s_hadc.Init.ScanConvMode          = DISABLE;
    s_hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    s_hadc.Init.LowPowerAutoWait      = DISABLE;
    s_hadc.Init.ContinuousConvMode    = DISABLE;
    s_hadc.Init.DiscontinuousConvMode = DISABLE;
    s_hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    s_hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    s_hadc.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    s_hadc.Init.SamplingTimeCommon    = ADC_SAMPLETIME_71CYCLES_5;
    if (HAL_ADC_Init(&s_hadc) != HAL_OK)
    {
        APP_ErrorHandler();
    }

    if (HAL_ADC_Calibration_Start(&s_hadc) != HAL_OK)
    {
        APP_ErrorHandler();
    }

    ch.Channel      = PIN_BN_ADC_CHANNEL;
    ch.Rank         = ADC_RANK_CHANNEL_NUMBER;
    ch.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    if (HAL_ADC_ConfigChannel(&s_hadc, &ch) != HAL_OK)
    {
        APP_ErrorHandler();
    }
}

uint16_t Bsp_Adc_Read(uint32_t channel)
{
    ADC_ChannelConfTypeDef ch = {0};

    ch.Channel      = channel;
    ch.Rank         = ADC_RANK_CHANNEL_NUMBER;
    ch.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
    if (HAL_ADC_ConfigChannel(&s_hadc, &ch) != HAL_OK)
    {
        return BSP_ADC_INVALID;
    }

    if (HAL_ADC_Start(&s_hadc) != HAL_OK)
    {
        return BSP_ADC_INVALID;
    }
    if (HAL_ADC_PollForConversion(&s_hadc, 10u) != HAL_OK)
    {
        HAL_ADC_Stop(&s_hadc);
        return BSP_ADC_INVALID;
    }
    uint32_t v = HAL_ADC_GetValue(&s_hadc);
    HAL_ADC_Stop(&s_hadc);
    return (uint16_t)v;
}

uint16_t Bsp_Adc_ReadAvg(uint32_t channel, uint8_t n)
{
    uint32_t sum = 0;
    uint8_t  ok = 0;
    uint16_t v;

    if (n == 0u)
    {
        n = 1u;
    }
    for (uint8_t i = 0; i < n; i++)
    {
        /* 通道切换后第一次采样丢弃，等采样保持稳定 */
        (void)Bsp_Adc_Read(channel);

        v = Bsp_Adc_Read(channel);
        if (v == BSP_ADC_INVALID)
        {
            continue;          /* 失败样本不参与平均，否则会把均值往下拉 */
        }
        sum += v;
        ok++;
    }
    return (ok == 0u) ? BSP_ADC_INVALID : (uint16_t)(sum / ok);
}

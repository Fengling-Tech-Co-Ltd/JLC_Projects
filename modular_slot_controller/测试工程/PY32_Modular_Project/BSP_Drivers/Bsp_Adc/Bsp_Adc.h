/**
  ******************************************************************************
  * @file    Bsp_Adc.h
  * @brief   ADC1 单通道轮询采样（板号 PA5 / ADC_IN5）
  *
  * 必须用 3.3V 做参考（VREF=VDD），内部参考会破坏 R-2R 的比例测量。
  ******************************************************************************
  */
#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "main.h"

/* 读取失败时返回这个值（12 位 ADC 不可能出现），平均值计算会跳过它 */
#define BSP_ADC_INVALID   0xFFFFu

void     Bsp_Adc_Init(void);
uint16_t Bsp_Adc_Read(uint32_t channel);          /* 单次，12 位右对齐 */
uint16_t Bsp_Adc_ReadAvg(uint32_t channel, uint8_t n);

#endif /* __BSP_ADC_H */

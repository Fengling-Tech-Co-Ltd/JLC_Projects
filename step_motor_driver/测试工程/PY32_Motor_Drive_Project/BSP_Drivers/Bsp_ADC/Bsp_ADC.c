#include "Bsp_ADC/Bsp_ADC.h"


ADC_HandleTypeDef	ADCHandle;

/**
 * @brief       ADC 初始化配置
 * @param       htim:定时器句柄
 * @retval      无
 */
void ADC_Init(void)
{
	ADCHandle.Instance = ADC1;
	ADCHandle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;        
	ADCHandle.Init.Resolution            = ADC_RESOLUTION_12B;               
	ADCHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;              
	ADCHandle.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;       
	ADCHandle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;              
	ADCHandle.Init.LowPowerAutoWait      = DISABLE;                          
	ADCHandle.Init.ContinuousConvMode    = DISABLE;                          
	ADCHandle.Init.DiscontinuousConvMode = DISABLE;                          
	ADCHandle.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T1_CC4;     
	ADCHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;  
	ADCHandle.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN; 
	/* 配置所有通道采样时间 */
	ADCHandle.Init.SamplingTimeCommon    = ADC_SAMPLETIME_3CYCLES_5;        
	HAL_ADC_Init(&ADCHandle);                                 
	
	
	ADC_ChannelConfTypeDef        sConfig;
	/* 开启通道 */
	sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER; 
	/* 通道4加入规则序列 */
  sConfig.Channel      = ADC_CHANNEL_4; 
	HAL_ADC_ConfigChannel(&ADCHandle, &sConfig);
	
	/* ADC 校准 */
	HAL_ADCEx_Calibration_Start(&ADCHandle);
	
	/* 启动ADC 开启ADC中断 */
	HAL_ADC_Start_IT(&ADCHandle); 
}
/**
 * @brief       HAL_ADC_MspInit
 * @param       htim:定时器句柄
 * @retval      无
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  GPIO_InitTypeDef          GPIO_InitStruct;
  __HAL_RCC_GPIOA_CLK_ENABLE();             
	__HAL_RCC_ADC_CLK_ENABLE();                                                /* Enable ADC clock */

  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(ADC_COMP_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC_COMP_IRQn);        
}



/**
  * @brief  ADC Conversion Callback
  * @param  None
  * @retval None
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  
  printf("ADC Channel4: %d\r\n", hadc->Instance->DR);
}


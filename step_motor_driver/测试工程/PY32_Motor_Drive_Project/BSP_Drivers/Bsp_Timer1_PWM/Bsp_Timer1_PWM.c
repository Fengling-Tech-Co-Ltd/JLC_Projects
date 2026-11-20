#include "Bsp_Timer1_PWM/Bsp_Timer1_PWM.h"


/* 定时器x句柄 */
TIM_HandleTypeDef Advanced_Timer1_PWM_Handle;        



/**
 * @brief       高级定时器TIMX 通道Y PWM输出 初始化函数（使用PWM模式1）
 * @note
 *              高级定时器的时钟来自APB1, 而PCLK1 = 24Mhz, 我们设置PPRE1不分频, 因此
 *              高级定时器时钟 = 24Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       Prescaler: 	时钟预分频数
 * @param       Period: 		自动重装值
 * @retval      无
 */
void Advanced_Timer1_PWM_Init(uint16_t Prescaler, uint16_t Period)
{
	TIM_OC_InitTypeDef Advanced_Timer1_OC_PWM_Config;    /* 定时器PWM输出配置 */
	
	
  Advanced_Timer1_PWM_Handle.Instance = TIM1;																
	/* 定时器x */
	Advanced_Timer1_PWM_Handle.Init.Period = Period;                                    /* 自动重装载值 */	
  Advanced_Timer1_PWM_Handle.Init.Prescaler = Prescaler;                              /* 定时器分频 */
  Advanced_Timer1_PWM_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* 向上计数模式 */
  Advanced_Timer1_PWM_Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* 时钟分频因子 */
  Advanced_Timer1_PWM_Handle.Init.RepetitionCounter = 0;                              /* 不重复计数*/
  /* 自动重装载寄存器没有缓冲 */
  Advanced_Timer1_PWM_Handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; 
	HAL_TIM_PWM_Init(&Advanced_Timer1_PWM_Handle);                                      /* 初始化PWM */
	/* PWM中断回调函数重定义 */
	Advanced_Timer1_PWM_Handle.PWM_PulseFinishedCallback = HAL_Advanced_Timer1_PWM_PulseFinishedCallback;
	
	/* 配置TIM OC CH4 为ADC触发源 */
	TIM_MasterConfigTypeDef       sMasterConfig;
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC4REF;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	HAL_TIMEx_MasterConfigSynchronization(&Advanced_Timer1_PWM_Handle, &sMasterConfig);
	
  Advanced_Timer1_OC_PWM_Config.OCMode = TIM_OCMODE_PWM1;                             /* 模式选择PWM1 */
  Advanced_Timer1_OC_PWM_Config.Pulse = ((Period+1)/2);                           /* ( Period + 1 ) / 2设置比较值,此值用来确定占空比,默认50% */
  Advanced_Timer1_OC_PWM_Config.OCPolarity = TIM_OCPOLARITY_HIGH;                     /* 输出比较极性为高 */
	Advanced_Timer1_OC_PWM_Config.OCFastMode = TIM_OCFAST_DISABLE;
	Advanced_Timer1_OC_PWM_Config.OCIdleState = TIM_OCIDLESTATE_SET;
  Advanced_Timer1_OC_PWM_Config.OCNIdleState = TIM_OCNIDLESTATE_RESET;                /*空闲状态OC1N输出低电平 */             
  Advanced_Timer1_OC_PWM_Config.OCIdleState  = TIM_OCIDLESTATE_RESET; 								/*空闲状态OC1输出低电平*/
		
	/* 配置TIMx通道2 */
	HAL_TIM_PWM_ConfigChannel(&Advanced_Timer1_PWM_Handle, &Advanced_Timer1_OC_PWM_Config, TIM_CHANNEL_2); 
	/* 使能定时器x和定时器x更新中断 */
	HAL_TIM_PWM_Start_IT(&Advanced_Timer1_PWM_Handle, TIM_CHANNEL_2);
	
	/* 配置TIMx通道3 */
	HAL_TIM_PWM_ConfigChannel(&Advanced_Timer1_PWM_Handle, &Advanced_Timer1_OC_PWM_Config, TIM_CHANNEL_3); 
	/* 使能定时器x和定时器x更新中断 */
	HAL_TIM_PWM_Start_IT(&Advanced_Timer1_PWM_Handle, TIM_CHANNEL_3);
	
	/* 配置TIMx通道4 */
	HAL_TIM_PWM_ConfigChannel(&Advanced_Timer1_PWM_Handle, &Advanced_Timer1_OC_PWM_Config, TIM_CHANNEL_4); 
	/* 使能定时器x和定时器x更新中断 */
	HAL_TIM_PWM_Start_IT(&Advanced_Timer1_PWM_Handle, TIM_CHANNEL_4);

}



/**
 * @brief       定时器底层驱动，开启时钟，设置中断优先级
                此函数会被HAL_TIM_PWM_Init函数调用
 * @param       htim:定时器句柄
 * @retval      无
 */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1)
	{
		GPIO_InitTypeDef	Advanced_TIM1_PWM_Channal_GPIO_Config;
		
		/* 使能TIM时钟 */
		__HAL_RCC_TIM1_CLK_ENABLE();	
		/* PWM Output Channal GPIO时钟使能 */	
		Advanced_TIMx_PWM_CH2_GPIO_CLK_ENABLE();
		Advanced_TIMx_PWM_CH3_GPIO_CLK_ENABLE();
		Advanced_TIMx_PWM_CH4_GPIO_CLK_ENABLE();
		
		
		Advanced_TIM1_PWM_Channal_GPIO_Config.Mode = GPIO_MODE_AF_PP;
		Advanced_TIM1_PWM_Channal_GPIO_Config.Speed = GPIO_SPEED_FREQ_HIGH;
		Advanced_TIM1_PWM_Channal_GPIO_Config.Pull = GPIO_NOPULL;
		
		/* PWM Output Channal 2 */
		Advanced_TIM1_PWM_Channal_GPIO_Config.Pin = Advanced_TIMx_PWM_CH2_GPIO_PIN;
		Advanced_TIM1_PWM_Channal_GPIO_Config.Alternate = Advanced_TIMx_PWM_CH2_GPIO_AF;
		HAL_GPIO_Init(Advanced_TIMx_PWM_CH2_GPIO_PORT, &Advanced_TIM1_PWM_Channal_GPIO_Config);

		/* PWM Output Channal 3 */
		Advanced_TIM1_PWM_Channal_GPIO_Config.Pin = Advanced_TIMx_PWM_CH3_GPIO_PIN;
		Advanced_TIM1_PWM_Channal_GPIO_Config.Alternate = Advanced_TIMx_PWM_CH3_GPIO_AF;
		HAL_GPIO_Init(Advanced_TIMx_PWM_CH3_GPIO_PORT, &Advanced_TIM1_PWM_Channal_GPIO_Config);

		/* PWM Output Channal 4 */
		Advanced_TIM1_PWM_Channal_GPIO_Config.Pin = Advanced_TIMx_PWM_CH4_GPIO_PIN;
		Advanced_TIM1_PWM_Channal_GPIO_Config.Alternate = Advanced_TIMx_PWM_CH4_GPIO_AF;
		HAL_GPIO_Init(Advanced_TIMx_PWM_CH4_GPIO_PORT, &Advanced_TIM1_PWM_Channal_GPIO_Config);	
		
		
		HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 1);              /* 抢占优先级3，子优先级0 */
		HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);                      /* 使能TIM1_BRK_UP_TRG_COM_IRQn中断通道 */
		
	}	
}




/**
 * @brief       定时器更新中断回调函数
 * @param       htim:定时器句柄
 * @retval      无
 */
void HAL_Advanced_Timer1_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
		if (htim->Instance == TIM1)
    {
				static uint16_t Duty = 0;
				static uint8_t  Duty_Flag = 0;
					
				if(Duty_Flag)
				{
						Duty += 24;
						if(Duty >= 2400)
						{
								Duty = 2400;
								Duty_Flag = 0;
						}
				}else{
						Duty -= 24;
						if(Duty <= 0)
						{
								Duty = 0;
								Duty_Flag = 1;
						}	
				}
				SetGeneralTIMxPWMOutputChannalDuty(4, Duty, Duty, Duty, Duty);
    }
}


/**
 * @brief       设置定时器PWM输出占空比函数
 * @param       可变参数
 * @retval      无
 */
void SetGeneralTIMxPWMOutputChannalDuty(int arg_num, ...)
{
	int32_t *arg_addr = &arg_num;
	switch(arg_num)
	{
		case 4:	Advanced_Timer1_PWM_Handle.Instance->CCR4 = *(arg_addr+4);
		case 3:	Advanced_Timer1_PWM_Handle.Instance->CCR3 = *(arg_addr+3);
		case 2:	Advanced_Timer1_PWM_Handle.Instance->CCR2 = *(arg_addr+2);
		case 1:	Advanced_Timer1_PWM_Handle.Instance->CCR1 = *(arg_addr+1);
		default:break;
	}
}



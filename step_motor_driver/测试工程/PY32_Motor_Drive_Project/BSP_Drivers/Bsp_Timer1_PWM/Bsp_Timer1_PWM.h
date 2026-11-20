#ifndef __BSP_TIMER1_PWM_H
#define __BSP_TIMER1_PWM_H

#include "py32f0xx_hal.h"

                                      
					
					

#define		Advanced_TIMx_PWM_CH2_GPIO_PORT         	GPIOB
#define		Advanced_TIMx_PWM_CH2_GPIO_PIN          	GPIO_PIN_0
#define 	Advanced_TIMx_PWM_CH2_GPIO_AF    					GPIO_AF2_TIM1			/* 端口复用到TIM1 */  
#define		Advanced_TIMx_PWM_CH2_GPIO_CLK_ENABLE 		__HAL_RCC_GPIOB_CLK_ENABLE
					
#define		Advanced_TIMx_PWM_CH3_GPIO_PORT         	GPIOB
#define		Advanced_TIMx_PWM_CH3_GPIO_PIN          	GPIO_PIN_2
#define 	Advanced_TIMx_PWM_CH3_GPIO_AF    					GPIO_AF3_TIM1			/* 端口复用到TIM1 */  
#define		Advanced_TIMx_PWM_CH3_GPIO_CLK_ENABLE 		__HAL_RCC_GPIOB_CLK_ENABLE
					
#define		Advanced_TIMx_PWM_CH4_GPIO_PORT         	GPIOB
#define		Advanced_TIMx_PWM_CH4_GPIO_PIN          	GPIO_PIN_1
#define 	Advanced_TIMx_PWM_CH4_GPIO_AF    					GPIO_AF3_TIM1			/* 端口复用到TIM1 */  
#define		Advanced_TIMx_PWM_CH4_GPIO_CLK_ENABLE 		__HAL_RCC_GPIOB_CLK_ENABLE



void SetGeneralTIMxPWMOutputChannalDuty(int arg_num, ...);
void Advanced_Timer1_PWM_Init(uint16_t Prescaler, uint16_t Period);
void HAL_Advanced_Timer1_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);

#endif /* __BSP_TIMER1_PWM_H */


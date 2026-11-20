/**
  ******************************************************************************
  * @file    bsp_step.h
  * @brief   STEP 脉冲输出：轴1 = TIM16_CH1 (PB8)，轴2 = TIM1_CH1 (PA8)
  * @note    两轴各用一个定时器，转速互不影响。
  *          f_STEP = 48MHz / ((PSC+1) x (ARR+1))，50% 占空比。
  *          DRV8825 要求 STEP 高低电平各 >= 1.9us，即 f_STEP <= 250kHz。
  ******************************************************************************
  */
#ifndef __BSP_STEP_H
#define __BSP_STEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "py32f0xx_hal.h"
#include "bsp_motor_io.h"

#define STEP_TIM_CLK_HZ     48000000U
#define STEP_FREQ_MIN_HZ    10U
#define STEP_FREQ_MAX_HZ    250000U
#define STEP_ARR_MAX        65535U

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim16;

void step_init(void);
int  step_start(motor_axis_t axis, uint32_t freq_hz);
void step_stop(motor_axis_t axis);
uint32_t step_get_freq(motor_axis_t axis);

/* 每个 STEP 周期结束触发一次，app_motion 里实现计步与停机 */
void step_period_hook(motor_axis_t axis);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_STEP_H */

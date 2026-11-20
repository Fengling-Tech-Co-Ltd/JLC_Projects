/**
  ******************************************************************************
  * @file    bsp_vref.h
  * @brief   相电流设定：TIM3 双通道 20kHz PWM 经 RC 滤波得到 VREF
  * @note    I_CHOP = VREF / (5 x Rcs)，Rcs = 200mR，所以 VREF(1V) 对应 1A。
  *          VREF = 3.3V x CCR / 2400，CCR = I_mA x 2400 / 3300。
  ******************************************************************************
  */
#ifndef __BSP_VREF_H
#define __BSP_VREF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "py32f0xx_hal.h"
#include "bsp_motor_io.h"

#define VREF_SYSCLK_HZ            48000000U
#define VREF_PWM_FREQ_HZ          20000U
#define VREF_ARR                  2399U   /* 48MHz / 2400 = 20kHz */
#define VREF_PSC                  0U
#define VREF_VCC_MV               3300U

#define VREF_CURRENT_MIN_MA       1000U
#define VREF_CURRENT_MAX_MA       3500U
#define VREF_DEFAULT_CURRENT_MA   1200U   /* 1.5A 时结温约 121C，默认先按 1.2A */

/* RC 滤波 10k + 1uF，截止 15.9Hz，建立时间约 30ms */
#define VREF_SETTLE_MS            30U

extern TIM_HandleTypeDef htim3;

void vref_init(void);
void vref_set_current_ma(motor_axis_t axis, uint16_t ma);
uint16_t vref_get_current_ma(motor_axis_t axis);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_VREF_H */

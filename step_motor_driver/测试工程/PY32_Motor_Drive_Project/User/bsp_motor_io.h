/**
  ******************************************************************************
  * @file    bsp_motor_io.h
  * @brief   DRV8825 控制脚：DIR / nENBL / nRESET
  ******************************************************************************
  */
#ifndef __BSP_MOTOR_IO_H
#define __BSP_MOTOR_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "py32f0xx_hal.h"

/* 轴编号，数组下标直接用这个值 */
typedef enum
{
  MOTOR_AXIS_1 = 0,
  MOTOR_AXIS_2 = 1,
  MOTOR_AXIS_COUNT = 2
} motor_axis_t;

/* 上电后要留的 DIR 建立时间：DRV8825 要求 >= 650ns，这里留 5us 余量 */
#define MOTOR_DIR_SETUP_US   5U

void motor_io_init(void);
void motor_set_dir(motor_axis_t axis, uint8_t forward);
void motor_set_enable(motor_axis_t axis, uint8_t enable);
void motor_set_reset(motor_axis_t axis, uint8_t release);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_MOTOR_IO_H */

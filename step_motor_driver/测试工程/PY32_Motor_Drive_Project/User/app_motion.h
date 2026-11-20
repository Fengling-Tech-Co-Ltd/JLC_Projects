/**
  ******************************************************************************
  * @file    app_motion.h
  * @brief   单轴运动控制：按步数走 / 连续转 / 停止
  * @note    初版为定速运动，不做加减速；目标频率由调用者保证不超过电机
  *          在该负载下能起步的转速（全步、200 步/转，RPM = 0.3 x f_STEP）。
  ******************************************************************************
  */
#ifndef __APP_MOTION_H
#define __APP_MOTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "py32f0xx_hal.h"
#include "bsp_motor_io.h"

/* 返回值 */
#define MOTION_OK             0
#define MOTION_ERR_PARAM     -1
#define MOTION_ERR_BUSY      -2
#define MOTION_ERR_FREQ      -3

void    motion_init(void);

/* steps 带符号：正数正转，负数反转；freq_hz 10 ~ 250000 */
int     motion_move(motor_axis_t axis, int32_t steps, uint32_t freq_hz);

/* 连续转，freq_hz 带符号：正数正转，负数反转 */
int     motion_run(motor_axis_t axis, int32_t freq_hz);

void    motion_stop(motor_axis_t axis);
void    motion_stop_all(void);

uint8_t motion_is_running(motor_axis_t axis);
int32_t motion_steps_taken(motor_axis_t axis);

#ifdef __cplusplus
}
#endif

#endif /* __APP_MOTION_H */

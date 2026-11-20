/**
  ******************************************************************************
  * @file    app_motion.c
  * @brief   运动控制层：把 STEP 定时器 + 计步中断包成"走多少步 / 一直转"
  * @note    计步在 STEP 定时器的更新中断里做，中断频率等于 f_STEP，
  *          所以按步数走时建议 f_STEP <= 20kHz；更高频率请用连续转模式。
  ******************************************************************************
  */
#include "app_motion.h"
#include "bsp_step.h"

typedef struct
{
  volatile int32_t  steps_left;    /* 剩余步数，连续转时为 -1 */
  volatile int32_t  steps_taken;   /* 本次运动已发步数 */
  volatile uint8_t  running;
  volatile uint8_t  counting;      /* 1 = 开更新中断计步 */
  uint32_t          freq_hz;
} axis_rt_t;

static axis_rt_t s_axis[MOTOR_AXIS_COUNT];

static TIM_HandleTypeDef *axis_tim(motor_axis_t axis)
{
  return (axis == MOTOR_AXIS_1) ? &htim16 : &htim1;
}

/* 覆盖 bsp_step.c 里的弱定义：每个 STEP 周期进来一次 */
void step_period_hook(motor_axis_t axis)
{
  if ((axis >= MOTOR_AXIS_COUNT) || (s_axis[axis].counting == 0U))
  {
    return;
  }

  s_axis[axis].steps_taken++;

  if (s_axis[axis].steps_left > 0)
  {
    s_axis[axis].steps_left--;
    if (s_axis[axis].steps_left == 0)
    {
      s_axis[axis].running = 0;
      s_axis[axis].counting = 0;
      step_stop(axis);
    }
  }
}

void motion_init(void)
{
  for (uint32_t i = 0; i < MOTOR_AXIS_COUNT; i++)
  {
    s_axis[i].steps_left = 0;
    s_axis[i].steps_taken = 0;
    s_axis[i].running = 0;
    s_axis[i].counting = 0;
    s_axis[i].freq_hz = 0;
  }
}

int motion_move(motor_axis_t axis, int32_t steps, uint32_t freq_hz)
{
  if ((axis >= MOTOR_AXIS_COUNT) || (steps == 0) || (steps == (-2147483647 - 1)))
  {
    return MOTION_ERR_PARAM;
  }
  if (s_axis[axis].running)
  {
    return MOTION_ERR_BUSY;
  }

  motor_set_dir(axis, (steps > 0) ? 1U : 0U);

  s_axis[axis].steps_left = (steps > 0) ? steps : -steps;
  s_axis[axis].steps_taken = 0;
  s_axis[axis].counting = 1;
  s_axis[axis].running = 1;
  s_axis[axis].freq_hz = freq_hz;

  if (step_start(axis, freq_hz) != 0)
  {
    s_axis[axis].running = 0;
    s_axis[axis].counting = 0;
    s_axis[axis].freq_hz = 0;
    return MOTION_ERR_FREQ;
  }

  __HAL_TIM_ENABLE_IT(axis_tim(axis), TIM_IT_UPDATE);
  return MOTION_OK;
}

int motion_run(motor_axis_t axis, int32_t freq_hz)
{
  uint32_t f;

  if (axis >= MOTOR_AXIS_COUNT)
  {
    return MOTION_ERR_PARAM;
  }
  if (s_axis[axis].running)
  {
    return MOTION_ERR_BUSY;
  }

  motor_set_dir(axis, (freq_hz > 0) ? 1U : 0U);
  f = (freq_hz > 0) ? (uint32_t)freq_hz : (uint32_t)(-freq_hz);

  s_axis[axis].steps_left = -1;
  s_axis[axis].steps_taken = 0;
  s_axis[axis].counting = 0;          /* 连续转不数步 */
  s_axis[axis].running = 1;
  s_axis[axis].freq_hz = f;

  if (step_start(axis, f) != 0)
  {
    s_axis[axis].running = 0;
    s_axis[axis].freq_hz = 0;
    return MOTION_ERR_FREQ;
  }
  __HAL_TIM_DISABLE_IT(axis_tim(axis), TIM_IT_UPDATE);
  return MOTION_OK;
}

void motion_stop(motor_axis_t axis)
{
  if (axis >= MOTOR_AXIS_COUNT)
  {
    return;
  }
  s_axis[axis].counting = 0;
  s_axis[axis].running = 0;
  s_axis[axis].freq_hz = 0;
  step_stop(axis);
}

void motion_stop_all(void)
{
  motion_stop(MOTOR_AXIS_1);
  motion_stop(MOTOR_AXIS_2);
}

uint8_t motion_is_running(motor_axis_t axis)
{
  return (axis < MOTOR_AXIS_COUNT) ? s_axis[axis].running : 0;
}

int32_t motion_steps_taken(motor_axis_t axis)
{
  return (axis < MOTOR_AXIS_COUNT) ? s_axis[axis].steps_taken : 0;
}

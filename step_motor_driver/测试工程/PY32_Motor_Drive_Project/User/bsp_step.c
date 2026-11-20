/**
  ******************************************************************************
  * @file    bsp_step.c
  * @brief   STEP 脉冲输出
  * @note    1. 轴1: TIM16_CH1 -> PB8，复用 AF2
  *          2. 轴2: TIM1_CH1  -> PA8，复用 AF2
  *          3. 低频时 ARR 会超过 65535，按目标频率自动选最小可用的 PSC；
  *             f_STEP 低于约 732Hz 才会用到预分频。
  *          4. 只有"按步数走"时才开更新中断计步，连续转时关掉，避免高频中断。
  ******************************************************************************
  */
#include "bsp_step.h"
#include "main.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim16;

static uint32_t s_freq[MOTOR_AXIS_COUNT];

static TIM_HandleTypeDef *axis_handle(motor_axis_t axis)
{
  return (axis == MOTOR_AXIS_1) ? &htim16 : &htim1;
}

static uint32_t axis_channel(motor_axis_t axis)
{
  (void)axis;
  /* 两轴都只用 CH1 */
  return TIM_CHANNEL_1;
}

static void calc_psc_arr(uint32_t freq_hz, uint32_t *psc, uint32_t *arr)
{
  uint32_t p = 0;
  uint32_t ticks = STEP_TIM_CLK_HZ / freq_hz;

  while ((ticks > (STEP_ARR_MAX + 1U)) && (p < STEP_ARR_MAX))
  {
    p++;
    ticks = STEP_TIM_CLK_HZ / ((p + 1U) * freq_hz);
  }
  if (ticks < 2U)
  {
    ticks = 2U;   /* 占空比至少要有高低两拍 */
  }
  *psc = p;
  *arr = ticks - 1U;
}

static void timer_base_init(TIM_HandleTypeDef *htim, TIM_TypeDef *inst)
{
  htim->Instance = inst;
  htim->Init.Prescaler = 0;
  htim->Init.CounterMode = TIM_COUNTERMODE_UP;
  htim->Init.Period = 999;
  htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim->Init.RepetitionCounter = 0;
  /* 关掉 ARR 预装载：改频率时新周期立即生效，不会先跑一拍旧 ARR */
  htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_PWM_Init(htim) != HAL_OK)
  {
    APP_ErrorHandler();
  }
}

static void timer_channel_config(TIM_HandleTypeDef *htim)
{
  TIM_OC_InitTypeDef oc = {0};

  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 0;                       /* 未启动前保持低电平 */
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  oc.OCIdleState = TIM_OCIDLESTATE_RESET;
  oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(htim, &oc, TIM_CHANNEL_1) != HAL_OK)
  {
    APP_ErrorHandler();
  }
}

void step_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_TIM16_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* PA8 -> TIM1_CH1 (AF2) */
  gpio.Pin = GPIO_PIN_8;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF2_TIM1;
  HAL_GPIO_Init(GPIOA, &gpio);

  /* PB8 -> TIM16_CH1 (AF2) */
  gpio.Pin = GPIO_PIN_8;
  gpio.Alternate = GPIO_AF2_TIM16;
  HAL_GPIO_Init(GPIOB, &gpio);

  timer_base_init(&htim16, TIM16);
  timer_base_init(&htim1, TIM1);
  timer_channel_config(&htim16);
  timer_channel_config(&htim1);

  /* 计步中断优先级低于串口，避免高频 STEP 把控制台挤掉 */
  HAL_NVIC_SetPriority(TIM16_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM16_IRQn);
  HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

  s_freq[MOTOR_AXIS_1] = 0;
  s_freq[MOTOR_AXIS_2] = 0;
}

int step_start(motor_axis_t axis, uint32_t freq_hz)
{
  TIM_HandleTypeDef *htim;
  uint32_t psc, arr;

  if (axis >= MOTOR_AXIS_COUNT)
  {
    return -1;
  }
  if ((freq_hz < STEP_FREQ_MIN_HZ) || (freq_hz > STEP_FREQ_MAX_HZ))
  {
    return -2;
  }

  htim = axis_handle(axis);
  calc_psc_arr(freq_hz, &psc, &arr);

  __HAL_TIM_DISABLE(htim);
  __HAL_TIM_SET_PRESCALER(htim, psc);
  __HAL_TIM_SET_AUTORELOAD(htim, arr);
  __HAL_TIM_SET_COMPARE(htim, axis_channel(axis), (arr + 1U) / 2U);

  s_freq[axis] = freq_hz;
  HAL_TIM_PWM_Start(htim, axis_channel(axis));
  return 0;
}

void step_stop(motor_axis_t axis)
{
  if (axis >= MOTOR_AXIS_COUNT)
  {
    return;
  }
  HAL_TIM_PWM_Stop(axis_handle(axis), axis_channel(axis));
  __HAL_TIM_DISABLE_IT(axis_handle(axis), TIM_IT_UPDATE);
  s_freq[axis] = 0;
}

uint32_t step_get_freq(motor_axis_t axis)
{
  return (axis < MOTOR_AXIS_COUNT) ? s_freq[axis] : 0;
}

/* HAL 的周期中断回调：分派到具体的轴 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM16)
  {
    step_period_hook(MOTOR_AXIS_1);
  }
  else if (htim->Instance == TIM1)
  {
    step_period_hook(MOTOR_AXIS_2);
  }
}

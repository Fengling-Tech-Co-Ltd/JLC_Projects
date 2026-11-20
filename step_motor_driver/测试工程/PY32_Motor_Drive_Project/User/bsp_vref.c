/**
  ******************************************************************************
  * @file    bsp_vref.c
  * @brief   TIM3 双通道 VREF PWM
  * @note    轴1 -> PB4 / TIM3_CH1，轴2 -> PB5 / TIM3_CH2，两路复用 AF1。
  *          通道与轴的对应关系只在 s_channel[] 里体现，改板只改这张表。
  ******************************************************************************
  */
#include "bsp_vref.h"
#include "main.h"

TIM_HandleTypeDef htim3;

/* 轴 -> TIM3 通道 */
static const uint32_t s_channel[MOTOR_AXIS_COUNT] =
{
  TIM_CHANNEL_1,   /* 轴1: PB4 */
  TIM_CHANNEL_2,   /* 轴2: PB5 */
};

static uint16_t s_current_ma[MOTOR_AXIS_COUNT];

static uint32_t current_to_ccr(uint16_t ma)
{
  /* CCR = ma x 2400 / 3300，四舍五入 */
  return (((uint32_t)ma * (VREF_ARR + 1U)) + (VREF_VCC_MV / 2U)) / VREF_VCC_MV;
}

static uint16_t clamp_current(uint16_t ma)
{
  if (ma < VREF_CURRENT_MIN_MA)
  {
    return VREF_CURRENT_MIN_MA;
  }
  if (ma > VREF_CURRENT_MAX_MA)
  {
    return VREF_CURRENT_MAX_MA;
  }
  return ma;
}

void vref_init(void)
{
  GPIO_InitTypeDef gpio = {0};
  TIM_OC_InitTypeDef oc = {0};

  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  gpio.Pin = GPIO_PIN_4 | GPIO_PIN_5;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = GPIO_AF1_TIM3;
  HAL_GPIO_Init(GPIOB, &gpio);

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = VREF_PSC;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = VREF_ARR;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.RepetitionCounter = 0;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  /* 上电先输出 0% 占空比，VREF = 0，电机不会出力 */
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 0;
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  oc.OCIdleState = TIM_OCIDLESTATE_RESET;
  oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_1) != HAL_OK)
  {
    APP_ErrorHandler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_2) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  s_current_ma[MOTOR_AXIS_1] = 0;
  s_current_ma[MOTOR_AXIS_2] = 0;
}

void vref_set_current_ma(motor_axis_t axis, uint16_t ma)
{
  uint16_t cur;

  if (axis >= MOTOR_AXIS_COUNT)
  {
    return;
  }
  cur = clamp_current(ma);
  s_current_ma[axis] = cur;
  __HAL_TIM_SET_COMPARE(&htim3, s_channel[axis], current_to_ccr(cur));
}

uint16_t vref_get_current_ma(motor_axis_t axis)
{
  if (axis >= MOTOR_AXIS_COUNT)
  {
    return 0;
  }
  return s_current_ma[axis];
}

/**
  ******************************************************************************
  * @file    bsp_motor_io.c
  * @brief   DRV8825 控制脚
  * @note    引脚来自功能板网表：
  *            轴1 DIR=PA0  nENBL=PF3  nRESET=PB3
  *            轴2 DIR=PA5  nENBL=PA4  nRESET=PA6
  *          三个脚都是普通 GPIO，不占定时器。
  ******************************************************************************
  */
#include "bsp_motor_io.h"

typedef struct
{
  GPIO_TypeDef *dir_port;
  uint16_t      dir_pin;
  GPIO_TypeDef *en_port;
  uint16_t      en_pin;
  GPIO_TypeDef *rst_port;
  uint16_t      rst_pin;
} motor_io_map_t;

static const motor_io_map_t s_io[MOTOR_AXIS_COUNT] =
{
  {GPIOA, GPIO_PIN_0, GPIOF, GPIO_PIN_3, GPIOB, GPIO_PIN_3},  /* 轴1 */
  {GPIOA, GPIO_PIN_5, GPIOA, GPIO_PIN_4, GPIOA, GPIO_PIN_6},  /* 轴2 */
};

/* 粗略的微秒延时：48MHz 下空转，只用于 DIR 建立时间这类宽松场合 */
static void motor_delay_us(uint32_t us)
{
  volatile uint32_t n = us * 12U;
  while (n--)
  {
    __NOP();
  }
}

void motor_io_init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;

  gpio.Pin = GPIO_PIN_0; HAL_GPIO_Init(GPIOA, &gpio);  /* DIR_1  */
  gpio.Pin = GPIO_PIN_5; HAL_GPIO_Init(GPIOA, &gpio);  /* DIR_2  */
  gpio.Pin = GPIO_PIN_4; HAL_GPIO_Init(GPIOA, &gpio);  /* NENBL_2 */
  gpio.Pin = GPIO_PIN_6; HAL_GPIO_Init(GPIOA, &gpio);  /* RESET_2 */
  gpio.Pin = GPIO_PIN_3; HAL_GPIO_Init(GPIOB, &gpio);  /* RESET_1 */
  gpio.Pin = GPIO_PIN_3; HAL_GPIO_Init(GPIOF, &gpio);  /* NENBL_1 */

  /* 安全初值：两轴禁用、nRESET 压住、DIR 默认正转 */
  motor_set_dir(MOTOR_AXIS_1, 1);
  motor_set_dir(MOTOR_AXIS_2, 1);
  motor_set_enable(MOTOR_AXIS_1, 0);
  motor_set_enable(MOTOR_AXIS_2, 0);
  motor_set_reset(MOTOR_AXIS_1, 0);
  motor_set_reset(MOTOR_AXIS_2, 0);
}

void motor_set_dir(motor_axis_t axis, uint8_t forward)
{
  if (axis >= MOTOR_AXIS_COUNT)
  {
    return;
  }
  HAL_GPIO_WritePin(s_io[axis].dir_port, s_io[axis].dir_pin,
                    forward ? GPIO_PIN_SET : GPIO_PIN_RESET);
  /* DIR 必须在 STEP 上升沿前稳定，DRV8825 要求 >= 650ns */
  motor_delay_us(MOTOR_DIR_SETUP_US);
}

void motor_set_enable(motor_axis_t axis, uint8_t enable)
{
  if (axis >= MOTOR_AXIS_COUNT)
  {
    return;
  }
  /* nENBL 低有效：enable=1 输出低电平 */
  HAL_GPIO_WritePin(s_io[axis].en_port, s_io[axis].en_pin,
                    enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void motor_set_reset(motor_axis_t axis, uint8_t release)
{
  if (axis >= MOTOR_AXIS_COUNT)
  {
    return;
  }
  /* nRESET 高正常：release=1 输出高电平 */
  HAL_GPIO_WritePin(s_io[axis].rst_port, s_io[axis].rst_pin,
                    release ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

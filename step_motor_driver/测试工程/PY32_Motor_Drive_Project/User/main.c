/**
  ******************************************************************************
  * @file    main.c
  * @brief   双轴步进电机驱动（PY32F030K28T6 + DRV8825 x2）
  * @note    上电顺序（README 点名的坑，不能调换）：
  *            1. nENBL 拉高（禁用）、nRESET 拉低（压住驱动）
  *            2. 设好两轴 VREF
  *            3. 等 30ms 让 VREF 的 RC 滤波建立
  *            4. 放开 nRESET，等 >=10us
  *            5. nENBL 拉低使能
  *            6. 之后再发 STEP
  *          顺序错了电机会在 VREF 未建立时猛冲。
  ******************************************************************************
  */
#include "main.h"
#include "bsp_clock.h"
#include "bsp_motor_io.h"
#include "bsp_vref.h"
#include "bsp_step.h"
#include "app_motion.h"
#include "app_console.h"

/* 临时引脚自检：置 1 时 PF1 当普通 GPIO 每秒翻转，不初始化电机与串口，
   用于万用表量 J1 pin2 到底是 MCU/走线问题还是探针侧问题。测完改回 0。 */
#define PF1_TOGGLE_TEST   0

/* 电机驱动自检：串口暂时不通，开机后自动跑一段固定动作，用来确认两轴都能动。
   动作：轴1 正转一圈 -> 停 0.8s -> 反转一圈 -> 停 0.8s -> 轴2 同样一轮 -> 循环。
   全步 200 步/转，400Hz 约 120rpm，属于能可靠起步的低速。
   要改速度/步数就改下面三个宏。 */
#define MOTOR_DEMO_ENABLE 1
#define DEMO_STEP_HZ      400U
#define DEMO_STEPS        200            /* 200 步 = 一圈（全步） */
#define DEMO_WAIT_MS      800U

static void APP_MotorPowerUp(void);
static void APP_MotorDemo(void);

/*  GPIO 未初始化前不碰电机脚：时钟没开时访问外设可能出问题 */
static uint8_t s_io_ready = 0;

int main(void)
{
  /* 初始化所有外设，Flash接口，SysTick */
  HAL_Init();
  /* 系统时钟 48MHz：HSI 24MHz x PLL2 */
  bsp_clock_init();

#if PF1_TOGGLE_TEST
  {
    GPIO_InitTypeDef g = {0};

    /* PF1 当普通输出，1Hz 方波（0.5s 高 / 0.5s 低），电机与串口都不初始化 */
    __HAL_RCC_GPIOF_CLK_ENABLE();
    g.Pin = GPIO_PIN_1;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &g);

    while (1)
    {
      HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_1);
      HAL_Delay(500);
    }
  }
#endif

  /* 外设初始化：先 GPIO 定住安全电平，再配定时器 */
  motor_io_init();
  s_io_ready = 1;
  vref_init();
  step_init();
  motion_init();
  console_init();

  /* VREF 建立完成后才使能驱动 */
  APP_MotorPowerUp();

  console_write("\r\nPY32 stepper ready. 'H' for help.\r\n");

#if MOTOR_DEMO_ENABLE
  APP_MotorDemo();
#endif

  while (1)
  {
    console_poll();
  }
}

/**
  * @brief  等某一轴停下来，带超时保护，超时说明计步中断没走通
  * @param  axis 轴号
  * @retval 0 = 正常停止，-1 = 超时
  */
static int APP_WaitAxisStop(motor_axis_t axis)
{
  uint32_t ms = 0;

  while (motion_is_running(axis) != 0U)
  {
    console_poll();
    HAL_Delay(1);
    if (++ms > 5000U)
    {
      motion_stop(axis);
      return -1;
    }
  }
  return 0;
}

/**
  * @brief  电机自检动作：两轴各正反转一圈，循环
  * @param  无
  * @retval 无
  */
static void APP_MotorDemo(void)
{
  HAL_Delay(1000);

  while (1)
  {
    /* 轴1 正转一圈 */
    motion_move(MOTOR_AXIS_1, DEMO_STEPS, DEMO_STEP_HZ);
    APP_WaitAxisStop(MOTOR_AXIS_1);
    HAL_Delay(DEMO_WAIT_MS);

    /* 轴1 反转一圈 */
    motion_move(MOTOR_AXIS_1, -DEMO_STEPS, DEMO_STEP_HZ);
    APP_WaitAxisStop(MOTOR_AXIS_1);
    HAL_Delay(DEMO_WAIT_MS);

    /* 轴2 正转一圈 */
    motion_move(MOTOR_AXIS_2, DEMO_STEPS, DEMO_STEP_HZ);
    APP_WaitAxisStop(MOTOR_AXIS_2);
    HAL_Delay(DEMO_WAIT_MS);

    /* 轴2 反转一圈 */
    motion_move(MOTOR_AXIS_2, -DEMO_STEPS, DEMO_STEP_HZ);
    APP_WaitAxisStop(MOTOR_AXIS_2);
    HAL_Delay(DEMO_WAIT_MS);
  }
}

/**
  * @brief  两轴上电使能顺序
  * @param  无
  * @retval 无
  */
static void APP_MotorPowerUp(void)
{
  uint32_t axis;

  for (axis = 0; axis < MOTOR_AXIS_COUNT; axis++)
  {
    motor_set_enable((motor_axis_t)axis, 0);   /* 禁用 */
    motor_set_reset((motor_axis_t)axis, 0);    /* nRESET 低，驱动处于复位 */
    vref_set_current_ma((motor_axis_t)axis, VREF_DEFAULT_CURRENT_MA);
  }

  /* VREF 的 RC 为 10k/1uF，截止 15.9Hz，约 30ms 建立 */
  HAL_Delay(VREF_SETTLE_MS);

  for (axis = 0; axis < MOTOR_AXIS_COUNT; axis++)
  {
    motor_set_reset((motor_axis_t)axis, 1);    /* 放开 nRESET */
  }
  HAL_Delay(1);

  for (axis = 0; axis < MOTOR_AXIS_COUNT; axis++)
  {
    motor_set_enable((motor_axis_t)axis, 1);   /* 使能 */
  }
  HAL_Delay(1);
}

/**
  * @brief  错误执行函数
  * @param  无
  * @retval 无
  */
void APP_ErrorHandler(void)
{
  /* 关掉电机再停在这里，避免故障状态下一直出力 */
  if (s_io_ready)
  {
    motor_set_enable(MOTOR_AXIS_1, 0);
    motor_set_enable(MOTOR_AXIS_2, 0);
  }

  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  输出产生断言错误的源文件名和行号
  * @param  file：源文件名指针
  * @param  line：发生断言错误的行号
  * @retval 无
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/

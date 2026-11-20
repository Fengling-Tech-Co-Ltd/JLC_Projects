/**
  ******************************************************************************
  * @file    app_console.c
  * @brief   USART1 文本命令口
  * @note    命令一览（行尾 \r\n）：
  *            I1 <mA> / I2 <mA>      设相电流，1000 ~ 3500
  *            M1 <steps> <Hz>        按步数走，steps 带符号表示方向
  *            R1 <Hz>                连续转，Hz 带符号表示方向
  *            R2 <Hz>
  *            S                      两轴停止
  *            EN 0 | EN 1            两轴使能 / 禁用
  *            ?                      查询状态
  *          串口只用 PF0/PF1 两脚，复用 AF0。
  ******************************************************************************
  */
#include <string.h>

#include "app_console.h"
#include "app_motion.h"
#include "bsp_step.h"
#include "bsp_vref.h"
#include "bsp_motor_io.h"
#include "main.h"

UART_HandleTypeDef huart1;

static uint8_t s_rx_byte;
static char    s_line[64];
static uint8_t s_line_len;
static char    s_ready[64];
static volatile uint8_t s_ready_flag;

/* ---------------------------------------------------------------- 输出 */

void console_write(const char *s)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), 100);
}

static void write_u32(uint32_t v)
{
  char buf[12];
  uint32_t i = 0;

  if (v == 0U)
  {
    console_write("0");
    return;
  }
  while ((v > 0U) && (i < sizeof(buf)))
  {
    buf[i++] = (char)('0' + (v % 10U));
    v /= 10U;
  }
  while (i > 0U)
  {
    char c = buf[--i];
    HAL_UART_Transmit(&huart1, (uint8_t *)&c, 1, 20);
  }
}

static void write_axis_status(motor_axis_t axis)
{
  const char *name = (axis == MOTOR_AXIS_1) ? "A1 " : "A2 ";

  console_write(name);
  console_write("cur=");
  write_u32(vref_get_current_ma(axis));
  console_write(" f=");
  write_u32(step_get_freq(axis));
  console_write(" run=");
  write_u32(motion_is_running(axis));
  console_write(" took=");
  write_u32((uint32_t)motion_steps_taken(axis));
  console_write("\r\n");
}

/* ---------------------------------------------------------------- 输入 */

static const char *skip_space(const char *p)
{
  while (*p == ' ')
  {
    p++;
  }
  return p;
}

static int parse_int(const char **pp, int32_t *out)
{
  const char *p = skip_space(*pp);
  int32_t sign = 1;
  int32_t v = 0;

  if ((*p == '+') || (*p == '-'))
  {
    sign = (*p == '-') ? -1 : 1;
    p++;
  }
  if ((*p < '0') || (*p > '9'))
  {
    return -1;
  }
  while ((*p >= '0') && (*p <= '9'))
  {
    v = (v * 10) + (int32_t)(*p - '0');
    p++;
  }
  *out = v * sign;
  *pp = p;
  return 0;
}

static int parse_axis(char c, motor_axis_t *axis)
{
  if (c == '1')
  {
    *axis = MOTOR_AXIS_1;
    return 0;
  }
  if (c == '2')
  {
    *axis = MOTOR_AXIS_2;
    return 0;
  }
  return -1;
}

static void handle_line(char *line)
{
  const char *p = skip_space(line);
  int32_t a = 0;
  int32_t b = 0;
  motor_axis_t axis;
  int rc;

  if (*p == '\0')
  {
    return;
  }

  switch (*p)
  {
    case 'I':
    case 'i':
      if (parse_axis(p[1], &axis) != 0)
      {
        console_write("ERR axis\r\n");
        return;
      }
      p += 2;
      if (parse_int(&p, &a) != 0)
      {
        console_write("ERR value\r\n");
        return;
      }
      vref_set_current_ma(axis, (uint16_t)a);
      console_write("OK I\r\n");
      break;

    case 'M':
    case 'm':
      if (parse_axis(p[1], &axis) != 0)
      {
        console_write("ERR axis\r\n");
        return;
      }
      p += 2;
      if ((parse_int(&p, &a) != 0) || (parse_int(&p, &b) != 0) || (b <= 0))
      {
        console_write("ERR value\r\n");
        return;
      }
      rc = motion_move(axis, a, (uint32_t)b);
      write_u32((rc == MOTION_OK) ? 0U : (uint32_t)(-rc));
      console_write((rc == MOTION_OK) ? " OK M\r\n" : " ERR M\r\n");
      break;

    case 'R':
    case 'r':
      if (parse_axis(p[1], &axis) != 0)
      {
        console_write("ERR axis\r\n");
        return;
      }
      p += 2;
      if ((parse_int(&p, &a) != 0) || (a == 0))
      {
        console_write("ERR value\r\n");
        return;
      }
      rc = motion_run(axis, a);
      write_u32((rc == MOTION_OK) ? 0U : (uint32_t)(-rc));
      console_write((rc == MOTION_OK) ? " OK R\r\n" : " ERR R\r\n");
      break;

    case 'S':
    case 's':
      motion_stop_all();
      console_write("OK S\r\n");
      break;

    case 'E':
    case 'e':
      p++;
      if (parse_int(&p, &a) != 0)
      {
        console_write("ERR value\r\n");
        return;
      }
      if (a != 0)
      {
        motor_set_enable(MOTOR_AXIS_1, 1);
        motor_set_enable(MOTOR_AXIS_2, 1);
      }
      else
      {
        motion_stop_all();
        motor_set_enable(MOTOR_AXIS_1, 0);
        motor_set_enable(MOTOR_AXIS_2, 0);
      }
      console_write("OK EN\r\n");
      break;

    case '?':
      write_axis_status(MOTOR_AXIS_1);
      write_axis_status(MOTOR_AXIS_2);
      break;

    case 'H':
    case 'h':
      console_write("I1 <mA> | M1 <steps> <Hz> | R1 <Hz> | S | EN 0/1 | ?\r\n");
      break;

    default:
      console_write("ERR cmd\r\n");
      break;
  }
}

/* ---------------------------------------------------------------- 中断 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  char c = (char)s_rx_byte;

  if (huart->Instance == USART1)
  {
    if ((c == '\r') || (c == '\n'))
    {
      if ((s_line_len > 0U) && (s_ready_flag == 0U))
      {
        memcpy(s_ready, s_line, s_line_len);
        s_ready[s_line_len] = '\0';
        s_ready_flag = 1U;
      }
      s_line_len = 0;
    }
    else if ((c >= 32) && (c < 127) && (s_line_len < (sizeof(s_line) - 1U)))
    {
      s_line[s_line_len++] = c;
    }
    else
    {
      /* 丢弃不可见字符 */
    }
    HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef gpio = {0};

  if (huart->Instance != USART1)
  {
    return;
  }

  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  /* PF1 = USART1_TX，PF0 = USART1_RX，都是 AF0 */
  gpio.Pin = GPIO_PIN_1;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_PULLUP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = GPIO_AF0_USART1;
  HAL_GPIO_Init(GPIOF, &gpio);

  gpio.Pin = GPIO_PIN_0;
  HAL_GPIO_Init(GPIOF, &gpio);

  HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void console_init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = CONSOLE_BAUDRATE;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  s_line_len = 0;
  s_ready_flag = 0;
  HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
}

void console_poll(void)
{
  char local[64];

  if (s_ready_flag == 0U)
  {
    return;
  }
  memcpy(local, s_ready, sizeof(local));
  s_ready_flag = 0U;
  handle_line(local);
}

/**
  ******************************************************************************
  * @file    Bsp_PinMap.h
  * @brief   模块插槽小板（PY32F002AF15P6TU / TSSOP20）全部引脚定义
  *
  * 唯一依据：固件测试工程_README.md 第 3 节 + 2026-11-20 小板原理图。
  * 不要再从旧 KiCad 工程或旧文档推引脚。
  ******************************************************************************
  * 引脚总表（TSSOP20）：
  *   1  VSS            GND
  *   2  PA13           SWDIO       调试口，禁止复用
  *   3  PA14           SWCLK       调试口，禁止复用
  *   4  PB3            BIT1        槽位编码 bit1，输入
  *   5  PB6/PF4-BOOT0  BOOT0       启动选择，不作 GPIO
  *   6  PA6            BIT0        槽位编码 bit0，输入
  *   7  PA5            B_N          板号 ADC，3.3V 参考
  *   8  PF0            NC           悬空，需要 RX 时才用
  *   9  PF1            USART_TX     调试串口发送
  *   10 PF2            NRST         复位，不作 GPIO
  *   11 PA0            I2C2_SDA     本地 ADS1115 总线，软件 I2C
  *   12 PA1            I2C2_SCL     本地 ADS1115 总线，软件 I2C
  *   13 PA2            I2C_SDA      金手指 I2C1，AF12，从机
  *   14 PA3            I2C_SCL      金手指 I2C1，AF12，从机
  *   15 PA4            NC           悬空
  *   16 PA7            WATER_LEVEL  液位，干高湿低
  *   17 PB0            MOS          24V 负载，高导通
  *   18 PB2            PUMP         12V 泵，高导通
  *   19 PB1            VALVE        12V 阀，高导通
  *   20 VCC            3V3
  ******************************************************************************
  */
#ifndef __BSP_PINMAP_H
#define __BSP_PINMAP_H

#include "main.h"

/* ============================ 开关量输出 ============================ */
/* 三路都是低边 AO3400A，栅极 10Ω 串阻 + 100k 下拉，高电平导通 */
#define PIN_VALVE_PORT        GPIOB
#define PIN_VALVE_PIN         GPIO_PIN_1     /* 19 脚，12V 阀 J1 */
#define PIN_PUMP_PORT         GPIOB
#define PIN_PUMP_PIN          GPIO_PIN_2     /* 18 脚，12V 泵 J4 */
#define PIN_MOS_PORT          GPIOB
#define PIN_MOS_PIN           GPIO_PIN_0     /* 17 脚，24V 负载 J2 */

/* ============================ 开关量输入 ============================ */
#define PIN_BIT0_PORT         GPIOA
#define PIN_BIT0_PIN          GPIO_PIN_6     /* 6 脚，槽位 bit0 */
#define PIN_BIT1_PORT         GPIOB
#define PIN_BIT1_PIN          GPIO_PIN_3     /* 4 脚，槽位 bit1 */
#define PIN_LEVEL_PORT        GPIOA
#define PIN_LEVEL_PIN         GPIO_PIN_7     /* 16 脚，干高湿低 */

/* ============================== 模拟输入 ============================= */
#define PIN_BN_PORT           GPIOA
#define PIN_BN_PIN            GPIO_PIN_5     /* 7 脚，板号 R-2R 电压 */
#define PIN_BN_ADC_CHANNEL    ADC_CHANNEL_5

/* ======================= 金手指 I2C（硬件 I2C1） ===================== */
#define PIN_I2C1_PORT         GPIOA
#define PIN_I2C1_SDA_PIN      GPIO_PIN_2     /* 13 脚 */
#define PIN_I2C1_SCL_PIN      GPIO_PIN_3     /* 14 脚 */
#define PIN_I2C1_AF           GPIO_AF12_I2C

/* ===================== 本地 I2C2（软件，ADS1115） ==================== */
#define PIN_I2C2_PORT         GPIOA
#define PIN_I2C2_SDA_PIN      GPIO_PIN_0     /* 11 脚 */
#define PIN_I2C2_SCL_PIN      GPIO_PIN_1     /* 12 脚 */

/* ============================ 调试串口 ============================== */
#define PIN_USART_PORT        GPIOF
#define PIN_USART_TX_PIN      GPIO_PIN_1     /* 9 脚 */
#define PIN_USART_AF          GPIO_AF8_USART1

/* ============================== 悬空脚 ============================== */
#define PIN_NC_PA4            GPIO_PIN_4
#define PIN_NC_PF0            GPIO_PIN_0

#endif /* __BSP_PINMAP_H */

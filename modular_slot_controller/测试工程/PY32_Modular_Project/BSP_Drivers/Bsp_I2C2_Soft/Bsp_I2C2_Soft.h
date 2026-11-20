/**
  ******************************************************************************
  * @file    Bsp_I2C2_Soft.h
  * @brief   本地 I2C2 软件模拟（PA0 = SDA, PA1 = SCL），驱 ADS1115
  *
  * PA0/PA1 没有硬件 I2C 复用，只能软件模拟。开漏 + 外部 4.7k 上拉（R4/R5）。
  * 24MHz 下 100kHz 半周期约 5µs，用 NOP 延时，实际速率建议用示波器校准。
  ******************************************************************************
  */
#ifndef __BSP_I2C2_SOFT_H
#define __BSP_I2C2_SOFT_H

#include "main.h"

/* 半周期延时循环次数：24MHz 下每圈约 4~6 个周期，20 圈约 4~5µs。
 * 实测偏快就加大，偏慢就减小。 */
#ifndef BSP_I2C2_DELAY_LOOPS
#define BSP_I2C2_DELAY_LOOPS   20u
#endif

void    Bsp_I2C2_Init(void);

/* 总线恢复：SDA 被从机拉死时打 9 个 SCL 脉冲 + 一个 STOP */
void    Bsp_I2C2_Recover(void);

uint8_t Bsp_I2C2_Write(uint8_t dev7, const uint8_t *buf, uint16_t len); /* 1=全部 ACK */
uint8_t Bsp_I2C2_Read(uint8_t dev7, uint8_t *buf, uint16_t len);
uint8_t Bsp_I2C2_WriteRead(uint8_t dev7, const uint8_t *wb, uint16_t wlen,
                           uint8_t *rb, uint16_t rlen);
uint8_t Bsp_I2C2_Probe(uint8_t dev7);
void    Bsp_I2C2_Scan(void);

#endif /* __BSP_I2C2_SOFT_H */

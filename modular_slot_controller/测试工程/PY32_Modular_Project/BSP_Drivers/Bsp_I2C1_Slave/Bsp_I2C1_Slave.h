/**
  ******************************************************************************
  * @file    Bsp_I2C1_Slave.h
  * @brief   金手指 I2C1 从机（PA2 = SDA, PA3 = SCL, AF12）
  *
  * 小板是 16 槽系统里的从机，地址由位置决定：addr = 0x20 + position - 1。
  * 基址取 0x20：板号自动识别设计方案 8.4 把 0x10~0x13 留给大板 MCU，
  * 小板占 0x20~0x2F，避免同一条金手指总线上撞地址。
  *
  * 帧约定（主机协议未定前的最小实现）：
  *   写：固定 2 字节 [寄存器号, 值]
  *   读：固定回 8 字节寄存器组
  ******************************************************************************
  */
#ifndef __BSP_I2C1_SLAVE_H
#define __BSP_I2C1_SLAVE_H

#include "main.h"

#define BSP_I2C1_ADDR_BASE   0x20u
#define HOST_TX_MAX          16u

/* 启动从机监听。addr7 为 7 位地址（不含读写位），内部左移一位写 OAR1 */
void Bsp_I2C1_Slave_Init(uint8_t addr7);

/* 设置读事务的数据源（由 app_host 维护） */
void Bsp_I2C1_Slave_SetTxBuf(uint8_t *buf, uint16_t len);

/* 1 = 当前停在监听态、没有挂起传输，此时才能安全改地址重初始化 */
uint8_t Bsp_I2C1_Slave_IsIdle(void);

/* 主机写数据完成后的回调，实现在 app_host.c */
void Bsp_I2C1_OnWrite(const uint8_t *buf, uint16_t len);

#endif /* __BSP_I2C1_SLAVE_H */

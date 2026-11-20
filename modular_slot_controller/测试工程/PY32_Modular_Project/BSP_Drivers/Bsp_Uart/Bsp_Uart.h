/**
  ******************************************************************************
  * @file    Bsp_Uart.h
  * @brief   调试串口（只用发送）：PF1 = USART1_TX，AF8
  *
  * 本板没有接 USART1_RX，因此只配 TX，模式为 UART_MODE_TX。
  * 需要回读时把 PF0 接上并改成 UART_MODE_TX_RX。
  ******************************************************************************
  */
#ifndef __BSP_UART_H
#define __BSP_UART_H

#include "main.h"

/* PF1 = USART1_TX, 复用 AF8 */
#define BSP_UART_TX_GPIO_CLK_ENABLE   __HAL_RCC_GPIOF_CLK_ENABLE
#define BSP_UART_TX_GPIO_PORT         GPIOF
#define BSP_UART_TX_GPIO_PIN          GPIO_PIN_1
#define BSP_UART_TX_GPIO_AF           GPIO_AF8_USART1

#define BSP_UART_INSTANCE             USART1
#define BSP_UART_BAUDRATE             115200u
#define BSP_UART_TX_TIMEOUT_MS        100u

void Bsp_Uart_Init(void);
void Bsp_Uart_SendByte(uint8_t byte);
void Bsp_Uart_SendString(const char *str);
void Bsp_Uart_SendBuf(const uint8_t *buf, uint16_t len);

/* 自带 vsnprintf，不依赖 microlib 重定向，单次最多 160 字节 */
void Bsp_Uart_Printf(const char *fmt, ...);

#endif /* __BSP_UART_H */

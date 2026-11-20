/**
  ******************************************************************************
  * @file    app_console.h
  * @brief   USART1 调试命令口（PF0 = RX，PF1 = TX，115200 8N1）
  ******************************************************************************
  */
#ifndef __APP_CONSOLE_H
#define __APP_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "py32f0xx_hal.h"

#define CONSOLE_BAUDRATE   115200U

extern UART_HandleTypeDef huart1;

void console_init(void);
void console_poll(void);
void console_write(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* __APP_CONSOLE_H */

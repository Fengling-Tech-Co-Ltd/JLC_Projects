/**
  ******************************************************************************
  * @file    Bsp_Uart.c
  * @brief   调试串口（PF1 = USART1_TX，AF8，只发不收）
  ******************************************************************************
  */
#include "Bsp_Uart.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef BspUart1Handle;

void Bsp_Uart_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    BSP_UART_TX_GPIO_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin       = BSP_UART_TX_GPIO_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;        /* 复用推挽 */
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = BSP_UART_TX_GPIO_AF;    /* AF8 = USART1 */
    HAL_GPIO_Init(BSP_UART_TX_GPIO_PORT, &gpio);

    BspUart1Handle.Instance          = BSP_UART_INSTANCE;
    BspUart1Handle.Init.BaudRate     = BSP_UART_BAUDRATE;
    BspUart1Handle.Init.WordLength   = UART_WORDLENGTH_8B;
    BspUart1Handle.Init.StopBits     = UART_STOPBITS_1;
    BspUart1Handle.Init.Parity       = UART_PARITY_NONE;
    BspUart1Handle.Init.Mode         = UART_MODE_TX;   /* 只有 TX 引脚 */
    BspUart1Handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    BspUart1Handle.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&BspUart1Handle) != HAL_OK)
    {
        APP_ErrorHandler();
    }
}

void Bsp_Uart_SendBuf(const uint8_t *buf, uint16_t len)
{
    if (buf == NULL || len == 0u)
    {
        return;
    }
    HAL_UART_Transmit(&BspUart1Handle, (uint8_t *)buf, len, BSP_UART_TX_TIMEOUT_MS);
}

void Bsp_Uart_SendByte(uint8_t byte)
{
    Bsp_Uart_SendBuf(&byte, 1u);
}

void Bsp_Uart_SendString(const char *str)
{
    if (str == NULL)
    {
        return;
    }
    Bsp_Uart_SendBuf((const uint8_t *)str, (uint16_t)strlen(str));
}

void Bsp_Uart_Printf(const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    int n;

    if (fmt == NULL)
    {
        return;
    }
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n <= 0)
    {
        return;
    }
    if (n > (int)sizeof(buf) - 1)
    {
        n = (int)sizeof(buf) - 1;   /* 截断，不越界 */
    }
    Bsp_Uart_SendBuf((const uint8_t *)buf, (uint16_t)n);
}

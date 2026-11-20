/**
  ******************************************************************************
  * @file    Bsp_Gpio.c
  * @brief   悬空脚与开关量输入
  ******************************************************************************
  */
#include "Bsp_Gpio.h"
#include "Bsp_PinMap/Bsp_PinMap.h"

void Bsp_Gpio_SafeInit(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    /* PA4 与 PF0 当前悬空，配模拟输入（不驱动、不采）。
     * 注意 PA13/PA14/PF2/PB6 一律不碰。 */
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin  = PIN_NC_PA4;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = PIN_NC_PF0;
    HAL_GPIO_Init(GPIOF, &gpio);
}

void Bsp_Gpio_InputInit(void)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;    /* 电平由大板 / MCP6541 驱动，不加上下拉 */

    gpio.Pin = PIN_BIT0_PIN;
    HAL_GPIO_Init(PIN_BIT0_PORT, &gpio);

    gpio.Pin = PIN_BIT1_PIN;
    HAL_GPIO_Init(PIN_BIT1_PORT, &gpio);

    gpio.Pin = PIN_LEVEL_PIN;
    HAL_GPIO_Init(PIN_LEVEL_PORT, &gpio);
}

/**
  ******************************************************************************
  * @file    Bsp_Output.c
  * @brief   三路低边开关输出
  *
  * 安全要点：先把输出锁存器写成低，再切成推挽输出。
  * 直接使能输出的话，锁存器的旧值会先被推出去，可能有几十纳秒的高电平毛刺，
  * 对 24V 负载来说不可接受。
  ******************************************************************************
  */
#include "Bsp_Output.h"
#include "Bsp_PinMap/Bsp_PinMap.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    const char   *name;
} output_map_t;

static const output_map_t s_map[] = {
    {PIN_VALVE_PORT, PIN_VALVE_PIN, "VALVE"},
    {PIN_PUMP_PORT,  PIN_PUMP_PIN,  "PUMP"},
    {PIN_MOS_PORT,   PIN_MOS_PIN,   "MOS"},
};

#define OUT_COUNT (sizeof(s_map) / sizeof(s_map[0]))

void Bsp_Output_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 1. 先写低：此时引脚还没使能输出，只是改锁存器 */
    HAL_GPIO_WritePin(s_map[0].port, PIN_VALVE_PIN | PIN_PUMP_PIN | PIN_MOS_PIN, GPIO_PIN_RESET);

    /* 2. 再切成推挽输出 */
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin   = PIN_VALVE_PIN | PIN_PUMP_PIN | PIN_MOS_PIN;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* 3. 兜底再写一次低 */
    HAL_GPIO_WritePin(s_map[0].port, PIN_VALVE_PIN | PIN_PUMP_PIN | PIN_MOS_PIN, GPIO_PIN_RESET);
}

void Bsp_Output_Set(Bsp_Output_Id_t id, uint8_t on)
{
    if ((uint32_t)id >= OUT_COUNT)
    {
        return;
    }
    HAL_GPIO_WritePin(s_map[id].port, s_map[id].pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t Bsp_Output_Get(Bsp_Output_Id_t id)
{
    if ((uint32_t)id >= OUT_COUNT)
    {
        return 0;
    }
    return (HAL_GPIO_ReadPin(s_map[id].port, s_map[id].pin) == GPIO_PIN_SET) ? 1u : 0u;
}

void Bsp_Output_AllOff(void)
{
    for (uint32_t i = 0; i < OUT_COUNT; i++)
    {
        HAL_GPIO_WritePin(s_map[i].port, s_map[i].pin, GPIO_PIN_RESET);
    }
}

const char *Bsp_Output_Name(Bsp_Output_Id_t id)
{
    return ((uint32_t)id < OUT_COUNT) ? s_map[id].name : "?";
}

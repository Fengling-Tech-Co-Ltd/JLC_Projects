/**
  ******************************************************************************
  * @file    Bsp_Output.h
  * @brief   三路低边开关：VALVE(PB1) / PUMP(PB2) / MOS(PB0)，高电平导通
  ******************************************************************************
  */
#ifndef __BSP_OUTPUT_H
#define __BSP_OUTPUT_H

#include "main.h"

typedef enum
{
    OUT_VALVE = 0,   /* PB1，12V 阀，J1 */
    OUT_PUMP  = 1,   /* PB2，12V 泵，J4 */
    OUT_MOS   = 2    /* PB0，24V 负载，J2 */
} Bsp_Output_Id_t;

/* 先写锁存器为低，再切成推挽输出：上电瞬间不会误动作 */
void Bsp_Output_Init(void);

void    Bsp_Output_Set(Bsp_Output_Id_t id, uint8_t on);
uint8_t Bsp_Output_Get(Bsp_Output_Id_t id);
void    Bsp_Output_AllOff(void);

/* 把 id 翻译成引脚，供测试代码打印用 */
const char *Bsp_Output_Name(Bsp_Output_Id_t id);

#endif /* __BSP_OUTPUT_H */

/**
  ******************************************************************************
  * @file    app_level.h
  * @brief   液位检测：PA7（MCP6541 推挽，无上拉）
  * @note    极性可配：APP_LEVEL_DRY_IS_HIGH（app_config.h）。方案文档定义干=高，
  *          2026-11-20 这批板实测相反，当前配置为干=低。
  ******************************************************************************
  */
#ifndef __APP_LEVEL_H
#define __APP_LEVEL_H

#include "main.h"

typedef enum
{
    APP_LEVEL_DRY = 0,
    APP_LEVEL_WET = 1
} App_Level_t;

void        App_Level_Init(void);
App_Level_t App_Level_Read(void);     /* 连续 3 次一致才翻转 */
uint8_t     App_Level_Raw(void);      /* 原始电平：1 = 高（干） */

#endif /* __APP_LEVEL_H */

/**
  ******************************************************************************
  * @file    app_level.c
  * @brief   液位输入（PA7）
  *
  * MCP6541 推挽输出：干态高、湿态低。翻转点约 R_w = 220kΩ。
  * 软件做 3 次一致确认，抑制电极表面波动。
  ******************************************************************************
  */
#include "app_level.h"
#include "app_config.h"
#include "Bsp_PinMap/Bsp_PinMap.h"

static App_Level_t s_level;
static uint8_t     s_cnt;

void App_Level_Init(void)
{
    /* 上电先按当前电平定初值，不引入一次假翻转 */
    s_level = App_Level_Raw() ? APP_LEVEL_DRY : APP_LEVEL_WET;
    s_cnt   = APP_LEVEL_CONFIRM;
}

uint8_t App_Level_Raw(void)
{
    return (HAL_GPIO_ReadPin(PIN_LEVEL_PORT, PIN_LEVEL_PIN) == GPIO_PIN_SET) ? 1u : 0u;
}

App_Level_t App_Level_Read(void)
{
    /* 极性由 APP_LEVEL_DRY_IS_HIGH 决定，见 app_config.h 的说明 */
#if APP_LEVEL_DRY_IS_HIGH
    App_Level_t now = App_Level_Raw() ? APP_LEVEL_DRY : APP_LEVEL_WET;
#else
    App_Level_t now = App_Level_Raw() ? APP_LEVEL_WET : APP_LEVEL_DRY;
#endif

    if (now == s_level)
    {
        s_cnt = APP_LEVEL_CONFIRM;
        return s_level;
    }

    if (s_cnt > 0u)
    {
        s_cnt--;
    }
    if (s_cnt == 0u)
    {
        s_level = now;
        s_cnt   = APP_LEVEL_CONFIRM;
    }
    return s_level;
}

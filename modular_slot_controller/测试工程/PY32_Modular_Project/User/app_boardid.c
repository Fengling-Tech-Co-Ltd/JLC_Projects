/**
  ******************************************************************************
  * @file    app_boardid.c
  * @brief   板号与槽位读取
  *
  * 板号：PA5 读 R-2R 电压，3.3V 参考，12 位。判决门限 512 / 1536 / 2560。
  *       上电先等 20ms 稳定，每次平均 8 次，连续 3 次同档才算有效。
  * 槽位：PA6 = BIT0，PB3 = BIT1，直接读电平。
  * 位置：position = board_id*4 + slot_id + 1（1~16）。
  ******************************************************************************
  */
#include "app_boardid.h"
#include "app_config.h"
#include "Bsp_Adc/Bsp_Adc.h"
#include "Bsp_PinMap/Bsp_PinMap.h"

static App_Position_t s_pos;
static uint8_t        s_cand;
static uint8_t        s_cand_cnt;

void App_BoardId_Init(void)
{
    /* 上电等 R-2R 与滤波电容稳定（文档 10~50ms） */
    HAL_Delay(APP_BOARDID_SETTLE_MS);

    s_pos.board_id = 0;
    s_pos.slot_id  = 0;
    s_pos.position = 1;
    s_pos.adc_raw  = 0;
    s_pos.valid    = 0;
    s_cand         = 0xFF;
    s_cand_cnt     = 0;
}

uint8_t App_BoardId_Classify(uint16_t raw)
{
    if (raw < 512u)
    {
        return 0;
    }
    if (raw < 1536u)
    {
        return 1;
    }
    if (raw < 2560u)
    {
        return 2;
    }
    return 3;
}

App_Position_t App_BoardId_Read(void)
{
    uint16_t raw = Bsp_Adc_ReadAvg(PIN_BN_ADC_CHANNEL, APP_BOARDID_AVG_N);

    /* 采样全失败时保持上一次的判断，不要把 0xFFFF 当成 board_id=3 */
    if (raw != BSP_ADC_INVALID)
    {
        uint8_t cls = App_BoardId_Classify(raw);

        s_pos.adc_raw = raw;

        /* 连续 N 次同档才更新，避免插拔瞬间读飞 */
        if (cls == s_cand)
        {
            if (s_cand_cnt < APP_BOARDID_CONFIRM)
            {
                s_cand_cnt++;
            }
        }
        else
        {
            s_cand     = cls;
            s_cand_cnt = 1;
        }
    }

    uint8_t bit0 = (HAL_GPIO_ReadPin(PIN_BIT0_PORT, PIN_BIT0_PIN) == GPIO_PIN_SET) ? 1u : 0u;
    uint8_t bit1 = (HAL_GPIO_ReadPin(PIN_BIT1_PORT, PIN_BIT1_PIN) == GPIO_PIN_SET) ? 1u : 0u;

    if (s_cand_cnt >= APP_BOARDID_CONFIRM && s_cand <= 3u)
    {
        s_pos.board_id = s_cand;
        s_pos.valid    = 1;
    }
    s_pos.slot_id  = (uint8_t)((bit1 << 1) | bit0);
    s_pos.position = (uint8_t)(s_pos.board_id * 4u + s_pos.slot_id + 1u);
    return s_pos;
}

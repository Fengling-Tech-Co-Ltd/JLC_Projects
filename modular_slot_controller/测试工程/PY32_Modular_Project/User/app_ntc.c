/**
  ******************************************************************************
  * @file    app_ntc.c
  * @brief   ADS1115 两次单次转换 + NTC 阻值/温度换算
  *
  * 换算依据《固件测试工程_README.md》7.2：
  *   d = V(ntc)  = AIN0-AIN1，配置字 0x8383
  *   b = V(B)    = AIN1-GND，配置字 0xD383
  *   R = 33000 * d / b
  *   T = 1/(1/298.15 + ln(R/10000)/3950) - 273.15
  ******************************************************************************
  */
#include "app_ntc.h"

#include <math.h>

#include "Bsp_I2C2_Soft/Bsp_I2C2_Soft.h"

/* 写配置字 -> 等转换 -> 读结果，返回 1 表示成功 */
static uint8_t ads1115_single(uint16_t cfg, int16_t *out)
{
    uint8_t buf[3];
    uint8_t rbuf[2];

    buf[0] = ADS1115_CFG_REG;
    buf[1] = (uint8_t)(cfg >> 8);
    buf[2] = (uint8_t)(cfg & 0xFFu);
    if (!Bsp_I2C2_Write(ADS1115_ADDR7, buf, 3u))
    {
        return 0;
    }

    /* 128SPS 一次 7.8ms，文档要求 ≥9ms */
    HAL_Delay(10);

    /* 指向结果寄存器后读 2 字节 */
    buf[0] = ADS1115_CONV_REG;
    if (!Bsp_I2C2_WriteRead(ADS1115_ADDR7, buf, 1u, rbuf, 2u))
    {
        return 0;
    }
    if (out)
    {
        *out = (int16_t)(((uint16_t)rbuf[0] << 8) | rbuf[1]);
    }
    return 1;
}

void App_Ntc_Init(void)
{
    Bsp_I2C2_Init();
    Bsp_I2C2_Scan();
}

App_Ntc_t App_Ntc_Read(void)
{
    App_Ntc_t r;
    r.raw_vntc = 0;
    r.raw_vb   = 0;
    r.r_ntc    = 0.0f;
    r.temp_c   = 0.0f;

    if (!ads1115_single(ADS1115_CFG_VNTC, &r.raw_vntc) ||
        !ads1115_single(ADS1115_CFG_VB, &r.raw_vb))
    {
        r.status = NTC_I2C_ERR;
        return r;
    }

    /* 开路：V(ntc) 顶到满量程，V(B) 掉到 0；短路正相反。
     * 门限换算（PGA=±4.096V，1 码 = 125µV）：
     *   短路时 V_B=3.29V → 26320 码；60°C 上沿 V_B≈3.06V → 24480 码。
     *   取 25000 码（3.125V）作短路门限，两侧都留了余量。
     *   注意不能用 30000：那是 3.75V，超过 3.3V 电源轨，永远达不到。 */
    if (r.raw_vb < 64)
    {
        r.status = NTC_OPEN;
        return r;
    }
    if (r.raw_vntc < 64 && r.raw_vb > 25000)
    {
        r.status = NTC_SHORT;
        return r;
    }

    r.r_ntc = (float)NTC_R_REF_OHM * (float)r.raw_vntc / (float)r.raw_vb;

    if (r.r_ntc <= 0.0f)
    {
        r.status = NTC_SHORT;
        return r;
    }

    float t_k = 1.0f / (1.0f / (float)NTC_T25_K + logf(r.r_ntc / (float)NTC_R25_OHM) / (float)NTC_B_VALUE);
    r.temp_c = t_k - 273.15f;
    r.status = NTC_OK;
    return r;
}

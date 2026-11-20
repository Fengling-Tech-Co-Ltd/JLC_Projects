/**
  ******************************************************************************
  * @file    app_ntc.h
  * @brief   ADS1115 + NTC 测温（本地 I2C2，地址 0x48）
  ******************************************************************************
  */
#ifndef __APP_NTC_H
#define __APP_NTC_H

#include "main.h"

#define ADS1115_ADDR7        0x48u
#define ADS1115_CFG_REG      0x01u
#define ADS1115_CONV_REG     0x00u

/* 两次单次转换的配置字（文档 7.2） */
#define ADS1115_CFG_VNTC     0x8383u   /* AIN0-AIN1, ±4.096V, 单次, 128SPS */
#define ADS1115_CFG_VB       0xD383u   /* AIN1-GND,   ±4.096V, 单次, 128SPS */

#define NTC_R_REF_OHM        33000.0    /* 文档公式里的 33k */
#define NTC_R25_OHM          10000.0
#define NTC_B_VALUE          3950.0
#define NTC_T25_K            298.15

typedef enum
{
    NTC_OK = 0,
    NTC_OPEN,      /* 探头开路：V_ntc 接近满量程，V_B 接近 0 */
    NTC_SHORT,     /* 探头短路：V_ntc 接近 0，V_B 接近 3.3V */
    NTC_I2C_ERR    /* ADS1115 没有应答 */
} App_Ntc_Status_t;

typedef struct
{
    int16_t  raw_vntc;   /* AIN0-AIN1 原始码 */
    int16_t  raw_vb;     /* AIN1-GND 原始码 */
    float    r_ntc;      /* 算出的 NTC 阻值，Ω */
    float    temp_c;     /* 摄氏温度 */
    App_Ntc_Status_t status;
} App_Ntc_t;

void     App_Ntc_Init(void);
App_Ntc_t App_Ntc_Read(void);

#endif /* __APP_NTC_H */

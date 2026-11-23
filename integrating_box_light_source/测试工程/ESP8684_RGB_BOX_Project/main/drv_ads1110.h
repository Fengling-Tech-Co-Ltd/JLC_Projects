/*
 * drv_ads1110.h — 16 位 ADC，I2C 0x48
 */
#ifndef DRV_ADS1110_H
#define DRV_ADS1110_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* 连续 / 15SPS / 增益 1 */
esp_err_t ads1110_init(void);

/* 读一次：16 位有符号原始值 + 回读到的配置字节 */
esp_err_t ads1110_read_raw(int16_t *raw, uint8_t *cfg);

/* 原始值 -> 电压(V)，满量程 ±2.048V */
float ads1110_raw_to_volt(int16_t raw);

/* 连续读 n 次取平均（15SPS 下一次约 66ms，n 次约 n*66ms） */
esp_err_t ads1110_read_volt_avg(int n, float *volt, bool *saturated);

#endif /* DRV_ADS1110_H */

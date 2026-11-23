/*
 * app_light.h — 四色光源应用层
 */
#ifndef APP_LIGHT_H
#define APP_LIGHT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "light_cfg.h"

/* 上电：四路全关 -> 配 ADC -> 读暗底 */
esp_err_t light_init(void);

void light_all_off(void);

/* 直接给 code（0~3102）或给目标 LED 电流（mA） */
esp_err_t light_set_code(light_ch_t ch, uint16_t code);
esp_err_t light_set_ma(light_ch_t ch, float mA);

uint16_t light_get_code(light_ch_t ch);
float    light_get_ma(light_ch_t ch);

/* 单路光电流测量。分时模式下会临时关掉其余三路再恢复。 */
esp_err_t light_measure_ua(light_ch_t ch, float *ua, bool *saturated);

/* 分时(true，默认) / 同时(false) */
void light_set_timeshare(bool on);
bool light_get_timeshare(void);

/* 闭环：目标光电流（µA），0 表示该路不参与闭环 */
void  light_set_target_ua(light_ch_t ch, float ua);
float light_get_target_ua(light_ch_t ch);
void  light_set_loop(bool on);
bool  light_get_loop(void);

/* 一次标定：四路 50% 电流测光电流，得到 µA/code */
esp_err_t light_calibrate(void);

/* 保存 / 加载标定（NVS） */
esp_err_t light_cal_save(void);
esp_err_t light_cal_load(void);

/* 直接读 ADS1110 VIN+ 的原始电压（不扣暗底），用于诊断光强板。 */
esp_err_t light_read_sensor_volt(float *volt);

/* 强制一次单次转换后读取原始码/配置字/电压，用于判断 ADC 是否真的在转换。 */
esp_err_t light_read_sensor_oneshot(int16_t *raw, uint8_t *cfg, float *volt);

/* 100ms 连续监视开关：打开后 light_monitor_tick() 持续打印 [VIN] 电压。 */
void light_monitor_set(bool on);
bool light_get_monitor(void);
void light_monitor_tick(void);

/* 周期任务，建议 1s 调一次 */
void light_task(void);

/* 故障/状态查询，供控制台打印 */
void light_status_str(char *buf, int len);

#endif /* APP_LIGHT_H */

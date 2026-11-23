/*
 * drv_ads1110.c — 16 位 ADC
 *
 * 配置字位序（与 fl1s_driver/HDL/ads1110 一致，已按手册核对）：
 *   bit7    ST/DRDY（写 1 启动单次转换）
 *   bit4    SC（0=连续，1=单次）
 *   bit3:2  DR（00=240SPS，01=60，10=30，11=15）
 *   bit1:0  PGA（00=×1 … 11=×8）
 * 因此 连续/15SPS/×1 = 0x0C。
 */
#include "drv_ads1110.h"

#include "bsp_i2c.h"
#include "light_cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ads1110";

esp_err_t ads1110_init(void)
{
    uint8_t cfg = ADS1110_CFG_CONT;
    esp_err_t err = bsp_i2c_write(ADS1110_ADDR, &cfg, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "config failed: %s", esp_err_to_name(err));
        return err;
    }
    /* 连续模式下第一次转换要等一个周期（15SPS ≈ 66ms） */
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

esp_err_t ads1110_read_raw(int16_t *raw, uint8_t *cfg)
{
    uint8_t d[3] = {0};
    esp_err_t err = bsp_i2c_read(ADS1110_ADDR, d, sizeof(d));
    if (err != ESP_OK) {
        return err;
    }
    if (raw) {
        *raw = (int16_t)(((uint16_t)d[0] << 8) | d[1]);
    }
    if (cfg) {
        *cfg = d[2];
    }
    return ESP_OK;
}

float ads1110_raw_to_volt(int16_t raw)
{
    return (float)raw / 32768.0f * ADS1110_FS_V;
}

esp_err_t ads1110_read_volt_avg(int n, float *volt, bool *saturated)
{
    if (n < 1) {
        n = 1;
    }
    int32_t sum = 0;
    int ok = 0;
    bool sat = false;

    for (int i = 0; i < n; i++) {
        int16_t raw = 0;
        if (ads1110_read_raw(&raw, NULL) == ESP_OK) {
            if (raw > ADS1110_SAT_RAW || raw < -ADS1110_SAT_RAW) {
                sat = true;
            }
            sum += raw;
            ok++;
        }
        /* 15SPS 一次 66ms；留一点余量 */
        vTaskDelay(pdMS_TO_TICKS(70));
    }
    if (ok == 0) {
        return ESP_FAIL;
    }
    if (volt) {
        *volt = ads1110_raw_to_volt((int16_t)(sum / ok));
    }
    if (saturated) {
        *saturated = sat;
    }
    return ESP_OK;
}

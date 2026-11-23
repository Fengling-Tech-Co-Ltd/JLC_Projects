/*
 * app_light.c — 四色光源应用层
 *
 * 职责：通道状态、目标电流、分时测量、慢速闭环、标定与 NVS 存取。
 * 不含任何 I2C 细节，也不直接操作 GPIO。
 */
#include "app_light.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "drv_mcp4728.h"
#include "drv_ads1110.h"
#include "bsp_i2c.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "light";

typedef struct {
    uint32_t magic;
    float    dark_v;
    float    gain[LIGHT_CH_NUM];   /* µA / code */
    uint32_t crc;
} cal_blob_t;

#define CAL_MAGIC 0x4C413031u   /* "LA01" */

static uint16_t s_code[LIGHT_CH_NUM];
static float    s_tgt_ua[LIGHT_CH_NUM];
static float    s_gain[LIGHT_CH_NUM];
static float    s_dark_v;

static bool s_timeshare = true;
static bool s_loop;
static bool s_sensor_fault;
static bool s_dac_fault;

/* ---------------------------------------------------------------- */
static uint32_t blob_crc(const cal_blob_t *b)
{
    const uint8_t *p = (const uint8_t *)b;
    uint32_t sum = 0;
    for (size_t i = 0; i < offsetof(cal_blob_t, crc); i++) {
        sum = sum * 31u + p[i];
    }
    return sum;
}

static void gain_default(void)
{
    for (int i = 0; i < LIGHT_CH_NUM; i++) {
        s_gain[i] = 0.0f;   /* 0 = 未标定 */
    }
}

esp_err_t light_init(void)
{
    gain_default();
    memset(s_tgt_ua, 0, sizeof(s_tgt_ua));

    /* 1a. 先发 General Call Wake-Up。MCP4728 上电时可能从 EEPROM 带着
     *     power-down 位起来：此时 I2C 写照样 ACK，但输出脚是关断的，
     *     表现为"寄存器写成功、CTRL 却一直是 0V"。 */
    if (mcp4728_wakeup() != ESP_OK) {
        s_dac_fault = true;
        ESP_LOGW(TAG, "MCP4728 wake-up 失败，后续写入可能不生效");
    }

    /* 1b. 四路先关断：MCP4728 上电会把 EEPROM 旧值直接输出 */
    mcp4728_set_all(0, 0, 0, 0);
    for (int i = 0; i < LIGHT_CH_NUM; i++) {
        s_code[i] = 0;
    }

    /* 2. ADC 连续 15SPS / 增益 1。
     * ADS1110 上电偶发不应答（实测复位后 config write 有时 NACK），
     * 多试几次，避免一次失败就把 sensor_fault 锁死。 */
    bool adc_ok = false;
    for (int i = 0; i < 5; i++) {
        if (ads1110_init() == ESP_OK) {
            adc_ok = true;
            break;
        }
        ESP_LOGW(TAG, "ADS1110 init 第 %d 次失败，100ms 后重试", i + 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (!adc_ok) {
        s_sensor_fault = true;
        ESP_LOGW(TAG, "ADS1110 不在线，光强闭环不可用，只能开环");
    } else {
        s_sensor_fault = false;
        float v = 0.0f;
        if (ads1110_read_volt_avg(ADS_AVG_N, &v, NULL) == ESP_OK) {
            s_dark_v = v;
        }
    }

    /* 3. 标定 */
    if (light_cal_load() != ESP_OK) {
        ESP_LOGW(TAG, "没有可用标定，按未标定运行");
    }

    ESP_LOGI(TAG, "init done: dark=%.4fV timeshare=%d sensor_fault=%d",
             s_dark_v, (int)s_timeshare, (int)s_sensor_fault);
    return ESP_OK;
}

void light_all_off(void)
{
    mcp4728_set_all(0, 0, 0, 0);
    for (int i = 0; i < LIGHT_CH_NUM; i++) {
        s_code[i] = 0;
    }
}

esp_err_t light_set_code(light_ch_t ch, uint16_t code)
{
    if (ch >= LIGHT_CH_NUM) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = mcp4728_set(ch, code);
    if (err == ESP_OK) {
        s_code[ch] = (code > DAC_CODE_MAX) ? DAC_CODE_MAX : code;
        s_dac_fault = false;
    } else {
        s_dac_fault = true;
    }
    return err;
}

esp_err_t light_set_ma(light_ch_t ch, float mA)
{
    return light_set_code(ch, mcp4728_code_for_current(mA));
}

uint16_t light_get_code(light_ch_t ch)
{
    return (ch < LIGHT_CH_NUM) ? s_code[ch] : 0;
}

float light_get_ma(light_ch_t ch)
{
    return (ch < LIGHT_CH_NUM) ? mcp4728_current_from_code(s_code[ch]) : 0.0f;
}

/* 传感器上电偶发失灵后的恢复：灭灯 -> 重新初始化 -> 重采暗底 -> 恢复原亮度。
 * 这样一次复位时的 NACK 不会让 meas 永久不可用。 */
static esp_err_t sensor_recover(void)
{
    uint16_t saved[LIGHT_CH_NUM];
    memcpy(saved, s_code, sizeof(saved));

    for (int i = 0; i < LIGHT_CH_NUM; i++) {
        mcp4728_set((light_ch_t)i, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(LIGHT_SETTLE_MS));

    esp_err_t err = ads1110_init();
    if (err == ESP_OK) {
        float v = 0.0f;
        if (ads1110_read_volt_avg(ADS_AVG_N, &v, NULL) == ESP_OK) {
            s_dark_v = v;
        }
        s_sensor_fault = false;
        ESP_LOGW(TAG, "ADS1110 已恢复，重采暗底 dark=%.4fV", s_dark_v);
    }

    for (int i = 0; i < LIGHT_CH_NUM; i++) {
        if (saved[i] != 0) {
            mcp4728_set((light_ch_t)i, saved[i]);
        }
    }
    return err;
}

esp_err_t light_measure_ua(light_ch_t ch, float *ua, bool *saturated)
{
    if (ch >= LIGHT_CH_NUM) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_sensor_fault) {
        if (sensor_recover() != ESP_OK) {
            return ESP_ERR_INVALID_STATE;
        }
    }

    uint16_t saved[LIGHT_CH_NUM];
    memcpy(saved, s_code, sizeof(saved));

    /* 分时模式：临时关掉其余三路，读完整光才恢复。
     * 这一步会改变输出，只能在曝光间隙或标定窗口调用。 */
    if (s_timeshare) {
        for (int i = 0; i < LIGHT_CH_NUM; i++) {
            if (i != (int)ch && saved[i] != 0) {
                mcp4728_set((light_ch_t)i, 0);
            }
        }
    }
    vTaskDelay(pdMS_TO_TICKS(LIGHT_SETTLE_MS));

    float v = 0.0f;
    bool sat = false;
    esp_err_t err = ads1110_read_volt_avg(ADS_AVG_N, &v, &sat);

    if (s_timeshare) {
        for (int i = 0; i < LIGHT_CH_NUM; i++) {
            if (i != (int)ch && saved[i] != 0) {
                mcp4728_set((light_ch_t)i, saved[i]);
            }
        }
    }

    if (err != ESP_OK) {
        s_sensor_fault = true;
        return err;
    }

    float u = (v - s_dark_v) / TIA_OHM * 1e6f;   /* µA */
    if (u < 0.0f) {
        u = 0.0f;
    }
    if (ua) {
        *ua = u;
    }
    if (saturated) {
        *saturated = sat;
    }
    return ESP_OK;
}

/* 直接读 ADS1110 VIN+（OPA333 输出）的电压，不扣暗底、不做分时切换。
 * 用于判断"光照变化有没有真的进到 ADC"，与 meas 的 µA 计算无关。 */
esp_err_t light_read_sensor_volt(float *volt)
{
    if (s_sensor_fault) {
        if (sensor_recover() != ESP_OK) {
            return ESP_ERR_INVALID_STATE;
        }
    }
    int16_t raw = 0;
    esp_err_t err = ads1110_read_raw(&raw, NULL);
    if (err != ESP_OK) {
        return err;
    }
    if (volt) {
        *volt = ads1110_raw_to_volt(raw);
    }
    return ESP_OK;
}

/* 强制单次转换（写 0x9C = ST + 单次 + 15SPS + 增益1），等 70ms 后读。
 * 用来绕开"连续模式没有更新"的情况。 */
esp_err_t light_read_sensor_oneshot(int16_t *raw_out, uint8_t *cfg_out, float *volt)
{
    uint8_t cfg = (uint8_t)(ADS1110_CFG_ONESHOT | ADS1110_CFG_START);   /* 0x9C */
    esp_err_t err = bsp_i2c_write(ADS1110_ADDR, &cfg, 1);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(70));

    int16_t raw = 0;
    uint8_t rd = 0;
    err = ads1110_read_raw(&raw, &rd);
    if (err != ESP_OK) {
        return err;
    }
    if (raw_out) *raw_out = raw;
    if (cfg_out) *cfg_out = rd;
    if (volt)    *volt = ads1110_raw_to_volt(raw);
    return ESP_OK;
}

static bool s_mon;

void light_monitor_set(bool on)
{
    s_mon = on;
}

bool light_get_monitor(void)
{
    return s_mon;
}

/* 由主循环按 100ms 调用；只在 mon on 时打印。 */
void light_monitor_tick(void)
{
    if (!s_mon) {
        return;
    }
    float v = 0.0f;
    if (light_read_sensor_volt(&v) == ESP_OK) {
        printf("[VIN] %.4f V\n", (double)v);
    } else {
        printf("[VIN] read failed\n");
    }
}

void light_set_timeshare(bool on)
{
    s_timeshare = on;
}

bool light_get_timeshare(void)
{
    return s_timeshare;
}

void light_set_target_ua(light_ch_t ch, float ua)
{
    if (ch < LIGHT_CH_NUM) {
        s_tgt_ua[ch] = ua;
    }
}

float light_get_target_ua(light_ch_t ch)
{
    return (ch < LIGHT_CH_NUM) ? s_tgt_ua[ch] : 0.0f;
}

void light_set_loop(bool on)
{
    s_loop = on;
}

bool light_get_loop(void)
{
    return s_loop;
}

/* 四路各设 50% 电流，测一次光电流，得到 µA/code */
esp_err_t light_calibrate(void)
{
    if (s_sensor_fault) {
        return ESP_ERR_INVALID_STATE;
    }
    const uint16_t cal_code = mcp4728_code_for_current(I_FULL_MA * 0.5f);
    for (int i = 0; i < LIGHT_CH_NUM; i++) {
        light_set_code((light_ch_t)i, cal_code);
        vTaskDelay(pdMS_TO_TICKS(20));
        float ua = 0.0f;
        bool sat = false;
        if (light_measure_ua((light_ch_t)i, &ua, &sat) != ESP_OK) {
            light_all_off();
            return ESP_FAIL;
        }
        s_gain[i] = (cal_code > 0) ? (ua / (float)cal_code) : 0.0f;
        ESP_LOGI(TAG, "cal ch%d: code=%u -> %.3f uA (%.5f uA/code)%s",
                 i, cal_code, ua, s_gain[i], sat ? " [饱和]" : "");
    }
    light_all_off();
    return ESP_OK;
}

esp_err_t light_cal_save(void)
{
    cal_blob_t b = {0};
    b.magic = CAL_MAGIC;
    b.dark_v = s_dark_v;
    memcpy(b.gain, s_gain, sizeof(b.gain));
    b.crc = blob_crc(&b);

    nvs_handle_t h;
    esp_err_t err = nvs_open(LIGHT_CAL_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_blob(h, "cal", &b, sizeof(b));
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    ESP_LOGI(TAG, "cal save %s", err == ESP_OK ? "OK" : "FAIL");
    return err;
}

esp_err_t light_cal_load(void)
{
    cal_blob_t b = {0};
    size_t len = sizeof(b);
    nvs_handle_t h;
    esp_err_t err = nvs_open(LIGHT_CAL_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_blob(h, "cal", &b, &len);
    nvs_close(h);
    if (err != ESP_OK || len != sizeof(b) || b.magic != CAL_MAGIC ||
        b.crc != blob_crc(&b)) {
        return ESP_ERR_INVALID_STATE;
    }
    s_dark_v = b.dark_v;
    memcpy(s_gain, b.gain, sizeof(s_gain));
    return ESP_OK;
}

/* 1s 一拍，闭环时每拍处理一路，四路 4 秒一圈 */
void light_task(void)
{
    static int ch;

    if (!s_loop || s_sensor_fault) {
        return;
    }
    float ua = 0.0f;
    bool sat = false;
    if (light_measure_ua((light_ch_t)ch, &ua, &sat) != ESP_OK) {
        return;
    }
    if (sat) {
        ESP_LOGW(TAG, "ch%d 光电流饱和，降低电流或换小跨阻", ch);
        light_set_code((light_ch_t)ch, s_code[ch] / 2);
        ch = (ch + 1) & 0x03;
        return;
    }

    float tgt = s_tgt_ua[ch];
    if (tgt > 0.0f) {
        int32_t code = (int32_t)s_code[ch] +
                       (int32_t)((tgt - ua) * LIGHT_DEFAULT_KP);
        if (code < 0) code = 0;
        if (code > DAC_CODE_MAX) code = DAC_CODE_MAX;
        light_set_code((light_ch_t)ch, (uint16_t)code);
        ESP_LOGD(TAG, "ch%d tgt=%.3f ua=%.3f code=%ld",
                 ch, tgt, ua, (long)code);
    }
    ch = (ch + 1) & 0x03;
}

static const char *ch_name(int i)
{
    static const char *n[LIGHT_CH_NUM] = {"R", "G", "B", "IR"};
    return (i >= 0 && i < LIGHT_CH_NUM) ? n[i] : "?";
}

void light_status_str(char *buf, int len)
{
    if (!buf || len <= 0) {
        return;
    }
    int n = snprintf(buf, len,
                     "timeshare=%d loop=%d dark=%.4fV dac_fault=%d sensor_fault=%d\n",
                     (int)s_timeshare, (int)s_loop, s_dark_v,
                     (int)s_dac_fault, (int)s_sensor_fault);
    for (int i = 0; i < LIGHT_CH_NUM && n < len; i++) {
        n += snprintf(buf + n, len - n,
                      "  %-2s code=%4u ctrl=%.3fV I=%.1fmA tgt=%.2fuA gain=%.5f\n",
                      ch_name(i), s_code[i], mcp4728_code_to_volt(s_code[i]),
                      mcp4728_current_from_code(s_code[i]), s_tgt_ua[i], s_gain[i]);
    }
}

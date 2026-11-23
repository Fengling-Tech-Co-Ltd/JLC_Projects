/*
 * main.c — 积分盒四色光源板 固件入口（ESP8684 / ESP32-C2）
 *
 * 上电顺序（顺序不能乱）：
 *   1. NVS
 *   2. I2C 总线（GPIO0=SDA / GPIO1=SCL, 100kHz）
 *   3. 扫总线，期望 0x60(MCP4728) 与 0x48(ADS1110)
 *   4. light_init：Wake-Up -> 四路关断 -> 配 ADC -> 读暗底 -> 载标定
 *   5. 上电点灯：本版四路全部点亮（R/G/B/IR 各 15% / 50.0mA）
 *   6. 起控制台
 *   7. 1s 周期跑闭环
 */
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_console.h"
#include "app_light.h"
#include "bsp_i2c.h"
#include "light_cfg.h"

static const char *TAG = "app";

/* 上电点某一路：按目标电流点亮并把结果打进日志。
 * 必须在 light_init() 之后调用（它要先在灭灯状态采暗底）。 */
static void boot_light_ch(light_ch_t ch, float mA, const char *name)
{
    esp_err_t err = light_set_ma(ch, mA);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s LEDs ON: %.1f mA, code=%u (~%.1f mA by code)",
                 name, (double)mA,
                 (unsigned)light_get_code(ch),
                 (double)light_get_ma(ch));
    } else {
        ESP_LOGE(TAG, "%s LEDs ON failed: %s", name, esp_err_to_name(err));
    }
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(bsp_i2c_init());

    int n = bsp_i2c_scan();
    bool has_dac = (bsp_i2c_probe(MCP4728_ADDR) == ESP_OK);
    bool has_adc = (bsp_i2c_probe(ADS1110_ADDR) == ESP_OK);
    ESP_LOGI(TAG, "scan: %d device(s), dac(0x%02X)=%d adc(0x%02X)=%d",
             n, MCP4728_ADDR, (int)has_dac, ADS1110_ADDR, (int)has_adc);

    if (!has_dac) {
        ESP_LOGE(TAG, "MCP4728 不在线，调光不可用，检查排线与上拉");
    }

    light_init();

    /* 上电点灯：本版四路全部点亮（R/G/B/IR 各 15% / 50.0mA）。
     * 四路 R_SET 都是 0.30Ω，满量程 333.3mA；每路 4 颗灯珠串联，只能整路一起亮。
     * 必须在 light_init 之后（先采暗底），控制台启动之前点。 */
    if (LIGHT_BOOT_R_MA > 0.0f) {
        boot_light_ch(CH_R, LIGHT_BOOT_R_MA, "red");
    } else {
        ESP_LOGI(TAG, "boot red LEDs disabled (LIGHT_BOOT_R_MA=0)");
    }
    if (LIGHT_BOOT_G_MA > 0.0f) {
        boot_light_ch(CH_G, LIGHT_BOOT_G_MA, "green");
    } else {
        ESP_LOGI(TAG, "boot green LEDs disabled (LIGHT_BOOT_G_MA=0)");
    }
    if (LIGHT_BOOT_B_MA > 0.0f) {
        boot_light_ch(CH_B, LIGHT_BOOT_B_MA, "blue");
    } else {
        ESP_LOGI(TAG, "boot blue LEDs disabled (LIGHT_BOOT_B_MA=0)");
    }
    if (LIGHT_BOOT_IR_MA > 0.0f) {
        boot_light_ch(CH_IR, LIGHT_BOOT_IR_MA, "IR");
    } else {
        ESP_LOGI(TAG, "boot IR LEDs disabled (LIGHT_BOOT_IR_MA=0)");
    }

    console_start();

    ESP_LOGI(TAG, "ready. R/G/B/IR 已按上电默认点亮，可用 `set r|g|b|ir <mA>` / `off` 调整");

    /* 100ms 基础拍：监视 ADS1110 VIN+（mon on 时打印）；
     * 每 10 拍（1s）跑一次闭环任务。 */
    uint32_t slow_cnt = 0;
    while (1) {
        light_monitor_tick();
        if (++slow_cnt >= (LIGHT_TASK_PERIOD_MS / 100u)) {
            slow_cnt = 0;
            light_task();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

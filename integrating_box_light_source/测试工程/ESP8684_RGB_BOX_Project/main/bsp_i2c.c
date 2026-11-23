/*
 * bsp_i2c.c — I2C 主机板级封装
 *
 * 用 IDF 5.x 的 i2c_master 驱动（不是 legacy 的 i2c.h）：
 * 原生带事务超时，返回 esp_err_t，重试逻辑放在这一层，上层不用管。
 */
#include "bsp_i2c.h"

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "light_cfg.h"

static const char *TAG = "bsp_i2c";

static i2c_master_bus_handle_t s_bus;
static struct {
    uint8_t addr;
    i2c_master_dev_handle_t dev;
} s_dev[4];
static int s_dev_cnt;

esp_err_t bsp_i2c_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = LIGHT_I2C_PORT,
        .sda_io_num = LIGHT_I2C_SDA_IO,
        .scl_io_num = LIGHT_I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,             /* 同步事务，不用队列 */
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "new_master_bus failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "bus ready: SDA=IO%d SCL=IO%d %dHz",
             LIGHT_I2C_SDA_IO, LIGHT_I2C_SCL_IO, LIGHT_I2C_FREQ_HZ);
    return ESP_OK;
}

/* 按地址取 device handle，第一次用到时挂上去 */
static i2c_master_dev_handle_t dev_for(uint8_t addr7)
{
    for (int i = 0; i < s_dev_cnt; i++) {
        if (s_dev[i].addr == addr7) {
            return s_dev[i].dev;
        }
    }
    if (s_dev_cnt >= (int)(sizeof(s_dev) / sizeof(s_dev[0]))) {
        return NULL;
    }
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr7,
        .scl_speed_hz = LIGHT_I2C_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = i2c_master_bus_add_device(s_bus, &cfg, &dev);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "add_device 0x%02X failed: %s", addr7, esp_err_to_name(err));
        return NULL;
    }
    s_dev[s_dev_cnt].addr = addr7;
    s_dev[s_dev_cnt].dev = dev;
    s_dev_cnt++;
    return dev;
}

esp_err_t bsp_i2c_write(uint8_t addr7, const uint8_t *buf, size_t len)
{
    i2c_master_dev_handle_t dev = dev_for(addr7);
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = ESP_FAIL;
    for (int i = 0; i < LIGHT_I2C_RETRY; i++) {
        err = i2c_master_transmit(dev, buf, len, LIGHT_I2C_TIMEOUT_MS);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    ESP_LOGW(TAG, "write 0x%02X len=%u failed: %s",
             addr7, (unsigned)len, esp_err_to_name(err));
    return err;
}

esp_err_t bsp_i2c_read(uint8_t addr7, uint8_t *buf, size_t len)
{
    i2c_master_dev_handle_t dev = dev_for(addr7);
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = ESP_FAIL;
    for (int i = 0; i < LIGHT_I2C_RETRY; i++) {
        err = i2c_master_receive(dev, buf, len, LIGHT_I2C_TIMEOUT_MS);
        if (err == ESP_OK) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    ESP_LOGW(TAG, "read 0x%02X len=%u failed: %s",
             addr7, (unsigned)len, esp_err_to_name(err));
    return err;
}

esp_err_t bsp_i2c_write_read(uint8_t addr7, const uint8_t *wbuf, size_t wlen,
                             uint8_t *rbuf, size_t rlen)
{
    i2c_master_dev_handle_t dev = dev_for(addr7);
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return i2c_master_transmit_receive(dev, wbuf, wlen, rbuf, rlen,
                                       LIGHT_I2C_TIMEOUT_MS);
}

esp_err_t bsp_i2c_probe(uint8_t addr7)
{
    return i2c_master_probe(s_bus, addr7, LIGHT_I2C_TIMEOUT_MS);
}

int bsp_i2c_scan(void)
{
    int found = 0;
    for (uint8_t a = 0x08; a < 0x78; a++) {
        if (bsp_i2c_probe(a) == ESP_OK) {
            ESP_LOGI(TAG, "ACK @ 0x%02X", a);
            found++;
        }
    }
    if (found == 0) {
        ESP_LOGW(TAG, "总线上没有器件，查 4.7k 上拉与排线");
    }
    return found;
}

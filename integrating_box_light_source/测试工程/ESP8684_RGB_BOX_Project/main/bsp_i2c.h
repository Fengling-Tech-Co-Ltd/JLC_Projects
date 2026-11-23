/*
 * bsp_i2c.h — I2C 主机板级封装（ESP-IDF 5.x i2c_master 驱动）
 */
#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* 建总线。只需在 app_main 里调一次。 */
esp_err_t bsp_i2c_init(void);

/* 7 位地址语义，内部按需缓存 device handle；失败按 LIGHT_I2C_RETRY 重试 */
esp_err_t bsp_i2c_write(uint8_t addr7, const uint8_t *buf, size_t len);
esp_err_t bsp_i2c_read(uint8_t addr7, uint8_t *buf, size_t len);
esp_err_t bsp_i2c_write_read(uint8_t addr7, const uint8_t *wbuf, size_t wlen,
                             uint8_t *rbuf, size_t rlen);

/* 探测单个地址，返回 ESP_OK 表示有 ACK */
esp_err_t bsp_i2c_probe(uint8_t addr7);

/* 扫 0x08~0x77，逐条打印，返回发现的数量 */
int bsp_i2c_scan(void);

#endif /* BSP_I2C_H */

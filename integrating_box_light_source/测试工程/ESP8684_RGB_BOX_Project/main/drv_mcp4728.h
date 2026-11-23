/*
 * drv_mcp4728.h — 四路 12 位 DAC，I2C 0x60
 */
#ifndef DRV_MCP4728_H
#define DRV_MCP4728_H

#include <stdint.h>
#include "esp_err.h"
#include "light_cfg.h"

/* General Call Wake-Up（0x00, 0x09）：清掉四路的 power-down 位。
 * 器件上电时可能从 EEPROM 带着 PD 位起来，这时普通写虽然 ACK，输出脚却是关断的。 */
esp_err_t mcp4728_wakeup(void);

/* 只写 DAC 输入寄存器，不碰 EEPROM */
esp_err_t mcp4728_set(light_ch_t ch, uint16_t code);
esp_err_t mcp4728_set_all(uint16_t r, uint16_t g, uint16_t b, uint16_t ir);

/* 目标 LED 电流(mA) <-> DAC code */
uint16_t mcp4728_code_for_current(float mA);
float    mcp4728_current_from_code(uint16_t code);

/* code -> CTRL 电压(V)，调试用 */
float mcp4728_code_to_volt(uint16_t code);

#endif /* DRV_MCP4728_H */

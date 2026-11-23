/*
 * drv_mcp4728.c — 四路 12 位 DAC
 *
 * 命令字（MCP4728 手册 5.6.3 Multi-Write，只写输入寄存器、不碰 EEPROM）：
 *   第 1 字节 = 0x60 << 1 | W  （由 bsp_i2c 拼）
 *   第 2 字节 = 0 1 0 0 0 DAC1 DAC0 UDAC
 *            = 0x40 | (ch << 1) | UDAC
 *
 * 注意通道号在 bit2:1，不是在 bit1:0。写成 0x40|ch 会让 G/B/IR 三路
 * 落到错误的通道上（G 重复写 A、B 错位到 B、IR 重复写 B）。
 * LDAC# 已接地，输出立即更新，UDAC 位取 0。
 */
#include "drv_mcp4728.h"

#include "bsp_i2c.h"
#include "esp_log.h"

static const char *TAG = "mcp4728";

/* 通用呼叫地址 0x00 + 命令字 0x09 = General Call Wake-Up。
 * 数据手册 5.4：该命令清掉四路输入寄存器的 PD1/PD0，输出恢复。
 * 不改 EEPROM，也不改已有的 DAC code。 */
#define MCP4728_GC_ADDR          0x00u
#define MCP4728_GC_WAKEUP        0x09u

esp_err_t mcp4728_wakeup(void)
{
    uint8_t cmd = MCP4728_GC_WAKEUP;
    esp_err_t err = bsp_i2c_write(MCP4728_GC_ADDR, &cmd, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "general-call wake-up failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t mcp4728_set(light_ch_t ch, uint16_t code)
{
    if (code > DAC_CODE_MAX) {
        code = DAC_CODE_MAX;
    }
    uint8_t buf[3] = {
        (uint8_t)(0x40 | ((ch & 0x03) << 1)),
        (uint8_t)((code >> 8) & 0x0F),
        (uint8_t)(code & 0xFF),
    };
    esp_err_t err = bsp_i2c_write(MCP4728_ADDR, buf, sizeof(buf));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "set ch%d code=%u failed: %s",
                 (int)ch, code, esp_err_to_name(err));
    }
    return err;
}

esp_err_t mcp4728_set_all(uint16_t r, uint16_t g, uint16_t b, uint16_t ir)
{
    esp_err_t err = mcp4728_set(CH_R, r);
    if (err == ESP_OK) err = mcp4728_set(CH_G, g);
    if (err == ESP_OK) err = mcp4728_set(CH_B, b);
    if (err == ESP_OK) err = mcp4728_set(CH_IR, ir);
    return err;
}

uint16_t mcp4728_code_for_current(float mA)
{
    if (mA <= 0.0f) {
        return 0;
    }
    /* 1/10 满量程以下按文档公式会落到 0.1~0.4V，而 0.3V 以下 AL8862 关断、
     * 0.3~0.4V 手册没定义。这里一律钳到 10%（0.4V / 33.3mA），
     * 需要真正关断就传 0。 */
    float v = CTRL_MIN_V + (mA / I_FULL_MA - 0.1f) / 0.9f * (CTRL_MAX_V - CTRL_MIN_V);
    if (v < CTRL_MIN_V) v = CTRL_MIN_V;
    if (v > CTRL_MAX_V) v = CTRL_MAX_V;

    uint16_t code = (uint16_t)(v / DAC_VREF_V * 4096.0f + 0.5f);
    return (code > DAC_CODE_MAX) ? DAC_CODE_MAX : code;
}

float mcp4728_current_from_code(uint16_t code)
{
    float v = (float)code * DAC_VREF_V / 4096.0f;
    if (v < CTRL_OFF_V) {
        return 0.0f;
    }
    if (v > CTRL_MAX_V) {
        return I_FULL_MA;
    }
    return I_FULL_MA * (0.1f + 0.9f * (v - CTRL_MIN_V) / (CTRL_MAX_V - CTRL_MIN_V));
}

float mcp4728_code_to_volt(uint16_t code)
{
    return (float)code * DAC_VREF_V / 4096.0f;
}

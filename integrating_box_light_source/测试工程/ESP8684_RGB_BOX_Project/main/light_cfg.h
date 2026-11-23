/*
 * light_cfg.h — 积分盒四色光源板 固件常量
 *
 * 全部硬件相关数字集中在这个文件，换板只改这里。
 * 依据：AI_固件交接文档.md（2026-11，方案已审查）
 */
#ifndef LIGHT_CFG_H
#define LIGHT_CFG_H

/* ---------------- I2C 总线 ---------------- */
#define LIGHT_I2C_PORT          I2C_NUM_0
#define LIGHT_I2C_SDA_IO        0           /* GPIO0 */
#define LIGHT_I2C_SCL_IO        1           /* GPIO1 */
/* 100kHz：实测 400kHz 下 ADS1110 读操作大量 NACK、MCP4728 写入偶发丢数据
 * （表现为写成功但某个通道不变）。文档也建议长排线/大电容时降到 100kHz。 */
#define LIGHT_I2C_FREQ_HZ       100000
#define LIGHT_I2C_TIMEOUT_MS    50
#define LIGHT_I2C_RETRY         3

/* ---------------- 器件地址（7 位） ---------------- */
#define MCP4728_ADDR            0x60        /* A2/A1/A0 = 000 */
#define ADS1110_ADDR            0x48        /* A0 = 0 */

/* ---------------- MCP4728 / AL8862 ---------------- */
#define DAC_CODE_MAX            3102        /* 2.5V @ VREF=VDD=3.3V */
#define DAC_VREF_V              3.3f
#define I_FULL_MA               333.3f      /* 0.1V / R_SET(0.30Ω)，本板实际装 300mΩ */
#define CTRL_OFF_V              0.3f        /* 低于此值 AL8862 关断 */
#define CTRL_MIN_V              0.4f        /* 10% 电流 */
#define CTRL_MAX_V              2.5f        /* 100% 电流，再高只是钳位 */

/* ---------------- ADS1110 / 跨阻 ---------------- */
#define ADS1110_CFG_CONT       0x0C        /* 连续 / 15SPS / 增益1 */
#define ADS1110_CFG_ONESHOT    0x1C        /* 单次 / 15SPS / 增益1 */
#define ADS1110_CFG_START      0x80        /* 写 1 启动单次转换 */
#define ADS1110_FS_V           2.048f
#define ADS1110_SAT_RAW        32000       /* 接近满量程判饱和 */
#define TIA_OHM                 47000.0f   /* 跨阻 47kΩ */
#define ADS_AVG_N               8

/* ---------------- 应用 ---------------- */
typedef enum {
    CH_R = 0,
    CH_G = 1,
    CH_B = 2,
    CH_IR = 3,
} light_ch_t;

#define LIGHT_CH_NUM            4
#define LIGHT_SETTLE_MS         10         /* 设电流后等 ADC 前的稳定时间 */
#define LIGHT_TASK_PERIOD_MS    1000
#define LIGHT_DEFAULT_KP        2.0f       /* code / µA，实测再调 */
#define LIGHT_CAL_NAMESPACE     "light_cal"

/* 上电点灯：本版（2026-11-23）按要求**四路全部点亮**
 * （红 CH1 / 绿 CH2 / 蓝 CH3 / 红外 CH4，各 15% / 50.0mA）。
 * 四路 R_SET 都是 0.30Ω，满量程 333.3mA；50mA = 15%，CTRL 约 0.52V。
 * 实测 10%（33.3mA）这一档灯不亮，AL8862 在最小档起不来，所以默认取 15%。
 * 设 0 表示该路上电不点灯。调亮度改这里，或用控制台 `set r|g|b|ir <mA>`。
 * 红外不可见，不能靠眼睛判"灯亮"，要看光强板 BPW34S→OPA333→ADS1110 的
 * `meas ir` 读数是否从暗底明显上升。 */
#define LIGHT_BOOT_R_MA        50.0f
#define LIGHT_BOOT_G_MA        50.0f
#define LIGHT_BOOT_B_MA        50.0f
#define LIGHT_BOOT_IR_MA       50.0f

#endif /* LIGHT_CFG_H */

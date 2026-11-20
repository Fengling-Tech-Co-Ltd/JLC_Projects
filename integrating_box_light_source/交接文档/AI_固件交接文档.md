# 积分盒四色光源板 固件交接文档

> 面向另一个工作区的 AI/工程师，直接据此编写固件。
> 本文只覆盖「光模块功能板 / 积分盒光源」，不包含板号识别、步进电机、NTC 等其他项目。
> 硬件参数已经过审查，本文不修改硬件；只定义软件接口、寄存器、控制流程、标定和验收。

## 1. 系统概述

胶片翻拍用积分盒四色 LED 光源：

- 四路独立：R / G / B / IR，每路 4 颗 3535 灯珠串联。
- 恒流驱动：AL8862SP-13，滞环降压，`I_OUT = 0.1V / R_SET`。
- 调光：MCP4728 四路 12 位 DAC，CH1–CH4 → 四片 AL8862 的 CTRL，DC 模拟调光。
- 光强反馈：BPW34S → OPA333 跨阻（47kΩ ∥ 10pF）→ ADS1110 16 位 ADC。
- 电源：24V 输入 → PTC / TVS / SS54 → AL8862 和 LED；LM317 线性产生 3.3V 给 DAC、ADC、运放和 I2C 上拉。

信号流：

```
主机 I2C ──> MCP4728 CH1~CH4 ──> AL8862 CTRL ──> 4 串 LED
   │                                                    │
   └──<── ADS1110 <── OPA333 跨阻 <── BPW34S <── 积分球光
```

## 2. 关键文件与路径

| 文件 | 路径 |
| --- | --- |
| 项目根目录 | `D:\桌面\工作内容\私活\光模块功能板` |
| KiCad 工程 | `D:\桌面\工作内容\私活\光模块功能板\kicad_integrating_box_light_source-main\kicad_integrating_box_light_source-main\integrating_box_light_source.kicad_pro` |
| 设计说明 | 同目录 `README.md` |
| MCP4728 数据手册 | 同目录 `datasheets\MCP4728.pdf` |
| AL8862 数据手册 | 同目录 `datasheets\AL8862.pdf`、`AL8862_中文.pdf` |
| 立创 EDA 工程 | `D:\桌面\工作内容\私活\光模块功能板\JLC_integrating_box_light_source\JLC_Integrating_Sphere_Light_Source_2026-11-10.epro` |
| 可复用 ADS1110 驱动 | `D:\桌面\工作内容\KEIL原程序\fl1s_driver-main\HDL\ads1110\`（`ads1110.c/.h`，PY32 HAL） |

ADS1110 驱动来自另一个 PY32 工程，但配置字、读时序和电压换算与这块板一致，可以直接复用或移植。

## 3. 电气与接口摘要

### 3.1 I2C 总线

- 3.3V 逻辑，400kHz，7 位地址。
- 上拉 4.7kΩ，主板上已有，主机/传感器板不要重复加强上拉。
- 器件地址：
  - MCP4728：`0x60`（A2/A1/A0=000）。
  - ADS1110：`0x48`（A0=0；若传感器板地址脚不同，范围 0x48–0x4F）。
- 光强板是独立小板，通过 J3（3V3/GND/SDA/SCL）接到主板 J2/J4 的 I2C 总线。
- 如果排线较长、总线电容超过约 75pF，把 4.7k 降到 3.3k 或把速率降到 100kHz。

### 3.2 通道映射

| 逻辑通道 | MCP4728 通道 | DAC 命令字节 | 对应 AL8862 |
| --- | --- | --- | --- |
| R | CH1 / A | 0x40 | 红色驱动 |
| G | CH2 / B | 0x41 | 绿色驱动 |
| B | CH3 / C | 0x42 | 蓝色驱动 |
| IR | CH4 / D | 0x43 | 红外驱动 |

### 3.3 AL8862 CTRL

- `I_LED(100%) = 0.1V / R_SET`，设计值 R_SET=0.30Ω → 333mA。
- CTRL 0.4–2.5V 对应 10%–100%，线性。
- CTRL < 0.3V：关断。
- CTRL > 2.6V：钳位 100%，不要再往上加。
- 10% 电流约 33.3mA。

### 3.4 ADS1110

- 16 位 ΔΣ，增益 1，15SPS。
- 满量程 ±2.048V；单端 VIN+ 到 GND 最大 2.048V。
- LSB = 62.5µV。
- 跨阻 47kΩ：`I_ph = V_ADC / 47000`，1.33nA/LSB。
- 47k 时最大可测光电流 = 2.048V / 47k = 43.6µA。软件遇到接近满量程要报饱和，不要继续加大电流。
- BPW34S 典型响应度 0.6A/W；光功率 `P ≈ I_ph / 0.6`，实际按通道标定。

## 4. 软件分层

建议四层，平台无关：

```
app_light_source    通道管理、目标电流、标定表、闭环、故障
drv_mcp4728         0x60，四路 DAC
drv_ads1110         0x48，16 位 ADC
bsp_i2c             400kHz，7 位地址，读写超时/重试
```

### 4.1 建议接口

```c
typedef enum {
    CH_R = 0,
    CH_G = 1,
    CH_B = 2,
    CH_IR = 3
} light_ch_t;

void     light_init(void);
void     light_all_off(void);

void     mcp4728_set(light_ch_t ch, uint16_t code);
void     mcp4728_set_all(uint16_t r, uint16_t g, uint16_t b, uint16_t ir);

uint16_t code_for_current(float mA);
float    current_from_code(uint16_t code);
void     light_set_current(light_ch_t ch, float mA);

float    ads1110_read_voltage(void);
float    ads1110_read_photocurrent(void);
float    light_measure_ch(light_ch_t ch);

void     light_calibrate(void);
void     light_task_1s(void);
```

## 5. 上电初始化

```c
void light_init(void)
{
    i2c_init(400000);

    // 1. 四路先关断，避免 MCP4728 EEPROM 旧值直接点亮
    mcp4728_set_all(0, 0, 0, 0);

    // 2. ADS1110 连续 15SPS，增益 1
    ads1110_config(ADS1110_MODE_CONTINUOUS,
                   ADS1110_DR_15SPS,
                   ADS1110_PGA_1);

    HAL_Delay(100);  // 等第一次转换

    // 3. 读暗底，后续减掉
    s_dark = ads1110_read_voltage();
}
```

注意：

- MCP4728 的 LDAC# 已接地，DAC 写完立即更新，不需要 LDAC 脉冲或软件更新命令。
- RDY/BSY# 悬空；只写 DAC 输入寄存器，不写 EEPROM，就不用管它。
- 如果确实要写 EEPROM，单次/顺序写后必须等 25–50ms，且不要频繁写。
- 按 VREF=VDD、Gain=1 实现；如果上电读回不是这个配置，按 MCP4728 数据手册 5.6.5/5.6.6 配置。

## 6. MCP4728 驱动

### 6.1 写一个通道

7 位地址 `0x60`；多写命令字节 `0x40 | (ch & 0x03)`；数据为 12 位，高字节低 4 位 + 低字节。

```c
#define MCP4728_ADDR 0x60
#define DAC_CODE_MAX 3102   // 2.5V

void mcp4728_set(light_ch_t ch, uint16_t code)
{
    if (code > DAC_CODE_MAX) code = DAC_CODE_MAX;

    uint8_t cmd = 0x40 | (ch & 0x03);
    uint8_t data[2] = { (code >> 8) & 0x0F, code & 0xFF };

    i2c_write(MCP4728_ADDR, cmd, data, 2);
}
```

四路一起写：

```c
void mcp4728_set_all(uint16_t r, uint16_t g, uint16_t b, uint16_t ir)
{
    mcp4728_set(CH_R,  r);
    mcp4728_set(CH_G,  g);
    mcp4728_set(CH_B,  b);
    mcp4728_set(CH_IR, ir);
}
```

### 6.2 代码与电流

```c
#define I_FULL_MA 333.0f

uint16_t code_for_current(float mA)
{
    if (mA <= 0) return 0;

    float v = 0.4f + (mA / I_FULL_MA - 0.1f) / 0.9f * 2.1f;
    if (v > 2.5f) v = 2.5f;

    return (uint16_t)(v / 3.3f * 4096.0f + 0.5f);
}

float current_from_code(uint16_t code)
{
    float v = code * 3.3f / 4096.0f;

    if (v < 0.3f) return 0;
    if (v > 2.5f) return I_FULL_MA;

    return I_FULL_MA * (0.1f + 0.9f * (v - 0.4f) / 2.1f);
}
```

对应关系：

| 目标 | CTRL | DAC code | LED 电流 |
| --- | --- | --- | --- |
| 关断 | 0V | 0 | 0 |
| 10% | 0.4V | 497 | 33.3mA |
| 50% 电流 | 1.33V | 1655 | 166.5mA |
| 100% | 2.5V | 3102 | 333mA |

## 7. ADS1110 驱动

### 7.1 连续模式

配置字 `0x0C`：连续、15SPS、增益 1。

```c
float ads1110_read_voltage(void)
{
    uint8_t d[3];

    i2c_read(ADS1110_ADDR, d, 3);   // 2 字节数据 + 1 字节配置

    int16_t raw = (int16_t)((d[0] << 8) | d[1]);
    return raw / 32768.0f * 2.048f;
}

float ads1110_read_photocurrent(void)
{
    float v = ads1110_read_voltage() - s_dark;
    if (v < 0) v = 0;
    return v / 47000.0f;   // A
}
```

### 7.2 单次模式

- 先写 `0x1C` 配置（单次、15SPS、增益 1）。
- 再写 `0x9C` 启动转换。
- 等 85ms。
- 读 3 字节。

### 7.3 复用现有驱动

`fl1s_driver-main/HDL/ads1110` 已经实现：

- `ads1110_config()`
- `ads1110_single_read()`
- `ads1110_output2voltage()`

配置为 `ADS1110_MODE_CONTINUOUS / ADS1110_DR_15SPS / ADS1110_PGA_1` 即可；地址用 `ADS1110_ADDR_A0`（0x48）。

### 7.4 采样与滤波

- 15SPS 一次 66ms；建议 8–16 次平均。
- 上电先读暗底，后续每次减去。
- 如果 raw 接近 32767，判为饱和，降低电流或提示硬件量程不足。

## 8. 应用控制

### 8.1 两种工作模式

**A. R/G/B/IR 分时点亮（推荐）**

如果扫描流程是一路一路曝光，单颗光电二极管正好逐路闭环：

1. 关掉其他三路。
2. 目标通道设初始 code。
3. 等 10ms。
4. 读 ADC，算光电流。
5. `code += Kp × (target - measured)`，钳 0–3102。
6. 重复 3–5 次，收敛后记录。
7. 切下一路。

**B. 四路同时点亮**

单颗宽带 BPW34S 分不清通道，不能对四路同时做独立闭环。可行做法：

- 用标定表给四路前馈 code，保证 R/G/B/IR 比例。
- 光电二极管只做总光强闭环，四路 code 同比例微调。
- 需要分通道闭环时，只能分时测量，或换带滤光的颜色传感器。

### 8.2 闭环伪代码

```c
void light_task_1s(void)
{
    static uint8_t ch = 0;

    float target = s_target_iph[ch];
    float measured = light_measure_ch((light_ch_t)ch);

    float err = target - measured;
    int32_t delta = (int32_t)(err * s_kp[ch]);
    int32_t code = (int32_t)s_code[ch] + delta;

    if (code < 0) code = 0;
    if (code > DAC_CODE_MAX) code = DAC_CODE_MAX;

    mcp4728_set((light_ch_t)ch, (uint16_t)code);
    s_code[ch] = (uint16_t)code;

    ch = (ch + 1) & 0x03;
}
```

`light_measure_ch()` 内部要暂时关掉其他三路、读 ADC、再恢复；这一步会改变输出光，调用方要选择在曝光间隙或标定窗口执行。

### 8.3 标定流程

1. 四路全关，读 ADC，记录暗底 `s_dark`。
2. 单路点亮，设 50% 电流，读光电流，算该通道 `光电流/code` 增益。
3. 扫 10/25/50/75/100%，记录线性度；必要时做查表或二次拟合。
4. 依次标定 R/G/B/IR。
5. 标定结果存 MCU Flash，上电加载。
6. 正常工作时用标定表当前馈，慢速积分修正温漂。

## 9. 时序建议

| 操作 | 时间 |
| --- | --- |
| I2C 400kHz 写一个 DAC 通道 | 约 30µs |
| MCP4728 输出建立 | 6µs |
| AL8862 软启动 | 约 0.1ms |
| 设置电流后等待再读 ADC | 10ms |
| ADS1110 15SPS 一次转换 | 66ms |
| 8 次平均 | 约 0.5s |
| 四路轮询一轮 | 约 2s（含平均） |

对光强温漂，1–2s 一轮足够；不要用 1ms 任务阻塞等 ADC。

## 10. 任务调度

- 10ms：I2C 状态机、DAC 写队列、ADC 读取状态机。
- 100ms：单通道光电流采样、滤波。
- 1s：闭环调整、通道轮询、故障检测。
- 事件：设置目标电流/目标光强、开关通道、重新标定。

## 11. 故障处理

- I2C NACK/超时：重试 3 次，标记器件故障。
- DAC 写后读回配置，确认写入成功。
- ADC 接近满量程：报饱和，降低电流或提示硬件量程不足。
- 设了电流但光电流无响应：判 LED 开路 / AL8862 故障。
- 上电默认全关，避免 EEPROM 旧值点亮。
- 光强板未接或 I2C 无 ACK：报传感器故障，四路可降级为开环。

## 12. 调试与验收

1. I2C 扫描，应看到 `0x60` 和 `0x48`。
2. 四路 DAC 写 0，确认 LED 全灭，ADC 读暗底。
3. R 路设 50% 电流，量 CTRL 电压（约 1.33V）、LED 电流和光电流。
4. 依次测 G/B/IR。
5. 扫 code 497→3102，确认 ADC 不饱和。
6. 四路满载跑 10 分钟，看温漂和闭环收敛。
7. 拔掉光强板，确认故障检测和开环降级。

## 13. 待确认项

- 主机平台：PY32 / ESP32 / PC USB-I2C；I2C 引脚由另一个工作区确定。
- 工作方式：R/G/B/IR 分时，还是四路同时。
- 每路目标光电流或 R/G/B/IR 比例。
- 标定数据存哪里（MCU Flash / 上位机 / EEPROM）。
- 光强板排线长度、总线电容、是否需要降到 100kHz。
- 主板 J2/J4 与光强板 J3 的 1–4 脚线序，以原理图/丝印为准。

## 14. 不要做的事

- 不要频繁写 MCP4728 EEPROM；正常调光只写输入寄存器。
- 不要把 DAC code 超过 3102（2.5V）；AL8862 在 2.6V 以上只是钳位，不会更亮。
- 不要用一颗宽带光电二极管同时做四路独立闭环。
- 不要上电就依赖 MCP4728 EEPROM 默认值，先写全关。
- 不要在 I2C 总线上重复加强上拉；主板已有 4.7k。
- 不要自行修改硬件参数；软件按设计值实现，实测不符时报给硬件负责人。

## 15. 最小代码骨架

```c
#define MCP4728_ADDR  0x60
#define ADS1110_ADDR  0x48
#define DAC_CODE_MAX  3102
#define I_FULL_MA     333.0f

typedef enum { CH_R = 0, CH_G, CH_B, CH_IR } light_ch_t;

static uint16_t s_code[4];
static float    s_dark;
static float    s_target_iph[4];
static float    s_kp[4];

void light_init(void)
{
    i2c_init(400000);
    mcp4728_set_all(0, 0, 0, 0);
    ads1110_config(ADS1110_MODE_CONTINUOUS,
                   ADS1110_DR_15SPS,
                   ADS1110_PGA_1);
    HAL_Delay(100);
    s_dark = ads1110_read_voltage();
}

void light_set_current(light_ch_t ch, float mA)
{
    uint16_t code = code_for_current(mA);
    mcp4728_set(ch, code);
    s_code[ch] = code;
}

void light_all_off(void)
{
    mcp4728_set_all(0, 0, 0, 0);
    for (int i = 0; i < 4; i++) s_code[i] = 0;
}
```

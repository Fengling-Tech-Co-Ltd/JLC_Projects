# 步进电机驱动功能板（双轴）

PY32F030K28T6 主控 + 两片 DRV8825PWPR 的双轴步进电机驱动板，24 V 单电源输入，相电流 1.0~3.5 A 可调。含立创EDA 工程与一套已跑通的固件测试工程。

## 1. 硬件概览

| 项 | 值 |
|---|---|
| 主控 | PY32F030K28T6，Cortex-M0+，LQFP32，48 MHz，64 KB Flash / 8 KB SRAM |
| 驱动 | DRV8825PWPR × 2，ETSSOP-28 带散热焊盘 |
| 电机接口 | JST PH 2.0mm 4 芯卧插 × 2，额定 2 A |
| 输入 | 24 V DC，2 A 以上；PTC 自恢复保险 + 双向 TVS + 肖特基防反接 |
| 3.3 V | LM317MDT 线性稳压，带预负载 |
| 限流设定 | MCU 20 kHz PWM + RC 滤波，两轴独立 |
| 细分 | 全步（200 步/转），MODE0/1/2 硬件下拉固定 |
| 调试口 | 2×4 2.0mm 排针，含 SWD 与 USART1 |

## 2. 目录结构

```
设计文件/
  ProPrj_步进电机驱动功能板_2026-11-11.epro   立创EDA 专业版工程（当前依据）
  JLC_步进电机驱动功能板/README.md             硬件详细说明：引脚表、关键计算、器件清单
  step_motor_driver_with_hall_sensor/          带霍尔传感器的早期 KiCad 版本（非当前依据）
测试工程/
  PY32_Motor_Drive_Project/                    固件（PY32F030，Keil 工程）
    交接文档.md                                 硬件引脚、时钟、上电顺序、控制台命令
    User/ EWARM/ MDK-ARM/ 等
```

**硬件细节看 `设计文件/JLC_步进电机驱动功能板/README.md`**：里面有大板与小板的完整引脚对照、MCU↔DRV8825 网表、电机接口线序、定时器分工，以及更完整的计算过程。

## 3. 关键计算

| 项 | 值 | 依据 |
|---|---|---|
| 相电流设定 | `I_CHOP = VREF / (5 × Rcs)`，Rcs = 200 mΩ | 所以 VREF 每 1 V 对应 1 A |
| 电流范围 | 1.0 ~ 3.5 A，推荐工作点 1.5 A | 板级设计区间 |
| VREF PWM | TIM3，48 MHz，PSC=0，ARR=2399 → 20 kHz | `VREF = 3.3 × CCR / 2400` |
| CCR 换算 | `CCR = mA × 2400 / 3300` | 1.2 A → 873；1.5 A → 1091 |
| VREF RC | 10 kΩ / 1 µF，截止 15.9 Hz，建立约 30 ms | 设完电流必须等，否则电机猛冲 |
| STEP 频率 | `f = 48 MHz / ((PSC+1)(ARR+1))`，50% 占空比 | 上限 250 kHz（高低各 ≥1.9 µs） |
| 转速 | 全步 200 步/转，`RPM = 0.3 × f_STEP` | 1 kHz→300 rpm，10 kHz→3000 rpm |
| 3V3 | 3.28 V | `1.25 × (1 + 390/240)` |
| 母线储能 | 151 µF | 入口 57 µF + 每片驱动本地 47 µF |

定时器分工：TIM3 出两路 VREF（同频 20 kHz、占空比独立）；TIM16 出 STEP_1，TIM1 出 STEP_2。**STEP 与 VREF 不能共用定时器**，同一外设的通道共用 ARR。

## 4. 固件（测试工程）

`测试工程/PY32_Motor_Drive_Project`，详细说明见其 `交接文档.md`。当前状态：**两轴电机实测能转**。

- 时钟：HSI 24 M × PLL2 = 48 MHz；不能用 HSE（PF0/PF1 已作 USART，且无外部晶振）
- 上电顺序（不能调换）：nENBL 拉高禁用 → 写 VREF → 等 30 ms → 放开 nRESET → 等 10 µs → nENBL 拉低使能 → 才允许发 STEP
- 默认相电流 **1.2 A**（1.5 A 时结温约 121°C，散热余量不足）
- 控制台：USART1 115200 文本命令
- 两个自检宏：`PF1_TOGGLE_TEST`（PF1 当 GPIO 翻转，查走线）、`MOTOR_DEMO_ENABLE`（开机自动跑动作）

## 5. 已知设计与验证状态

**已完成**

- 5 张原理图（PWR / MCU / 驱动一 / 驱动二 / DEBUG）完成，引脚复用与外围参数已按手册核对
- 立创EDA 工程可打开，BOM 中的关键器件已选定
- 固件：双轴 STEP/DIR/ENBL、20 kHz VREF 调流、按步数运动与连续转、控制台均已实现，两轴实机转动验证通过

**未完成 / 已知取舍**

| 项 | 说明 |
|---|---|
| nFAULT | 只做上拉，未接回 MCU，**故障不可上报** |
| DECAY | 悬空 = 混合衰减，未做软件可选 |
| 满载散热 | 1.5 A 时结温约 121°C，固件默认降到 1.2 A；需实测底板温度 |
| PCB | 已绘制完成，**尚未打样** |
| 交付件 | BOM、Gerber 待补齐 |
| Rcs 封装功耗 | 200 mΩ 采样电阻需 2512 / 1 W / 1%，功耗待复核 |
| 霍尔版本 | `step_motor_driver_with_hall_sensor` 是早期 KiCad 变体，非当前打板依据 |

## 6. 打开方式

- **硬件**：立创EDA 专业版打开 `设计文件/ProPrj_步进电机驱动功能板_2026-11-11.epro`
  - 工程内网络名统一用下划线（`STEP_1` / `STEP_2`），立创EDA 会把带空格的同名网络当成两个网络
- **固件**：Keil MDK 打开 `测试工程/PY32_Motor_Drive_Project/MDK-ARM/Project.uvprojx`
- **参考手册**：DRV8825 及兼容型号（HR8825 / TMI8420 / A4988 / HR4988）的资料在本地数据手册库的电机驱动分类下

## 7. 提交到 Git 的建议

优先保留：`设计文件/*.epro`、`设计文件/JLC_步进电机驱动功能板/README.md`、`测试工程/PY32_Motor_Drive_Project`（源码 + 交接文档）。

建议**不要**提交：

| 排除项 | 原因 |
|---|---|
| `测试工程/.../MDK-ARM/Output/`、`Objects/`、`Listings/` | 编译产物，单是 Output 就 40 MB 量级 |
| `*.uvguix.*`、`JLinkLog.txt` | 本机调试配置与日志 |
| `step_motor_driver_with_hall_sensor/` 里的 `.history/`、`*backups/` | KiCad 本地历史 |

.gitignore 参考：

```gitignore
Output/
Objects/
Listings/
.history/
*backups/
*.uvguix.*
JLinkLog.txt
```

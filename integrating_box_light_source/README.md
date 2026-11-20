# 积分盒四色光源板（光模块功能板）

胶片翻拍用「积分盒」的四色 LED 光源板与配套资料。四路 R/G/B/IR 独立恒流驱动，DAC 模拟调光，光电二极管做光强反馈。

硬件有 KiCad 与立创EDA 专业版两套工程，固件在 ESP8684（ESP32-C2）上运行。

## 1. 系统构成

```
主机 I2C ──> MCP4728 CH1~CH4 ──> AL8862 CTRL ──> 四串 LED
   │                                                   │
   └──<── ADS1110 <── OPA333 跨阻 <── BPW34S <── 积分球光
```

- **四路独立**：R / G / B / IR，每路 4 颗 3535 灯珠串联，只能整路一起亮
- **恒流驱动**：AL8862SP-13，滞环降压，`I_OUT = 0.1 V / R_SET`，CTRL 0.4~2.5 V 模拟调光
- **调光**：MCP4728 四路 12 位 DAC（I2C 0x60），CH1~CH4 对应 R/G/B/IR
- **光强反馈**：BPW34S → OPA333 跨阻（47 kΩ ∥ 10 pF）→ ADS1110（16 位 ΔΣ ADC，I2C 0x48）
- **电源**：24 V 输入 → 保险丝 + TVS + SS54 防反接 → AL8862 与 LED；LM317 线性产生 3.3 V 给 DAC/ADC/运放（模拟电源用线性，避开开关噪声）

## 2. 仓库结构

| 路径 | 内容 |
|---|---|
| `设计文件/JLC_Integrating_Sphere_Light_Source_2026-11-10.epro` | **立创EDA 专业版工程（当前打板依据）** |
| `设计文件/SCH_积分匀光箱板_2026-11-20.pdf` | 当前原理图 PDF |
| `设计文件/JLC_integrating_box_light_source/` | 立创EDA 工程的归档副本（含 zip） |
| `设计文件/kicad_integrating_box_light_source-main/` | KiCad 工程：层次原理图、PCB、BOM、DXF、器件手册 |
| `交接文档/AI_固件交接文档.md` | 固件接口、寄存器、控制流程、标定与验收 |
| `测试工程/` | 预留 |

KiCad 子工程内部：

| 文件 | 内容 |
|---|---|
| `integrating_box_light_source.kicad_pro/.kicad_sch` | 顶层：J1(24V)、J2(I2C)、上拉、7 个层次图 |
| `power_protection.kicad_sch` | 24 V 输入防护 + LM317 3V3 |
| `led_cc_driver*.kicad_sch`（含 _g/_b/_ir） | 四路恒流驱动 |
| `dac_with_external_vref.kicad_sch` | MCP4728 四路 DAC |
| `light_sensor.kicad_sch` | 光强检测前端 |
| `datasheets/` | AL8862（中/英）、MCP4728 |
| `BOM_立创编号.csv` | 下单核对用 |

## 3. 硬件要点

| 项 | 值 / 说明 |
|---|---|
| I2C | 3.3 V 逻辑，7 位地址；主板 4.7 kΩ 上拉，传感器板不要重复加强上拉 |
| MCP4728 | 0x60（A2/A1/A0=000），VREF=VDD，LDAC# 接地（写完立即更新） |
| ADS1110 | 0x48（A0=0），连续 15 SPS、增益 1，LSB 62.5 µV |
| 光强板接口 | 独立小板，J3（3V3/GND/SDA/SCL）接到主板 J2/J4 |
| 排线较长时 | 总线电容超约 75 pF 就把上拉降到 3.3 kΩ 或把速率降到 100 kHz（固件实际按 100 kHz 跑） |

## 4. 关键计算

| 项目 | 值 | 依据 |
|---|---|---|
| LED 电流（100%） | **333 mA**（KiCad 设计，R_SET=0.30 Ω） | `I_OUT = 0.1 V / R_SET` |
| LED 电流（实物实测） | **200 mA**（立创版实装 R_SET=0.50 Ω） | 固件按 200 mA 满量程换算 |
| CTRL 映射 | 0.4~2.5 V ↔ 10%~100%；<0.3 V 关断；>2.6 V 钳位 | AL8862 手册 |
| DAC 满码 | 3102（2.5 V @ VREF = 3.3 V） | — |
| 电感 | 100 µH | 满载 570~700 kHz，手册推荐 33~100 µH |
| 3V3 | 3.28 V | `1.25 × (1 + 390/240)` |
| 跨阻 | 47 kΩ ∥ 10 pF，带宽 339 kHz | 1.33 nA/LSB，最大可测光电流 43.6 µA |

> **注意 R_SET 差异**：KiCad 版原理图按 0.30 Ω（333 mA）计算，立创版实装 0.50 Ω，实测满量程约 200 mA。两份资料不是同一版硬件，**以你手上的板子实物为准**，改版时统一。

## 5. 灯珠与立创编号

| 通道 | 位号 | 型号 | 立创编号 |
|---|---|---|---|
| 红 | LED_R1–R4 | XL-HD3535SURC-A2 | C3646952 |
| 绿 | LED_G1–G4 | JNJ-LEHG0113W120 | C19185875 |
| 蓝 | LED_B1–B4 | XL-HD3535UBC-A2 | C3646953 |
| 红外 | LED_IR1–IR4 | XL-HD3535HIR-T4A2 | C5221244 |
| 驱动 IC | U4–U7 | AL8862SP-13 | C526360 |

灯珠编号写在符号的 `LCSC Part` 字段里，`BOM_立创编号.csv` 由 KiCad 网表导出，可直接用于下单核对。

## 6. 打开方式

- **立创EDA 专业版**：打开 `.epro`，工程面板切换图页
- **KiCad 10**：打开 `integrating_box_light_source.kicad_pro`
  - `sym-lib-table` → `LCSC.kicad_sym`（AL8862 符号）
  - `fp-lib-table` → `integrating_box_light_source.pretty`（自建 3535 封装）
- **只看图**：直接开 `SCH_积分匀光箱板_2026-11-20.pdf`

## 7. 固件

固件工程在 ESP 侧：`F:\Boards_tests\ESP8684_design\ESP8684_RGB_BOX_Project`（独立仓库/工程，不在本目录内），自带 README 说明编译、烧录与控制台命令。

要点摘录：

| 项 | 值 |
|---|---|
| 芯片 | ESP8684（ESP32-C2），ESP-IDF v5.3.3 |
| I2C | GPIO0 = SDA，GPIO1 = SCL，**100 kHz**（400 kHz 实测不稳） |
| 控制台 | UART0 115200，命令：`scan/off/set/code/meas/tgt/loop/mode/cal/calsave/status/vin/mon` |
| 上电默认 | 只点亮红外 50 mA（25%）；R/G/B 关闭 |

固件侧的实测结论：10%（20 mA）档 AL8862 起不来，最低可用约 25%；红外不可见，判断点亮与否要看 `meas ir` 的读数。

## 8. 状态

**已完成**

- 原理图整机 ERC 0 error / 0 warning（KiCad 版）
- 立创EDA 版已打板，实板可点灯、可调光
- 固件：四路 DAC、光强采样、闭环与标定、控制台均已实现，编译烧录通过
- 红外通道实机点亮验证

**未完成 / 待确认**

- R/G/B 三路的实机点灯与光强对照尚未做
- 标定精度未与实测光功率对比；闭环未做长时间温漂记录
- R_SET 两个版本不一致（333 mA / 200 mA），需统一
- 其余阻容/连接器的立创编号待补齐
- PCB 布局在 KiCad 版中标注为"进行中"，最终以立创版为准

## 9. 提交到 Git 的建议

建议**不要**把下面这些提交进仓库，体积大且无信息增益：

| 排除项 | 原因 |
|---|---|
| `kicad_.../integrating_box_light_source-backups/` | 历史备份 zip，每个 140~210 KB |
| `kicad_.../.history/` | KiCad 本地历史 |
| `*.zip`（三个 5~6 MB 的外层压缩包） | 与解压后的工程重复 |
| `Imported integrating_box_light_source/` | Altium 导入的中间产物，非当前依据 |

建议保留：`.epro`、`.kicad_pro/.kicad_sch/.kicad_pcb`、`datasheets/`、`BOM_立创编号.csv`、DXF、`SCH_*.pdf`、两份交接文档。

.gitignore 参考：

```gitignore
*backups*/
.history/
*.zip
Imported */
```

# JLC_PROJECTS

硬件（立创EDA / KiCad）与配套固件的工程合集。每个子目录是一个独立项目，目录内自带 README 说明硬件要点、关键计算、固件用法与已知问题。

| 目录 | 项目 | 内容 |
|---|---|---|
| [`step_motor_driver/`](step_motor_driver/) | 双轴步进电机驱动板 | PY32F030K28T6 + DRV8825PWPR × 2，24 V 输入，含立创EDA 工程与已跑通的固件测试工程 |
| [`integrating_box_light_source/`](integrating_box_light_source/) | 积分盒四色光源板 | R/G/B/IR 四路恒流 LED，MCP4728 调光 + BPW34S/ADS1110 光强反馈，含 KiCad、立创EDA 与固件 |
| [`film_light_meter/`](film_light_meter/) | 胶片相机测光板 | PY32L020 + 双墨水屏 + OPT3001 光强 + QMA6100P 姿态，验证板；含方案文档与器件手册 |
| [`modular_slot_controller/`](modular_slot_controller/) | 模块插槽控制板 | 4 大板 × 4 小板 = 16 槽位；大板板号链与槽位编码，小板测温/液位/负载与金手指 I2C，含固件测试工程 |

## 阅读建议

每个子目录的 `README.md` 是入口：

- 硬件怎么接、怎么算 → 各项目 README 的"关键计算"一节
- 固件怎么编、怎么烧、怎么调 → 各项目的 `测试工程/` 与其 README
- 打板依据 → `设计文件/` 下的 `.epro`（立创EDA）或 `.kicad_pro`（KiCad）

## 说明

- 仓库只收录源工程、文档与固件源码；编译产物（`Output/`、`Objects/`、`Listings/`）、本机调试配置（`*.uvguix.*`）、
  本地历史（`.history/`、`*backups/`）与外层压缩包均不纳入
- 器件手册按用途归档在各项目目录内，文件名保留供应商原名，便于与采购规格书对照

# 模块插槽小板 固件测试工程（交接文档）

> 目标板：小板（模块板），PY32F002AF15P6TU，TSSOP20
> 系统：4 块大板 × 4 个小板 = 16 个槽位
> 依据：`D:\桌面\工作内容\私活\模块插槽控制板\固件测试工程_README.md` + 2026-11-20 小板原理图
> 状态更新：2026-11-20

## 1. 这个工程是什么

16 槽系统里小板的 **bring-up / 测试固件**，不是产品固件。上电后依次验证：时钟、串口、板号与槽位、液位、三路输出、本地 I2C + ADS1115 测温、金手指 I2C 从机。

固件只做"读状态、控输出、被主机查询"，业务逻辑留给后续产品固件。

## 2. 硬件与引脚

芯片 PY32F002AF15P6TU（DFP 名 `PY32F002Ax5`，20 KB Flash / 3 KB RAM），HSI 24 MHz 直出，**无 PLL、无 LSE**。

全部引脚定义集中在 [BSP_Drivers/Bsp_PinMap/Bsp_PinMap.h](BSP_Drivers/Bsp_PinMap/Bsp_PinMap.h)，其它文件只引用它。

| 引脚 | 功能 | 方向/电平 | 备注 |
|---|---|---|---|
| PA13 / PA14 | SWDIO / SWCLK | — | **不配置**，配成 GPIO 会失去调试口 |
| PF2 | NRST | — | **不配置** |
| PF1 | USART1_TX | AF8，115200 8N1 | 只发不收，H1 pin2 |
| PA5 | BOARD_NUM | ADC_IN5，12 位 | 必须 3.3V 参考，R-2R 分压 |
| PA6 / PB3 | BIT0 / BIT1 | 输入 | 槽位编码 |
| PA7 | WATER_LEVEL | 输入 | MCP6541 推挽，极性见 §7 |
| PB1 / PB2 / PB0 | VALVE / PUMP / MOS | 输出，高导通 | 低边 AO3400A |
| PA0 / PA1 | I2C2 SDA / SCL | 开漏，软件 I2C 100 kHz | 外部 4.7k（R4/R5） |
| PA2 / PA3 | I2C1 SDA / SCL | AF12，硬件从机 | 金手指总线 |
| PA4 / PF0 | 悬空 | 模拟输入 | 不驱动 |

**编号约定（容易搞混，务必注意）**

- `board_id` 是 **0 基**：0/1/2/3 = **板一/板二/板三/板四**
- `slot_id` 也是 **0 基**：0/1/2/3 = CN1/CN2/CN3/CN4 = **第 1/2/3/4 个槽位**
- `position = board_id*4 + slot_id + 1`，范围 1~16
- 从机地址 = `0x20 + position - 1`，即 0x20~0x2F
  - 基址用 0x20 而不是 0x10：板号方案 8.4 把 0x10~0x13 留给大板 MCU

## 3. 目录结构

```
User/                        应用层与框架
  main.c                     初始化顺序 + 测试模式主循环
  app_config.h               *** 测试模式与液位极性开关 ***
  app_boardid.c/.h           PA5 板号 + PA6/PB3 槽位 -> position
  app_ntc.c/.h               ADS1115 两次单次转换 + NTC 阻值/温度
  app_level.c/.h             PA7 液位，3 次确认
  app_host.c/.h              金手指 I2C 从机寄存器组与写命令
  py32f0xx_it.c/.h           仅异常与 SysTick（I2C1 中断入口在驱动里）
BSP_Drivers/
  Bsp_PinMap/                引脚定义（唯一来源）
  Bsp_Clock/                 HSI 24MHz
  Bsp_Gpio/                  悬空脚与开关量输入
  Bsp_Output/                三路输出，先写低再使能
  Bsp_Adc/                   ADC1 + PA5，失败返回 0xFFFF 并被平均跳过
  Bsp_Uart/                  PF1 TX，含 Bsp_Uart_Printf
  Bsp_I2C2_Soft/             PA0/PA1 软件 I2C + 9 脉冲恢复
  Bsp_I2C1_Slave/            PA2/PA3 硬件从机（Listen 模式）
tools/
  ntc_monitor.py             SWD 实时监视脚本（不占串口）
```

## 4. 初始化顺序（main.c）

1. `HAL_Init()`
2. `Bsp_Clock_Init()`：HSI 24 MHz
3. `Bsp_Gpio_SafeInit()` + `Bsp_Output_Init()`：**先写低再使能输出**，上电不误动作
4. `Bsp_Uart_Init()`
5. `Bsp_Adc_Init()`（含校准）
6. `Bsp_Gpio_InputInit()` → `App_Level_Init()` → `App_BoardId_Init()`
7. `App_Ntc_Init()`：软件 I2C + 总线扫描
8. 连读板号直到 `valid=1`（最多 10 次），再用 position 定从机地址并 `App_Host_Init()`
9. 进入 `APP_TEST_MODE` 对应的主循环

## 5. 测试模式

改 [User/app_config.h](User/app_config.h) 里的 `APP_TEST_MODE` 后重新编译。小板没有 USART_RX，不能用命令行。

| 模式 | 行为 |
|---|---|
| `TEST_MODE_SELF`（默认） | 每秒打印板号/槽位/液位/NTC/输出状态，并刷新从机寄存器 |
| `TEST_MODE_OUTPUT` | 三路输出轮流 500 ms 开 / 500 ms 关（负载断开做静态测试） |
| `TEST_MODE_SENSOR` | 位置 + 液位 + NTC 原始码/阻值/温度 |
| `TEST_MODE_HOST` | 只维护从机寄存器组，2 秒打印一次地址与温度 |
| `TEST_MODE_POSITION` | 只刷位置号 |

串口输出里 NTC 状态：`OK` / `OPEN` / `SHORT` / `I2CERR`。

## 6. 金手指 I2C 从机协议（最小实现，待主机确认）

- 地址：`0x20 + position - 1`
- **写**：固定 2 字节 `[寄存器号, 值]`
  - `0x10` 输出掩码：bit0 VALVE、bit1 PUMP、bit2 MOS（1 = 导通）
  - `0x11` 任意值 = 三路全关
- **读**：固定回 8 字节寄存器组

| 偏移 | 内容 |
|---|---|
| 0x00 | position（1~16） |
| 0x01 | board_id（低 4 位） \| slot_id（高 4 位） |
| 0x02 | 状态位：bit0 液位(1=湿)、bit1 valve、bit2 pump、bit3 mos、bit4 pos_valid |
| 0x03 | 温度低字节（int16，0.1℃） |
| 0x04 | 温度高字节 |
| 0x05 | NTC 状态：0 OK / 1 OPEN / 2 SHORT / 3 I2C 错 |
| 0x06 | 板号 ADC 原始码低字节 |
| 0x07 | 板号 ADC 原始码高字节 |

实现要点（都在 [Bsp_I2C1_Slave.c](BSP_Drivers/Bsp_I2C1_Slave/Bsp_I2C1_Slave.c) 里注释过）：

- `HAL_I2C_EnableListen_IT` 不会开 NVIC，`MspInit` 里自己开 `I2C1_IRQn`
- `HAL_I2C_Slave_Receive_IT/Transmit_IT` 要求 READY 状态，LISTEN 下必然 BUSY；必须用 `Slave_Seq_*_IT`
- `OwnAddress1` 要传 7 位地址**左移一位**，HAL 按 `OAR1[7:1]` 掩码写入
- 读事务发送前做快照，避免主机读到半新半旧的寄存器；位置变化只在总线空闲时重初始化

## 7. 液位极性

方案文档 2.3 定义：**干=高、湿=低**。

[User/app_config.h](User/app_config.h) 里 `APP_LEVEL_DRY_IS_HIGH` 控制极性，当前为 **1（干=高）**，与 MCP6541 比较器一致。

历史：2026-11-20 曾有一块板 J3 悬空时读到低电平，临时把宏改成 0；后来那块板干烧报废，在用的板读到高电平，已改回 1。换板后若干湿颠倒，只改这一行并重编译。

## 8. NTC 测温

ADS1115（0x48，ADDR 接地）挂在软件 I2C 上：

| 用途 | 配置字 | MUX |
|---|---|---|
| d = V(AIN0−AIN1)，NTC 两端电压 | `0x8383` | AIN0-AIN1 |
| b = V(AIN1−GND)，分压下臂电压 | `0xD383` | AIN1-GND |

```
R = 33000 × d / b
T = 1/(1/298.15 + ln(R/10000)/3950) − 273.15
```

开路：d 接近满量程、b≈0 → `OPEN`；短路：d≈0、b>3.1 V（>25000 码）→ `SHORT`。

## 9. 工具：实时监视脚本

```bat
D:\Python\python.exe tools\ntc_monitor.py              :: 一直刷
D:\Python\python.exe tools\ntc_monitor.py --count 5    :: 刷 5 次
```

走 SWD 轮询（halt → 读 RAM → resume），**不占串口**，也不影响固件运行。符号地址从 `MDK-ARM\Output\Project.map` 自动解析。

显示：

```
时间         板      槽位 NTC           温度    液位
18:32:01   板1      4 OPEN          --     干   (board_id=0 slot_id=3 adc=0 valid=1)
```

注意事项：

- 同一颗探针**一次只能跑一个 pyocd 进程**，窗口开着时不要再从命令行读或烧录
- 烧录前先关窗口，否则会互相抢探针

## 10. 编译与烧录

```bat
:: 编译（全量重编）
"H:\Keil_5_MDK524\STM32\UV4\UV4.exe" -r "<本目录>\MDK-ARM\Project.uvprojx" -j0 -o build.log
```

- Target 名 `PY32F002A_Project`，器件 `PY32F002Ax5`
- **优化等级必须是 -O2**：-O0 时镜像 20784 字节，超出 20480 上限；-O2 后 15484 字节
- 烧录（探针 UID 用 `pyocd list` 现场查，不写进仓库）：

```bat
D:\Python\python.exe -m pyocd flash ^
  --pack "H:\Keil_5_MDK524\STM32\ARM\PACK\.Download\Puya.PY32F0xx_DFP.1.2.5.pack" ^
  -u <唯一ID> -t py32f002ax5 -f 100000 -M under-reset ^
  "<本目录>\MDK-ARM\Output\Project.hex"
```

通过判据：`Erased N bytes (M sectors), programmed K bytes (P pages)`，`P > 0`；若全是 `identical` 表示板上已是同一版。

## 11. 已验证 / 未验证

**已在实机验证**

| 项 | 结果 |
|---|---|
| 时钟、SWD、烧录 | 正常，烧录后可运行 |
| 板号 ADC | 板一读 0~141 码，板四档读到 4068 码，分档正确 |
| 槽位 | CN1~CN4 依次读出 slot_id 0/1/2/3，位置 1/2/3/4 |
| 从机地址 | 随位置变化 0x20→0x21→0x23→0x2F，重初始化链路正常 |
| 软件 I2C | SCL 补上拉后 ADS1115 有应答（状态从 I2CERR 变为 OPEN/SHORT） |
| 液位 | 空载读"干"（当前板，极性宏=1） |

**未验证 / 待硬件**

| 项 | 说明 |
|---|---|
| NTC 实际温度 | 两块板 J5 空载都报 SHORT 或 OPEN，尚未接 5.751 kΩ 标准电阻验 38℃ |
| T11 主机读写 | 从机已实现，但还没有主机侧脚本跑过完整读写 |
| 三路带载 | 只做过静态电平，未接 12V/24V 负载 |
| 液位湿态 | 未接电极验证"湿" |

## 12. 已知问题与注意事项

1. **J5 空载读数在部分板上异常**：曾出现 d≈0、b≈3.2 V（判 SHORT），而电路上开路应当是 d 高、b≈0。已确认不是固件问题（配置字与分压公式都核对过），怀疑是 J5 短接或 PCB 上 AIN0/AIN1 反接。换板后若复现，按 §8 的判据先用万用表量 J5。
2. **液位极性因板而异**：见 §7。
3. **软件 I2C 速率靠 NOP 校准**：`BSP_I2C2_DELAY_LOOPS = 20`，24 MHz 下约 4~5 µs 半周期。换编译器优化等级或主频会变，建议示波器确认。
4. **板号判决依赖 3.3 V 参考**，不能切内部参考。
5. **槽位 BIT0/BIT1 电阻未贴时可能浮空**（原理图 Value 为空），读数不稳属硬件待补项。
6. **串口只有 TX**：需要命令行走线把 PF0 接到 H1 pin3，并改 `UART_MODE_TX_RX`。

## 13. 下一步

1. J5 接 5.751 kΩ，确认状态变 OK、温度 ≈38.0℃，完成 T8/T9。
2. J3 接电极或短接，确认"湿"，完成 T10。
3. 主机侧写一个扫描/读写脚本，验证 T11（地址 0x20~0x2F、8 字节寄存器组、2 字节写）。
4. 三路输出接 12 V / 24 V 负载，做带载测试 T3。

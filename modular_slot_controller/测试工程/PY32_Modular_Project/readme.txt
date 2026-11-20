================================================================================
              模块插槽小板 固件测试工程（PY32F002AF15P6TU）
================================================================================

一、这是什么
  16 槽系统里的小板（槽位模块）bring-up 固件。只做测试验证，不含产品逻辑。
  规格依据：D:\桌面\工作内容\私活\模块插槽控制板\固件测试工程_README.md

  上电后依次：时钟 24MHz -> 输出安全关断 -> 串口横幅 -> ADC -> 软件 I2C
  -> ADS1115 -> 液位 -> 板号/槽位 -> 金手指 I2C 从机 -> 按测试模式进主循环。

二、工程结构（沿用 PY32 工程族约定）
  User\                        应用层与框架
    main.c                     初始化顺序 + 测试模式主循环
    app_config.h               *** 测试模式选择：APP_TEST_MODE ***
    app_boardid.c/.h           PA5 板号 ADC + PA6/PB3 槽位 -> position
    app_ntc.c/.h               ADS1115 两次单次转换 + NTC 阻值/温度
    app_level.c/.h             PA7 液位，3 次确认
    app_host.c/.h              金手指 I2C 从机寄存器组与写命令
    py32f0xx_it.c/.h           仅异常与 SysTick；I2C1 事件走 HAL 回调
  BSP_Drivers\                 板级驱动
    Bsp_PinMap\Bsp_PinMap.h    全部引脚定义（唯一引脚来源）
    Bsp_Clock\                 HSI 24MHz
    Bsp_Gpio\                  悬空脚模拟输入、开关量输入
    Bsp_Output\                PB1/PB2/PB0，先写低再使能
    Bsp_Adc\                   ADC1 + PA5（ADC_IN5）
    Bsp_Uart\                  PF1 USART1_TX，115200，只发
    Bsp_I2C2_Soft\             PA0/PA1 软件 I2C
    Bsp_I2C1_Slave\            PA2/PA3 硬件 I2C1 从机（Listen 模式）
  CMSIS\  PY32F0xx_HAL_Driver\  EWARM\  MDK-ARM\   与空工程一致

三、引脚（详见 Bsp_PinMap.h）
  PA13/PA14  SWDIO/SWCLK   不配置
  PF2        NRST          不配置
  PF1        USART1_TX     AF8，115200 8N1，只发
  PA5        BOARD_NUM     ADC_IN5，12 位，3.3V 参考
  PA6        BIT0          槽位 bit0
  PB3        BIT1          槽位 bit1
  PA7        WATER_LEVEL   干高湿低
  PB1/PB2/PB0  VALVE/PUMP/MOS  高导通
  PA0/PA1    I2C2 SDA/SCL  软件 I2C，外部 4.7k
  PA2/PA3    I2C1 SDA/SCL  硬件从机，AF12
  PA4/PF0    悬空           配模拟输入

四、测试模式（改 app_config.h 一行后重新编译）
  TEST_MODE_SELF      0  自检：每秒打印板号/槽位/液位/NTC/输出状态
  TEST_MODE_OUTPUT    1  三路输出轮流 500ms 开 / 500ms 关（先断开负载做 T2）
  TEST_MODE_SENSOR    2  位置 + 液位 + NTC 原始码/阻值/温度
  TEST_MODE_HOST      3  只跑金手指从机，打印当前地址
  TEST_MODE_POSITION  4  只刷位置号，方便插拔四个槽位
  小板 H1 没有 USART_RX，不能用命令行；需要命令行走线把 PF0 接到 H1 pin3。

五、金手指 I2C 从机协议（最小实现，等主机方确认）
  从机地址 = 0x20 + position - 1（0x20~0x2F），position = board_id*4 + slot_id + 1（1~16）
  基址用 0x20：板号方案 8.4 把 0x10~0x13 留给大板 MCU，小板不能占用

  命名对照（别搞混）：board_id 是 0 基，对外说"板 N"时 N = board_id + 1
    board_id 0 = 板一    board_id 1 = 板二
    board_id 2 = 板三    board_id 3 = 板四
  串口/寄存器里看到的 board 字段是 0 基值，监视脚本已按"板一/板二"显示。

  槽位同样是 0 基：slot_id 0/1/2/3 = CN1/CN2/CN3/CN4 = 第 1/2/3/4 个槽位。
  监视脚本的"槽位"列已换成 1 基，括号里保留原始 slot_id。
  写事务：固定 2 字节 [寄存器号, 值]
    0x10  输出掩码 bit0=VALVE bit1=PUMP bit2=MOS（1=导通）
    0x11  任意值 = 三路全关
  读事务：固定回 8 字节寄存器组
    0x00 position
    0x01 board_id | (slot_id << 4)
    0x02 状态位 bit0 液位(1=湿) bit1 valve bit2 pump bit3 mos bit4 pos_valid
    0x03 温度低字节（int16，0.1℃）
    0x04 温度高字节
    0x05 NTC 状态 0=OK 1=开路 2=短路 3=I2C 错
    0x06 板号 ADC 原始码低字节
    0x07 板号 ADC 原始码高字节

六、上板验证顺序（对应 README 6.4）
  T1  上电 + SWD 连接 + 烧录           看串口是否出横幅
  T2  三路输出静态（负载断开）          TEST_MODE_OUTPUT，量栅极/连接器
  T4  板号 0~3                         插不同大板或加模拟电压
  T5  槽位 0~3                         插 CN1~CN4
  T6  位置 1~16                        TEST_MODE_POSITION
  T7  I2C2 + ADS1115                   TEST_MODE_SELF 里看 ntc=I2CERR 或 OK
  T8  NTC 38℃（J5 接 5.751k）          R≈5.75k，T≈38.0℃
  T9  NTC 开路/短路                    串口显示 OPEN / SHORT
  T10 液位                             干 DRY / 湿 WET
  T11 金手指 I2C                       主机按第五节读写
  T12 串口 TX                          115200 收日志
  T13 反复上下电                       输出默认关断，位置稳定

七、已知限制
  1. 软件 I2C 半周期用 NOP 循环（BSP_I2C2_DELAY_LOOPS=20），24MHz 下约 4~5µs，
     实际速率请用示波器确认；偏快调大、偏慢调小。
  2. 从机写事务必须正好 2 字节，其它长度不处理（协议未定前的最小实现）。
  3. 位置变化时会用新地址重启从机（Bsp_I2C1_Slave_Init），重连瞬间主机扫描可能
     漏掉一帧，属正常。
  4. 板号判决依赖 3.3V 参考；若换成内部参考，比例测量会失效。
  5. 槽位 BIT0/BIT1 未贴电阻时可能浮空，读数不稳定，属硬件待补项。

八、编译
  Keil MDK 打开 MDK-ARM\Project.uvprojx，Target = PY32F002A_Project。
  命令行全量编译：
    "H:\Keil_5_MDK524\STM32\UV4\UV4.exe" -r "<工程目录>\MDK-ARM\Project.uvprojx" -j0 -o build.log

================================================================================

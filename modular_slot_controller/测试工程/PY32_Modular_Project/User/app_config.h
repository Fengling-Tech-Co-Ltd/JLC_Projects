/**
  ******************************************************************************
  * @file    app_config.h
  * @brief   测试模式选择（小板 H1 没有 USART_RX，命令行不可用，只能编译期选）
  *
  * 需要 UART 命令行时：把 PF0 接到 H1 pin3，改成 USART_RX，再把模式换成
  * TEST_MODE_UART，并在 bsp_usart 里加接收。
  ******************************************************************************
  */
#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#define TEST_MODE_SELF      0   /* 自检：周期打印板号/槽位/液位/输出状态 */
#define TEST_MODE_OUTPUT    1   /* 三路输出轮流开关，测 T2/T3 */
#define TEST_MODE_SENSOR    2   /* 板号/槽位/NTC/液位连续打印，测 T4~T10 */
#define TEST_MODE_HOST      3   /* 只跑金手指 I2C 从机，测 T11 */
#define TEST_MODE_POSITION  4   /* 只读位置，测 T6 */

/* ==== 当前测试模式：改这一行 ==== */
#define APP_TEST_MODE       TEST_MODE_SELF

/* 自检/T2 输出脉冲时长 */
#define APP_OUTPUT_PULSE_MS 500u

/* 板号采样参数 */
#define APP_BOARDID_AVG_N       8u      /* 每次平均次数（文档 8~16） */
#define APP_BOARDID_CONFIRM     3u      /* 连续同档次数 */
#define APP_BOARDID_SETTLE_MS   20u     /* 上电后等 R-2R 稳定 */

/* 液位确认次数（文档 3~5） */
#define APP_LEVEL_CONFIRM       3u

/* 液位极性：1 = 干态为高电平（与 MCP6541 比较器输出极性一致，方案文档 2.3）
 *           0 = 干态为低电平（仅当某块板比较器输入反接时才用）
 *
 * 2026-11-20：先前那块读数相反的板已干烧报废，当前在用的板 J3 悬空时
 * PA7 为高，与比较器"干=高"一致，按 1 跑。
 * 换板后若干湿颠倒，只改这一行并重编译。 */
#define APP_LEVEL_DRY_IS_HIGH   1

#endif /* __APP_CONFIG_H */

/**
  ******************************************************************************
  * @file    main.c
  * @brief   模块插槽小板 固件测试工程（PY32F002AF15P6TU）
  *
  * 初始化顺序按《固件测试工程_README.md》6.2 节：
  *   时钟 → GPIO 安全态 → 输出先关断 → 串口 → ADC → 软件 I2C → ADS1115
  *   → 输入/液位 → 板号槽位 → 金手指 I2C 从机 → 主循环
  * 测试模式在 app_config.h 里用 APP_TEST_MODE 选（小板没有 USART_RX）。
  ******************************************************************************
  */
#include "main.h"

#include "app_config.h"
#include "Bsp_Adc/Bsp_Adc.h"
#include "Bsp_Clock/Bsp_Clock.h"
#include "Bsp_Gpio/Bsp_Gpio.h"
#include "Bsp_Output/Bsp_Output.h"
#include "Bsp_PinMap/Bsp_PinMap.h"
#include "Bsp_Uart/Bsp_Uart.h"
#include "app_boardid.h"
#include "app_host.h"
#include "app_level.h"
#include "app_ntc.h"

static void app_print_banner(void)
{
    Bsp_Uart_Printf("\r\n==== slot module test fw ====\r\n");
    Bsp_Uart_Printf("chip=PY32F002A  clk=HSI 24MHz  build=%s %s\r\n", __DATE__, __TIME__);
    Bsp_Uart_Printf("mode=%d\r\n", (int)APP_TEST_MODE);
}

/* NTC 温度转成 0.1℃ 的整数，供寄存器组和打印共用 */
static int16_t app_temp_dc(const App_Ntc_t *n)
{
    if (n->status != NTC_OK)
    {
        return 0;
    }
    return (int16_t)(n->temp_c * 10.0f);
}

static const char *app_ntc_state_str(App_Ntc_Status_t s)
{
    switch (s)
    {
    case NTC_OK:      return "OK";
    case NTC_OPEN:    return "OPEN";
    case NTC_SHORT:   return "SHORT";
    default:          return "I2CERR";
    }
}

/* ---------------- 自检：打印全部状态并刷新从机寄存器 ---------------- */
static void app_loop_self(void)
{
    App_Position_t pos = App_BoardId_Read();
    App_Level_t    lvl = App_Level_Read();
    App_Ntc_t      ntc = App_Ntc_Read();
    int16_t        tdc = app_temp_dc(&ntc);

    App_Host_Refresh(pos.position, pos.board_id, pos.slot_id, pos.valid,
                     (lvl == APP_LEVEL_WET) ? 1u : 0u, &tdc, (uint8_t)ntc.status,
                     pos.adc_raw);

    Bsp_Uart_Printf("adc=%4u board=%u slot=%u pos=%2u valid=%u level=%s "
                    "ntc=%s R=%.0f T=%.1fC\r\n",
                    (unsigned)pos.adc_raw, (unsigned)pos.board_id, (unsigned)pos.slot_id,
                    (unsigned)pos.position, (unsigned)pos.valid,
                    (lvl == APP_LEVEL_DRY) ? "DRY" : "WET",
                    app_ntc_state_str(ntc.status), (double)ntc.r_ntc, (double)ntc.temp_c);
    HAL_Delay(1000);
}

/* ---------------- 只读位置 ---------------- */
static void app_loop_position(void)
{
    App_Position_t pos = App_BoardId_Read();
    Bsp_Uart_Printf("adc=%4u board=%u slot=%u pos=%2u valid=%u addr=0x%02X\r\n",
                    (unsigned)pos.adc_raw, (unsigned)pos.board_id,
                    (unsigned)pos.slot_id, (unsigned)pos.position, (unsigned)pos.valid,
                    (unsigned)App_Host_Address(pos.position));
    App_Host_Refresh(pos.position, pos.board_id, pos.slot_id, pos.valid, 0u, NULL, 0u, pos.adc_raw);
    HAL_Delay(500);
}

/* ---------------- 三路输出轮流开关（负载先断开做 T2） ---------------- */
static void app_loop_output(void)
{
    for (Bsp_Output_Id_t id = OUT_VALVE; id <= OUT_MOS; id++)
    {
        Bsp_Output_Set(id, 1);
        Bsp_Uart_Printf("%s ON\r\n", Bsp_Output_Name(id));
        HAL_Delay(APP_OUTPUT_PULSE_MS);

        Bsp_Output_Set(id, 0);
        Bsp_Uart_Printf("%s OFF\r\n", Bsp_Output_Name(id));
        HAL_Delay(APP_OUTPUT_PULSE_MS);
    }
}

/* ---------------- 传感器：位置 + 液位 + NTC ---------------- */
static void app_loop_sensor(void)
{
    App_Position_t pos = App_BoardId_Read();
    App_Level_t    lvl = App_Level_Read();
    App_Ntc_t      ntc = App_Ntc_Read();
    int16_t        tdc = app_temp_dc(&ntc);

    App_Host_Refresh(pos.position, pos.board_id, pos.slot_id, pos.valid,
                     (lvl == APP_LEVEL_WET) ? 1u : 0u, &tdc, (uint8_t)ntc.status,
                     pos.adc_raw);

    Bsp_Uart_Printf("pos=%2u level=%s ntc=%s d=%d b=%d R=%.0f T=%.1fC\r\n",
                    (unsigned)pos.position,
                    (lvl == APP_LEVEL_DRY) ? "DRY" : "WET",
                    app_ntc_state_str(ntc.status),
                    (int)ntc.raw_vntc, (int)ntc.raw_vb,
                    (double)ntc.r_ntc, (double)ntc.temp_c);
    HAL_Delay(500);
}

/* ---------------- 金手指从机：只维护寄存器组 ---------------- */
static void app_loop_host(void)
{
    App_Position_t pos = App_BoardId_Read();
    App_Level_t    lvl = App_Level_Read();
    App_Ntc_t      ntc = App_Ntc_Read();
    int16_t        tdc = app_temp_dc(&ntc);

    App_Host_Refresh(pos.position, pos.board_id, pos.slot_id, pos.valid,
                     (lvl == APP_LEVEL_WET) ? 1u : 0u, &tdc, (uint8_t)ntc.status,
                     pos.adc_raw);

    /* 负温度要单独取符号：C 的整数除法向零取整，-5/10 会得 0，符号会丢 */
    {
        int32_t tv = tdc;
        char    sign = '+';
        if (tv < 0)
        {
            sign = '-';
            tv = -tv;
        }
        Bsp_Uart_Printf("host ready: addr=0x%02X pos=%u temp=%c%ld.%ld\r\n",
                        (unsigned)App_Host_Address(pos.position), (unsigned)pos.position,
                        sign, (long)(tv / 10), (long)(tv % 10));
    }
    HAL_Delay(2000);
}

int main(void)
{
    HAL_Init();

    /* README 6.2：时钟最先，后面 HAL_Delay 才按 24MHz 计时 */
    Bsp_Clock_Init();

    /* 紧接着进安全态：不用的脚不驱动，输出先拉低再使能 */
    Bsp_Gpio_SafeInit();
    Bsp_Output_Init();

    Bsp_Uart_Init();
    app_print_banner();

    Bsp_Adc_Init();
    Bsp_Gpio_InputInit();
    App_Level_Init();
    App_BoardId_Init();
    App_Ntc_Init();

    /* 位置要连续 3 次同档才有效，地址又依赖位置：
     * 这里最多读 10 次（每次间隔 10ms）等到 valid，再用位置定从机地址。
     * 否则 board_id=1~3 的板会先用错误的 0x20 起来，主机上电扫描会读错。 */
    App_Position_t pos = App_BoardId_Read();
    for (uint8_t i = 0; i < 10u && !pos.valid; i++)
    {
        HAL_Delay(10);
        pos = App_BoardId_Read();
    }
    App_Host_Init(pos.position);
    Bsp_Uart_Printf("boot pos: adc=%u board=%u slot=%u pos=%u slave_addr=0x%02X\r\n",
                    (unsigned)pos.adc_raw, (unsigned)pos.board_id, (unsigned)pos.slot_id,
                    (unsigned)pos.position, (unsigned)App_Host_Address(pos.position));

    while (1)
    {
        switch (APP_TEST_MODE)
        {
        case TEST_MODE_POSITION: app_loop_position(); break;
        case TEST_MODE_OUTPUT:   app_loop_output();   break;
        case TEST_MODE_SENSOR:   app_loop_sensor();   break;
        case TEST_MODE_HOST:     app_loop_host();     break;
        case TEST_MODE_SELF:
        default:                 app_loop_self();     break;
        }
    }
}

void APP_ErrorHandler(void)
{
    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
    while (1)
    {
    }
}
#endif /* USE_FULL_ASSERT */

/*
 * app_console.c — UART0 命令行
 *
 * 命令：
 *   scan            扫 I2C 总线
 *   off             四路全关
 *   set <ch> <mA>   按 LED 电流设置（ch: 0-3 或 r/g/b/ir）
 *   code <ch> <n>   直接给 DAC code（0-3102）
 *   meas [ch]       测光电流，省略 ch 则四路轮测
 *   tgt <ch> <uA>   闭环目标光电流
 *   loop on|off     闭环开关
 *   mode a|b        a=分时（默认）, b=同时
 *   cal             四路 50% 标定
 *   calsave         标定写入 NVS
 *   status          当前状态
 */
#include "app_console.h"

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_light.h"
#include "bsp_i2c.h"
#include "drv_mcp4728.h"
#include "light_cfg.h"

static const char *TAG = "console";

static int parse_ch(const char *s)
{
    if (s == NULL || s[0] == '\0') {
        return -1;
    }
    if (s[1] == '\0' && s[0] >= '0' && s[0] <= '3') {
        return s[0] - '0';
    }
    switch (s[0]) {
    case 'r': case 'R': return CH_R;
    case 'g': case 'G': return CH_G;
    case 'b': case 'B': return CH_B;
    case 'i': case 'I': return CH_IR;
    default: return -1;
    }
}

/* 打印时带上硬件通道号，避免把内部枚举 CH_IR=3 误读成"第三个/绿色"。
 * 逻辑枚举 R=0/G=1/B=2/IR=3，对应 MCP4728 CH1/CH2/CH3/CH4。 */
static const char *ch_label(int ch)
{
    static const char *n[LIGHT_CH_NUM] = {"R(CH1)", "G(CH2)", "B(CH3)", "IR(CH4)"};
    return (ch >= 0 && ch < LIGHT_CH_NUM) ? n[ch] : "?";
}

static int cmd_scan(int argc, char **argv)
{
    (void)argc; (void)argv;
    int n = bsp_i2c_scan();
    printf("found %d device(s)\n", n);
    return 0;
}

static int cmd_off(int argc, char **argv)
{
    (void)argc; (void)argv;
    light_all_off();
    printf("all off\n");
    return 0;
}

static int cmd_set(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: set <ch> <mA>\n");
        return 1;
    }
    int ch = parse_ch(argv[1]);
    float mA = 0.0f;
    if (ch < 0 || sscanf(argv[2], "%f", &mA) != 1) {
        printf("bad args\n");
        return 1;
    }
    if (light_set_ma((light_ch_t)ch, mA) != ESP_OK) {
        printf("i2c write failed\n");
        return 1;
    }
    printf("%s -> %.1f mA (code=%u, ctrl=%.3fV)\n", ch_label(ch), mA,
           light_get_code((light_ch_t)ch),
           (double)mcp4728_code_to_volt(light_get_code((light_ch_t)ch)));
    return 0;
}

static int cmd_code(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: code <ch> <0-3102>\n");
        return 1;
    }
    int ch = parse_ch(argv[1]);
    int code = -1;
    if (ch < 0 || sscanf(argv[2], "%d", &code) != 1 || code < 0 || code > DAC_CODE_MAX) {
        printf("bad args\n");
        return 1;
    }
    if (light_set_code((light_ch_t)ch, (uint16_t)code) != ESP_OK) {
        printf("i2c write failed\n");
        return 1;
    }
    printf("%s code=%d ctrl=%.3fV\n", ch_label(ch), code,
           (double)mcp4728_code_to_volt((uint16_t)code));
    return 0;
}

static int cmd_meas(int argc, char **argv)
{
    int ch = (argc >= 2) ? parse_ch(argv[1]) : -1;
    if (argc >= 2 && ch < 0) {
        printf("bad channel\n");
        return 1;
    }
    if (argc < 2) {
        for (int i = 0; i < LIGHT_CH_NUM; i++) {
            float ua = 0.0f;
            bool sat = false;
            esp_err_t err = light_measure_ua((light_ch_t)i, &ua, &sat);
            if (err != ESP_OK) {
                printf("%s: err %s\n", ch_label(i), esp_err_to_name(err));
            } else {
                printf("%s: %.3f uA%s\n", ch_label(i), (double)ua, sat ? " [饱和]" : "");
            }
        }
        return 0;
    }
    float ua = 0.0f;
    bool sat = false;
    esp_err_t err = light_measure_ua((light_ch_t)ch, &ua, &sat);
    if (err != ESP_OK) {
        printf("err %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("%s: %.3f uA%s\n", ch_label(ch), (double)ua, sat ? " [饱和]" : "");
    return 0;
}

static int cmd_tgt(int argc, char **argv)
{
    if (argc < 3) {
        printf("usage: tgt <ch> <uA>\n");
        return 1;
    }
    int ch = parse_ch(argv[1]);
    float ua = 0.0f;
    if (ch < 0 || sscanf(argv[2], "%f", &ua) != 1) {
        printf("bad args\n");
        return 1;
    }
    light_set_target_ua((light_ch_t)ch, ua);
    printf("%s target %.3f uA\n", ch_label(ch), (double)ua);
    return 0;
}

static int cmd_loop(int argc, char **argv)
{
    if (argc < 2) {
        printf("loop is %s\n", light_get_loop() ? "on" : "off");
        return 0;
    }
    bool on = (strcmp(argv[1], "on") == 0);
    light_set_loop(on);
    printf("loop %s\n", on ? "on" : "off");
    return 0;
}

static int cmd_mode(int argc, char **argv)
{
    if (argc < 2) {
        printf("mode is %s\n", light_get_timeshare() ? "a (分时)" : "b (同时)");
        return 0;
    }
    bool ts = (strcmp(argv[1], "a") == 0);
    light_set_timeshare(ts);
    printf("mode %s\n", ts ? "a (分时)" : "b (同时)");
    return 0;
}

static int cmd_cal(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("calibrating, 四路各 50%% 电流...\n");
    if (light_calibrate() != ESP_OK) {
        printf("calibrate failed\n");
        return 1;
    }
    printf("done, 用 calsave 保存\n");
    return 0;
}

static int cmd_calsave(int argc, char **argv)
{
    (void)argc; (void)argv;
    if (light_cal_save() != ESP_OK) {
        printf("save failed\n");
        return 1;
    }
    printf("saved\n");
    return 0;
}

static int cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;
    char buf[640];
    light_status_str(buf, sizeof(buf));
    printf("%s", buf);
    return 0;
}

static int cmd_vin(int argc, char **argv)
{
    (void)argc; (void)argv;
    int16_t raw = 0;
    uint8_t cfg = 0;
    float v = 0.0f;
    esp_err_t err = light_read_sensor_oneshot(&raw, &cfg, &v);
    if (err != ESP_OK) {
        printf("ADS1110 VIN+ read failed: %s\n", esp_err_to_name(err));
        return 1;
    }
    printf("ADS1110 raw=%d (0x%04X) cfg=0x%02X VIN+=%.4f V\n",
           raw, (unsigned)(uint16_t)raw, (unsigned)cfg, (double)v);
    return 0;
}

static int cmd_mon(int argc, char **argv)
{
    if (argc < 2) {
        printf("mon is %s\n", light_get_monitor() ? "on" : "off");
        return 0;
    }
    bool on = (strcmp(argv[1], "on") == 0);
    light_monitor_set(on);
    printf("monitor %s\n", on ? "on (100ms, [VIN] lines)" : "off");
    return 0;
}

esp_err_t console_start(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "light>";
    repl_cfg.max_cmdline_length = 128;

    esp_console_dev_uart_config_t uart_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_cfg, &repl_cfg, &repl));

    esp_console_register_help_command();

    const esp_console_cmd_t cmds[] = {
        {"scan",    "扫描 I2C 总线",        NULL, cmd_scan,    NULL},
        {"off",     "四路全关",             NULL, cmd_off,     NULL},
        {"set",     "set <ch> <mA>",        NULL, cmd_set,     NULL},
        {"code",    "code <ch> <n>",        NULL, cmd_code,    NULL},
        {"meas",    "meas [ch]",            NULL, cmd_meas,    NULL},
        {"tgt",     "tgt <ch> <uA>",        NULL, cmd_tgt,     NULL},
        {"loop",    "loop on|off",          NULL, cmd_loop,    NULL},
        {"mode",    "mode a|b",             NULL, cmd_mode,    NULL},
        {"cal",     "四路 50% 标定",        NULL, cmd_cal,     NULL},
        {"calsave", "标定存 NVS",           NULL, cmd_calsave, NULL},
        {"status",  "打印状态",             NULL, cmd_status,  NULL},
        {"vin",     "读 ADS1110 VIN+ 电压",  NULL, cmd_vin,     NULL},
        {"mon",     "mon on|off 连续监视",   NULL, cmd_mon,     NULL},
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }

    esp_err_t err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "start_repl failed: %s", esp_err_to_name(err));
        return err;
    }
    /* 不要在这里硬编码波特率：实际值由 Kconfig 决定（本板 esp32c2+26MHz
     * 时是 74880，见 sdkconfig.defaults 的说明）。 */
    ESP_LOGI(TAG, "控制台就绪（UART0 %d）", CONFIG_ESP_CONSOLE_UART_BAUDRATE);
    return ESP_OK;
}

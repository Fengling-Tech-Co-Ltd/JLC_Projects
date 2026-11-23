#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"

/*
 * ESP8684 空工程模板（最简）
 * - 原为 INA226 OCV 电池容量测试，检测程序已全部移除
 * - OLED (SSD1306) 驱动代码也已移除
 * - 仅保留 I2C 总线接线/地址配置与初始化，主循环空转
 * - 上电把 GPIO6 配成推挽输出并输出低（用途待明确，见 gpio_out_init 的注释）
 */

/* ======================== I2C 主机配置 ======================== */
#define I2C_MASTER_NUM          I2C_NUM_0       /* 使用 I2C0 控制器 */
#define I2C_MASTER_SDA_IO       0               /* SDA 引脚 = GPIO0 */
#define I2C_MASTER_SCL_IO       1               /* SCL 引脚 = GPIO1 */
#define I2C_MASTER_FREQ_HZ      100000          /* I2C 时钟 100kHz (标准模式) */
#define I2C_MASTER_TIMEOUT_MS   100             /* I2C 通信超时 100ms */

/* 初始化 ESP8684 I2C 主机 (SDA/SCL 引脚 + 内部上拉 + 时钟频率) */
static void i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,              /* 主机模式 */
        .sda_io_num = I2C_MASTER_SDA_IO,      /* SDA 引脚 */
        .scl_io_num = I2C_MASTER_SCL_IO,      /* SCL 引脚 */
        .sda_pullup_en = GPIO_PULLUP_ENABLE,  /* 启用 SDA 内部上拉 (建议外部加 4.7kΩ) */
        .scl_pullup_en = GPIO_PULLUP_ENABLE,  /* 启用 SCL 内部上拉 (建议外部加 4.7kΩ) */
        .master.clk_speed = I2C_MASTER_FREQ_HZ, /* I2C 时钟频率 */
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/* ======================== GPIO6 输出 ======================== */
/* 初始化指定引脚为推挽输出，并输出给定电平(0/1)。
 *
 * 注意：ESP32-C2 上 GPIO5/GPIO6/GPIO7 的复位默认功能是 MTDI/MTCK/MTDO(JTAG)，
 * 并非 GPIO，复位后引脚状态由芯片决定，不受本代码控制。因此必须“先写输出
 * 锁存器、再使能输出”，否则使能输出的瞬间引脚会先驱动锁存器的旧值，产生
 * 一个毛刺。 */
static void gpio_out_init(gpio_num_t pin, int level)
{
    /* 1) 先写输出锁存器。此时引脚还没使能输出，写值只改锁存器，
     *    等下一步使能输出时引脚直接驱动目标电平，不会出现旧值毛刺。 */
    gpio_set_level(pin, level);

    /* 2) 再切到 GPIO 功能并配置为推挽输出（关内部上拉/下拉，不用中断） */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),          /* 选中目标引脚 */
        .mode         = GPIO_MODE_OUTPUT,       /* 推挽输出 */
        .pull_up_en   = GPIO_PULLUP_DISABLE,    /* 关掉 MTCK(JTAG) 的默认上拉 */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  /* 不用内部下拉 */
        .intr_type    = GPIO_INTR_DISABLE,      /* 不用中断 */
    };
    gpio_config(&io_conf);

    /* 3) 兜底再确认一次目标电平 */
    gpio_set_level(pin, level);
}

/* 程序入口：初始化 I2C 主机后空转 */
void app_main(void)
{
    printf("ESP8684 blank template starting... (SDA=IO%d, SCL=IO%d)\n",
           I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);

    /* GPIO6 输出低。用途暂时没有引脚分配表可查，先按“输出低”处理。 */
    gpio_out_init(GPIO_NUM_6, 0);
    printf("GPIO%d initialized as output LOW.\n", (int)GPIO_NUM_6);

    i2c_master_init();
    printf("I2C initialized.\n");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  /* 空转 */
    }
}

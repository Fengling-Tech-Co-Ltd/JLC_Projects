/*
 * app_console.h — UART0 命令行（esp_console）
 */
#ifndef APP_CONSOLE_H
#define APP_CONSOLE_H

#include "esp_err.h"

/* 注册命令并启动 REPL，需在 light_init 之后调用 */
esp_err_t console_start(void);

#endif /* APP_CONSOLE_H */

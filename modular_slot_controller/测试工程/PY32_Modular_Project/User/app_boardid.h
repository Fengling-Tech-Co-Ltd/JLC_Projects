/**
  ******************************************************************************
  * @file    app_boardid.h
  * @brief   板号(PA5 ADC) + 槽位(PA6/PB3) → 位置号
  ******************************************************************************
  */
#ifndef __APP_BOARDID_H
#define __APP_BOARDID_H

#include "main.h"

/* 命名对照（务必别混）：
 *   board_id = 0  →  板一（第 1 块大板）
 *   board_id = 1  →  板二
 *   board_id = 2  →  板三
 *   board_id = 3  →  板四
 * 代码里是 0 基，对外一律说"板 N" = board_id + 1。
 * position = board_id*4 + slot_id + 1 用的仍是 0 基 board_id。
 *
 * 槽位同理，slot_id 也是 0 基：
 *   slot_id = 0 → CN1（槽位 1）   slot_id = 1 → CN2（槽位 2）
 *   slot_id = 2 → CN3（槽位 3）   slot_id = 3 → CN4（槽位 4）
 * 对外说"第 N 个槽位"时 N = slot_id + 1。 */
typedef struct
{
    uint8_t  board_id;   /* 0~3，来自 PA5 电压分档；0 = 板一 */
    uint8_t  slot_id;    /* 0~3，BIT1<<1 | BIT0 */
    uint8_t  position;   /* 1~16 = board_id*4 + slot_id + 1 */
    uint16_t adc_raw;    /* 最近一次平均后的 12 位码 */
    uint8_t  valid;      /* 1 = 连续 APP_BOARDID_CONFIRM 次同档 */
} App_Position_t;

void            App_BoardId_Init(void);
App_Position_t  App_BoardId_Read(void);

/* 把 12 位码判成 0~3（门限 512 / 1536 / 2560），供测试打印 */
uint8_t         App_BoardId_Classify(uint16_t raw);

#endif /* __APP_BOARDID_H */

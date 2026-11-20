/**
  ******************************************************************************
  * @file    app_host.h
  * @brief   金手指 I2C 从机的寄存器组与应用层维护
  *
  * 寄存器组（只读，主机一次读 8 字节）：
  *   0x00  position (1~16)
  *   0x01  board_id | (slot_id << 4)
  *   0x02  状态位：bit0 液位(1=湿) bit1 valve bit2 pump bit3 mos bit4 pos_valid
  *   0x03  NTC 温度低字节（int16，单位 0.1℃，有符号）
  *   0x04  NTC 温度高字节
  *   0x05  NTC 状态：0 OK / 1 开路 / 2 短路 / 3 I2C 错
  *   0x06  板号 ADC 原始码低字节
  *   0x07  板号 ADC 原始码高字节
  *
  * 写入（固定 2 字节 [寄存器号, 值]）：
  *   0x10  输出掩码：bit0 valve，bit1 pump，bit2 mos（高=导通）
  *   0x11  任意写入 = 全部关断
  ******************************************************************************
  */
#ifndef __APP_HOST_H
#define __APP_HOST_H

#include "main.h"

#define HOST_REG_NUM          8u
#define HOST_REG_POSITION     0x00u
#define HOST_REG_IDS          0x01u
#define HOST_REG_STATUS       0x02u
#define HOST_REG_TEMP_L       0x03u
#define HOST_REG_TEMP_H       0x04u
#define HOST_REG_NTC_STATE    0x05u
#define HOST_REG_ADC_L        0x06u
#define HOST_REG_ADC_H        0x07u
#define HOST_CMD_OUTPUT_MASK  0x10u
#define HOST_CMD_ALL_OFF      0x11u

/* 由固件算出的从机地址：0x20 + position - 1（0x20~0x2F） */
uint8_t App_Host_Address(uint8_t position);

/* 启动/重启从机（位置变化时地址要跟着变） */
void App_Host_Init(uint8_t position);

/* 每轮刷新寄存器组内容 */
void App_Host_Refresh(uint8_t position, uint8_t board_id, uint8_t slot_id,
                      uint8_t pos_valid, uint8_t level_wet,
                      const int16_t *temp_dc, uint8_t ntc_state, uint16_t adc_raw);

/* 主机写入后的落点（Bsp_I2C1_OnWrite 调用） */
void App_Host_OnWrite(uint8_t reg, uint8_t value);

void App_Host_AllOff(void);

#endif /* __APP_HOST_H */

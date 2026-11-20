/**
  ******************************************************************************
  * @file    app_host.c
  * @brief   从机寄存器组与写命令落地
  ******************************************************************************
  */
#include "app_host.h"
#include "Bsp_I2C1_Slave/Bsp_I2C1_Slave.h"
#include "Bsp_Output/Bsp_Output.h"

static uint8_t s_regs[HOST_REG_NUM];
static uint8_t s_cur_addr;

uint8_t App_Host_Address(uint8_t position)
{
    if (position == 0u)
    {
        position = 1u;
    }
    return (uint8_t)(BSP_I2C1_ADDR_BASE + position - 1u);
}

void App_Host_Init(uint8_t position)
{
    for (uint8_t i = 0; i < HOST_REG_NUM; i++)
    {
        s_regs[i] = 0;
    }
    s_regs[HOST_REG_POSITION] = position;

    Bsp_I2C1_Slave_SetTxBuf(s_regs, HOST_REG_NUM);
    s_cur_addr = App_Host_Address(position);
    Bsp_I2C1_Slave_Init(s_cur_addr);
}

void App_Host_Refresh(uint8_t position, uint8_t board_id, uint8_t slot_id,
                      uint8_t pos_valid, uint8_t level_wet,
                      const int16_t *temp_dc, uint8_t ntc_state, uint16_t adc_raw)
{
    /* 位置变了就要换地址：主板是按位置扫的，地址不对就是"没插好"。
     * 但重初始化会关 PE，不能打断正在进行的事务，所以只在监听空闲时做。 */
    uint8_t want = App_Host_Address(position);
    if (want != s_cur_addr && Bsp_I2C1_Slave_IsIdle())
    {
        Bsp_I2C1_Slave_SetTxBuf(s_regs, HOST_REG_NUM);
        s_cur_addr = want;
        Bsp_I2C1_Slave_Init(s_cur_addr);
    }

    s_regs[HOST_REG_POSITION] = position;
    s_regs[HOST_REG_IDS]      = (uint8_t)((board_id & 0x0Fu) | ((slot_id & 0x0Fu) << 4));

    uint8_t st = 0;
    st |= (level_wet ? 1u : 0u) << 0;
    st |= (Bsp_Output_Get(OUT_VALVE) ? 1u : 0u) << 1;
    st |= (Bsp_Output_Get(OUT_PUMP) ? 1u : 0u) << 2;
    st |= (Bsp_Output_Get(OUT_MOS) ? 1u : 0u) << 3;
    st |= (pos_valid ? 1u : 0u) << 4;
    s_regs[HOST_REG_STATUS] = st;

    int16_t t = (temp_dc != NULL) ? *temp_dc : 0;
    s_regs[HOST_REG_TEMP_L]   = (uint8_t)((uint16_t)t & 0xFFu);
    s_regs[HOST_REG_TEMP_H]   = (uint8_t)(((uint16_t)t >> 8) & 0xFFu);
    s_regs[HOST_REG_NTC_STATE] = ntc_state;
    s_regs[HOST_REG_ADC_L]    = (uint8_t)(adc_raw & 0xFFu);
    s_regs[HOST_REG_ADC_H]    = (uint8_t)((adc_raw >> 8) & 0xFFu);
}

/* 输出状态位在写命令后立刻同步，否则主机写完马上回读会看到旧值 */
static void host_sync_output_bits(void)
{
    uint8_t st = s_regs[HOST_REG_STATUS];
    st &= (uint8_t)~0x0Eu;                                     /* 清 valve/pump/mos 三位 */
    st |= (uint8_t)((Bsp_Output_Get(OUT_VALVE) ? 1u : 0u) << 1);
    st |= (uint8_t)((Bsp_Output_Get(OUT_PUMP) ? 1u : 0u) << 2);
    st |= (uint8_t)((Bsp_Output_Get(OUT_MOS) ? 1u : 0u) << 3);
    s_regs[HOST_REG_STATUS] = st;
}

void App_Host_OnWrite(uint8_t reg, uint8_t value)
{
    switch (reg)
    {
    case HOST_CMD_OUTPUT_MASK:
        Bsp_Output_Set(OUT_VALVE, (value & 0x01u) ? 1u : 0u);
        Bsp_Output_Set(OUT_PUMP,  (value & 0x02u) ? 1u : 0u);
        Bsp_Output_Set(OUT_MOS,   (value & 0x04u) ? 1u : 0u);
        host_sync_output_bits();
        break;

    case HOST_CMD_ALL_OFF:
        Bsp_Output_AllOff();
        host_sync_output_bits();
        break;

    default:
        /* 只读寄存器，写入忽略 */
        break;
    }
}

void App_Host_AllOff(void)
{
    Bsp_Output_AllOff();
}

/* Bsp_I2C1_Slave 的回调落点：收到 2 字节后按 [reg, value] 解析 */
void Bsp_I2C1_OnWrite(const uint8_t *buf, uint16_t len)
{
    if (buf == NULL || len < 2u)
    {
        return;
    }
    App_Host_OnWrite(buf[0], buf[1]);
}

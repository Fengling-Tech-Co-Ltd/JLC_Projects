/**
  ******************************************************************************
  * @file    Bsp_I2C1_Slave.c
  * @brief   金手指 I2C1 从机（PA2/PA3，AF12，开漏，Listen 模式）
  *
  * 三个必须遵守的 HAL 细节（都已在 py32f0xx_hal_i2c.c 里核对）：
  *   1) HAL_I2C_EnableListen_IT 只在 State==READY 时生效，且**不会**开 NVIC，
  *      所以 MspInit 里要自己 HAL_NVIC_EnableIRQ，并实现 I2C1_IRQHandler。
  *   2) 老式 HAL_I2C_Slave_Receive_IT/Transmit_IT 要求 State==READY，
  *      在 LISTEN 状态下必然返回 HAL_BUSY；必须用 Slave_Seq_*_IT。
  *   3) OAR1 是按 I2C_OAR1_ADD1_7(0xFE) 掩码写入的，OwnAddress1 要传 7 位地址
  *      左移一位，直接传 7 位会让实际地址变成一半。
  *
  * 帧约定（主机协议未定前的最小实现）：
  *   写：固定 2 字节 [寄存器号, 值]
  *   读：固定回 8 字节寄存器组（发送前做一次快照，避免主机读到半新半旧的数据）
  ******************************************************************************
  */
#include "Bsp_I2C1_Slave.h"
#include "Bsp_PinMap/Bsp_PinMap.h"

#include <string.h>

static I2C_HandleTypeDef s_hi2c1;
static uint8_t  s_rx[2];
static uint8_t *s_tx;                 /* 应用提供的寄存器组 */
static uint16_t s_tx_len;
static uint8_t  s_snap[HOST_TX_MAX];  /* 读事务用的快照 */
static uint16_t s_snap_len;

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef gpio = {0};

    if (hi2c->Instance != I2C1)
    {
        return;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_I2C_CLK_ENABLE();

    gpio.Mode      = GPIO_MODE_AF_OD;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = PIN_I2C1_AF;

    gpio.Pin = PIN_I2C1_SDA_PIN;
    HAL_GPIO_Init(PIN_I2C1_PORT, &gpio);
    gpio.Pin = PIN_I2C1_SCL_PIN;
    HAL_GPIO_Init(PIN_I2C1_PORT, &gpio);

    /* PY32F0xx I2C 勘误：引脚切复用后复位一次，避免 BUSY 位被锁死 */
    __HAL_RCC_I2C_FORCE_RESET();
    __HAL_RCC_I2C_RELEASE_RESET();

    /* Listen 模式只置外设中断位，NVIC 要自己开，否则中断永远进不来 */
    HAL_NVIC_SetPriority(I2C1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(I2C1_IRQn);
}

void Bsp_I2C1_Slave_SetTxBuf(uint8_t *buf, uint16_t len)
{
    if (len > HOST_TX_MAX)
    {
        len = HOST_TX_MAX;
    }
    s_tx     = buf;
    s_tx_len = len;
}

uint8_t Bsp_I2C1_Slave_IsIdle(void)
{
    /* 只有停在 LISTEN 且没有挂起的收发，才允许重新配置地址 */
    return (s_hi2c1.State == HAL_I2C_STATE_LISTEN) ? 1u : 0u;
}

void Bsp_I2C1_Slave_Init(uint8_t addr7)
{
    s_hi2c1.Instance             = I2C1;
    s_hi2c1.Init.ClockSpeed      = 100000u;
    s_hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    s_hi2c1.Init.OwnAddress1     = (uint16_t)(addr7 << 1);   /* OAR1 要 8 位形式 */
    s_hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&s_hi2c1) != HAL_OK)
    {
        APP_ErrorHandler();
    }
    (void)HAL_I2C_EnableListen_IT(&s_hi2c1);
}

/* 出错后先把状态拉回 READY，否则 EnableListen_IT 一定返回 HAL_BUSY */
static void slave_resume_listen(void)
{
    if (s_hi2c1.State != HAL_I2C_STATE_READY)
    {
        s_hi2c1.State = HAL_I2C_STATE_READY;
    }
    (void)HAL_I2C_EnableListen_IT(&s_hi2c1);
}

/* ============================ HAL 回调 ============================ */

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
    (void)AddrMatchCode;

    if (TransferDirection == I2C_DIRECTION_TRANSMIT)
    {
        /* 主机写：收固定 2 字节 [reg, value] */
        (void)HAL_I2C_Slave_Seq_Receive_IT(hi2c, s_rx, sizeof(s_rx), I2C_FIRST_FRAME);
    }
    else if (s_tx != NULL && s_tx_len > 0u)
    {
        /* 主机读：先把寄存器组拷成快照，再发，避免发送过程中被主循环改写 */
        s_snap_len = s_tx_len;
        memcpy(s_snap, s_tx, s_snap_len);
        (void)HAL_I2C_Slave_Seq_Transmit_IT(hi2c, s_snap, s_snap_len, I2C_FIRST_FRAME);
    }
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    (void)hi2c;
    Bsp_I2C1_OnWrite(s_rx, sizeof(s_rx));
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    (void)hi2c;
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
    /* 一次事务结束，恢复监听 */
    slave_resume_listen();
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    (void)hi2c;
    /* BERR/ARLO/AF 等错误后同样要回到监听态，否则从机"失联" */
    slave_resume_listen();
}

/* Listen 模式的外设中断入口 */
void I2C1_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&s_hi2c1);
    HAL_I2C_ER_IRQHandler(&s_hi2c1);
}

/**
  ******************************************************************************
  * @file    Bsp_I2C2_Soft.c
  * @brief   PA0/PA1 软件 I2C（开漏，外部 4.7k 上拉）
  ******************************************************************************
  */
#include "Bsp_I2C2_Soft.h"
#include "Bsp_PinMap/Bsp_PinMap.h"
#include "Bsp_Uart/Bsp_Uart.h"

#define SDA_HIGH()   HAL_GPIO_WritePin(PIN_I2C2_PORT, PIN_I2C2_SDA_PIN, GPIO_PIN_SET)
#define SDA_LOW()    HAL_GPIO_WritePin(PIN_I2C2_PORT, PIN_I2C2_SDA_PIN, GPIO_PIN_RESET)
#define SCL_HIGH()   HAL_GPIO_WritePin(PIN_I2C2_PORT, PIN_I2C2_SCL_PIN, GPIO_PIN_SET)
#define SCL_LOW()    HAL_GPIO_WritePin(PIN_I2C2_PORT, PIN_I2C2_SCL_PIN, GPIO_PIN_RESET)
#define SDA_READ()   (HAL_GPIO_ReadPin(PIN_I2C2_PORT, PIN_I2C2_SDA_PIN) == GPIO_PIN_SET)

static void i2c2_delay(void)
{
    for (volatile uint32_t i = 0; i < BSP_I2C2_DELAY_LOOPS; i++)
    {
        __NOP();
    }
}

void Bsp_I2C2_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_OUTPUT_OD;      /* 开漏，靠外部 4.7k 上拉 */
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pin   = PIN_I2C2_SDA_PIN | PIN_I2C2_SCL_PIN;
    HAL_GPIO_Init(PIN_I2C2_PORT, &gpio);

    SDA_HIGH();
    SCL_HIGH();
    Bsp_I2C2_Recover();
}

void Bsp_I2C2_Recover(void)
{
    SDA_HIGH();
    for (uint8_t i = 0; i < 9u; i++)
    {
        SCL_LOW();
        i2c2_delay();
        SCL_HIGH();
        i2c2_delay();
    }
    /* 补一个 STOP：SCL 高时 SDA 由低到高 */
    SCL_LOW();
    i2c2_delay();
    SDA_LOW();
    i2c2_delay();
    SCL_HIGH();
    i2c2_delay();
    SDA_HIGH();
    i2c2_delay();
}

static void i2c2_start(void)
{
    SDA_HIGH();
    i2c2_delay();
    SCL_HIGH();
    i2c2_delay();
    SDA_LOW();      /* SCL 高时 SDA 下降 = START */
    i2c2_delay();
    SCL_LOW();
    i2c2_delay();
}

static void i2c2_stop(void)
{
    SDA_LOW();
    i2c2_delay();
    SCL_HIGH();
    i2c2_delay();
    SDA_HIGH();     /* SCL 高时 SDA 上升 = STOP */
    i2c2_delay();
}

/* 返回 1 = 从机 ACK */
static uint8_t i2c2_write_byte(uint8_t byte)
{
    uint8_t ack;

    for (uint8_t i = 0; i < 8u; i++)
    {
        if (byte & 0x80u)
        {
            SDA_HIGH();
        }
        else
        {
            SDA_LOW();
        }
        i2c2_delay();
        SCL_HIGH();
        i2c2_delay();
        SCL_LOW();
        i2c2_delay();
        byte <<= 1;
    }

    /* 第 9 个时钟读 ACK */
    SDA_HIGH();      /* 释放 SDA，交给从机 */
    i2c2_delay();
    SCL_HIGH();
    i2c2_delay();
    ack = SDA_READ() ? 0u : 1u;
    SCL_LOW();
    i2c2_delay();
    return ack;
}

static uint8_t i2c2_read_byte(uint8_t ack)
{
    uint8_t byte = 0;

    SDA_HIGH();      /* 释放 SDA，准备接收 */
    for (uint8_t i = 0; i < 8u; i++)
    {
        i2c2_delay();
        SCL_HIGH();
        i2c2_delay();
        byte <<= 1;
        if (SDA_READ())
        {
            byte |= 1u;
        }
        SCL_LOW();
        i2c2_delay();
    }

    /* 主机回应 ACK/NACK */
    if (ack)
    {
        SDA_LOW();
    }
    else
    {
        SDA_HIGH();
    }
    i2c2_delay();
    SCL_HIGH();
    i2c2_delay();
    SCL_LOW();
    i2c2_delay();
    SDA_HIGH();
    return byte;
}

uint8_t Bsp_I2C2_Write(uint8_t dev7, const uint8_t *buf, uint16_t len)
{
    i2c2_start();
    if (!i2c2_write_byte((uint8_t)(dev7 << 1)))
    {
        i2c2_stop();
        return 0;
    }
    for (uint16_t i = 0; i < len; i++)
    {
        if (!i2c2_write_byte(buf[i]))
        {
            i2c2_stop();
            return 0;
        }
    }
    i2c2_stop();
    return 1;
}

uint8_t Bsp_I2C2_Read(uint8_t dev7, uint8_t *buf, uint16_t len)
{
    i2c2_start();
    if (!i2c2_write_byte((uint8_t)((dev7 << 1) | 1u)))
    {
        i2c2_stop();
        return 0;
    }
    for (uint16_t i = 0; i < len; i++)
    {
        buf[i] = i2c2_read_byte(i < (uint16_t)(len - 1u));
    }
    i2c2_stop();
    return 1;
}

uint8_t Bsp_I2C2_WriteRead(uint8_t dev7, const uint8_t *wb, uint16_t wlen,
                           uint8_t *rb, uint16_t rlen)
{
    i2c2_start();
    if (!i2c2_write_byte((uint8_t)(dev7 << 1)))
    {
        i2c2_stop();
        return 0;
    }
    for (uint16_t i = 0; i < wlen; i++)
    {
        if (!i2c2_write_byte(wb[i]))
        {
            i2c2_stop();
            return 0;
        }
    }
    /* 重复起始 */
    i2c2_start();
    if (!i2c2_write_byte((uint8_t)((dev7 << 1) | 1u)))
    {
        i2c2_stop();
        return 0;
    }
    for (uint16_t i = 0; i < rlen; i++)
    {
        rb[i] = i2c2_read_byte(i < (uint16_t)(rlen - 1u));
    }
    i2c2_stop();
    return 1;
}

uint8_t Bsp_I2C2_Probe(uint8_t dev7)
{
    i2c2_start();
    uint8_t ack = i2c2_write_byte((uint8_t)(dev7 << 1));
    i2c2_stop();
    return ack;
}

void Bsp_I2C2_Scan(void)
{
    uint8_t found = 0;

    for (uint8_t a = 0x08u; a < 0x78u; a++)
    {
        if (Bsp_I2C2_Probe(a))
        {
            Bsp_Uart_Printf("I2C2 ACK @ 0x%02X\r\n", (int)a);
            found++;
        }
    }
    Bsp_Uart_Printf("I2C2 scan done, %d device(s)\r\n", (int)found);
}

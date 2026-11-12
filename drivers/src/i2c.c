/**
 * @file    i2c.c
 * @brief   [DRIVER] I2C2 master ghi kiểu polling (SPL: I2C_Init, I2C_GenerateSTART, I2C_CheckEvent...).
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 */
#include "i2c.h"
#include "stm32f10x.h"

#define I2C_TIMEOUT  20000UL   /* số vòng chờ tối đa cho mỗi bước (~ vài ms ở 72 MHz) */

static uint32_t s_speed;

static void hw_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB10 = SCL, PB11 = SDA: alternate function open-drain (điện trở kéo lên có sẵn trên module OLED) */
    GPIO_InitTypeDef g;
    g.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
    g.GPIO_Mode  = GPIO_Mode_AF_OD;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &g);

    I2C_DeInit(I2C2);
    I2C_InitTypeDef i;
    I2C_StructInit(&i);
    i.I2C_Mode                = I2C_Mode_I2C;
    i.I2C_DutyCycle           = I2C_DutyCycle_2;
    i.I2C_OwnAddress1         = 0x00;
    i.I2C_Ack                 = I2C_Ack_Enable;
    i.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i.I2C_ClockSpeed          = s_speed;
    I2C_Init(I2C2, &i);
    I2C_Cmd(I2C2, ENABLE);
}

void i2c2_init(uint32_t speed_hz)
{
    s_speed = speed_hz;
    hw_init();
}

/* Chờ một sự kiện I2C; false nếu quá hạn hoặc thiết bị NACK */
static bool wait_event(uint32_t event)
{
    for (uint32_t n = 0; n < I2C_TIMEOUT; n++) {
        if (I2C_CheckEvent(I2C2, event) == SUCCESS) {
            return true;
        }
        if (I2C_GetFlagStatus(I2C2, I2C_FLAG_AF) == SET) {   /* NACK: không có thiết bị trả lời */
            return false;
        }
    }
    return false;
}

/* Gỡ lỗi: phát STOP, xóa cờ NACK; nếu bus vẫn kẹt BUSY thì reset khối I2C và khởi tạo lại */
static void recover(void)
{
    I2C_GenerateSTOP(I2C2, ENABLE);
    I2C_ClearFlag(I2C2, I2C_FLAG_AF);
    for (uint32_t n = 0; n < I2C_TIMEOUT; n++) {
        if (I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY) == RESET) {
            return;
        }
    }
    I2C_SoftwareResetCmd(I2C2, ENABLE);
    I2C_SoftwareResetCmd(I2C2, DISABLE);
    hw_init();
}

bool i2c2_write(uint8_t addr7, uint8_t ctrl, const uint8_t *data, uint16_t len)
{
    uint32_t n = 0;
    while (I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY) == SET) {
        if (++n > I2C_TIMEOUT) { recover(); return false; }
    }

    I2C_GenerateSTART(I2C2, ENABLE);
    if (!wait_event(I2C_EVENT_MASTER_MODE_SELECT)) { recover(); return false; }               /* EV5 */

    I2C_Send7bitAddress(I2C2, (uint8_t)(addr7 << 1), I2C_Direction_Transmitter);
    if (!wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) { recover(); return false; } /* EV6 */

    I2C_SendData(I2C2, ctrl);
    for (uint16_t k = 0; k < len; k++) {
        if (!wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTING)) { recover(); return false; }     /* EV8 */
        I2C_SendData(I2C2, data[k]);
    }
    if (!wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { recover(); return false; }          /* EV8_2 */

    I2C_GenerateSTOP(I2C2, ENABLE);
    return true;
}

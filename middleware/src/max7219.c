/**
 * @file    max7219.c
 * @brief   [MIDDLEWARE] Điều khiển MAX7219 dựa trên driver spi + gpio.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "max7219.h"
#include "board.h"
#include "spi.h"
#include "gpio.h"

/* Địa chỉ thanh ghi MAX7219 (bảng lệnh trong slide SPI) */
#define REG_DIGIT0       0x01U
#define REG_DECODE_MODE  0x09U
#define REG_INTENSITY    0x0AU
#define REG_SCAN_LIMIT   0x0BU
#define REG_SHUTDOWN     0x0CU
#define REG_DISPLAY_TEST 0x0FU

static inline void cs_low(void)  { gpio_reset(BOARD_MAX7219_CS_PORT, BOARD_MAX7219_CS_PIN); }
static inline void cs_high(void) { spi1_wait_idle(); gpio_set(BOARD_MAX7219_CS_PORT, BOARD_MAX7219_CS_PIN); }

/* Gửi cùng một lệnh tới tất cả module trong chuỗi */
static void send_all(uint8_t reg, uint8_t data)
{
    cs_low();
    for (uint8_t d = 0; d < MAX7219_NUM_DEVICES; d++) {
        spi1_write16((uint16_t)((uint16_t)reg << 8) | data);
    }
    cs_high();                         /* cạnh lên của CS chốt dữ liệu */
}

void max7219_init(void)
{
    gpio_config(BOARD_MAX7219_CS_PORT, BOARD_MAX7219_CS_PIN, GPIO_MODE_OUT_PP);
    gpio_set(BOARD_MAX7219_CS_PORT, BOARD_MAX7219_CS_PIN);
    spi1_init();

    send_all(REG_DISPLAY_TEST, 0x00);  /* tắt chế độ test */
    send_all(REG_DECODE_MODE, 0x00);   /* không giải mã BCD: điều khiển từng LED */
    send_all(REG_SCAN_LIMIT, 0x07);    /* quét đủ 8 hàng */
    send_all(REG_INTENSITY, 0x00);
    for (uint8_t r = 0; r < 8U; r++) {
        send_all((uint8_t)(REG_DIGIT0 + r), 0x00);
    }
    send_all(REG_SHUTDOWN, 0x00);      /* bắt đầu ở trạng thái tắt */
}

void max7219_set_intensity(uint8_t level)
{
    send_all(REG_INTENSITY, (uint8_t)(level & 0x0FU));
}

void max7219_set_shutdown(bool shutdown)
{
    send_all(REG_SHUTDOWN, shutdown ? 0x00U : 0x01U);
}

void max7219_write_frame(const uint8_t *rows)
{
    for (uint8_t r = 0; r < 8U; r++) {
        cs_low();
        /* Khung đầu tiên đẩy vào sẽ đi xa nhất -> gửi module cuối trước */
        for (int d = (int)MAX7219_NUM_DEVICES - 1; d >= 0; d--) {
            spi1_write16((uint16_t)((uint16_t)(REG_DIGIT0 + r) << 8) | rows[d * 8 + r]);
        }
        cs_high();
    }
}

/**
 * @file    ssd1306.c
 * @brief   [MIDDLEWARE] Điều khiển OLED SSD1306 qua I2C2.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 * Mỗi lần ghi I2C bắt đầu bằng một byte điều khiển:
 *   0x00 = các byte sau là LỆNH, 0x40 = các byte sau là DỮ LIỆU điểm ảnh.
 */
#include "ssd1306.h"
#include "i2c.h"

#define CTRL_CMD   0x00U
#define CTRL_DATA  0x40U

static bool cmds(const uint8_t *c, uint16_t n)
{
    return i2c2_write(OLED_I2C_ADDR, CTRL_CMD, c, n);
}

bool ssd1306_init(void)
{
    i2c2_init(400000UL);                       /* I2C chế độ nhanh 400 kHz */

    const uint8_t init[] = {
        0xAE,                                  /* tắt màn trong lúc cấu hình */
        0xD5, 0x80,                            /* tần số dao động mặc định */
        0xA8, (uint8_t)(OLED_HEIGHT - 1U),     /* số hàng quét */
        0xD3, 0x00,                            /* không dịch dọc */
        0x40,                                  /* hàng bắt đầu = 0 */
        0x8D, 0x14,                            /* bật bơm điện áp nội (module 3,3-5 V) */
        0x20, 0x00,                            /* địa chỉ hóa ngang: ghi liên tục hết page này sang page sau */
        0xA1,                                  /* lật ngang: cột 0 ở bên trái */
        0xC8,                                  /* lật dọc: page 0 ở trên cùng */
        0xDA, (OLED_HEIGHT == 64U) ? 0x12 : 0x02,
        0x81, 0xCF,                            /* độ tương phản */
        0xD9, 0xF1,                            /* thời gian nạp trước */
        0xDB, 0x40,                            /* mức VCOMH */
        0xA4,                                  /* hiện theo nội dung RAM */
        0xA6,                                  /* không đảo màu */
        0xAF                                   /* bật màn */
    };
    return cmds(init, (uint16_t)sizeof(init));
}

bool ssd1306_flush(const uint8_t *fb)
{
    const uint8_t win[] = {
        0x21, 0x00, (uint8_t)(OLED_WIDTH - 1U),            /* cột 0..127 */
        0x22, 0x00, (uint8_t)(OLED_HEIGHT / 8U - 1U)       /* page 0..7 */
    };
    if (!cmds(win, (uint16_t)sizeof(win))) {
        return false;
    }
    /* Gửi từng page (128 byte) một lần để mỗi giao dịch I2C ngắn */
    for (uint16_t p = 0; p < OLED_HEIGHT / 8U; p++) {
        if (!i2c2_write(OLED_I2C_ADDR, CTRL_DATA, &fb[p * OLED_WIDTH], (uint16_t)OLED_WIDTH)) {
            return false;
        }
    }
    return true;
}

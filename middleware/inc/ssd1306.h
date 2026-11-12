/**
 * @file    ssd1306.h
 * @brief   [MIDDLEWARE] Màn OLED 0.96" SSD1306 giao tiếp I2C (4 chân: GND, VCC, SCL, SDA),
 *          dựa trên driver i2c (I2C2).
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 * Bộ nhớ màn hình: 128 cột x (OLED_HEIGHT / 8) "page", mỗi page cao 8 điểm ảnh.
 * Byte fb[page * 128 + x] chứa 8 điểm ảnh của cột x trong page đó, bit 0 = hàng trên cùng.
 *
 * Địa chỉ I2C thường là 0x3C (một số module là 0x3D, xem chữ in sau lưng module).
 * Màn 0.91" (128x32) thì đặt OLED_HEIGHT = 32.
 */
#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>

#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR   0x3CU
#endif
#ifndef OLED_HEIGHT
#define OLED_HEIGHT     64U
#endif
#define OLED_WIDTH      128U
#define OLED_FB_SIZE    (OLED_WIDTH * OLED_HEIGHT / 8U)

/** Khởi tạo I2C2 và màn. false nếu màn không trả lời (sai dây, sai địa chỉ). */
bool ssd1306_init(void);

/** Gửi toàn bộ khung hình (OLED_FB_SIZE byte). false nếu lỗi I2C. */
bool ssd1306_flush(const uint8_t *fb);

#endif /* SSD1306_H */

/**
 * @file    font5x7.h
 * @brief   [MIDDLEWARE] Font 5x7 (ASCII 32..126) để chạy chữ khi ghép từ 4 module MAX7219 trở lên.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#define FONT5X7_FIRST  32U
#define FONT5X7_COUNT  95U

extern const uint8_t g_font5x7[FONT5X7_COUNT][5];

#endif /* FONT5X7_H */

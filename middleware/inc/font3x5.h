/**
 * @file    font3x5.h
 * @brief   [MIDDLEWARE] Font chữ số 3x5 để hiện được 2 chữ số trên một ma trận 8x8.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Mỗi ký tự 5 hàng, mỗi hàng 3 bit: bit 2 = cột trái.
 * Thứ tự: '0'..'9' rồi '-' (chỉ số 10) và ' ' (chỉ số 11).
 */
#ifndef FONT3X5_H
#define FONT3X5_H

#include <stdint.h>

#define FONT3X5_DASH   10U
#define FONT3X5_SPACE  11U
#define FONT3X5_COUNT  12U

extern const uint8_t g_font3x5[FONT3X5_COUNT][5];

#endif /* FONT3X5_H */

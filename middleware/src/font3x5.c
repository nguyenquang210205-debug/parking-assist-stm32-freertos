/**
 * @file    font3x5.c
 * @brief   [MIDDLEWARE] Font chữ số 3x5 (bit 2 = cột trái, mảng theo hàng từ trên xuống).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "font3x5.h"

const uint8_t g_font3x5[FONT3X5_COUNT][5] = {
    {7, 5, 5, 5, 7},   /* 0 */
    {2, 6, 2, 2, 7},   /* 1 */
    {7, 1, 7, 4, 7},   /* 2 */
    {7, 1, 7, 1, 7},   /* 3 */
    {5, 5, 7, 1, 1},   /* 4 */
    {7, 4, 7, 1, 7},   /* 5 */
    {7, 4, 7, 5, 7},   /* 6 */
    {7, 1, 1, 2, 2},   /* 7 */
    {7, 5, 7, 5, 7},   /* 8 */
    {7, 5, 7, 1, 7},   /* 9 */
    {0, 0, 7, 0, 0},   /* - */
    {0, 0, 0, 0, 0},   /*   */
};

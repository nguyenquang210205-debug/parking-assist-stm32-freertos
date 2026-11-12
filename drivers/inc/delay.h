/**
 * @file    delay.h
 * @brief   [DRIVER] Trễ chính xác cỡ micro giây bằng bộ đếm chu kỳ DWT->CYCCNT của Cortex-M3.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 *
 * Chỉ dùng cho các trễ RẤT NGẮN (vài chục us, ví dụ xung Trig 10 us của HC-SR04).
 * Trễ cỡ mili giây phải dùng vTaskDelay() để nhường CPU cho task khác.
 */
#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>

void delay_init(void);            /* bật DWT cycle counter */
void delay_us(uint32_t us);       /* chờ bận us micro giây (72 chu kỳ / us) */

#endif /* DELAY_H */

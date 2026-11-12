/**
 * @file    pwm.h
 * @brief   [DRIVER] TIM3 kênh 4 phát xung vuông (PWM) trên PB1. Dùng SPL.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Dùng cho còi THỤ ĐỘNG (passive buzzer): loại này không tự kêu khi cấp điện một chiều,
 * phải cấp xung vuông vài kHz thì màng loa mới rung thành tiếng.
 *  - TIM3 đếm 1 MHz (PSC = TIMCLK / 1 MHz - 1), ARR = 1 MHz / tần số - 1.
 *  - Chế độ PWM1: chân ở mức cao khi CNT < CCR4.
 *  - Kêu: CCR4 = (ARR + 1) / 2 (độ rộng 50%). Im: CCR4 = 0 (chân luôn ở mức thấp).
 */
#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include <stdbool.h>

/** Cấu hình PB1 = TIM3_CH4, tần số freq_hz, ban đầu tắt (chân mức thấp). */
void tim3_ch4_pwm_init(uint32_t freq_hz);

/** on = true: phát xung 50%; false: chân giữ mức thấp. */
void tim3_ch4_pwm_enable(bool on);

#endif /* PWM_H */

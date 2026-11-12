/**
 * @file    buzzer.h
 * @brief   [MIDDLEWARE] Còi chip 2 chân (loại tròn đen nhỏ) cảnh báo khi xe lùi gần tường.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * ===== Cách mặc định: còi CHỦ ĐỘNG, cấp 5 V, KHÔNG cần transistor (BUZZER_PASSIVE = 0) =====
 *
 *        5V ──────── (+) CÒI (−) ──────┬──── PB12
 *                                      └──── PB13
 *
 *  - PB12 và PB13 cấu hình OPEN-DRAIN: muốn còi kêu thì kéo cả hai chân xuống 0 V -> còi nhận
 *    đủ 5 V (to hơn nhiều so với 3,3 V). Muốn còi im thì "thả" hai chân (không nối GND, cũng
 *    không cấp điện) -> không có dòng qua còi.
 *  - Bắt buộc dùng chân CHỊU ĐƯỢC 5 V (FT): PB12, PB13 đúng là FT. KHÔNG làm cách này với PB1
 *    hay chân PAx (không chịu 5 V, sẽ hỏng chân).
 *  - Dùng 2 chân song song để chia dòng: còi 5 V ăn ~20-30 mA, mỗi chân chỉ gánh ~10-15 mA,
 *    nằm trong giới hạn an toàn của STM32 (tối đa 25 mA mỗi chân).
 *
 * ===== Còi THỤ ĐỘNG (BUZZER_PASSIVE = 1) =====
 *  Loại này phải cấp xung vuông vài kHz mới kêu. Code phát PWM bằng TIM3_CH4 trên PB1:
 *        PB1 ──── (+) CÒI (−) ──── GND
 *
 *  Cách nhận biết: nối (+) vào 3V3, (−) vào GND trong 1 giây. Kêu "tít" liên tục -> chủ động.
 *  Chỉ nghe "tạch" một cái -> thụ động. Chân (+) thường dài hơn. Nhớ bóc tem dán trên nắp còi.
 */
#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>

#ifndef BUZZER_PASSIVE
#define BUZZER_PASSIVE      0        /* 0 = còi chủ động (5 V qua PB12+PB13), 1 = còi thụ động (PWM PB1) */
#endif

#ifndef BUZZER_FREQ_HZ
#define BUZZER_FREQ_HZ      2700U    /* tần số cho còi thụ động (thử 2300..4000 Hz cho to nhất) */
#endif

void buzzer_init(void);          /* cấu hình chân, còi im */
void buzzer_set(bool on);

#endif /* BUZZER_H */

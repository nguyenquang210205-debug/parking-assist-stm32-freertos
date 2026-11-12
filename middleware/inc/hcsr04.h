/**
 * @file    hcsr04.h
 * @brief   [MIDDLEWARE] Cảm biến siêu âm HC-SR04 (4 chân: VCC, Trig, Echo, GND).
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 *
 * Một lần đo:
 *   1. hcsr04_trigger(): phát xung 10 us ở chân Trig (PB0).
 *   2. Module phát 8 xung siêu âm 40 kHz rồi kéo Echo lên mức cao.
 *   3. Khi nghe tiếng vọng (hoặc hết khoảng 38 ms nếu không có vật;
 *      một số bản clone giữ lâu hơn, driver capture tự loại xung > 65 ms), Echo xuống mức thấp.
 *   4. TIM4 đo độ rộng xung Echo (us) -> callback trong ISR.
 * Khoảng cách = t_echo x vận tốc âm (343 m/s) / 2  ~  t_echo(us) / 5.83 (mm).
 * Giữa hai lần đo nên cách nhau >= 60 ms (theo datasheet HC-SR04) để tiếng vọng cũ tắt hẳn.
 * Nguồn của HC-SR04 là 5 V; Echo ra mức 5 V nên phải nối vào chân FT (PB6).
 */
#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

/** Callback chạy trong ISR khi có kết quả: echo_us = độ rộng xung Echo. */
typedef void (*hcsr04_echo_cb_t)(uint16_t echo_us);

void hcsr04_init(hcsr04_echo_cb_t cb);
/** Phát xung Trig 10 us. Gọi từ task; kết quả về qua callback. */
void hcsr04_trigger(void);

#endif /* HCSR04_H */

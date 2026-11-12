/**
 * @file    capture.h
 * @brief   [DRIVER] TIM4 ở chế độ PWM input trên PB6 (TIM4_CH1): đo độ rộng xung mức cao
 *          với độ phân giải 1 us. Dùng SPL.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 *
 * Nguyên lý (slide Timer - Input capture, RM0008 mục "PWM input mode"):
 *  - TIM4 đếm 1 MHz (PSC = 72 - 1), ARR = 0xFFFF.
 *  - TI1 (chân PB6) được nối tới HAI bộ bắt: IC1 bắt cạnh LÊN, IC2 bắt cạnh XUỐNG.
 *  - Slave mode RESET theo TI1FP1: mỗi cạnh lên xóa CNT về 0.
 *  => Ở cạnh xuống, CCR2 = số us từ cạnh lên tới cạnh xuống = ĐỘ RỘNG XUNG.
 *  Ngắt CC2 (cạnh xuống) gọi callback với độ rộng xung. Phần cứng tự đo nên kết quả
 *  không bị ảnh hưởng bởi độ trễ vào ngắt hay việc scheduler đang chạy task nào.
 *
 *  Chỉ nhận xung bắt đầu SAU capture_tim4_arm() (ngắt CC1 đánh dấu cạnh lên), mỗi lần arm
 *  một xung. Xung dài hơn 65,5 ms (CNT tràn, UIF bật) báo width_us = 0 (không hợp lệ).
 *  Prescaler tính từ clock thật đọc ở RCC, không phụ thuộc hằng số 72 MHz.
 */
#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>

/** Callback chạy trong ISR TIM4 khi đo xong một xung. width_us = độ rộng xung mức cao,
 *  0 nếu xung quá dài (timer tràn). */
typedef void (*capture_cb_t)(uint16_t width_us);

/** Khởi tạo TIM4 + PB6, bật ngắt CC2 với mức ưu tiên irq_prio. */
void capture_tim4_init(uint8_t irq_prio, capture_cb_t cb);

/** Sẵn sàng nhận MỘT xung mới. Gọi ngay trước khi phát xung kích (Trig). */
void capture_tim4_arm(void);

/** Bật/tắt ngắt (khi không cần đo, ví dụ bãi trống, để giảm ngắt thừa). */
void capture_tim4_enable(int enable);

#endif /* CAPTURE_H */

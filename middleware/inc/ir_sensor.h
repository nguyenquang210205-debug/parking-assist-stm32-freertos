/**
 * @file    ir_sensor.h
 * @brief   [MIDDLEWARE] Module hồng ngoại tránh vật cản 3 chân (VCC, GND, OUT) dùng LM393.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 *
 * LED phát hồng ngoại (bóng trong) chiếu ra, đầu thu (bóng đen) nhận ánh sáng phản xạ.
 * Có vật trong khoảng 2..30 cm (chỉnh bằng biến trở): OUT = 0 và LED báo trên module sáng.
 * Nếu module của nhóm ngược mức, đổi IR_SENSOR_ACTIVE_LEVEL thành 1.
 */
#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include <stdbool.h>

#ifndef IR_SENSOR_ACTIVE_LEVEL
#define IR_SENSOR_ACTIVE_LEVEL 0
#endif

/** Callback chạy trong ISR mỗi khi OUT đổi mức (chưa chống nhiễu). */
typedef void (*ir_sensor_edge_cb_t)(void);

void ir_sensor_init(ir_sensor_edge_cb_t cb);
bool ir_sensor_detected(void);

#endif /* IR_SENSOR_H */

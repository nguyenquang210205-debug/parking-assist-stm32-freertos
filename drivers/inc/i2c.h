/**
 * @file    i2c.h
 * @brief   [DRIVER] I2C2 master (PB10 = SCL, PB11 = SDA), chỉ GHI, kiểu polling có thời gian chờ.
 *          Dùng SPL.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 * Vì sao I2C2 mà không phải I2C1: I2C1 mặc định nằm ở PB6/PB7, mà PB6 đang là Echo của HC-SR04
 * (TIM4_CH1). I2C2 ở PB10/PB11 đang trống, không cần remap.
 *
 * Một lần ghi: START -> địa chỉ 7 bit + W -> byte điều khiển -> dữ liệu -> STOP.
 * Mỗi bước có giới hạn thời gian chờ: nếu thiết bị không trả lời (NACK, đứt dây) hàm trả về
 * false và tự gỡ bus, không treo chương trình.
 */
#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

void i2c2_init(uint32_t speed_hz);

/** Ghi `ctrl` rồi `len` byte `data` tới thiết bị addr7 (địa chỉ 7 bit). true nếu thành công. */
bool i2c2_write(uint8_t addr7, uint8_t ctrl, const uint8_t *data, uint16_t len);

#endif /* I2C_H */

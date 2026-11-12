/**
 * @file    parking_view.h
 * @brief   [APPLICATION] Vẽ khung hình cho ma trận LED từ trạng thái đỗ xe. Module thuần logic
 *          (chỉ tính mảng điểm ảnh, không gửi SPI) -> có unit test.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Một module 8x8 (MAX7219_NUM_DEVICES = 1):
 *   TRONG    : sáng đỏ toàn bộ (chưa có xe)
 *   XE VAO   : (hồng ngoại đã thấy xe) xa (>= slow) -> mũi tên chạy xuống ("đi vào");
 *              gần (< slow): dưới 10 cm -> 1 chữ số TO (hàng 0..6) + thanh vạch ở hàng 7;
 *              10..99 cm (nếu đặt slow lớn) -> 2 chữ số nhỏ 3x5 + thanh vạch 2 hàng;
 *              thanh vạch ngắn dần khi tới gần tường; mất tín hiệu -> "--"
 *   DUNG     : chữ S (Stop) - xe đã sát tường (<= stop)
 *   DA DO    : chữ P, giữ suốt khi xe còn đỗ
 *   XE RA    : vẫn chữ P cho tới khi xe ra khỏi cửa, rồi về sáng đỏ toàn bộ
 * Từ 4 module trở lên (32 cột): hiện chữ bằng font 5x7: "123cm", "STOP", "P 12".
 *
 * Còi (pv_beep): XE VAO trong vùng gần -> bíp, càng gần tường bíp càng nhanh;
 *                DUNG -> kêu liên tục; các trạng thái khác -> im.
 * Độ sáng: lấy từ cài đặt, intensity_night khi trời tối, intensity_day khi trời sáng
 *          (mặc định tối sáng mạnh hơn, đổi bằng lệnh "set day/night").
 */
#ifndef PARKING_VIEW_H
#define PARKING_VIEW_H

#include <stdint.h>
#include <stdbool.h>
#include "parking_types.h"

#define PV_MAX_DEVICES     4U
#define PV_BEEP_SLOW_MS    800U    /* chu kỳ bíp ở mép vùng gần (slow) */
#define PV_BEEP_FAST_MS    120U    /* chu kỳ bíp sát vạch dừng (stop) */

typedef struct {
    bool    on;          /* false -> cho MAX7219 vào shutdown */
    uint8_t intensity;   /* 0..15 */
} pv_output_t;

/**
 * Vẽ khung hình.
 * @param rows   mảng 8 * ndev byte: rows[d * 8 + r] là hàng r của module d, bit 7 = cột trái
 * @param ndev   số module ghép (1..PV_MAX_DEVICES); module 0 ở bên trái
 * @param now_ms thời gian hiện tại (ms) - dùng cho nhấp nháy, hoạt hình, thời gian hiện chữ P
 */
pv_output_t pv_render(const park_view_t *v, uint32_t now_ms, uint8_t *rows, uint8_t ndev);

/** Còi có đang kêu ở thời điểm now_ms không (gọi đều đặn, ví dụ mỗi 50 ms). */
bool pv_beep(const park_view_t *v, uint32_t now_ms);

#endif /* PARKING_VIEW_H */

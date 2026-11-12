/**
 * @file    cpu_view.h
 * @brief   [APPLICATION] Vẽ màn hình giám sát cho OLED 128x64 (hoặc 128x32): trạng thái đỗ xe,
 *          khoảng cách, % tải CPU và đồ thị tải CPU theo thời gian.
 *          Module thuần logic (chỉ tính mảng điểm ảnh, không gửi I2C) -> có unit test.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 * Bố cục 128x64:
 *   ┌──────────────────────────────┐
 *   │ DA DO                 2.8cm  │  hàng 0..7  : trạng thái đỗ xe + khoảng cách (đã lọc)
 *   │──────────────────────────────│  hàng 9     : gạch ngăn cách
 *   │ CPU  3.2%        (chữ to)    │  hàng 11..24: tải CPU theo run-time stats
 *   │ IH 3.1 thang 10%       125s  │  hàng 27..33: tải theo idle hook, thang đồ thị, thời gian chạy
 *   │ - - - - - - - - - - - - - -  │  hàng 37    : vạch chấm = đỉnh thang
 *   │            ▂ ▃▂ ▅▃▂▂ ▃  ▂ ▆  │  hàng 38..63: 60 giây gần nhất, mỗi giây 1 cột rộng 2 điểm,
 *   └──────────────────────────────┘               mới nhất ở bên phải
 * Thang đồ thị tự chọn theo giá trị lớn nhất trong 60 s: 5%, 10%, 20%, 50% hoặc 100%,
 * để tải nhỏ (vài %) vẫn nhìn rõ.
 * Màn 128x32: trạng thái + khoảng cách, "CPU x%" chữ nhỏ, đồ thị ở hàng 20..31.
 */
#ifndef CPU_VIEW_H
#define CPU_VIEW_H

#include <stdint.h>
#include <stdbool.h>

#define CV_WIDTH     128U
#define CV_HIST      60U         /* số mẫu lưu (1 mẫu / giây) */
#define CV_NONE      0xFFFFU     /* không có giá trị (ví dụ idle hook khi CPU đang ngủ) */
#define CV_DIST_NONE 0xFFFFU     /* không đo được khoảng cách (= DIST_NONE) */

typedef struct {
    uint16_t load_x10[CV_HIST];  /* tải CPU x10 (123 = 12.3%) */
    uint8_t  count;
    uint8_t  head;               /* vị trí ghi tiếp theo */
} cv_history_t;

void     cv_history_init(cv_history_t *h);
void     cv_history_push(cv_history_t *h, uint16_t load_x10);
/** Mẫu thứ i tính từ CŨ nhất (i = 0) tới mới nhất (i = count - 1). */
uint16_t cv_history_at(const cv_history_t *h, uint8_t i);
/** Đỉnh thang đồ thị (x10): 50, 100, 200, 500 hoặc 1000. */
uint16_t cv_scale_x10(const cv_history_t *h);

/**
 * Vẽ một khung hình vào fb (128 * height / 8 byte, định dạng page của SSD1306).
 * @param load_x10  tải CPU hiện tại (run-time stats)
 * @param ih_x10    tải theo idle hook, CV_NONE nếu không dùng được
 * @param state     tên trạng thái đỗ xe (ví dụ "XE VAO", "DA DO")
 * @param dist_mm   khoảng cách đã lọc (mm), CV_DIST_NONE nếu không đo được
 */
void cv_render(uint8_t *fb, uint8_t height, const cv_history_t *h,
               uint16_t load_x10, uint16_t ih_x10, uint32_t uptime_s,
               const char *state, uint16_t dist_mm);

#endif /* CPU_VIEW_H */

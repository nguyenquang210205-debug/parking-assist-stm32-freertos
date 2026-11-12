/**
 * @file    dist_filter.h
 * @brief   [APPLICATION] Đổi thời gian Echo -> khoảng cách và lọc nhiễu bằng bộ lọc trung vị.
 *          Module thuần logic (không phụ thuộc phần cứng, FreeRTOS) -> có unit test.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 *
 * Các tầng lọc:
 *  1. Chặn gai: mẫu lệch quá DF_JUMP_MM so với kết quả hiện tại bị bỏ (HC-SR04 thỉnh thoảng
 *     trả về giá trị sai lớn do tiếng vọng phụ từ sàn / thành mô hình). Nếu DF_JUMP_CONFIRM mẫu
 *     liền nhau cùng lệch và gần nhau thì đó là xe dịch thật -> nhận ngay, không bị trễ.
 *  2. Cửa sổ 5 mẫu (DF_WIN, 0,3 s): sắp xếp, lấy trung bình 3 mẫu giữa -> bỏ nhiễu nhỏ, đứng số.
 *  (Chống nhảy số trên màn do tầng "khóa số" pk_stable_cm() trong parking_fsm đảm nhận.)
 *  Bản cũ (9 mẫu + giới hạn 1 cm mỗi lần đo) làm số hiện chậm 2-7 cm khi xe đang chạy.
 *  0. Vùng sát (<= 5 cm): bỏ tiếng vọng kép (Echo báo gấp 2-3 lần khoảng cách thật, làm số
 *     nhảy từ S lên 6-7 cm), và khi mất Echo thì giữ giá trị lâu hơn (~2 s).
 * Mất tín hiệu: giữ giá trị cũ thêm DF_MAX_MISS - 1 lần đo; mất liên tiếp DF_MAX_MISS lần
 * thì báo DIST_NONE (không có vật).
 * Hiệu chỉnh: đặt vật phẳng cách cảm biến đúng 10 cm (thước), xem lệnh "sonar"; nếu số thô
 * ra 108 mm thì đặt DIST_OFFSET_MM = -8.
 */
#ifndef DIST_FILTER_H
#define DIST_FILTER_H

#include <stdint.h>
#include "parking_types.h"

#define DIST_MIN_MM   20U      /* HC-SR04 không đo được gần hơn ~2 cm */
#define DIST_MAX_MM   4000U    /* xa hơn 4 m coi như không có vật */
#ifndef DIST_OFFSET_MM
#define DIST_OFFSET_MM  0      /* bù sai lệch cố định của module (mm, có dấu), xem phần hiệu chỉnh */
#endif
#ifndef DF_WIN
#define DF_WIN        5U       /* 5 mẫu: trung bình 3 mẫu giữa (3 = bám nhanh hơn nhưng dễ nhảy số) */
#endif
#define DF_JUMP_MM    30U      /* mẫu lệch > 3 cm so với kết quả = nghi là gai nhiễu */
#define DF_JUMP_CONFIRM 3U     /* 3 mẫu lệch liền nhau, gần nhau -> xe dịch thật, nhận luôn */
#define DF_NEAR_MM       50U   /* dưới 5 cm: vùng sát, có tiếng vọng kép và hay mất Echo */
#define DF_NEAR_STEP_MM  15U   /* ở vùng sát: tăng quá 1,5 cm trong 1 lần đo = nghi tiếng vọng kép */
#define DF_NEAR_CONFIRM  83U   /* tiếng vọng kép phải kéo dài ~5 s (lâu hơn 4 s chờ chữ P) mới tin là xe đã đi */
#define DF_NEAR_MAX_MISS 33U   /* ở vùng sát: mất Echo ~2 s mới báo không có vật */
#define DF_MAX_MISS   5U       /* mất 5 lần liên tiếp (~0,3 s) mới báo không có vật */
#define DF_ACQUIRE    3U       /* sau khi mất tín hiệu: cần đủ 3 mẫu mới báo lại khoảng cách */

typedef struct {
    uint16_t buf[DF_WIN];
    uint8_t  count;      /* số mẫu hợp lệ trong cửa sổ (<= DF_WIN) */
    uint8_t  idx;        /* vị trí ghi tiếp theo (vòng) */
    uint8_t  miss;       /* số lần đo lỗi liên tiếp */
    uint8_t  jump;       /* số mẫu lệch xa liên tiếp (đang chờ xác nhận) */
    uint8_t  ghost;      /* số mẫu tiếng vọng kép liên tiếp (vùng sát) */
    uint16_t cand;       /* mẫu lệch xa gần nhất */
    uint16_t out;        /* kết quả gần nhất */
} dist_filter_t;

/** Đổi độ rộng xung Echo (us) ra mm; DIST_NONE nếu ngoài tầm đo [DIST_MIN_MM, DIST_MAX_MM]. */
uint16_t dist_echo_us_to_mm(uint32_t echo_us);

void     dist_filter_init(dist_filter_t *f);
/** Đưa một mẫu (mm hoặc DIST_NONE) vào bộ lọc, trả về khoảng cách đã lọc hoặc DIST_NONE. */
uint16_t dist_filter_push(dist_filter_t *f, uint16_t mm);

#endif /* DIST_FILTER_H */

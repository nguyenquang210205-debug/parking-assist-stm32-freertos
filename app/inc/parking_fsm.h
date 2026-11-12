/**
 * @file    parking_fsm.h
 * @brief   [APPLICATION] Máy trạng thái của chỗ đỗ xe. Module thuần logic: không gọi FreeRTOS,
 *          không truy cập phần cứng; thời gian được truyền vào (now_ms) -> có unit test.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 *  Diễn biến (theo mô tả đề tài):
 *
 *  EMPTY    --(hồng ngoại ở cửa thấy xe)----------------------------------> APPROACH
 *             (chỉ hồng ngoại đánh thức; siêu âm không tự bật bước này)
 *  APPROACH --(d <= stop)--------------------------------------------------> STOP
 *  APPROACH --(hồng ngoại trống >= PK_APPROACH_ABORT_MS và xe chưa vào vùng gần)--> EMPTY
 *  STOP     --(d > stop + HYST)--------------------------------------------> APPROACH
 *  STOP     --(ở vùng dừng liên tục >= park_hold)--------------------------> PARKED  (park_count++)
 *  PARKED   --(xe ra xa hơn stop + LEAVE_MARGIN liên tục PK_LEAVE_CONFIRM_MS)--> LEAVING
 *  LEAVING  --(hồng ngoại thấy xe rồi hết thấy: xe đã qua cửa ra)-----------> EMPTY   (leave_count++)
 *  LEAVING  --(hồng ngoại không thấy gì liên tục >= empty_timeout)---------> EMPTY   (xe được nhấc đi)
 *  LEAVING  --(d <= stop)--------------------------------------------------> STOP    (xe đỗ lại)
 *
 *  Ở LEAVING, hồng ngoại báo là CHÍNH chiếc xe đang ra, nên không bị tính thành xe mới vào.
 *  Vùng trễ HYST tránh nhảy qua lại STOP <-> APPROACH khi xe dừng đúng ngay vạch.
 */
#ifndef PARKING_FSM_H
#define PARKING_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "parking_types.h"

#define PK_HYST_MM          20U    /* trễ 2 cm quanh vạch dừng */
#define PK_LEAVE_MARGIN_MM  80U    /* xe đã đỗ chạy ra quá vạch dừng 8 cm = bắt đầu rời đi */
#define PK_LEAVE_CONFIRM_MS 500U   /* phải ra xa liên tục 0,5 s mới tin là xe rời đi */
#define PK_APPROACH_ABORT_MS 15000U /* xe vào rồi quay ra (không tới vùng gần): 15 s thì về trống */
#ifndef PK_SHOW_DOWN_MM
#define PK_SHOW_DOWN_MM     5U     /* số hiển thị chỉ GIẢM khi vào sâu hơn ranh giới 5 mm */
#endif
#ifndef PK_SHOW_UP_MM
#define PK_SHOW_UP_MM       20U    /* số hiển thị chỉ TĂNG khi ra xa hơn ranh giới 2 cm (xe lùi vào thì số chỉ giảm, không nhảy ngược) */
#endif

typedef struct {
    pk_state_t state;
    uint32_t   state_since_ms;
    bool       ir;               /* hồng ngoại ở cửa đang thấy vật */
    uint32_t   ir_off_since_ms;  /* thời điểm hồng ngoại hết thấy vật */
    bool       ir_seen;          /* LEAVING: hồng ngoại đã thấy xe đi qua cửa */
    bool       away;             /* PARKED: xe đang ở xa vạch dừng */
    uint32_t   away_since_ms;
    uint16_t   dist_mm;
    uint16_t   show_cm;          /* số cm hiển thị, đã khóa chống nhảy (pk_stable_cm) */
    uint16_t   park_count;
    uint16_t   leave_count;
} pk_fsm_t;

void pk_settings_default(pk_settings_t *s);
/** Kiểm tra bộ tham số hợp lệ (stop + trễ < slow, độ sáng 0..15, ...). */
bool pk_settings_valid(const pk_settings_t *s);

void pk_fsm_init(pk_fsm_t *f, uint32_t now_ms);

/**
 * "Khóa số" cho hiển thị: số đang hiện là n (tức khoảng [n cm, n+1 cm)).
 *  - Chỉ đổi xuống khi d <= n*10 - PK_SHOW_DOWN_MM (xe đã vào hẳn khoảng dưới).
 *  - Chỉ đổi lên khi d >= (n+1)*10 + PK_SHOW_UP_MM (xe đã ra hẳn khoảng trên).
 *  Nhiễu đo cỡ vài mm quanh ranh giới không làm số nhảy qua lại. Mất tín hiệu: giữ số cũ.
 */
uint16_t pk_stable_cm(uint16_t prev_cm, uint16_t d_mm);

/** Kết quả đo mới (đã lọc). Trả về true nếu trạng thái thay đổi.
 *  Siêu âm KHÔNG đánh thức hệ thống khi đang trống: chỉ hồng ngoại ở cửa vào làm việc đó. */
bool pk_fsm_on_distance(pk_fsm_t *f, uint16_t dist_mm, uint32_t now_ms, const pk_settings_t *s);
/** Cảm biến hồng ngoại ở cửa đổi trạng thái (đã chống nhiễu). Trả về true nếu trạng thái thay đổi. */
bool pk_fsm_on_ir(pk_fsm_t *f, bool detected, uint32_t now_ms, const pk_settings_t *s);

const char *pk_state_name(pk_state_t st);

#endif /* PARKING_FSM_H */

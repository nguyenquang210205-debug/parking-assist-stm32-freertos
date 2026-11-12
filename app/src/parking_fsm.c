/**
 * @file    parking_fsm.c
 * @brief   [APPLICATION] Cài đặt máy trạng thái chỗ đỗ xe (xem sơ đồ trong parking_fsm.h).
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 */
#include "parking_fsm.h"

void pk_settings_default(pk_settings_t *s)
{
    /* Ngưỡng cho MÔ HÌNH (chỗ đỗ xe đồ chơi); đổi được bằng lệnh "set" qua UART */
    s->stop_mm          = 30U;     /* 3 cm: hiện chữ S, còi kêu liên tục */
    s->slow_mm          = 100U;    /* dưới 10 cm: hiện 1 chữ số to (9, 8, ... 3) và bíp */
    s->park_hold_ms     = 4000U;   /* đứng yên ở vùng S 4 s -> đã đỗ (chữ P) */
    s->empty_timeout_ms = 5000U;   /* xe đang ra mà hồng ngoại không thấy gì 5 s -> coi như đã đi */
    s->intensity_day    = 4U;      /* trời sáng: vừa phải */
    s->intensity_night  = 15U;     /* trời tối: sáng mạnh nhất để tài xế dễ thấy */
}

bool pk_settings_valid(const pk_settings_t *s)
{
    return s->stop_mm >= PK_STOP_MM_MIN && s->stop_mm <= PK_STOP_MM_MAX &&
           s->slow_mm > s->stop_mm + PK_HYST_MM && s->slow_mm <= PK_SLOW_MM_MAX &&
           s->park_hold_ms >= PK_HOLD_MS_MIN && s->park_hold_ms <= PK_HOLD_MS_MAX &&
           s->empty_timeout_ms >= 1000U &&
           s->intensity_day <= PK_INTENSITY_MAX && s->intensity_night <= PK_INTENSITY_MAX;
}

void pk_fsm_init(pk_fsm_t *f, uint32_t now_ms)
{
    f->state           = PK_EMPTY;
    f->state_since_ms  = now_ms;
    f->ir              = false;
    f->ir_off_since_ms = now_ms;
    f->ir_seen         = false;
    f->away            = false;
    f->away_since_ms   = now_ms;
    f->dist_mm         = DIST_NONE;
    f->show_cm         = PK_CM_NONE;
    f->park_count      = 0;
    f->leave_count     = 0;
}

uint16_t pk_stable_cm(uint16_t prev_cm, uint16_t d_mm)
{
    if (d_mm == DIST_NONE) {
        return prev_cm;                                /* mất tín hiệu thoáng qua: nhớ số cũ */
    }
    uint16_t raw = (uint16_t)(d_mm / 10U);
    if (prev_cm == PK_CM_NONE) {
        return raw;
    }
    uint32_t lo = (uint32_t)prev_cm * 10U;             /* mép dưới khoảng đang hiện */
    uint32_t hi = lo + 10U;                            /* mép trên */
    if ((uint32_t)d_mm + PK_SHOW_DOWN_MM <= lo) {
        return raw;                                    /* đã vào hẳn khoảng dưới */
    }
    if ((uint32_t)d_mm >= hi + PK_SHOW_UP_MM) {
        return raw;                                    /* đã ra hẳn khoảng trên */
    }
    return prev_cm;                                    /* còn trong vùng trễ: giữ số cũ */
}

static bool go(pk_fsm_t *f, pk_state_t next, uint32_t now_ms)
{
    if (f->state == next) {
        return false;
    }
    if (next == PK_PARKED) {
        f->park_count++;
    }
    if (next == PK_EMPTY && f->state == PK_LEAVING) {
        f->leave_count++;
    }
    if (next == PK_EMPTY) {
        f->show_cm = PK_CM_NONE;
    }
    f->state = next;
    f->state_since_ms = now_ms;
    return true;
}

bool pk_fsm_on_distance(pk_fsm_t *f, uint16_t d, uint32_t now_ms, const pk_settings_t *s)
{
    f->dist_mm = d;
    f->show_cm = pk_stable_cm(f->show_cm, d);
    bool at_stop = (d != DIST_NONE) && (d <= s->stop_mm);

    switch (f->state) {
    case PK_EMPTY:
        break;                         /* chỉ hồng ngoại ở cửa vào mới chuyển sang XE VAO */

    case PK_APPROACH:
        if (at_stop) { return go(f, PK_STOP, now_ms); }
        /* Xe vào rồi quay ra / bị nhấc đi: cửa trống lâu mà xe chưa tới vùng gần */
        if (!f->ir && (d == DIST_NONE || d >= s->slow_mm) &&
            (now_ms - f->ir_off_since_ms) >= PK_APPROACH_ABORT_MS) {
            return go(f, PK_EMPTY, now_ms);
        }
        break;

    case PK_STOP:
        if (d != DIST_NONE && d > s->stop_mm + PK_HYST_MM) { return go(f, PK_APPROACH, now_ms); }
        if (at_stop && (now_ms - f->state_since_ms) >= s->park_hold_ms) {
            return go(f, PK_PARKED, now_ms);
        }
        break;

    case PK_PARKED: {
        bool far = (d == DIST_NONE) || (d > s->stop_mm + PK_LEAVE_MARGIN_MM);
        if (!far) {
            f->away = false;
        } else if (!f->away) {
            f->away = true;
            f->away_since_ms = now_ms;
        } else if ((now_ms - f->away_since_ms) >= PK_LEAVE_CONFIRM_MS) {
            f->ir_seen = f->ir;            /* xe có thể đang che hồng ngoại sẵn */
            f->ir_off_since_ms = now_ms;
            return go(f, PK_LEAVING, now_ms);
        }
        break;
    }

    case PK_LEAVING:
        if (at_stop) { return go(f, PK_STOP, now_ms); }
        if (!f->ir && (now_ms - f->ir_off_since_ms) >= s->empty_timeout_ms) {
            return go(f, PK_EMPTY, now_ms);  /* hồng ngoại không thấy gì: xe đã được nhấc đi */
        }
        break;

    default:
        return go(f, PK_EMPTY, now_ms);
    }
    return false;
}

bool pk_fsm_on_ir(pk_fsm_t *f, bool detected, uint32_t now_ms, const pk_settings_t *s)
{
    (void)s;
    bool was = f->ir;
    f->ir = detected;
    if (!detected && was) {
        f->ir_off_since_ms = now_ms;
    }

    switch (f->state) {
    case PK_EMPTY:
        if (detected) {
            f->ir_off_since_ms = now_ms;
            return go(f, PK_APPROACH, now_ms);   /* xe qua cửa: hiện mũi tên "đi vào" */
        }
        break;
    case PK_LEAVING:
        if (detected) {
            f->ir_seen = true;                   /* chính chiếc xe đang ra đi qua cửa */
        } else if (f->ir_seen) {
            return go(f, PK_EMPTY, now_ms);      /* xe đã ra khỏi cửa: về sáng đỏ */
        }
        break;
    default:
        break;                                   /* đang vào / đã đỗ: hồng ngoại không đổi gì */
    }
    return false;
}

const char *pk_state_name(pk_state_t st)
{
    static const char *const names[PK_STATE_COUNT] = { "TRONG", "XE VAO", "DUNG", "DA DO", "XE RA" };
    return (st < PK_STATE_COUNT) ? names[st] : "?";
}

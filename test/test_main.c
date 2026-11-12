/**
 * @file    test_main.c
 * @brief   Unit test chạy trên máy tính cho các module THUẦN LOGIC (không cần bo mạch):
 *          dist_filter.c (Người 1), parking_view.c (Người 2), parking_fsm.c (Người 3), xprintf.c.
 *          Chạy: make test
 */
#include <stdio.h>
#include <string.h>
#include "parking_fsm.h"
#include "dist_filter.h"
#include "parking_view.h"
#include "cpu_view.h"
#include "xprintf.h"

static int s_run, s_fail;

#define CHECK(cond) do { s_run++; if (!(cond)) { s_fail++; \
    printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

/* Bộ tham số cỡ lớn (vạch dừng 30 cm, vùng gần 1 m, tầm 2 m) cho các test kịch bản FSM/hiển thị
 * viết theo đơn vị dễ đọc; logic không phụ thuộc giá trị cụ thể. */
static void big_settings(pk_settings_t *s)
{
    pk_settings_default(s);
    s->stop_mm = 300U; s->slow_mm = 1000U;
    s->park_hold_ms = 3000U; s->empty_timeout_ms = 4000U;
}

/* ======================= dist_filter (Người 1) ======================= */
static void test_echo_conversion(void)
{
    CHECK(dist_echo_us_to_mm(0) == DIST_NONE);              /* quá gần / không có xung */
    CHECK(dist_echo_us_to_mm(100) == DIST_NONE);            /* 17 mm < 20 mm */
    CHECK(dist_echo_us_to_mm(583) == 100);                  /* 583 us ~ 10 cm */
    CHECK(dist_echo_us_to_mm(1749) == 300);                 /* ~30 cm */
    CHECK(dist_echo_us_to_mm(5831) == 1000);                /* ~1 m */
    CHECK(dist_echo_us_to_mm(23324) == 4000);               /* biên trên 4 m */
    CHECK(dist_echo_us_to_mm(30000) == DIST_NONE);          /* ngoài tầm 4 m (không có vật) */
}

static void test_median_filter(void)
{
    dist_filter_t f;
    dist_filter_init(&f);
    CHECK(dist_filter_push(&f, 500) == DIST_NONE);          /* chưa đủ 3 mẫu: chưa báo */
    CHECK(dist_filter_push(&f, 3900) == DIST_NONE);
    CHECK(dist_filter_push(&f, 510) == 510);                /* đủ 3 mẫu: trung vị loại gai 3900 */
    uint16_t r = dist_filter_push(&f, 505);
    CHECK(r >= 505 && r <= 510);
    r = dist_filter_push(&f, 40);                           /* gai nhỏ bị loại */
    CHECK(r >= 500 && r <= 510);

    /* Đứng yên sát ranh giới 6,9 / 7,0 cm: kết quả chỉ dao động trong vài mm */
    dist_filter_init(&f);
    static const uint16_t jitter[] = { 70, 68, 71, 69, 72, 68, 70, 69, 71, 68, 72, 70, 69, 71 };
    uint16_t lo = 0xFFFF, hi = 0, last = DIST_NONE;
    for (unsigned k = 0; k < sizeof(jitter) / sizeof(jitter[0]); k++) {
        last = dist_filter_push(&f, jitter[k]);
        if (k >= 2) { if (last < lo) { lo = last; } if (last > hi) { hi = last; } }
    }
    CHECK(hi - lo <= 2);

    /* Xe lùi thật sự: kết quả theo kịp sau khi xe dừng */
    for (uint16_t d = 70; d >= 40; d = (uint16_t)(d - 3)) { last = dist_filter_push(&f, d); }
    CHECK(last < 70);
    for (int k = 0; k < 6; k++) { last = dist_filter_push(&f, 40); }
    CHECK(last >= 40 && last <= 45);

    /* Gai nhiễu lẻ (xa hoặc gần) bị chặn, kết quả không đổi */
    dist_filter_init(&f);
    static const uint16_t spikes[] = { 80, 80, 80, 300, 81, 25, 80, 900, 22, 79 };
    for (unsigned k = 0; k < sizeof(spikes) / sizeof(spikes[0]); k++) {
        last = dist_filter_push(&f, spikes[k]);
        if (k >= DF_ACQUIRE - 1U) { CHECK(last >= 78 && last <= 82); }   /* sau khi đủ mẫu */
    }

    /* Xe dịch nhanh thật (nhảy > 3 cm và đứng ở chỗ mới): nhận sau DF_JUMP_CONFIRM mẫu */
    for (unsigned k = 1; k < DF_JUMP_CONFIRM; k++) { CHECK(dist_filter_push(&f, 200) <= 82); }
    CHECK(dist_filter_push(&f, 200) == 200);

    /* Xe lùi đều 10 cm/s (6 mm mỗi lần đo 60 ms): kết quả bám theo, trễ không quá 2 cm */
    dist_filter_init(&f);
    for (int k = 0; k < 3; k++) { dist_filter_push(&f, 150); }
    for (uint16_t d = 150; d >= 36; d = (uint16_t)(d - 6)) {
        last = dist_filter_push(&f, d);
        CHECK(last >= d && last <= d + 20U);
    }
}

/* Xe dí sát: tiếng vọng kép (~2 lần) và mất Echo không làm số nhảy khỏi vùng S */
static void test_filter_near_ghost(void)
{
    dist_filter_t f;
    dist_filter_init(&f);
    for (int k = 0; k < 3; k++) { dist_filter_push(&f, 25); }
    static const uint16_t near[] = { 50, 46, 25, DIST_NONE, 51, 49, 26, DIST_NONE, DIST_NONE, 54, 75, 24 };
    for (int r = 0; r < 20; r++) {
        for (unsigned k = 0; k < sizeof(near) / sizeof(near[0]); k++) {
            CHECK(dist_filter_push(&f, near[k]) <= 30);
        }
    }
    /* Xe chạy ra dần: số tăng theo */
    uint16_t last = 25;
    for (uint16_t d = 25; d <= 120; d = (uint16_t)(d + 6)) { last = dist_filter_push(&f, d); }
    CHECK(last >= 100);
}

static void test_filter_missing(void)
{
    dist_filter_t f;
    dist_filter_init(&f);
    CHECK(dist_filter_push(&f, DIST_NONE) == DIST_NONE);    /* chưa có gì */
    for (int k = 0; k < 3; k++) { dist_filter_push(&f, 800); }
    for (unsigned k = 1; k < DF_MAX_MISS; k++) {
        CHECK(dist_filter_push(&f, DIST_NONE) == 800);      /* mất ít lần: giữ giá trị */
    }
    CHECK(dist_filter_push(&f, DIST_NONE) == DIST_NONE);    /* mất DF_MAX_MISS lần: không có vật */
    CHECK(dist_filter_push(&f, 1200) == DIST_NONE);         /* bắt lại: cần đủ 3 mẫu mới */
    dist_filter_push(&f, 1200);
    CHECK(dist_filter_push(&f, 1200) == 1200);              /* lịch sử cũ đã bị xóa */
}

/* ======================= parking_fsm (Người 3) ======================= */
/* Đưa một chuỗi khoảng cách vào FSM, mỗi mẫu cách nhau step_ms */
static uint32_t feed(pk_fsm_t *f, const pk_settings_t *s, uint32_t t, uint16_t mm, int n, uint32_t step)
{
    for (int i = 0; i < n; i++) {
        t += step;
        pk_fsm_on_distance(f, mm, t, s);
    }
    return t;
}

static void test_settings(void)
{
    pk_settings_t s;
    pk_settings_default(&s);
    CHECK(pk_settings_valid(&s));
    pk_settings_t t = s; t.stop_mm = 1200;                  /* stop > slow */
    CHECK(!pk_settings_valid(&t));
    t = s; t.slow_mm = (uint16_t)(s.stop_mm + 10U);        /* vùng gần quá hẹp (< trễ 2 cm) */
    CHECK(!pk_settings_valid(&t));
    t = s; t.stop_mm = 20;                                  /* trong vùng mù của HC-SR04 */
    CHECK(!pk_settings_valid(&t));
    CHECK(s.stop_mm == 30 && s.slow_mm == 100 && s.park_hold_ms == 4000);  /* S 3 cm, đếm từ 9 cm, đỗ 4 s */
    t = s; t.park_hold_ms = 500;
    CHECK(!pk_settings_valid(&t));
    t = s; t.intensity_night = 16;                          /* MAX7219 chỉ có 0..15 */
    CHECK(!pk_settings_valid(&t));
}

/* Khóa số hiển thị: nhiễu vài mm quanh ranh giới không làm số nhảy */
static void test_stable_cm(void)
{
    uint16_t cm = PK_CM_NONE;
    cm = pk_stable_cm(cm, 72);
    CHECK(cm == 7);
    static const uint16_t noisy[] = { 69, 71, 68, 73, 67, 70, 79, 72, 66 };   /* 6,6..7,9 cm */
    bool same = true;
    for (unsigned k = 0; k < sizeof(noisy) / sizeof(noisy[0]); k++) {
        cm = pk_stable_cm(cm, noisy[k]);
        if (cm != 7) { same = false; }
    }
    CHECK(same);                                            /* vẫn đứng yên số 7 */
    cm = pk_stable_cm(cm, 65);                              /* vào hẳn 6,5 cm (sâu 5 mm) */
    CHECK(cm == 6);
    cm = pk_stable_cm(cm, 89);                              /* ra 8,9 cm: chưa đủ xa để tăng (cần +2 cm) */
    CHECK(cm == 6);
    cm = pk_stable_cm(cm, 90);                              /* ra 9,0 cm: tăng lại (hiện đúng 9) */
    CHECK(cm == 9);
    cm = pk_stable_cm(cm, 40);                              /* xe lùi nhanh: nhảy thẳng xuống */
    CHECK(cm == 4);
    CHECK(pk_stable_cm(cm, DIST_NONE) == 4);                /* mất tín hiệu: giữ số cũ */

    /* Đếm lùi khi xe lùi đều 1 mm mỗi lần đo: mỗi số chỉ xuất hiện một lần, không quay lại */
    cm = PK_CM_NONE;
    uint16_t prev = 0xFFFF; int changes = 0; bool monotonic = true;
    for (uint16_t d = 99; d >= 30; d--) {
        cm = pk_stable_cm(cm, d);
        if (cm != prev) { if (prev != 0xFFFF && cm > prev) { monotonic = false; } changes++; prev = cm; }
    }
    CHECK(monotonic && changes == 7);                       /* 9, 8, 7, 6, 5, 4, 3 */
}

/* Kịch bản đúng như đề tài: trống (đỏ) -> hồng ngoại -> mũi tên -> đếm lùi -> S -> 4 s -> P
 * -> giữ P -> xe ra qua cửa -> trống (đỏ). Mỗi lần đo cách nhau 60 ms. */
static void test_full_cycle(void)
{
    pk_fsm_t f; pk_settings_t s;
    pk_settings_default(&s);
    pk_fsm_init(&f, 0);

    /* Chưa có xe: siêu âm thấy gì cũng không đổi (chỉ hồng ngoại mới bật) */
    uint32_t t = feed(&f, &s, 0, 150, 20, 60);
    CHECK(f.state == PK_EMPTY);

    /* Xe qua cửa: hồng ngoại thấy -> XE VAO (mũi tên) */
    CHECK(pk_fsm_on_ir(&f, true, t, &s));
    CHECK(f.state == PK_APPROACH);
    t += 300;
    pk_fsm_on_ir(&f, false, t, &s);                         /* xe đi qua khỏi cửa */
    for (uint16_t d = 200; d > 30; d = (uint16_t)(d - 5)) {  /* lùi từ 20 cm vào tới 3,5 cm */
        t += 60;
        pk_fsm_on_distance(&f, d, t, &s);
        CHECK(f.state == PK_APPROACH);
    }
    t += 60;
    CHECK(pk_fsm_on_distance(&f, 30, t, &s));               /* chạm 3 cm -> S */
    CHECK(f.state == PK_STOP);

    t = feed(&f, &s, t, 29, 60, 60);                        /* 3,6 s: chưa đủ 4 s */
    CHECK(f.state == PK_STOP);
    t = feed(&f, &s, t, 29, 8, 60);
    CHECK(f.state == PK_PARKED && f.park_count == 1);

    /* Xe còn đỗ: giữ P mãi, kể cả lâu và có người đi qua cửa */
    t = feed(&f, &s, t, 29, 500, 60);
    pk_fsm_on_ir(&f, true, t, &s);
    pk_fsm_on_ir(&f, false, t + 200, &s);
    CHECK(f.state == PK_PARKED);

    /* Xe chạy ra: xa hơn 3 + 8 cm liên tục 0,5 s -> XE RA (vẫn hiện P) */
    t = feed(&f, &s, t, 150, 5, 60);
    CHECK(f.state == PK_PARKED);                            /* mới 0,3 s */
    t = feed(&f, &s, t, 200, 6, 60);
    CHECK(f.state == PK_LEAVING);
    /* Chính chiếc xe đi qua cửa ra: không bị tính là xe mới, hết che thì về trống (đỏ) */
    CHECK(!pk_fsm_on_ir(&f, true, t, &s));
    CHECK(f.state == PK_LEAVING);
    CHECK(pk_fsm_on_ir(&f, false, t + 300, &s));
    CHECK(f.state == PK_EMPTY && f.leave_count == 1);

    /* Xe tiếp theo vào: lại bắt đầu từ mũi tên */
    CHECK(pk_fsm_on_ir(&f, true, t + 5000, &s));
    CHECK(f.state == PK_APPROACH);
}

static void test_hysteresis(void)
{
    pk_fsm_t f; pk_settings_t s;
    pk_settings_default(&s);                                /* vạch dừng 3 cm, trễ 2 cm */
    pk_fsm_init(&f, 0);
    pk_fsm_on_ir(&f, true, 0, &s);
    pk_fsm_on_distance(&f, 30, 60, &s);
    CHECK(f.state == PK_STOP);
    pk_fsm_on_distance(&f, 40, 120, &s);                    /* dao động trong vùng trễ */
    pk_fsm_on_distance(&f, 50, 180, &s);
    CHECK(f.state == PK_STOP);
    pk_fsm_on_distance(&f, 55, 240, &s);                    /* vượt 5 cm: về đếm số */
    CHECK(f.state == PK_APPROACH);
}

/* Xe được nhấc đi (không đi qua cửa): hồng ngoại không thấy gì 5 s thì về trống */
static void test_lifted_away(void)
{
    pk_fsm_t f; pk_settings_t s;
    pk_settings_default(&s);
    pk_fsm_init(&f, 0);
    pk_fsm_on_ir(&f, true, 0, &s);
    pk_fsm_on_ir(&f, false, 200, &s);
    uint32_t t = feed(&f, &s, 200, 28, 80, 60);             /* tới vạch, đứng 4,8 s */
    CHECK(f.state == PK_PARKED);
    t = feed(&f, &s, t, DIST_NONE, 10, 60);                 /* nhấc xe: mất tín hiệu */
    CHECK(f.state == PK_LEAVING);
    t = feed(&f, &s, t, DIST_NONE, 70, 60);                 /* 4,2 s */
    CHECK(f.state == PK_LEAVING);
    t = feed(&f, &s, t, DIST_NONE, 20, 60);
    CHECK(f.state == PK_EMPTY);
    (void)t;
}

/* Hồng ngoại báo (người đi ngang) nhưng không có xe vào gần: 15 s thì về trống */
static void test_false_alarm(void)
{
    pk_fsm_t f; pk_settings_t s;
    pk_settings_default(&s);
    pk_fsm_init(&f, 0);
    pk_fsm_on_ir(&f, true, 0, &s);
    pk_fsm_on_ir(&f, false, 500, &s);
    uint32_t t = feed(&f, &s, 500, 300, 230, 60);           /* 13,8 s */
    CHECK(f.state == PK_APPROACH);
    t = feed(&f, &s, t, 300, 30, 60);
    CHECK(f.state == PK_EMPTY && f.park_count == 0 && f.leave_count == 0);
    (void)t;
}

/* Xe đang ra rồi đổi ý lùi lại sát tường: đỗ lại */
static void test_repark(void)
{
    pk_fsm_t f; pk_settings_t s;
    pk_settings_default(&s);
    pk_fsm_init(&f, 0);
    pk_fsm_on_ir(&f, true, 0, &s);
    pk_fsm_on_ir(&f, false, 100, &s);
    uint32_t t = feed(&f, &s, 100, 28, 80, 60);
    CHECK(f.state == PK_PARKED);
    t = feed(&f, &s, t, 150, 12, 60);
    CHECK(f.state == PK_LEAVING);
    t = feed(&f, &s, t, 28, 1, 60);
    CHECK(f.state == PK_STOP);
    feed(&f, &s, t, 28, 80, 60);
    CHECK(f.state == PK_PARKED && f.park_count == 2);
}

/* ======================= parking_view (Người 2) ======================= */
static int lit(const uint8_t *rows, int n)
{
    int c = 0;
    for (int i = 0; i < n; i++) { for (int b = 0; b < 8; b++) { c += (rows[i] >> b) & 1; } }
    return c;
}

static park_view_t make_view(pk_state_t st, uint16_t mm)
{
    park_view_t v;
    memset(&v, 0, sizeof(v));
    v.state = (uint8_t)st;
    v.dist_mm = mm;
    v.show_cm = PK_CM_NONE;                                 /* hiện theo dist_mm */
    big_settings(&v.settings);
    return v;
}

static void test_view_small(void)
{
    uint8_t rows[8];
    park_view_t v = make_view(PK_EMPTY, DIST_NONE);
    pv_output_t o = pv_render(&v, 0, rows, 1);
    CHECK(o.on && lit(rows, 8) == 64);                      /* chưa có xe: sáng đỏ toàn bộ */
    v.dark = true;
    o = pv_render(&v, 0, rows, 1);
    CHECK(o.intensity == v.settings.intensity_night);       /* trời tối: sáng đậm hơn */

    /* 57 cm: chữ số "5" và "7" ở hàng 0..4, thanh vạch ở hàng 6..7 */
    v = make_view(PK_APPROACH, 570);
    o = pv_render(&v, 0, rows, 1);
    CHECK(o.on && o.intensity == v.settings.intensity_day);
    CHECK(rows[0] == 0xEE);                                 /* "5" 111 + "7" 111 */
    CHECK(rows[5] == 0x00);
    CHECK(rows[6] == rows[7] && rows[6] == 0xF0);           /* (57-30)/(100-30) x 8 -> 4 cột */

    /* Gần vạch dừng: thanh vạch ngắn lại */
    v = make_view(PK_APPROACH, 320);
    pv_render(&v, 0, rows, 1);
    CHECK(rows[6] == 0x80);

    /* Xa: hiện mũi tên (không phải chữ số) và có chuyển động */
    v = make_view(PK_APPROACH, 1800);
    uint8_t a[8], b[8];
    pv_render(&v, 0, a, 1);
    pv_render(&v, 120, b, 1);
    CHECK(lit(a, 8) > 0 && memcmp(a, b, 8) != 0);

    /* Sát tường: chữ S, đứng yên (không nhấp nháy) */
    v = make_view(PK_STOP, 40);
    pv_render(&v, 0, a, 1);
    pv_render(&v, 250, b, 1);
    CHECK(a[0] == 0x3C && a[2] == 0x60 && a[4] == 0x06 && memcmp(a, b, 8) == 0);

    /* Đã đỗ: chữ P giữ mãi (kể cả sau rất lâu); đang ra vẫn là P */
    v = make_view(PK_PARKED, 280);
    v.state_since_ms = 1000;
    v.dark = true;
    o = pv_render(&v, 2000, a, 1);
    CHECK(o.on && o.intensity == v.settings.intensity_night && lit(a, 8) > 0);
    CHECK(v.settings.intensity_night > v.settings.intensity_day);
    o = pv_render(&v, 600000, b, 1);
    CHECK(o.on && memcmp(a, b, 8) == 0);
    v.state = PK_LEAVING;
    pv_render(&v, 0, b, 1);
    CHECK(memcmp(a, b, 8) == 0);

    /* Ngưỡng mô hình: dưới 10 cm hiện 1 chữ số TO, hàng 7 là thanh vạch */
    v = make_view(PK_APPROACH, 75);
    pk_settings_default(&v.settings);
    pv_render(&v, 0, a, 1);
    CHECK(a[0] == 0x7E && a[6] == 0x30);                    /* chữ số 7 */
    CHECK(a[7] == 0xFC);                                    /* (7,5-3)/(10-3) x 8 -> 6 cột */
    v.dist_mm = 31;
    pv_render(&v, 0, a, 1);
    CHECK(a[3] == 0x1C && a[7] == 0x80);                    /* chữ số 3, thanh vạch còn 1 cột */
    v.dist_mm = 120;
    pv_render(&v, 0, a, 1);
    CHECK(a[0] != 0x7E);                                    /* 12 cm >= 10 cm: mũi tên, không hiện số */
}

static void test_view_wide(void)
{
    uint8_t rows[32];
    park_view_t v = make_view(PK_APPROACH, 1234);
    pv_output_t o = pv_render(&v, 0, rows, 4);
    CHECK(o.on && lit(rows, 32) > 20);                      /* "123cm" */
    v = make_view(PK_STOP, 250);
    pv_render(&v, 0, rows, 4);
    CHECK(lit(rows, 32) > 20);                              /* "STOP" */
}

static int beeps_in(const park_view_t *v, uint32_t ms)   /* số lần còi bật trong ms mili giây */
{
    int n = 0; bool prev = false;
    for (uint32_t t = 0; t < ms; t++) {
        bool on = pv_beep(v, t);
        if (on && !prev) { n++; }
        prev = on;
    }
    return n;
}

static void test_beep(void)
{
    park_view_t v;
    memset(&v, 0, sizeof(v));
    pk_settings_default(&v.settings);                       /* S 3 cm, vùng gần dưới 10 cm */
    v.state = PK_APPROACH;

    v.dist_mm = 300;                                        /* 30 cm: còn xa -> im */
    CHECK(beeps_in(&v, 2000) == 0);
    v.dist_mm = DIST_NONE;
    CHECK(beeps_in(&v, 2000) == 0);

    v.dist_mm = 95;   int far  = beeps_in(&v, 4000);        /* 9,5 cm */
    v.dist_mm = 60;   int mid  = beeps_in(&v, 4000);        /* 6 cm */
    v.dist_mm = 35;   int near = beeps_in(&v, 4000);        /* 3,5 cm */
    CHECK(far > 0 && far < mid && mid < near);              /* càng gần bíp càng nhanh */

    v.state = PK_STOP; v.dist_mm = 25;                      /* sát tường: kêu liên tục */
    CHECK(pv_beep(&v, 0) && pv_beep(&v, 333) && pv_beep(&v, 999));
    v.state = PK_PARKED;                                    /* đã đỗ: im */
    CHECK(beeps_in(&v, 2000) == 0);
    v.state = PK_LEAVING; v.dist_mm = 100;                  /* đang ra: im */
    CHECK(beeps_in(&v, 2000) == 0);
}

/* ======================= cpu_view (Người 3) ======================= */
static int lit_fb(const uint8_t *fb, int n)
{
    int c = 0;
    for (int i = 0; i < n; i++) { for (int b = 0; b < 8; b++) { c += (fb[i] >> b) & 1; } }
    return c;
}

static int pixel(const uint8_t *fb, int x, int y)
{
    return (fb[(y / 8) * 128 + x] >> (y % 8)) & 1;
}

static void test_cpu_view(void)
{
    cv_history_t h;
    cv_history_init(&h);
    CHECK(cv_scale_x10(&h) == 50);                          /* chưa có mẫu: thang nhỏ nhất 5% */

    for (uint16_t k = 1; k <= 70; k++) { cv_history_push(&h, k); }   /* 70 mẫu, chỉ giữ 60 */
    CHECK(h.count == 60);
    CHECK(cv_history_at(&h, 0) == 11 && cv_history_at(&h, 59) == 70);  /* cũ nhất 1.1%, mới nhất 7.0% */
    CHECK(cv_scale_x10(&h) == 100);                         /* đỉnh 7% -> thang 10% */
    cv_history_push(&h, 340);
    CHECK(cv_scale_x10(&h) == 500);                         /* 34% -> thang 50% */

    static uint8_t fb[1024];
    cv_history_init(&h);
    cv_history_push(&h, 0);
    cv_history_push(&h, 50);                                /* mới nhất: 5% = đầy thang 5% */
    cv_render(fb, 64, &h, 50, 48, 125, "DA DO", 28);
    CHECK(lit_fb(fb, 1024) > 100);                          /* có chữ "CPU 5.0%" */
    CHECK(pixel(fb, 126, 62) && pixel(fb, 127, 38));        /* cột mới nhất cao gần hết đồ thị */
    CHECK(pixel(fb, 0, 9) && pixel(fb, 127, 9));            /* gạch ngăn cách dưới dòng trạng thái */
    int top_left = 0, top_right = 0;                        /* dòng 1: "DA DO" bên trái, "2.8cm" bên phải */
    for (int x = 0; x < 30; x++)   { for (int y = 0; y < 8; y++) { top_left  += pixel(fb, x, y); } }
    for (int x = 98; x < 128; x++) { for (int y = 0; y < 8; y++) { top_right += pixel(fb, x, y); } }
    CHECK(top_left > 10 && top_right > 10);
    cv_render(fb, 64, &h, 50, 48, 125, "TRONG", CV_DIST_NONE);   /* không đo được: hiện "--" */
    top_right = 0;
    for (int x = 110; x < 128; x++) { for (int y = 0; y < 8; y++) { top_right += pixel(fb, x, y); } }
    CHECK(top_right > 0 && top_right < 20);
    CHECK(!pixel(fb, 124, 62));                             /* mẫu 0% không có cột */
    CHECK(pixel(fb, 0, 63) && pixel(fb, 127, 63));          /* đường đáy */

    static uint8_t fb32[512];
    cv_render(fb32, 32, &h, 50, CV_NONE, 0, "XE VAO", 75);  /* màn 128x32 vẫn vẽ được */
    CHECK(pixel(fb32, 0, 31) && pixel(fb32, 127, 21));
}

/* ======================= xprintf ======================= */
static void test_xprintf(void)
{
    char b[32];
    xsnprintf(b, sizeof(b), "%u cm %s", 57u, "ok");
    CHECK(strcmp(b, "57 cm ok") == 0);
    xsnprintf(b, sizeof(b), "%-6s|%4u", "ab", 7u);
    CHECK(strcmp(b, "ab    |   7") == 0);
}

int main(void)
{
    test_echo_conversion();
    test_median_filter();
    test_filter_near_ghost();
    test_filter_missing();
    test_settings();
    test_full_cycle();
    test_hysteresis();
    test_stable_cm();
    test_false_alarm();
    test_lifted_away();
    test_repark();
    test_view_small();
    test_view_wide();
    test_beep();
    test_cpu_view();
    test_xprintf();
    printf("%d/%d kiem tra dat\n", s_run - s_fail, s_run);
    return s_fail ? 1 : 0;
}

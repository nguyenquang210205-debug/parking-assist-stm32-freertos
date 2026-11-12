/**
 * @file    parking_view.c
 * @brief   [APPLICATION] Vẽ khung hình ma trận LED cho từng trạng thái đỗ xe.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "parking_view.h"
#include "font3x5.h"
#include "font5x7.h"
#include <string.h>

/* ---------- Vẽ điểm ảnh lên khung có độ rộng 8 * ndev cột ---------- */
typedef struct {
    uint8_t *rows;
    uint8_t  width;
} canvas_t;

static void px(canvas_t *c, int x, int y)
{
    if (x < 0 || y < 0 || x >= (int)c->width || y >= 8) { return; }
    c->rows[(x / 8) * 8 + y] |= (uint8_t)(0x80U >> (x % 8));
}

static void all_on(canvas_t *c)                               /* bật hết mọi điểm */
{
    for (int x = 0; x < (int)c->width; x++) {
        for (int y = 0; y < 8; y++) { px(c, x, y); }
    }
}

static void bitmap8(canvas_t *c, int x0, const uint8_t *bm)   /* 8 hàng, bit 7 = trái */
{
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if ((bm[y] & (0x80U >> x)) != 0U) { px(c, x0 + x, y); }
        }
    }
}

static void glyph3x5(canvas_t *c, int x0, int y0, uint8_t idx)
{
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 3; x++) {
            if ((g_font3x5[idx][y] & (4U >> x)) != 0U) { px(c, x0 + x, y0 + y); }
        }
    }
}

static int text5x7_width(const char *s)
{
    return (int)strlen(s) * 6 - 1;
}

static void text5x7(canvas_t *c, int x0, const char *s)
{
    for (; *s != '\0'; s++, x0 += 6) {
        uint8_t ch = (uint8_t)*s;
        if (ch < FONT5X7_FIRST || ch >= FONT5X7_FIRST + FONT5X7_COUNT) { ch = '?'; }
        const uint8_t *g = g_font5x7[ch - FONT5X7_FIRST];
        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 7; y++) {
                if ((g[x] & (1U << y)) != 0U) { px(c, x0 + x, y); }
            }
        }
    }
}

static void text_center(canvas_t *c, const char *s)
{
    text5x7(c, ((int)c->width - text5x7_width(s)) / 2, s);
}

/* ---------- Hình 8x8 ---------- */
static const uint8_t BM_S[8]    = { 0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00 };  /* chữ S (Stop) */
static const uint8_t BM_P[8]    = { 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00 };
static const uint8_t CHEVRON_DOWN[3] = { 0x42, 0x24, 0x18 };   /* hình chữ V cao 3 hàng */

/* Các chữ V lặp lại mỗi 4 hàng, trượt 1 hàng mỗi 120 ms.
 * dir = +1: mũi tên chỉ xuống, chạy xuống ("tiến lên"); dir = -1: chỉ lên, chạy lên ("lùi ra"). */
static void arrows(canvas_t *c, int x0, uint32_t now_ms, int dir)
{
    int phase = (int)((now_ms / 120U) % 4U);
    int offset = (dir > 0) ? phase : (3 - phase);
    uint8_t bm[8] = { 0 };
    for (int k = -1; k < 2; k++) {
        int top = offset + 4 * k;
        for (int i = 0; i < 3; i++) {
            int y = top + i;
            uint8_t line = (dir > 0) ? CHEVRON_DOWN[i] : CHEVRON_DOWN[2 - i];
            if (y >= 0 && y < 8) { bm[y] |= line; }
        }
    }
    bitmap8(c, x0, bm);
}

static void number2(canvas_t *c, int x0, uint16_t value)   /* 0..99 */
{
    glyph3x5(c, x0, 0, (uint8_t)((value >= 10U) ? value / 10U : FONT3X5_SPACE));
    glyph3x5(c, x0 + 4, 0, (uint8_t)(value % 10U));
}

/* Chữ số TO cho 1 module 8x8: cao 7 điểm (hàng 0..6), hàng 7 dành cho thanh vạch */
static const uint8_t BM_DIGIT[10][7] = {
    { 0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C },   /* 0 */
    { 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E },   /* 1 */
    { 0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E },   /* 2 */
    { 0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C },   /* 3 */
    { 0x0C, 0x1C, 0x2C, 0x4C, 0x7E, 0x0C, 0x0C },   /* 4 */
    { 0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C },   /* 5 */
    { 0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C },   /* 6 */
    { 0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30 },   /* 7 */
    { 0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C },   /* 8 */
    { 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38 },   /* 9 */
};

/* Một chữ số to ở hàng 0..6 + thanh vạch 1 hàng (hàng 7) dài bar_px điểm */
static void digit_big(canvas_t *c, int x0, uint8_t dgt, int bar_px)
{
    uint8_t rows[8];
    for (int y = 0; y < 7; y++) { rows[y] = BM_DIGIT[dgt % 10U][y]; }
    rows[7] = (bar_px <= 0) ? 0U : (uint8_t)(0xFFU << (8 - ((bar_px > 8) ? 8 : bar_px)));
    bitmap8(c, x0, rows);
}

static void bar(canvas_t *c, int x0, int width, int y0, int len)
{
    for (int x = 0; x < len && x < width; x++) {
        px(c, x0 + x, y0);
        px(c, x0 + x, y0 + 1);
    }
}

/* Chiều dài thanh vạch: đầy (width) ở mép vùng chậm, 0 ở vạch dừng */
static int bar_len(const park_view_t *v, int width)
{
    if (v->dist_mm == DIST_NONE || v->dist_mm <= v->settings.stop_mm) { return 0; }
    uint32_t span = (uint32_t)(v->settings.slow_mm - v->settings.stop_mm);
    uint32_t rem  = (uint32_t)(v->dist_mm - v->settings.stop_mm);
    if (rem >= span) { return width; }
    return (int)((rem * (uint32_t)width + span - 1U) / span);    /* làm tròn lên: còn xa là còn vạch */
}

/* ---------- Một module 8x8 ---------- */
static bool render_small(canvas_t *c, const park_view_t *v, uint32_t now_ms)
{
    /* Số hiện trên màn: dùng số đã khóa chống nhảy (show_cm); chưa có thì lấy từ khoảng cách */
    uint16_t cm = (v->show_cm != PK_CM_NONE) ? v->show_cm : (uint16_t)(v->dist_mm / 10U);
    uint16_t d = v->dist_mm;
    switch ((pk_state_t)v->state) {
    case PK_APPROACH:
        if (d == DIST_NONE) {
            glyph3x5(c, 0, 0, FONT3X5_DASH);
            glyph3x5(c, 4, 0, FONT3X5_DASH);
        } else if (d >= v->settings.slow_mm || cm * 10U >= v->settings.slow_mm || cm > 99U) {
            arrows(c, 0, now_ms, +1);                           /* còn xa: mũi tên "đi vào" */
        } else if (cm <= 9U) {
            digit_big(c, 0, (uint8_t)cm, bar_len(v, 8));        /* 1 chữ số to: 9, 8, ... */
        } else {
            number2(c, 0, cm);                                  /* vùng gần đặt > 10 cm: 2 chữ số nhỏ */
            bar(c, 0, 8, 6, bar_len(v, 8));
        }
        return true;
    case PK_STOP:
        bitmap8(c, 0, BM_S);                     /* sát tường: chữ S đứng yên, còi kêu liên tục */
        return true;
    case PK_PARKED:
    case PK_LEAVING:
        bitmap8(c, 0, BM_P);                     /* đã đỗ: giữ chữ P cho tới khi xe ra khỏi cửa */
        return true;
    case PK_EMPTY:
    default:
        all_on(c);                               /* chưa có xe: sáng đỏ toàn bộ */
        return true;
    }
}

/* ---------- Từ 4 module (32 cột): hiện chữ ---------- */
static void fmt_cm(char *out, uint16_t mm)       /* "123cm" */
{
    uint16_t cm = (uint16_t)(mm / 10U);
    char tmp[6];
    int n = 0;
    do { tmp[n++] = (char)('0' + cm % 10U); cm /= 10U; } while (cm != 0U && n < 3);
    int k = 0;
    while (n > 0) { out[k++] = tmp[--n]; }
    out[k++] = 'c'; out[k++] = 'm'; out[k] = '\0';
}

static bool render_wide(canvas_t *c, const park_view_t *v, uint32_t now_ms)
{
    (void)now_ms;
    char buf[8];
    uint16_t d = v->dist_mm;
    switch ((pk_state_t)v->state) {
    case PK_APPROACH:
        if (d == DIST_NONE) {
            text_center(c, "--");
        } else {
            fmt_cm(buf, d);
            text_center(c, buf);
            if (v->state == PK_APPROACH) {
                int len = bar_len(v, c->width);
                for (int x = 0; x < len; x++) { px(c, x, 7); }
            }
        }
        return true;
    case PK_STOP:
        text_center(c, "STOP");
        return true;
    case PK_PARKED:
    case PK_LEAVING: {
        uint16_t n = (uint16_t)(v->park_count % 100U);
        buf[0] = 'P'; buf[1] = ' ';
        buf[2] = (char)('0' + n / 10U); buf[3] = (char)('0' + n % 10U); buf[4] = '\0';
        text_center(c, buf);
        return true;
    }
    case PK_EMPTY:
    default:
        all_on(c);
        return true;
    }
}

pv_output_t pv_render(const park_view_t *v, uint32_t now_ms, uint8_t *rows, uint8_t ndev)
{
    if (ndev == 0U) { ndev = 1U; }
    if (ndev > PV_MAX_DEVICES) { ndev = PV_MAX_DEVICES; }
    memset(rows, 0, (size_t)ndev * 8U);

    canvas_t c = { rows, (uint8_t)(ndev * 8U) };
    pv_output_t out;
    out.on = (ndev >= 4U) ? render_wide(&c, v, now_ms) : render_small(&c, v, now_ms);
    out.intensity = v->dark ? v->settings.intensity_night : v->settings.intensity_day;
    if (out.intensity > PK_INTENSITY_MAX) { out.intensity = PK_INTENSITY_MAX; }
    return out;
}

/* ---------- Còi cảnh báo ---------- */
bool pv_beep(const park_view_t *v, uint32_t now_ms)
{
    uint16_t d = v->dist_mm;
    if (v->state == PK_STOP) {
        return true;                                        /* sát tường: kêu liên tục */
    }
    if (v->state != PK_APPROACH || d == DIST_NONE || d >= v->settings.slow_mm) {
        return false;                                       /* xa tường, đã đỗ, đang ra: im */
    }
    /* Trong vùng gần: chu kỳ bíp tỉ lệ với khoảng cách còn lại tới vạch dừng.
     * Ở mép vùng gần (slow) -> PV_BEEP_SLOW_MS, sát vạch dừng (stop) -> PV_BEEP_FAST_MS. */
    uint32_t span = (uint32_t)(v->settings.slow_mm - v->settings.stop_mm);
    uint32_t rem  = (d > v->settings.stop_mm) ? (uint32_t)(d - v->settings.stop_mm) : 0U;
    uint32_t period = PV_BEEP_FAST_MS + rem * (PV_BEEP_SLOW_MS - PV_BEEP_FAST_MS) / span;
    return (now_ms % period) < (period / 2U);               /* nửa chu kỳ kêu, nửa chu kỳ im */
}

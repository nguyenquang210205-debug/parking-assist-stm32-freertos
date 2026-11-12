/**
 * @file    cpu_view.c
 * @brief   [APPLICATION] Vẽ màn hình giám sát tải CPU (chữ + đồ thị cột 60 giây).
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 */
#include "cpu_view.h"
#include "font5x7.h"
#include "xprintf.h"
#include <string.h>

/* ---------- Lịch sử ---------- */
void cv_history_init(cv_history_t *h)
{
    h->count = 0;
    h->head  = 0;
}

void cv_history_push(cv_history_t *h, uint16_t load_x10)
{
    h->load_x10[h->head] = load_x10;
    h->head = (uint8_t)((h->head + 1U) % CV_HIST);
    if (h->count < CV_HIST) { h->count++; }
}

uint16_t cv_history_at(const cv_history_t *h, uint8_t i)
{
    uint8_t oldest = (uint8_t)((h->head + CV_HIST - h->count) % CV_HIST);
    return h->load_x10[(oldest + i) % CV_HIST];
}

uint16_t cv_scale_x10(const cv_history_t *h)
{
    static const uint16_t steps[] = { 50U, 100U, 200U, 500U, 1000U };
    uint16_t mx = 0;
    for (uint8_t i = 0; i < h->count; i++) {
        uint16_t v = cv_history_at(h, i);
        if (v > mx) { mx = v; }
    }
    for (uint8_t k = 0; k < sizeof(steps) / sizeof(steps[0]); k++) {
        if (mx <= steps[k]) { return steps[k]; }
    }
    return 1000U;
}

/* ---------- Vẽ điểm ảnh (định dạng page của SSD1306) ---------- */
typedef struct {
    uint8_t *fb;
    uint8_t  height;
} canvas_t;

static void px(canvas_t *c, int x, int y)
{
    if (x < 0 || y < 0 || x >= (int)CV_WIDTH || y >= (int)c->height) { return; }
    c->fb[(y / 8) * (int)CV_WIDTH + x] |= (uint8_t)(1U << (y % 8));
}

/* Chữ 5x7, phóng to `scale` lần; trả về x sau chuỗi */
static int text(canvas_t *c, int x, int y, const char *s, int scale)
{
    for (; *s != '\0'; s++) {
        uint8_t ch = (uint8_t)*s;
        if (ch < FONT5X7_FIRST || ch >= FONT5X7_FIRST + FONT5X7_COUNT) { ch = '?'; }
        const uint8_t *g = g_font5x7[ch - FONT5X7_FIRST];
        for (int gx = 0; gx < 5; gx++) {
            for (int gy = 0; gy < 7; gy++) {
                if ((g[gx] & (1U << gy)) == 0U) { continue; }
                for (int dx = 0; dx < scale; dx++) {
                    for (int dy = 0; dy < scale; dy++) {
                        px(c, x + gx * scale + dx, y + gy * scale + dy);
                    }
                }
            }
        }
        x += 6 * scale;
    }
    return x;
}

static void fmt_pct(char *out, size_t n, uint16_t x10)
{
    if (x10 == CV_NONE) { xsnprintf(out, n, "--"); return; }
    xsnprintf(out, n, "%u.%u", (unsigned)(x10 / 10U), (unsigned)(x10 % 10U));
}

static void fmt_dist(char *out, size_t n, uint16_t mm)
{
    if (mm == CV_DIST_NONE)  { xsnprintf(out, n, "--"); return; }
    if (mm < 1000U)          { xsnprintf(out, n, "%u.%ucm", (unsigned)(mm / 10U), (unsigned)(mm % 10U)); return; }
    xsnprintf(out, n, "%ucm", (unsigned)(mm / 10U));
}

void cv_render(uint8_t *fb, uint8_t height, const cv_history_t *h,
               uint16_t load_x10, uint16_t ih_x10, uint32_t uptime_s,
               const char *state, uint16_t dist_mm)
{
    canvas_t c = { fb, height };
    memset(fb, 0, (size_t)CV_WIDTH * height / 8U);
    char num[8], line[24];

    /* Dòng 1: trạng thái đỗ xe (trái) + khoảng cách (phải) */
    text(&c, 0, 0, (state != NULL) ? state : "?", 1);
    fmt_dist(line, sizeof(line), dist_mm);
    text(&c, (int)CV_WIDTH - (int)strlen(line) * 6 + 1, 0, line, 1);
    for (int x = 0; x < (int)CV_WIDTH; x++) { px(&c, x, 9); }       /* gạch ngăn cách */

    uint16_t scale = cv_scale_x10(h);
    fmt_pct(num, sizeof(num), load_x10);
    xsnprintf(line, sizeof(line), "CPU %s%%", num);
    int gtop;
    if (height >= 64U) {
        text(&c, 0, 11, line, 2);                               /* tải CPU, chữ to (cao 14 điểm) */
        /* Dòng nhỏ: tải theo idle hook, thang đồ thị, thời gian chạy */
        fmt_pct(num, sizeof(num), ih_x10);
        xsnprintf(line, sizeof(line), "IH %s thang %u%%", num, (unsigned)(scale / 10U));
        int xend = text(&c, 0, 27, line, 1);
        xsnprintf(line, sizeof(line), "%us", (unsigned)uptime_s);
        int xup = (int)CV_WIDTH - (int)strlen(line) * 6 + 1;
        if (xup >= xend + 4) {                  /* chỉ vẽ thời gian chạy nếu không đè lên chữ */
            text(&c, xup, 27, line, 1);
        }
        gtop = 37;
    } else {
        text(&c, 0, 11, line, 1);                               /* màn 128x32: tải CPU chữ nhỏ */
        gtop = 20;
    }
    int gbot = (int)height - 1;                 /* đường đáy */
    int gh   = gbot - gtop;                     /* chiều cao tối đa của cột */

    for (int x = 0; x < (int)CV_WIDTH; x += 4) { px(&c, x, gtop); }   /* vạch chấm = đỉnh thang */
    for (int x = 0; x < (int)CV_WIDTH; x++)    { px(&c, x, gbot); }   /* đường đáy */

    /* Cột: mới nhất ở bên phải, mỗi mẫu rộng 2 điểm */
    for (uint8_t i = 0; i < h->count; i++) {
        uint32_t v = cv_history_at(h, i);
        if (v > scale) { v = scale; }
        int bh = (int)((v * (uint32_t)gh + scale - 1U) / scale);   /* làm tròn lên: >0% thì thấy ít nhất 1 điểm */
        int x0 = (int)CV_WIDTH - 2 * ((int)h->count - (int)i);
        for (int y = gbot - bh; y < gbot; y++) {
            px(&c, x0, y);
            px(&c, x0 + 1, y);
        }
    }
}

/**
 * @file    dist_filter.c
 * @brief   [APPLICATION] Đổi Echo -> mm, chặn gai nhiễu, lọc trung vị cửa sổ ngắn, xử lý mất tín hiệu.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 */
#include "dist_filter.h"

uint16_t dist_echo_us_to_mm(uint32_t echo_us)
{
    /* d = t x 343 m/s / 2 = t(us) x 0.1715 mm  (làm tròn), rồi bù sai lệch cố định của module */
    int32_t mm = (int32_t)((echo_us * 1715UL + 5000UL) / 10000UL) + DIST_OFFSET_MM;
    if (mm < (int32_t)DIST_MIN_MM || mm > (int32_t)DIST_MAX_MM) {
        return DIST_NONE;
    }
    return (uint16_t)mm;
}

void dist_filter_init(dist_filter_t *f)
{
    f->count = 0;
    f->idx   = 0;
    f->miss  = 0;
    f->jump  = 0;
    f->ghost = 0;
    f->cand  = DIST_NONE;
    f->out   = DIST_NONE;
}

/* Sắp xếp n mẫu rồi lấy phần GIỮA: n >= 5 lấy trung bình 3 mẫu chính giữa,
 * ít mẫu hơn lấy trung vị. */
static uint16_t middle(const uint16_t *src, uint8_t n)
{
    uint16_t a[DF_WIN];
    for (uint8_t i = 0; i < n; i++) {               /* sắp xếp chèn trên bản sao */
        uint16_t v = src[i];
        int8_t j = (int8_t)i - 1;
        while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; }
        a[j + 1] = v;
    }
    uint8_t m = (uint8_t)(n / 2U);
    if (n >= 5U) {
        return (uint16_t)(((uint32_t)a[m - 1U] + a[m] + a[m + 1U] + 1U) / 3U);
    }
    return a[m];
}

static uint16_t absdiff(uint16_t a, uint16_t b) { return (a > b) ? (uint16_t)(a - b) : (uint16_t)(b - a); }

static void put(dist_filter_t *f, uint16_t mm)
{
    f->buf[f->idx] = mm;
    f->idx = (uint8_t)((f->idx + 1U) % DF_WIN);
    if (f->count < DF_WIN) { f->count++; }
}

static void restart_at(dist_filter_t *f, uint16_t mm)
{
    f->count = 0;                                   /* xóa lịch sử cũ, bắt đầu từ chỗ mới */
    f->idx   = 0;
    f->jump  = 0;
    f->ghost = 0;
    put(f, mm);
    f->out = mm;
}

/* Vật ở rất gần (<= DF_NEAR_MM): sóng dội qua lại giữa vật và mặt cảm biến, Echo có thể báo
 * khoảng gấp 2-3 lần khoảng cách thật (tiếng vọng kép). Xe chạy ra thật thì mỗi lần đo (60 ms)
 * chỉ xa thêm vài mm, nên mẫu tăng vọt quá DF_NEAR_STEP_MM mà vẫn nằm trong tầm ~3 lần
 * khoảng cách hiện tại bị coi là tiếng vọng kép. */
static int is_ghost(uint16_t mm, uint16_t out)
{
    if (out > DF_NEAR_MM) {
        return 0;
    }
    return mm > out + DF_NEAR_STEP_MM && (uint32_t)mm <= 3UL * out + out / 4U;
}

uint16_t dist_filter_push(dist_filter_t *f, uint16_t mm)
{
    if (mm == DIST_NONE) {
        /* Ở sát cảm biến (vùng mù) module hay mất Echo: giữ giá trị lâu hơn thay vì báo "không có vật" */
        uint8_t limit = (f->out != DIST_NONE && f->out <= DF_NEAR_MM) ? DF_NEAR_MAX_MISS : DF_MAX_MISS;
        if (++f->miss >= limit) {
            dist_filter_init(f);                    /* vật đã ra khỏi tầm đo: bỏ lịch sử */
            f->miss = limit;
        }
        return f->out;                              /* mất ít lần: giữ giá trị cũ */
    }
    f->miss = 0;

    /* Tiếng vọng kép khi vật ở rất gần: bỏ. Chỉ khi nó kéo dài liên tục DF_NEAR_CONFIRM lần đo
     * (~5 s, xe đã bị nhấc ra và đặt lại chỗ khác) mới nhận. Xe chạy ra thật thì số tăng dần từng chút, không
     * nhảy gấp đôi, nên đi qua nhánh bình thường bên dưới. */
    if (f->out != DIST_NONE && is_ghost(mm, f->out)) {
        if (++f->ghost < DF_NEAR_CONFIRM) {
            return f->out;
        }
        restart_at(f, mm);
        return f->out;
    }
    f->ghost = 0;

    /* Chặn gai: mẫu lệch xa kết quả hiện tại bị giữ lại làm "ứng viên", chưa đưa vào cửa sổ.
     * Gai nhiễu thường đứng một mình -> bị bỏ. Nếu xe dịch nhanh thật thì các mẫu sau cũng lệch
     * và gần nhau -> đủ DF_JUMP_CONFIRM mẫu liền nhau thì nhận luôn, nhảy thẳng tới giá trị mới. */
    if (f->out != DIST_NONE && absdiff(mm, f->out) > DF_JUMP_MM) {
        if (f->jump > 0U && absdiff(mm, f->cand) <= DF_JUMP_MM) {
            f->jump++;
        } else {
            f->jump = 1U;
        }
        f->cand = mm;
        if (f->jump < DF_JUMP_CONFIRM) {
            return f->out;                          /* chưa chắc: giữ kết quả cũ */
        }
        restart_at(f, mm);                          /* đã chắc: nhảy tới chỗ mới */
        return f->out;
    }
    f->jump = 0;

    put(f, mm);
    uint16_t m = middle(f->buf, f->count);
    if (f->out == DIST_NONE && f->count < DF_ACQUIRE) {
        return DIST_NONE;                           /* mới bắt được vật: chờ đủ mẫu mới tin */
    }
    f->out = m;
    return f->out;
}

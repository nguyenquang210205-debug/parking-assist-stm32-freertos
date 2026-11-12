/**
 * @file    display_task.c
 * @brief   [APPLICATION] DisplayTask: task DUY NHẤT dùng ma trận LED (SPI1) và còi (PB1) ->
 *          gatekeeper của thiết bị báo hiệu, không cần mutex cho bus SPI.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Chạy tuần hoàn 50 ms bằng xTaskDelayUntil. Đọc trạng thái bằng xQueuePeek(qParkView),
 * gọi pv_render() (thuần logic) để tính khung hình, chỉ gửi SPI khi khung hình thay đổi.
 * Còi: pv_beep() cho biết còi kêu hay im ở thời điểm hiện tại -> bíp nhanh dần khi xe gần tường.
 * Trạng thái TRONG / đã đỗ quá 3 s: đưa MAX7219 vào shutdown (gần như không tốn điện).
 */
#include "app_interface.h"
#include "app_tasks.h"
#include "app_config.h"
#include "parking_view.h"
#include "max7219.h"
#include "buzzer.h"
#include "task.h"
#include <string.h>

/* parking_view hỗ trợ tối đa PV_MAX_DEVICES module */
typedef char check_num_devices[(MAX7219_NUM_DEVICES <= PV_MAX_DEVICES) ? 1 : -1];

typedef struct {
    uint8_t rows[8U * MAX7219_NUM_DEVICES];
    uint8_t intensity;
    bool    on;
} matrix_state_t;

static void apply(const uint8_t *rows, pv_output_t out, matrix_state_t *cur)
{
    if (!out.on) {
        if (cur->on) {
            max7219_set_shutdown(true);
            cur->on = false;
        }
        return;
    }
    if (memcmp(rows, cur->rows, sizeof(cur->rows)) != 0) {
        max7219_write_frame(rows);
        memcpy(cur->rows, rows, sizeof(cur->rows));
    }
    if (out.intensity != cur->intensity) {
        max7219_set_intensity(out.intensity);
        cur->intensity = out.intensity;
    }
    if (!cur->on) {
        max7219_set_shutdown(false);
        cur->on = true;
    }
}

static void display_task(void *arg)
{
    (void)arg;
    static matrix_state_t cur;           /* static: không chiếm stack */
    uint8_t rows[8U * MAX7219_NUM_DEVICES];

    app_wait_system_ready();
    max7219_init();                      /* khởi tạo xong ở trạng thái shutdown */
    buzzer_init();                       /* còi im */
    memset(&cur, 0, sizeof(cur));
    cur.intensity = 0xFF;                /* buộc gửi độ sáng ở khung đầu tiên */
    log_printf("[DISP] MAX7219 x%u san sang", (unsigned)MAX7219_NUM_DEVICES);

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        park_view_t v;
        if (xQueuePeek(g_qParkView, &v, 0) == pdPASS) {
            uint32_t now = app_now_ms();
            pv_output_t out = pv_render(&v, now, rows, (uint8_t)MAX7219_NUM_DEVICES);
            apply(rows, out, &cur);
            bool test = (xEventGroupGetBits(g_egSystem) & EVT_BEEP_TEST) != 0U;   /* lệnh "beep 1" */
            buzzer_set(test || pv_beep(&v, now));
        }
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
    }
}

void display_task_create(void)
{
    BaseType_t ok = xTaskCreate(display_task, "Display", STACK_DISPLAY, NULL, PRIO_DISPLAY, NULL);
    configASSERT(ok == pdPASS);
}

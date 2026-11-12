/**
 * @file    oled_task.c
 * @brief   [APPLICATION] OledTask: hiện trạng thái đỗ xe, khoảng cách, tải CPU (%) và đồ thị
 *          tải CPU 60 giây gần nhất lên OLED SSD1306.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 * Chạy mỗi 0,5 s (xTaskDelayUntil) để khoảng cách / trạng thái cập nhật nhanh; đồ thị CPU lấy 1 mẫu/giây. Lấy bảng thống kê CPU bằng cpu_monitor_get() (có mutex;
 * ConsoleTask là bên cập nhật bảng này mỗi giây), đẩy mẫu vào lịch sử, vẽ khung hình bằng
 * cv_render() (thuần logic), rồi gửi cả khung qua I2C2.
 *
 * Task DUY NHẤT dùng I2C2 -> gatekeeper của màn OLED, không cần mutex cho bus.
 * Ưu tiên thấp nhất (1): việc gửi 1 KB qua I2C (~25 ms mỗi giây) chỉ chạy khi các task
 * quan trọng hơn đã xong. Thời gian này được tính vào tải CPU (khoảng 2-3%).
 * Nếu không có màn (chưa cắm, sai địa chỉ) thì log một lần và thử lại mỗi 5 giây,
 * phần còn lại của hệ thống vẫn chạy bình thường.
 */
#include "app_interface.h"
#include "app_tasks.h"
#include "app_config.h"
#include "cpu_monitor.h"
#include "cpu_view.h"
#include "ssd1306.h"
#include "parking_fsm.h"
#include "task.h"

#define OLED_RETRY_S  5U

static void oled_task(void *arg)
{
    (void)arg;
    static uint8_t      fb[OLED_FB_SIZE];   /* static: không chiếm stack của task */
    static cpu_stats_t  st;
    static cv_history_t hist;

    cv_history_init(&hist);
    app_wait_system_ready();
    vTaskDelay(pdMS_TO_TICKS(250));                   /* lệch nhịp so với ConsoleTask lấy mẫu */

    bool ok = ssd1306_init();
    log_printf(ok ? "[OLED] SSD1306 san sang (I2C2 PB10/PB11, dia chi 0x%02X)"
                  : "[OLED] Khong thay man (dia chi 0x%02X) - kiem tra day SCL/SDA", (unsigned)OLED_I2C_ADDR);
    uint32_t retry = 0;
    uint32_t last_sample_s = 0xFFFFFFFFUL;

    TickType_t last = xTaskGetTickCount();
    for (;;) {
        (void)xTaskDelayUntil(&last, pdMS_TO_TICKS(OLED_PERIOD_MS));

        /* Đồ thị: 1 mẫu mỗi giây (khi ConsoleTask vừa cập nhật bảng thống kê) */
        if (cpu_monitor_get(&st, pdMS_TO_TICKS(20)) && st.uptime_s != last_sample_s) {
            last_sample_s = st.uptime_s;
            cv_history_push(&hist, st.load_runtime_x10);   /* luôn ghi lịch sử, kể cả khi chưa có màn */
        }

        if (!ok) {
            if (++retry < OLED_RETRY_S * (1000U / OLED_PERIOD_MS)) { continue; }
            retry = 0;
            ok = ssd1306_init();
            if (ok) { log_printf("[OLED] Da ket noi lai"); }
            else    { continue; }
        }

        /* Khi CPU ngủ (__WFI), idle hook không còn đúng -> chỉ tin run-time stats */
        uint16_t ih = st.sleep_enabled ? CV_NONE : st.load_idlehook_x10;
        park_view_t v;
        const char *state = "?";
        uint16_t    dist  = CV_DIST_NONE;
        if (xQueuePeek(g_qParkView, &v, 0) == pdPASS) {      /* mailbox: đọc không lấy ra */
            state = pk_state_name((pk_state_t)v.state);
            dist  = v.dist_mm;
        }
        cv_render(fb, (uint8_t)OLED_HEIGHT, &hist, st.load_runtime_x10, ih, st.uptime_s, state, dist);
        if (!ssd1306_flush(fb)) {
            ok = false;
            log_printf("[OLED] Mat ket noi, se thu lai");
        }
    }
}

void oled_task_create(void)
{
    BaseType_t ok = xTaskCreate(oled_task, "Oled", STACK_OLED, NULL, PRIO_OLED, NULL);
    configASSERT(ok == pdPASS);
}

/**
 * @file    sonar_task.c
 * @brief   [APPLICATION] SonarTask: đo khoảng cách bằng HC-SR04 theo chu kỳ, lọc trung vị,
 *          gửi kết quả cho ControlTask.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 *
 * Một chu kỳ:
 *   1. Xem trạng thái đỗ xe (peek qParkView) để chọn chu kỳ: có xe đang di chuyển -> 60 ms,
 *      trống hoặc đã đỗ -> 300 ms (đo thưa, CPU ngủ nhiều hơn).
 *   2. Phát xung Trig, chờ notification từ ISR TIM4 mang theo độ rộng xung Echo (tối đa 50 ms).
 *   3. Đổi ra mm, đưa qua bộ lọc, gửi EV_DIST vào qEvents, cập nhật mailbox qSonarStat.
 *   4. xTaskDelayUntil để giữ chu kỳ đều.
 * Giá trị Echo đi kèm notification (eSetValueWithOverwrite) -> không cần queue riêng cho ISR.
 */
#include "app_interface.h"
#include "app_tasks.h"
#include "app_config.h"
#include "hcsr04.h"
#include "dist_filter.h"
#include "task.h"

static TaskHandle_t s_task;

/* ---------- Ngữ cảnh ISR TIM4 ---------- */
static void on_echo_isr(uint16_t echo_us)
{
    BaseType_t woken = pdFALSE;
    if (s_task != NULL) {
        xTaskNotifyFromISR(s_task, echo_us, eSetValueWithOverwrite, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

/* ---------- Ngữ cảnh SonarTask ---------- */
static uint16_t choose_period(void)
{
    park_view_t v;
    if (xQueuePeek(g_qParkView, &v, 0) == pdPASS &&
        v.state == PK_EMPTY) {                       /* chưa có xe: đo thưa; đã đỗ vẫn đo nhanh để biết lúc xe rời đi */
        return SONAR_PERIOD_SLOW_MS;
    }
    return SONAR_PERIOD_FAST_MS;
}

static void sonar_task(void *arg)
{
    (void)arg;
    dist_filter_t filter;
    sonar_stat_t  st = { 0, DIST_NONE, DIST_NONE, SONAR_PERIOD_FAST_MS, 0, 0 };
    uint16_t      last_sent = 0;

    dist_filter_init(&filter);
    app_wait_system_ready();
    hcsr04_init(on_echo_isr);
    log_printf("[SON] HC-SR04 san sang (Trig PB0, Echo PB6/TIM4)");

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        st.period_ms = choose_period();

        (void)xTaskNotifyStateClear(NULL);           /* bỏ kết quả cũ (nếu có) */
        hcsr04_trigger();

        uint32_t echo_us = 0;
        uint16_t raw = DIST_NONE;
        if (xTaskNotifyWait(0, 0, &echo_us, pdMS_TO_TICKS(SONAR_ECHO_TIMEOUT_MS)) == pdTRUE) {
            raw = dist_echo_us_to_mm(echo_us);
        } else {
            echo_us = 0;                             /* quá hạn: không có xung Echo */
        }
        if (raw != DIST_NONE) { st.ok_count++; } else { st.miss_count++; }

        uint16_t filtered = dist_filter_push(&filter, raw);
        st.last_echo_us = (uint16_t)echo_us;
        st.last_raw_mm  = raw;
        st.filtered_mm  = filtered;
        (void)xQueueOverwrite(g_qSonarStat, &st);

        /* Gửi mỗi lần đo khi đang có xe; khi trống/đã đỗ chỉ gửi khi giá trị đổi > 1 cm
         * hoặc mất tín hiệu, để ControlTask vẫn kiểm tra được thời gian chờ */
        int diff = (int)filtered - (int)last_sent;
        if (st.period_ms == SONAR_PERIOD_FAST_MS || diff > 10 || diff < -10 || filtered == DIST_NONE) {
            (void)app_post_event(EV_DIST, 0, filtered, pdMS_TO_TICKS(10));
            last_sent = filtered;
        }

        if ((xEventGroupGetBits(g_egSystem) & EVT_WATCH_SONAR) != 0U) {
            log_printf("[SON] echo %u us  tho %u mm  loc %u mm", (unsigned)echo_us,
                       (unsigned)raw, (unsigned)filtered);
        }

        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(st.period_ms));
    }
}

void sonar_task_create(void)
{
    BaseType_t ok = xTaskCreate(sonar_task, "Sonar", STACK_SONAR, NULL, PRIO_SONAR, &s_task);
    configASSERT(ok == pdPASS);
}

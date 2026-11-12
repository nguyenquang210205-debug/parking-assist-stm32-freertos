/**
 * @file    sensor_task.c
 * @brief   [APPLICATION] SensorTask: đọc cảm biến hồng ngoại ở cửa vào và cảm biến quang,
 *          chống nhiễu, sinh sự kiện EV_IR / EV_LIGHT vào qEvents.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 *
 * Luồng xử lý (kỹ thuật "trì hoãn xử lý ngắt" trong slide FreeRTOS phần 2):
 *   1. ISR EXTI chỉ gửi task notification (bit cạnh) cho SensorTask.
 *   2. SensorTask khởi động lại (reset) software timer chống nhiễu của nguồn đó.
 *   3. Tín hiệu đứng yên đủ lâu -> timer hết hạn -> callback gửi notification (bit ổn định).
 *   4. SensorTask đọc mức chân, so với trạng thái cũ -> sinh sự kiện.
 */
#include "app_interface.h"
#include "app_tasks.h"
#include "app_config.h"
#include "ir_sensor.h"
#include "light_sensor.h"
#include "task.h"
#include "timers.h"
#include <stdint.h>

#define SRC_IR      0U
#define SRC_LIGHT   1U
#define SRC_COUNT   2U
#define NB_EDGE(src)    (1UL << (src))          /* từ ISR: chân vừa đổi mức */
#define NB_STABLE(src)  (1UL << (8U + (src)))   /* từ timer: chân đã ổn định */

static TaskHandle_t  s_task;
static TimerHandle_t s_debounce[SRC_COUNT];

/* ---------- Ngữ cảnh ISR ---------- */
static void notify_from_isr(uint32_t bits)
{
    BaseType_t woken = pdFALSE;
    if (s_task != NULL) {
        xTaskNotifyFromISR(s_task, bits, eSetBits, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

static void on_ir_edge_isr(void)    { notify_from_isr(NB_EDGE(SRC_IR)); }
static void on_light_edge_isr(void) { notify_from_isr(NB_EDGE(SRC_LIGHT)); }

/* ---------- Ngữ cảnh Timer daemon ---------- */
static void on_debounce_expired(TimerHandle_t t)
{
    uint32_t src = (uint32_t)(uintptr_t)pvTimerGetTimerID(t);
    xTaskNotify(s_task, NB_STABLE(src), eSetBits);
}

/* ---------- Ngữ cảnh SensorTask ---------- */
static bool read_source(uint32_t src)
{
    return (src == SRC_IR) ? ir_sensor_detected() : light_sensor_is_dark();
}

static void report(uint32_t src, bool active)
{
    if (src == SRC_IR) {
        log_printf("[SEN] Hong ngoai: %s", active ? "CO XE o cua" : "het vat");
        (void)app_post_event(EV_IR, 0, active ? 1U : 0U, pdMS_TO_TICKS(20));
    } else {
        log_printf("[SEN] Troi %s", active ? "TOI" : "SANG");
        (void)app_post_event(EV_LIGHT, 0, active ? 1U : 0U, pdMS_TO_TICKS(20));
    }
}

static void sensor_task(void *arg)
{
    (void)arg;
    bool state[SRC_COUNT];

    app_wait_system_ready();
    ir_sensor_init(on_ir_edge_isr);
    light_sensor_init(on_light_edge_isr);

    /* Đồng bộ trạng thái ban đầu cho ControlTask */
    for (uint32_t src = 0; src < SRC_COUNT; src++) {
        state[src] = read_source(src);
        report(src, state[src]);
    }

    for (;;) {
        uint32_t bits = 0;
        (void)xTaskNotifyWait(0, 0xFFFFFFFFUL, &bits, portMAX_DELAY);

        for (uint32_t src = 0; src < SRC_COUNT; src++) {
            if ((bits & NB_EDGE(src)) != 0U) {
                (void)xTimerReset(s_debounce[src], pdMS_TO_TICKS(10));   /* đếm lại từ đầu */
            }
            if ((bits & NB_STABLE(src)) != 0U) {
                bool now = read_source(src);
                if (now != state[src]) {             /* bằng mức cũ = chỉ là nhiễu */
                    state[src] = now;
                    report(src, now);
                }
            }
        }
    }
}

void sensor_task_create(void)
{
    s_debounce[SRC_IR]    = xTimerCreate("dbIR", pdMS_TO_TICKS(DEBOUNCE_IR_MS), pdFALSE,
                                         (void *)(uintptr_t)SRC_IR, on_debounce_expired);
    s_debounce[SRC_LIGHT] = xTimerCreate("dbLight", pdMS_TO_TICKS(DEBOUNCE_LIGHT_MS), pdFALSE,
                                         (void *)(uintptr_t)SRC_LIGHT, on_debounce_expired);
    configASSERT(s_debounce[SRC_IR] && s_debounce[SRC_LIGHT]);

    BaseType_t ok = xTaskCreate(sensor_task, "Sensor", STACK_SENSOR, NULL, PRIO_SENSOR, &s_task);
    configASSERT(ok == pdPASS);
}

/**
 * @file    control_task.c
 * @brief   [APPLICATION] ControlTask: bộ não của hệ thống. Nhận mọi sự kiện từ qEvents, chạy máy
 *          trạng thái đỗ xe, áp dụng cài đặt, công bố trạng thái qua mailbox qParkView.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 * Firmware hướng sự kiện: task chỉ chạy khi có sự kiện (xQueueReceive chặn vô thời hạn),
 * toàn bộ logic chuyển trạng thái nằm trong parking_fsm.c (thuần logic, có unit test).
 */
#include "app_interface.h"
#include "app_tasks.h"
#include "app_config.h"
#include "parking_fsm.h"
#include "task.h"

static void publish(const pk_fsm_t *f, const pk_settings_t *s, bool dark)
{
    park_view_t v = {
        .state          = (uint8_t)f->state,
        .dist_mm        = f->dist_mm,
        .show_cm        = f->show_cm,
        .ir_detected    = f->ir,
        .dark           = dark,
        .state_since_ms = f->state_since_ms,
        .park_count     = f->park_count,
        .leave_count    = f->leave_count,
        .settings       = *s,
    };
    (void)xQueueOverwrite(g_qParkView, &v);
}

/* Áp dụng một cài đặt: sửa trên bản sao, chỉ nhận nếu cả bộ tham số còn hợp lệ */
static void apply_setting(pk_settings_t *s, uint8_t id, uint16_t value)
{
    static const char *const names[SET_COUNT] = { "stop", "slow", "hold", "empty",
                                                     "day", "night" };
    pk_settings_t t = *s;
    uint32_t v = value;
    bool valid_conversion = true;
    switch (id) {
    case SET_STOP_CM:   t.stop_mm          = (uint16_t)(v * 10U);   break;
    case SET_SLOW_CM:   t.slow_mm          = (uint16_t)(v * 10U);   break;
    case SET_HOLD_S:    valid_conversion = pk_seconds_to_ms(value, &t.park_hold_ms);     break;
    case SET_EMPTY_S:   valid_conversion = pk_seconds_to_ms(value, &t.empty_timeout_ms); break;
    /* Độ sáng: giá trị > 255 ghim ở 255 để pk_settings_valid() từ chối (chỉ nhận 0..15) */
    case SET_DAY:       t.intensity_day    = (uint8_t)((v > 255U) ? 255U : v); break;
    case SET_NIGHT:     t.intensity_night  = (uint8_t)((v > 255U) ? 255U : v); break;
    default: return;
    }
    if (valid_conversion && pk_settings_valid(&t)) {
        *s = t;
        log_printf("[CTL] Dat %s = %u", names[id], (unsigned)value);
    } else {
        log_printf("[CTL] Tu choi %s=%u (gia tri/nguong khong hop le)", names[id], (unsigned)value);
    }
}

static void control_task(void *arg)
{
    (void)arg;
    pk_settings_t settings;
    pk_fsm_t      fsm;
    bool          dark = false;

    pk_settings_default(&settings);
    pk_fsm_init(&fsm, 0);
    publish(&fsm, &settings, dark);       /* có dữ liệu ngay cho các task khác peek */

    app_wait_system_ready();
    pk_fsm_init(&fsm, app_now_ms());
    log_printf("[CTL] San sang, trang thai %s", pk_state_name(fsm.state));

    for (;;) {
        app_event_t ev;
        if (xQueueReceive(g_qEvents, &ev, portMAX_DELAY) != pdPASS) {
            continue;
        }
        uint32_t   now  = app_now_ms();
        pk_state_t prev = fsm.state;
        bool changed = false;

        switch ((app_event_type_t)ev.type) {
        case EV_DIST:
            changed = pk_fsm_on_distance(&fsm, ev.value, now, &settings);
            break;
        case EV_IR:
            changed = pk_fsm_on_ir(&fsm, ev.value != 0U, now, &settings);
            break;
        case EV_LIGHT:
            dark = (ev.value != 0U);
            break;
        case EV_CMD_SET:
            apply_setting(&settings, ev.param, ev.value);
            break;
        case EV_CMD_RESET:
            pk_fsm_init(&fsm, now);
            changed = (prev != fsm.state);
            log_printf("[CTL] Xoa bo dem, ve trang thai TRONG");
            break;
        default:
            break;
        }

        if (changed) {
            if (fsm.dist_mm != DIST_NONE) {
                log_printf("[CTL] %s -> %s (%u cm)", pk_state_name(prev), pk_state_name(fsm.state),
                           (unsigned)(fsm.dist_mm / 10U));
            } else {
                log_printf("[CTL] %s -> %s", pk_state_name(prev), pk_state_name(fsm.state));
            }
            if (fsm.state == PK_PARKED) {
                log_printf("[CTL] Xe da do xong, luot do thu %u", (unsigned)fsm.park_count);
            }
        }
        publish(&fsm, &settings, dark);
    }
}

void control_task_create(void)
{
    BaseType_t ok = xTaskCreate(control_task, "Control", STACK_CONTROL, NULL, PRIO_CONTROL, NULL);
    configASSERT(ok == pdPASS);
}

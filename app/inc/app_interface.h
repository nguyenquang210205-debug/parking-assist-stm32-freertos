/**
 * @file    app_interface.h
 * @brief   [APPLICATION] "Hợp đồng" giao tiếp giữa các task. Cả nhóm thống nhất file này
 *          TRƯỚC khi code; sau đó mỗi người chỉ code task của mình.
 *
 * Quy tắc:
 *  - Task KHÔNG gọi hàm, KHÔNG đọc/ghi biến của task người khác.
 *  - Mọi trao đổi đi qua các kênh dưới đây. Kiểm tra lỗi: xem dữ liệu trên kênh đúng quy ước chưa
 *    -> sai thì lỗi ở bên gửi, đúng mà phản ứng sai thì lỗi ở bên nhận.
 *
 *  Kênh          Loại                        Gửi                            Nhận
 *  -----------   -------------------------   ----------------------------   ------------------------------
 *  qEvents       queue 10 x app_event_t      SonarTask, SensorTask,         ControlTask
 *                                            ConsoleTask
 *  qParkView     mailbox 1 x park_view_t     ControlTask (overwrite)        DisplayTask, SonarTask,
 *                                                                           ConsoleTask (peek)
 *  qSonarStat    mailbox 1 x sonar_stat_t    SonarTask (overwrite)          ConsoleTask (peek)
 *  qLog          queue 16 x log_msg_t        mọi task (log_printf)          LogTask
 *  egSystem      event group                 ConsoleTask                    mọi task
 *                  bit READY: hiệu chuẩn đo tải CPU xong, hệ thống bắt đầu chạy
 *                  bit WATCH: SonarTask in từng lần đo (lệnh "watch 1")
 */
#ifndef APP_INTERFACE_H
#define APP_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "parking_types.h"

/* ---------------- qEvents ---------------- */
typedef enum {
    EV_DIST = 0,     /* SonarTask: value = khoảng cách đã lọc (mm) hoặc DIST_NONE */
    EV_IR,           /* SensorTask: value = 1 hồng ngoại thấy vật, 0 hết vật (đã chống nhiễu) */
    EV_LIGHT,        /* SensorTask: value = 1 trời tối, 0 trời sáng (đã chống nhiễu) */
    EV_CMD_SET,      /* ConsoleTask: param = app_setting_id_t, value = giá trị mới */
    EV_CMD_RESET     /* ConsoleTask: xóa bộ đếm lượt xe, về trạng thái trống */
} app_event_type_t;

typedef enum {
    SET_STOP_CM = 0,  /* vạch dừng (cm) */
    SET_SLOW_CM,      /* bắt đầu vùng chậm (cm) */
    SET_HOLD_S,       /* thời gian đứng yên để coi là đã đỗ (s) */
    SET_EMPTY_S,      /* thời gian không thấy xe để về trống (s) */
    SET_DAY,          /* độ sáng LED khi trời sáng (0..15) */
    SET_NIGHT,        /* độ sáng LED khi trời tối (0..15) */
    SET_COUNT
} app_setting_id_t;

typedef struct {
    uint8_t  type;    /* app_event_type_t */
    uint8_t  param;
    uint16_t value;
} app_event_t;

/* ---------------- qParkView (mailbox): kiểu park_view_t trong parking_types.h ---------------- */

/* ---------------- qSonarStat (mailbox) ---------------- */
typedef struct {
    uint16_t last_echo_us;   /* độ rộng xung Echo lần đo gần nhất (0 nếu quá hạn) */
    uint16_t last_raw_mm;    /* khoảng cách thô, DIST_NONE nếu ngoài tầm */
    uint16_t filtered_mm;    /* sau bộ lọc trung vị */
    uint16_t period_ms;      /* chu kỳ đo hiện tại */
    uint32_t ok_count;       /* số lần đo có tiếng vọng hợp lệ */
    uint32_t miss_count;     /* số lần quá hạn hoặc ngoài tầm */
} sonar_stat_t;

/* ---------------- qLog ---------------- */
#define LOG_MSG_LEN 64U
typedef struct {
    char text[LOG_MSG_LEN];
} log_msg_t;

/* ---------------- egSystem ---------------- */
#define EVT_SYSTEM_READY  (1UL << 0)
#define EVT_WATCH_SONAR   (1UL << 1)
#define EVT_BEEP_TEST     (1UL << 2)   /* lệnh "beep 1": bật còi liên tục để thử phần cứng */

extern QueueHandle_t      g_qEvents;
extern QueueHandle_t      g_qParkView;
extern QueueHandle_t      g_qSonarStat;
extern QueueHandle_t      g_qLog;
extern EventGroupHandle_t g_egSystem;

/** Tạo toàn bộ queue / event group. Gọi một lần trong main() trước khi tạo task. */
void app_ipc_init(void);

/** Chặn task đến khi bit READY được đặt. */
void app_wait_system_ready(void);

/** Gửi sự kiện vào qEvents (dùng trong task hoặc callback software timer). */
bool app_post_event(uint8_t type, uint8_t param, uint16_t value, TickType_t wait);

/** Thời gian từ lúc khởi động (ms). */
uint32_t app_now_ms(void);

/** In một dòng log (tự thêm "\r\n"). Định dạng giống printf, xem xprintf.h. */
void log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* APP_INTERFACE_H */

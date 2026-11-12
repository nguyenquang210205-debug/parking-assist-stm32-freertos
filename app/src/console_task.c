/**
 * @file    console_task.c
 * @brief   [APPLICATION] ConsoleTask: (1) nhận lệnh từ máy tính qua USB-TTL, phân tích, chuyển
 *          thành sự kiện cho ControlTask hoặc trả về thông tin; (2) giám sát hệ thống: hiệu chuẩn
 *          và đo tải CPU mỗi giây, nháy LED PC13 báo hệ thống còn sống.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 * Trình tự khởi động:
 *   1. Mọi task khác đang chờ bit READY của egSystem -> chỉ còn Idle task chạy.
 *   2. ConsoleTask ngủ 1 s; trong lúc đó idle hook đếm được idle_max (CPU rảnh 100%).
 *   3. Đặt bit READY -> các task khác bắt đầu làm việc.
 *
 * Lệnh (kết thúc bằng Enter, terminal 115200 8N1):
 *   help                     danh sách lệnh
 *   status                   trạng thái chỗ đỗ, khoảng cách, bộ đếm, cài đặt
 *   sonar                    số liệu thô của HC-SR04 (echo, mm, tỉ lệ đo lỗi)
 *   watch <0|1>              in từng lần đo siêu âm
 *   set <ten> <gia tri>      stop|slow|detect|close (cm), hold|empty (giây)
 *   reset                    xóa bộ đếm, về trạng thái trống
 *   cpu                      bảng tải CPU + stack của từng task
 *   sleep <0|1>              cho CPU ngủ (__WFI) khi rảnh
 *   log <0|1>                in mỗi giây một dòng CSV (vẽ đồ thị trên máy tính)
 */
#include "app_interface.h"
#include "app_tasks.h"
#include "app_config.h"
#include "console.h"
#include "cpu_monitor.h"
#include "parking_fsm.h"
#include "board.h"
#include "task.h"
#include <string.h>

static TaskHandle_t s_task;
static bool         s_periodic_log;

/* Ngữ cảnh ISR: console middleware báo đã có một dòng hoàn chỉnh */
static void on_line_isr(void)
{
    BaseType_t woken = pdFALSE;
    if (s_task != NULL) {
        vTaskNotifyGiveFromISR(s_task, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

/* ---------------- Phân tích dòng lệnh ---------------- */
/* Tách từ đầu tiên của chuỗi: trả về con trỏ tới phần còn lại (hoặc NULL) và cắt chuỗi */
static char *split_arg(char *line)
{
    char *p = line;
    while (*p != '\0' && *p != ' ') { p++; }
    if (*p == '\0') { return NULL; }
    *p++ = '\0';
    while (*p == ' ') { p++; }
    return (*p != '\0') ? p : NULL;
}

static bool parse_uint(const char *s, uint32_t *out)
{
    if (s == NULL || *s == '\0') { return false; }
    uint32_t v = 0;
    while (*s != '\0') {
        if (*s < '0' || *s > '9') { return false; }
        v = v * 10U + (uint32_t)(*s++ - '0');
        if (v > 60000U) { return false; }
    }
    *out = v;
    return true;
}

static void print_mm(const char *label, uint16_t mm)
{
    if (mm == DIST_NONE) { log_printf("%s: --", label); }
    else                 { log_printf("%s: %u.%u cm", label, (unsigned)(mm / 10U), (unsigned)(mm % 10U)); }
}

static void cmd_help(void)
{
    log_printf("Lenh: help status sonar watch<0|1> beep<0|1> reset");
    log_printf("      set stop|slow <cm>");
    log_printf("      set hold|empty <giay>");
    log_printf("      set day|night <0..15>  (do sang LED)");
    log_printf("      cpu sleep<0|1> log<0|1>");
}

static void cmd_status(void)
{
    park_view_t v;
    if (xQueuePeek(g_qParkView, &v, 0) != pdPASS) {
        log_printf("Chua co du lieu");
        return;
    }
    log_printf("Trang thai: %s (%lu s)  hong ngoai: %s  troi: %s",
               pk_state_name((pk_state_t)v.state),
               (unsigned long)((app_now_ms() - v.state_since_ms) / 1000U),
               v.ir_detected ? "CO VAT" : "trong", v.dark ? "TOI" : "SANG");
    print_mm("Khoang cach", v.dist_mm);
    log_printf("Luot do: %u  luot roi: %u", (unsigned)v.park_count, (unsigned)v.leave_count);
    log_printf("Cai dat: stop %u slow %u cm",
               (unsigned)(v.settings.stop_mm / 10U), (unsigned)(v.settings.slow_mm / 10U));
    log_printf("        hold %u s  empty %u s", (unsigned)(v.settings.park_hold_ms / 1000U),
               (unsigned)(v.settings.empty_timeout_ms / 1000U));
    log_printf("        do sang LED: ngay %u  dem %u (dang dung %u)", (unsigned)v.settings.intensity_day,
               (unsigned)v.settings.intensity_night,
               (unsigned)(v.dark ? v.settings.intensity_night : v.settings.intensity_day));
}

static void cmd_sonar(void)
{
    sonar_stat_t s;
    if (xQueuePeek(g_qSonarStat, &s, 0) != pdPASS) {
        log_printf("HC-SR04 chua do lan nao");
        return;
    }
    log_printf("Echo %u us  chu ky %u ms", (unsigned)s.last_echo_us, (unsigned)s.period_ms);
    print_mm("Tho", s.last_raw_mm);
    print_mm("Da loc", s.filtered_mm);
    uint32_t total = s.ok_count + s.miss_count;
    log_printf("Do duoc %lu / %lu lan (loi %lu%%)", (unsigned long)s.ok_count, (unsigned long)total,
               (unsigned long)(total ? (s.miss_count * 100U / total) : 0U));
}

static void cmd_set(char *args)
{
    static const char *const names[SET_COUNT] = { "stop", "slow", "hold", "empty",
                                                     "day", "night" };
    char *val = (args != NULL) ? split_arg(args) : NULL;
    uint32_t v = 0;
    if (args == NULL || !parse_uint(val, &v) || v > 4000U) {
        log_printf("Cu phap: set <ten> <so>, vi du: set stop 25");
        return;
    }
    for (uint8_t id = 0; id < SET_COUNT; id++) {
        if (strcmp(args, names[id]) == 0) {
            /* ControlTask tự kiểm tra tính hợp lệ của cả bộ tham số */
            (void)app_post_event(EV_CMD_SET, id, (uint16_t)v, pdMS_TO_TICKS(50));
            return;
        }
    }
    log_printf("Khong co tham so '%s'", args);
}

static void cmd_cpu(void)
{
    static cpu_stats_t st;   /* static: tránh chiếm ~240 byte stack của task */
    if (!cpu_monitor_get(&st, pdMS_TO_TICKS(50))) {
        log_printf("Khong lay duoc thong ke");
        return;
    }
    log_printf("Uptime %lus  sleep %s  heap trong %lu (min %lu) B",
               (unsigned long)st.uptime_s, st.sleep_enabled ? "BAT" : "TAT",
               (unsigned long)st.heap_free, (unsigned long)st.heap_min_free);
    log_printf("Tai CPU: idle-hook %u.%u%%  run-time %u.%u%%",
               (unsigned)(st.load_idlehook_x10 / 10U), (unsigned)(st.load_idlehook_x10 % 10U),
               (unsigned)(st.load_runtime_x10 / 10U), (unsigned)(st.load_runtime_x10 % 10U));
    log_printf("  (idle %lu lan/s, hieu chuan %lu lan/s)",
               (unsigned long)st.idle_count, (unsigned long)st.idle_max);
    log_printf("%-10s %4s %7s %10s", "Task", "Prio", "CPU", "Stack trong");
    for (uint8_t i = 0; i < st.task_count; i++) {
        const cpu_task_stat_t *t = &st.tasks[i];
        log_printf("%-10s %4u %4u.%u%% %6u word", t->name, (unsigned)t->priority,
                   (unsigned)(t->cpu_x10 / 10U), (unsigned)(t->cpu_x10 % 10U),
                   (unsigned)t->stack_free_words);
    }
}

static void handle_line(char *line)
{
    char *arg = split_arg(line);
    uint32_t v = 0;
    bool has_num = parse_uint(arg, &v);

    if (strcmp(line, "help") == 0)        { cmd_help(); }
    else if (strcmp(line, "status") == 0) { cmd_status(); }
    else if (strcmp(line, "sonar") == 0)  { cmd_sonar(); }
    else if (strcmp(line, "set") == 0)    { cmd_set(arg); }
    else if (strcmp(line, "reset") == 0)  { (void)app_post_event(EV_CMD_RESET, 0, 0, pdMS_TO_TICKS(50)); }
    else if (strcmp(line, "beep") == 0 && has_num && v <= 1U) {
        if (v == 1U) { (void)xEventGroupSetBits(g_egSystem, EVT_BEEP_TEST); }
        else         { (void)xEventGroupClearBits(g_egSystem, EVT_BEEP_TEST); }
        log_printf("[BEEP] Coi thu: %s (PB12+PB13 keo xuong GND)", (v == 1U) ? "BAT" : "TAT");
    }
    else if (strcmp(line, "watch") == 0 && has_num && v <= 1U) {
        if (v == 1U) { (void)xEventGroupSetBits(g_egSystem, EVT_WATCH_SONAR); }
        else         { (void)xEventGroupClearBits(g_egSystem, EVT_WATCH_SONAR); }
    } else if (strcmp(line, "cpu") == 0) {
        cmd_cpu();
    } else if (strcmp(line, "sleep") == 0 && has_num && v <= 1U) {
        cpu_monitor_set_sleep(v == 1U);
        log_printf("CPU ngu khi ranh: %s", (v == 1U) ? "BAT" : "TAT");
    } else if (strcmp(line, "log") == 0 && has_num && v <= 1U) {
        s_periodic_log = (v == 1U);
        if (s_periodic_log) { log_printf("CSV: CPU,thoi_gian_s,tai_idlehook,tai_runtime,ngu,trang_thai,cm"); }
    } else {
        log_printf("Lenh khong hop le (%s). Go 'help'", line);
    }
}

/* ---------------- Giám sát: chạy mỗi giây ---------------- */
static void monitor_tick(void)
{
    cpu_monitor_sample();
    board_led_toggle();
    if (!s_periodic_log) {
        return;
    }
    cpu_stats_t *st = NULL;
    static cpu_stats_t buf;
    if (cpu_monitor_get(&buf, pdMS_TO_TICKS(10))) { st = &buf; }
    park_view_t v;
    bool has_view = (xQueuePeek(g_qParkView, &v, 0) == pdPASS);
    if (st != NULL && has_view) {
        /* Định dạng CSV gọn để dán vào Excel / vẽ đồ thị */
        log_printf("CPU,%lu,%u.%u,%u.%u,%u,%u,%d", (unsigned long)st->uptime_s,
                   (unsigned)(st->load_idlehook_x10 / 10U), (unsigned)(st->load_idlehook_x10 % 10U),
                   (unsigned)(st->load_runtime_x10 / 10U), (unsigned)(st->load_runtime_x10 % 10U),
                   (unsigned)st->sleep_enabled, (unsigned)v.state,
                   (v.dist_mm == DIST_NONE) ? -1 : (int)(v.dist_mm / 10U));
    }
}

static void console_task(void *arg)
{
    (void)arg;
    char line[CONSOLE_LINE_MAX];

    /* --- Hiệu chuẩn đo tải CPU khi các task khác còn đang chờ READY --- */
    vTaskDelay(pdMS_TO_TICKS(10));
    cpu_monitor_calibrate_begin();
    vTaskDelay(pdMS_TO_TICKS(MONITOR_CALIB_MS));
    uint32_t idle_max = cpu_monitor_calibrate_end();
    (void)xEventGroupSetBits(g_egSystem, EVT_SYSTEM_READY);

    log_printf("=== Ho tro do xe (FreeRTOS %s) - go 'help' ===", tskKERNEL_VERSION_NUMBER);
    log_printf("[MON] Hieu chuan: idle_max = %lu lan/s", (unsigned long)idle_max);

    TickType_t next = xTaskGetTickCount() + pdMS_TO_TICKS(MONITOR_PERIOD_MS);
    for (;;) {
        /* Ngủ đến khi có dòng lệnh mới HOẶC tới mốc đo tải CPU tiếp theo */
        TickType_t now  = xTaskGetTickCount();
        TickType_t wait = ((int32_t)(next - now) > 0) ? (next - now) : 0;
        if (ulTaskNotifyTake(pdTRUE, wait) > 0U) {
            while (console_readline(line, sizeof(line))) {
                handle_line(line);
            }
        }
        if ((int32_t)(xTaskGetTickCount() - next) >= 0) {
            next += pdMS_TO_TICKS(MONITOR_PERIOD_MS);
            monitor_tick();
        }
    }
}

void console_task_create(void)
{
    cpu_monitor_init();
    console_init(CONSOLE_BAUD, on_line_isr);
    BaseType_t ok = xTaskCreate(console_task, "Console", STACK_CONSOLE, NULL, PRIO_CONSOLE, &s_task);
    configASSERT(ok == pdPASS);
}

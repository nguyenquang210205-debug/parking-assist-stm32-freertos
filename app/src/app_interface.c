/**
 * @file    app_interface.c
 * @brief   [APPLICATION] Tạo các kênh giao tiếp dùng chung + hàm tiện ích gửi sự kiện / log.
 * @owner   Cả nhóm (Người 3 giữ file)
 */
#include "app_interface.h"
#include "task.h"
#include "xprintf.h"
#include <stdarg.h>
#include <string.h>

QueueHandle_t      g_qEvents;
QueueHandle_t      g_qParkView;
QueueHandle_t      g_qSonarStat;
QueueHandle_t      g_qLog;
EventGroupHandle_t g_egSystem;

void app_ipc_init(void)
{
    g_qEvents    = xQueueCreate(10, sizeof(app_event_t));
    g_qParkView  = xQueueCreate(1, sizeof(park_view_t));    /* mailbox: overwrite / peek */
    g_qSonarStat = xQueueCreate(1, sizeof(sonar_stat_t));   /* mailbox */
    g_qLog       = xQueueCreate(16, sizeof(log_msg_t));
    g_egSystem   = xEventGroupCreate();

    configASSERT(g_qEvents && g_qParkView && g_qSonarStat && g_qLog && g_egSystem);
}

void app_wait_system_ready(void)
{
    (void)xEventGroupWaitBits(g_egSystem, EVT_SYSTEM_READY,
                              pdFALSE,          /* không xóa bit: mọi task đều thấy */
                              pdTRUE, portMAX_DELAY);
}

bool app_post_event(uint8_t type, uint8_t param, uint16_t value, TickType_t wait)
{
    app_event_t ev = { type, param, value };
    return xQueueSendToBack(g_qEvents, &ev, wait) == pdPASS;
}

uint32_t app_now_ms(void)
{
    return (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
}

void log_printf(const char *fmt, ...)
{
    log_msg_t msg;
    va_list ap;
    va_start(ap, fmt);
    int n = xvsnprintf(msg.text, LOG_MSG_LEN - 2U, fmt, ap);
    va_end(ap);
    if (n < 0) { return; }
    size_t len = strlen(msg.text);
    msg.text[len++] = '\r';
    msg.text[len++] = '\n';
    msg.text[len] = '\0';
    /* Chờ tối đa 50 ms nếu qLog đầy; quá hạn thì bỏ dòng log (không làm treo task gọi) */
    (void)xQueueSendToBack(g_qLog, &msg, pdMS_TO_TICKS(50));
}

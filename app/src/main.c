/**
 * @file    main.c
 * @brief   [APPLICATION] Điểm vào: khởi tạo bo mạch, tạo kênh giao tiếp, tạo task, chạy scheduler.
 *          main.c KHÔNG chứa logic chức năng và KHÔNG truy cập thanh ghi.
 *
 * Đề tài: Hệ thống hỗ trợ đỗ xe thông minh trên nền FreeRTOS.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "app_interface.h"
#include "app_tasks.h"

int main(void)
{
    board_init();          /* 72 MHz, clock GPIO, nhóm ưu tiên NVIC, LED PC13 */
    app_ipc_init();        /* qEvents, qParkView, qSonarStat, qLog, egSystem */

    /* Người 2: tạo trước để kênh log sẵn sàng */
    log_task_create();
    display_task_create();
    /* Người 3 */
    control_task_create();
    console_task_create();
    oled_task_create();    /* màn OLED giám sát tải CPU */
    /* Người 1 */
    sonar_task_create();
    sensor_task_create();

    vTaskStartScheduler(); /* tạo Idle task + Timer daemon, không bao giờ trả về nếu đủ heap */

    for (;;) { }           /* chỉ tới đây khi thiếu heap cho Idle/Timer task */
}

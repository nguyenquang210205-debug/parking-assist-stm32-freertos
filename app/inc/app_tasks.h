/**
 * @file    app_tasks.h
 * @brief   [APPLICATION] Hàm tạo task của từng module. main() chỉ gọi các hàm này.
 */
#ifndef APP_TASKS_H
#define APP_TASKS_H

/* Người 1 - Đo khoảng cách + cảm biến */
void sonar_task_create(void);
void sensor_task_create(void);

/* Người 2 - Hiển thị + xuất dữ liệu */
void display_task_create(void);
void log_task_create(void);

/* Người 3 - Điều khiển + giám sát hệ thống */
void control_task_create(void);
void console_task_create(void);
void oled_task_create(void);

#endif /* APP_TASKS_H */

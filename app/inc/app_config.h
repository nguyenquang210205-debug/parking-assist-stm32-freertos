/**
 * @file    app_config.h
 * @brief   [APPLICATION] Bảng ưu tiên, kích thước stack và chu kỳ của các task.
 * @owner   Cả nhóm thống nhất (đổi giá trị phải báo các thành viên)
 *
 *  Task          Người  Ưu tiên  Stack (word)  Kích hoạt
 *  -----------   -----  -------  ------------  ------------------------------------------------
 *  SonarTask       1       3        160        tuần hoàn 60 ms (đang có xe) / 300 ms (trống, đã đỗ)
 *  SensorTask      1       3        160        notification từ ISR EXTI + timer chống nhiễu
 *  (Timer daemon)  -       3        160        software timer của FreeRTOS
 *  ControlTask     3       2        192        qEvents
 *  ConsoleTask     3       2        256        notification từ ISR USART1 + mốc 1 s đo tải CPU
 *  DisplayTask     2       1        192        tuần hoàn 50 ms (20 khung hình/giây)
 *  LogTask         2       1        160        qLog
 *  OledTask        3       1        192        tuần hoàn 0,5 s (trạng thái, khoảng cách, tải CPU + đồ thị lên OLED I2C)
 *  (Idle)          -       0        128        khi không task nào sẵn sàng
 *
 * Nguyên tắc chọn ưu tiên: task lấy dữ liệu cảm biến phải đúng nhịp, chạy rất ngắn -> cao;
 * task quyết định ở giữa; task hiển thị và in log tốn thời gian, không gấp -> thấp.
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define PRIO_SONAR     3
#define PRIO_SENSOR    3
#define PRIO_CONTROL   2
#define PRIO_CONSOLE   2
#define PRIO_DISPLAY   1
#define PRIO_LOG       1
#define PRIO_OLED      1

#define STACK_SONAR    160
#define STACK_SENSOR   160
#define STACK_CONTROL  192
#define STACK_CONSOLE  256
#define STACK_DISPLAY  192
#define STACK_LOG      160
#define STACK_OLED     192

#define SONAR_PERIOD_FAST_MS     60U     /* HC-SR04 cần >= 60 ms giữa 2 lần đo */
#define SONAR_PERIOD_SLOW_MS     300U    /* khi trống / đã đỗ: đo thưa để tiết kiệm */
#define SONAR_ECHO_TIMEOUT_MS    50U     /* HC-SR04 tự hạ Echo sau ~38 ms nếu không có vật */
#define DEBOUNCE_IR_MS           50U
#define DEBOUNCE_LIGHT_MS        1000U   /* sáng/tối phải ổn định 1 s */
#define DISPLAY_PERIOD_MS        50U
#define MONITOR_PERIOD_MS        1000U
#define MONITOR_CALIB_MS         1000U
#define OLED_PERIOD_MS           500U    /* vẽ lại OLED mỗi 0,5 s (đồ thị CPU vẫn 1 mẫu/giây) */
#define CONSOLE_BAUD             115200U

#endif /* APP_CONFIG_H */

Nhiệm vụ 3 - ControlTask, ConsoleTask và OledTask
1. Mục tiêu
Phụ trách điều khiển trạng thái đỗ xe và giám sát hoạt động của hệ thống
STM32F103 sử dụng FreeRTOS, gồm xử lý sự kiện, nhận lệnh UART và hiển thị OLED.
2. Các file phụ trách
- app/src/control_task.c
- app/src/parking_fsm.c
- app/inc/parking_fsm.h
- app/src/console_task.c
- app/src/oled_task.c
- app/src/cpu_view.c
- app/inc/cpu_view.h
- middleware/src/cpu_monitor.c
- middleware/src/ssd1306.c
3. ControlTask
- Nhận sự kiện từ queue g_qEvents bằng xQueueReceive.
- Xử lý sự kiện khoảng cách, hồng ngoại, ánh sáng và lệnh cài đặt/reset.
- Gọi các hàm trong parking_fsm.c để cập nhật trạng thái đỗ xe.
- Công bố trạng thái cho các task khác qua mailbox g_qParkView bằng xQueueOverwrite.
4. Parking FSM
- Quản lý các trạng thái EMPTY, APPROACH, STOP, PARKED và LEAVING.
- Chuyển trạng thái dựa trên tín hiệu hồng ngoại, khoảng cách và thời gian.
- Quản lý bộ đếm lượt đỗ và lượt rời khỏi chỗ đỗ.
- Tách logic trạng thái khỏi FreeRTOS và phần cứng để có thể kiểm thử độc lập.
5. ConsoleTask
- Nhận dòng lệnh từ UART qua task notification do ISR thông báo.
- Hỗ trợ các lệnh help, status, sonar, watch, set, reset, cpu, sleep, log và beep.
- Chuyển lệnh điều khiển thành sự kiện gửi tới ControlTask.
- Hiệu chuẩn, lấy mẫu tải CPU mỗi giây và nháy LED PC13 để báo hệ thống đang chạy.
- Đặt bit EVT_SYSTEM_READY sau khi hiệu chuẩn để các task khác bắt đầu hoạt động.
6. OledTask
- Sử dụng màn OLED SSD1306 giao tiếp qua I2C2 (PB10/PB11).
- Đọc trạng thái và khoảng cách từ g_qParkView bằng xQueuePeek.
- Đọc số liệu tải CPU qua cpu_monitor_get và lưu lịch sử để vẽ đồ thị.
- Cập nhật màn hình theo chu kỳ OLED_PERIOD_MS (500 ms) bằng xTaskDelayUntil.
- Nếu OLED chưa kết nối hoặc mất kết nối, thử khởi tạo lại theo chu kỳ.
7. Tích hợp
- control_task_create, console_task_create và oled_task_create được gọi trong app/src/main.c.
- Queue, mailbox và event group được khai báo, quản lý qua app_interface.
- Các mức ưu tiên và kích thước stack được cấu hình trong app/inc/app_config.h.
8. Git workflow
- Nhánh: feature/task3-documentation
- Nhánh tích hợp: develop
Tài liệu này chỉ mô tả nhiệm vụ 3. Không thay đổi mã nguồn STM32 FreeRTOS.

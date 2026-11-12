# Nhiệm vụ 1 - SonarTask và SensorTask

## 1. Mục tiêu
Phụ trách đo khoảng cách từ đuôi xe tới tường (HC-SR04), đọc cảm biến hồng ngoại ở cửa vào
và cảm biến quang, lọc nhiễu rồi gửi sự kiện cho ControlTask qua queue `qEvents`.

## 2. Các file phụ trách
- Driver: `drivers/src/gpio.c`, `exti.c`, `capture.c` (TIM4 input capture), `delay.c` (DWT)
- Middleware: `middleware/src/hcsr04.c`, `ir_sensor.c`, `light_sensor.c`
- App: `app/src/sonar_task.c`, `app/src/sensor_task.c`, `app/src/dist_filter.c`, `app/inc/dist_filter.h`

## 3. Giao tiếp phần cứng
| Module | Chân | Ngoại vi | Mức logic |
|---|---|---|---|
| HC-SR04 Trig | PB0 | GPIO output | Xung 12 us (datasheet yêu cầu >= 10 us) |
| HC-SR04 Echo | PB6 | TIM4_CH1 (PWM input) | 5 V, PB6 chịu được 5 V |
| Hồng ngoại OUT | PA1 | EXTI1, cả 2 cạnh | 0 = có vật |
| Quang trở DO | PA0 | EXTI0, cả 2 cạnh | 1 = trời tối |

## 4. SonarTask (ưu tiên 3, stack 160 word)
- Chu kỳ: 60 ms khi có xe, 300 ms khi bãi trống (xem trạng thái bằng `xQueuePeek(qParkView)`).
- Mỗi chu kỳ:
  1. `hcsr04_trigger()`: bật capture TIM4 rồi phát xung Trig.
  2. ISR TIM4 đo độ rộng xung Echo (1 tick = 1 us) và gửi giá trị về task bằng
     `xTaskNotifyFromISR(..., eSetValueWithOverwrite)`. Không cần queue riêng.
  3. Task chờ bằng `xTaskNotifyWait`, timeout 50 ms. Quá hạn thì coi như mất Echo.
  4. Đổi ra mm: `d = t x 343 m/s / 2 = t(us) x 0,1715 mm`, giới hạn từ 2 cm đến 4 m.
  5. Lọc bằng `dist_filter_push()`, gửi `EV_DIST` vào `qEvents`, ghi thống kê vào mailbox `qSonarStat`.
  6. `xTaskDelayUntil` để giữ chu kỳ đều.
- Khi bãi trống chỉ gửi `EV_DIST` lúc giá trị đổi hơn 1 cm, để giảm tải cho ControlTask.

## 5. Bộ lọc khoảng cách (`dist_filter`)
- Cửa sổ 5 mẫu: sắp xếp rồi lấy trung bình 3 mẫu giữa (bỏ mẫu lớn nhất và nhỏ nhất).
- Chặn gai nhiễu: mẫu lệch hơn 3 cm bị bỏ. Nếu 3 mẫu liền nhau cùng lệch và gần nhau
  thì đó là xe dịch thật, nhận luôn giá trị mới.
- Vùng sát (≤ 5 cm): bỏ tiếng vọng kép (Echo báo gấp 2-3 lần khoảng cách thật),
  giữ giá trị lâu hơn (khoảng 2 s) khi mất Echo.
- Mất tín hiệu 5 lần liên tiếp thì báo `DIST_NONE` (không có vật).
- Module thuần logic, không phụ thuộc phần cứng và FreeRTOS, nên có unit test (`make test`).

## 6. SensorTask (ưu tiên 3, stack 160 word)
Dùng kỹ thuật trì hoãn xử lý ngắt (deferred interrupt processing):
1. ISR EXTI chỉ gửi task notification (bit "cạnh") cho SensorTask.
2. SensorTask reset software timer chống nhiễu của nguồn đó: hồng ngoại 50 ms, quang trở 1 s.
3. Tín hiệu đứng yên đủ lâu thì timer hết hạn, callback gửi notification (bit "ổn định").
4. SensorTask đọc lại mức chân, so với trạng thái cũ. Khác thì gửi `EV_IR` hoặc `EV_LIGHT`
   vào `qEvents`, giống thì bỏ qua (chỉ là nhiễu).

Lúc khởi động, task đọc trạng thái ban đầu của cả 2 cảm biến và báo cho ControlTask.

## 7. Đầu vào / đầu ra
| Task | Đầu vào | Đầu ra |
|---|---|---|
| SonarTask | Xung Echo (TIM4), trạng thái từ `qParkView` | `EV_DIST` (mm) vào `qEvents`, `qSonarStat` |
| SensorTask | Ngắt EXTI0/EXTI1, timer chống nhiễu | `EV_IR` (1/0), `EV_LIGHT` (1/0) vào `qEvents` |

## 8. Tích hợp
- Cả 2 task chờ bit READY trong `egSystem` (`app_wait_system_ready()`) rồi mới khởi tạo cảm biến.
- Task được tạo trong `app/src/main.c` qua `sonar_task_create()` và `sensor_task_create()`.
- Queue, mailbox và event group do `app_interface` quản lý.

## 9. Git workflow
Nhánh: `feature/task1-documentation`
Nhánh tích hợp: `develop`

Tài liệu này bổ sung mô tả Nhiệm vụ 1, không thay đổi mã nguồn STM32 FreeRTOS.

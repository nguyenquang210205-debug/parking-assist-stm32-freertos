# Hệ thống hỗ trợ đỗ xe thông minh trên nền FreeRTOS

STM32F103C8 (Blue Pill) · FreeRTOS V11.1.0 · Standard Peripheral Library V3.5.0 · Keil MDK (ARM Compiler 6)

Mô hình một chỗ đỗ xe có tường chắn ở cuối. Ô tô **lùi** vào chỗ đỗ, hệ thống báo cho tài xế biết xe còn cách tường bao nhiêu để dừng đúng lúc.

**Các module:**
- **Cảm biến hồng ngoại** ở lối vào: phát hiện xe bắt đầu vào chỗ đỗ.
- **Cảm biến siêu âm HC-SR04** gắn trên tường cuối: đo khoảng cách từ đuôi xe tới tường.
- **Ma trận LED 8x8 (MAX7219)** gắn trên tường: hướng dẫn tài xế.
- **Còi chip** (buzzer): bíp cảnh báo, xe càng gần tường bíp càng nhanh.
- **Quang trở**: trời tối thì ma trận LED sáng mạnh hơn cho tài xế dễ nhìn.
- **Màn OLED 0.96" SSD1306 (I2C)**: hiện **khoảng cách**, **trạng thái đỗ xe**, **% tải CPU** và **đồ thị tải CPU theo thời gian** (60 giây gần nhất).
- Máy tính nối qua **USB-TTL** để xem log, đặt ngưỡng, xem bộ đếm lượt xe và tải CPU.

**Diễn biến khi xe vào đỗ** (ngưỡng mặc định, đổi được bằng lệnh UART):

| Tình huống | Ma trận LED 8x8 | Còi |
|---|---|---|
| Chưa có xe (hồng ngoại chưa thấy gì) | **Sáng đỏ toàn bộ** | Im |
| **Hồng ngoại** ở lối vào thấy xe (chỉ hồng ngoại mới bật bước này), xe còn cách tường ≥ 10 cm | **Mũi tên** "đi vào" | Im |
| Cách tường 3–10 cm | **1 chữ số to**: 9, 8, 7 … 3 (cập nhật liên tục) + thanh vạch ở hàng dưới cùng ngắn dần | **Bíp**, càng gần càng nhanh |
| Cách tường ≤ 3 cm | Chữ **S** (Stop) | **Kêu liên tục** |
| Xe đứng yên ở vùng S đủ **4 giây** | Chữ **P** (đã đỗ), **giữ suốt** khi xe còn đỗ | Im |
| Xe chạy ra khỏi chỗ đỗ | Vẫn chữ **P** cho tới khi xe đi qua cửa (hồng ngoại) | Im |
| Xe đã ra khỏi cửa (hoặc được nhấc đi, hồng ngoại không thấy gì 5 giây) | Trở lại **sáng đỏ toàn bộ** | Im |

Lưu ý: HC-SR04 có vùng mù khoảng 2 cm, nên vạch dừng nhỏ nhất đặt được là 3 cm (đang dùng đúng mức này).

## 1. Sơ đồ cắm chân

Xem hình `docs/wiring.png`.

| Module | Chân module | Chân Blue Pill | Ghi chú |
|---|---|---|---|
| Siêu âm HC-SR04 | VCC | **5V** | HC-SR04 cần nguồn 5 V |
| | Trig | **PB0** | Output, xung 10 µs |
| | Echo | **PB6** | TIM4_CH1. PB6 chịu được 5 V (FT) nên nối thẳng được |
| | GND | GND | |
| Còi chủ động 2 chân (không cần transistor) | Còi (+) | **5V** | Chân dài, có dấu +. Bóc tem dán trên nắp |
| | Còi (−) | **PB12 và PB13** (nối chung cả hai) | Open-drain, chân chịu 5 V; kéo xuống GND thì còi kêu |
| OLED SSD1306 (I2C) | VCC | **3V3** | |
| | GND | GND | |
| | SCL | **PB10** | I2C2 SCL |
| | SDA | **PB11** | I2C2 SDA. Không dùng I2C1 vì PB6 đang là Echo |
| Ma trận LED MAX7219 | VCC | **5V** | |
| | GND | GND | |
| | DIN | **PA7** | SPI1 MOSI |
| | CS | **PA4** | GPIO |
| | CLK | **PA5** | SPI1 SCK |
| Hồng ngoại vật cản | VCC | 3V3 | |
| | GND | GND | |
| | OUT | **PA1** | EXTI1. OUT = 0 khi có vật |
| Quang trở LM393 | VCC | 3V3 | |
| | GND | GND | |
| | DO | **PA0** | EXTI0. DO = 1 khi tối |
| Mạch USB-TTL | RX | **PA9** | USART1 TX (DMA1 kênh 4) |
| | TX | **PA10** | USART1 RX |
| | GND | GND | **Bắt buộc nối chung GND** |
| ST-Link | SWDIO / SWCLK / GND / 3.3V | cổng SWD | Nạp và gỡ lỗi |

**Lưu ý phần cứng:**
- Mọi GND phải nối chung.
- Không nối chân Echo của HC-SR04 (mức 5 V) vào các chân PA0–PA7. Những chân này **không** chịu được 5 V.
- Chỉnh biến trở trên module hồng ngoại để nó báo có vật trong khoảng 10–20 cm.
- Nếu module hồng ngoại hoặc quang trở của nhóm ngược mức logic, đổi `IR_SENSOR_ACTIVE_LEVEL` (trong `ir_sensor.h`) hoặc `LIGHT_SENSOR_DARK_LEVEL` (trong `light_sensor.h`).
- Nếu dùng loại 4 module ghép (FC-16, 32×8), đặt `MAX7219_NUM_DEVICES=4` trong Keil, ở mục *Options → C/C++ → Define*. Khi đó màn hiện chữ "12cm", "STOP", "P 01".

## 2. Phân công: mỗi người 2 task

| Người | Task | Mã nguồn phụ trách |
|---|---|---|
| **Người 1 – Đo khoảng cách + cảm biến** | SonarTask, SensorTask | Driver: `gpio`, `exti`, `capture` (TIM4 input capture), `delay` (DWT)<br>Middleware: `hcsr04`, `ir_sensor`, `light_sensor`<br>App: `sonar_task`, `sensor_task`, `dist_filter` (lọc trung vị) |
| **Người 2 – Hiển thị + xuất dữ liệu** | DisplayTask, LogTask | Driver: `spi`, `uart` (+DMA)<br>Middleware: `max7219`, `buzzer`, `font3x5`, `font5x7`, `console`<br>App: `display_task`, `parking_view`, `log_task` |
| **Người 3 – Điều khiển + giám sát hệ thống** | ControlTask, ConsoleTask | Driver: `clock`, `nvic`, `timer` (TIM2)<br>Middleware: `board`, `cpu_monitor`, `ssd1306`, `xprintf`<br>App: `control_task`, `parking_fsm`, `console_task`, `hooks`, `main` |

Phần dùng chung do cả nhóm thống nhất gồm `app_interface.h/.c` (kênh giao tiếp), `app_config.h` (ưu tiên, stack, chu kỳ), `parking_types.h` và bảng chân trong `board.h`.

## 3. Kiến trúc phân tầng

```
app/         Application  : task, máy trạng thái, bộ lọc, vẽ màn hình (không truy cập thanh ghi)
middleware/  Middleware   : thư viện thiết bị (HC-SR04, MAX7219, cảm biến, console, đo CPU) + FreeRTOS
drivers/     Driver       : bọc SPL (GPIO, EXTI, SPI, USART+DMA, TIM2, TIM4 capture, DWT)
platform/    SPL V3.5.0, CMSIS, startup
```

Tầng trên chỉ gọi tầng ngay dưới. Driver không phụ thuộc FreeRTOS: nó báo sự kiện bằng callback. `main.c` chỉ khởi tạo và tạo task.

## 4. Task và kênh giao tiếp

| Task | Ưu tiên | Kích hoạt | Việc |
|---|---|---|---|
| SonarTask | 3 | 60 ms khi có xe / 300 ms khi trống | Trig, chờ notification từ ISR TIM4, đổi ra mm, lọc trung vị, gửi `EV_DIST` |
| SensorTask | 3 | Notification từ EXTI + timer chống nhiễu | Hồng ngoại (50 ms), quang trở (1 s), gửi `EV_IR` / `EV_LIGHT` |
| ControlTask | 2 | qEvents | Máy trạng thái đỗ xe, cài đặt, bộ đếm, ghi mailbox `qParkView` |
| ConsoleTask | 2 | Notification từ ISR USART1 + mốc 1 s | Lệnh UART, hiệu chuẩn và đo tải CPU, nháy LED PC13 |
| OledTask | 1 | 0,5 s | Peek `qParkView` (trạng thái, khoảng cách), lấy thống kê CPU (mutex), vẽ khung hình, gửi qua I2C2 |
| DisplayTask | 1 | 50 ms | Peek `qParkView`, vẽ khung hình, gửi SPI khi có thay đổi, bật/tắt còi theo nhịp bíp |
| LogTask | 1 | qLog | Gatekeeper UART TX (DMA + binary semaphore) |

Các kênh giao tiếp:
- **queue:** `qEvents`, `qLog`;
- **mailbox:** `qParkView`, `qSonarStat`;
- **event group:** `egSystem` với bit READY và WATCH;
- task notification từ ISR, software timer, mutex (bảng thống kê CPU), binary semaphore (DMA).

**Máy trạng thái (`parking_fsm.c`):**
- TRONG → XE VAO: **chỉ** khi hồng ngoại ở cửa thấy xe (siêu âm không tự bật bước này).
- XE VAO → DUNG: khi khoảng cách ≤ vạch dừng (3 cm).
- DUNG → DA DO: khi xe đứng trong vùng dừng 4 s.
- DA DO → XE RA: khi xe ra xa hơn vạch dừng 8 cm liên tục 0,5 s (vẫn hiện P).
- XE RA → TRONG: khi hồng ngoại thấy xe đi qua cửa rồi hết thấy (chính chiếc xe đang ra, không bị tính là xe mới vào); hoặc hồng ngoại không thấy gì 5 s (xe được nhấc đi).
- XE VAO → TRONG: hồng ngoại báo nhưng không có xe tới vùng gần trong 15 s (người đi ngang).
- Có vùng trễ 2 cm quanh vạch dừng để tránh nhảy qua lại giữa DUNG và XE VAO.

## 5. Lệnh trên terminal (115200 8N1)

| Lệnh | Tác dụng |
|---|---|
| `help` | Danh sách lệnh |
| `status` | Trạng thái, khoảng cách, bộ đếm lượt đỗ/rời, cài đặt |
| `sonar` | Echo (µs), khoảng cách thô và đã lọc, tỉ lệ đo lỗi |
| `watch 0/1` | In từng lần đo siêu âm |
| `set stop/slow <cm>` | Vạch dừng: hiện S (mặc định 3), vùng gần: hiện số và bíp (10, tức đếm từ 9). Đặt `slow` > 10 thì từ 10 cm trở lên hiện 2 chữ số nhỏ |
| `set hold/empty <giây>` | Thời gian để coi là đã đỗ (4); xe đang ra mà hồng ngoại không thấy gì bao lâu thì coi như đã đi (5) |
| `beep 1` / `beep 0` | Bật / tắt còi liên tục để thử phần cứng còi (không cần cho xe chạy) |
| `set day/night <0..15>` | Độ sáng ma trận LED khi trời sáng (mặc định 4) và khi trời tối (mặc định 15) |
| `reset` | Xóa bộ đếm, về trạng thái trống |
| `cpu` | Tải CPU theo hai phương pháp, % CPU và stack trống của từng task |
| `sleep 0/1` | Cho CPU ngủ (`__WFI`) khi rảnh |
| `log 0/1` | Mỗi giây in một dòng CSV: thời gian, tải CPU, trạng thái, cm |

## 6. Biên dịch và nạp

**Keil:**
1. Cài pack *Keil::STM32F1xx_DFP*.
2. Mở `keil/ParkingAssist.uvprojx`. **Không** bật thêm StdPeriph hay Startup trong Manage Run-Time Environment.
3. Build bằng F7, nạp bằng F8 qua ST-Link (Port: SW).

**Không có Keil:**
- `make`: biên dịch bằng arm-none-eabi-gcc. Kết quả hiện tại: Flash khoảng 29,6 KB (45%), RAM khoảng 17,2 KB (84%, đã gồm 12 KB heap FreeRTOS).
- `make test`: unit test trên máy tính cho `dist_filter`, `parking_fsm`, `parking_view`, `xprintf`. Hiện đạt 143/143.

## 7. Ghi chú thay đổi

**Chuyển từ HY-SRF05 sang HC-SR04**
- Module SRF05 cũ bị hỏng: chương trình soi xung cho thấy nó chỉ phát một xung cố định 54 µs, không phụ thuộc khoảng cách.
- HC-SR04 dùng cùng giao thức Trig/Echo, cùng nguồn 5 V và cùng chân PB0/PB6. Nó chỉ có 4 chân, không có chân OUT.
- Driver đổi tên `srf05.c/.h` → `hcsr04.c/.h`, và các hàm thành `hcsr04_init()`, `hcsr04_trigger()`.
- Thời gian chờ Echo tăng từ 40 ms lên 50 ms, vì HC-SR04 giữ Echo khoảng 38 ms khi không có vật. Chu kỳ đo giữ 60 ms, đúng mức tối thiểu trong datasheet.

**Sửa driver bắt xung Echo (`drivers/src/capture.c`)**
- Chỉ nhận xung có cạnh lên sau lần kích hiện tại (`capture_tim4_arm()`), nên không lấy nhầm xung muộn của lần đo trước.
- Phát hiện TIM4 tràn: xung dài hơn 65,5 ms bị báo là đo lỗi, thay vì ra một khoảng cách giả.
- Prescaler tính từ clock thật đọc ở RCC, không phụ thuộc hằng số 72 MHz.

**Độ sáng ma trận LED theo quang trở**
- Trời tối thì màn LED **sáng mạnh hơn**: mặc định ban ngày mức 4, ban đêm mức 15 (MAX7219 có 16 mức, 0..15).
- Hai mức này nằm trong bộ cài đặt (`pk_settings_t`), đổi được khi đang chạy bằng lệnh `set day <0..15>` và `set night <0..15>`. Lệnh `status` in ra độ sáng đang dùng.

**Theo ý tưởng lùi xe vào chỗ đỗ (bản hiện tại)**
- Vạch dừng 5 cm: ma trận hiện chữ **S** (thay cho biển STOP nhấp nháy). Bỏ ngưỡng "đi quá" và dấu X.
- Hiện số cm từ 20 cm trở vào (2 chữ số nhỏ trên 8x8). Xa hơn thì hiện mũi tên.
- Thêm **còi chip 2 chân** ở PB1, đi qua transistor NPN (`middleware/buzzer.c`). Còi chủ động: bật/tắt chân. Còi thụ động: đặt `BUZZER_PASSIVE 1` trong `buzzer.h`, code phát PWM 2,7 kHz bằng TIM3_CH4 (`drivers/pwm.c`). Nhịp bíp tính trong `pv_beep()` (`parking_view.c`, có unit test): chu kỳ 800 ms ở mép vùng gần (20 cm), giảm dần còn 120 ms sát vạch dừng; tới vạch dừng thì kêu liên tục; đã đỗ thì im.

**Màn OLED giám sát tải CPU**
- OLED SSD1306 128x64 nối I2C2 (PB10 = SCL, PB11 = SDA), driver `drivers/i2c.c` (polling, có thời gian chờ, tự gỡ bus khi lỗi) và `middleware/ssd1306.c`.
- `OledTask` (ưu tiên 1, chu kỳ 1 s): lấy bảng thống kê từ `cpu_monitor_get()`, lưu 60 mẫu, vẽ bằng `cv_render()` (`app/cpu_view.c`, có unit test), rồi gửi khung 1 KB qua I2C.
- Dòng trên: tải CPU theo run-time stats (chữ to). Dòng dưới: tải theo idle hook (IH), thang đồ thị (tự chọn 5/10/20/50/100%), thời gian chạy. Bên dưới là đồ thị cột 60 giây.
- Không cắm OLED thì task log một lần rồi thử lại mỗi 5 giây, phần còn lại vẫn chạy bình thường. Màn 0.91" (128x32): đặt `OLED_HEIGHT=32`. Địa chỉ 0x3D: đặt `OLED_I2C_ADDR=0x3D`.
- Gửi 1 KB qua I2C mỗi giây mất khoảng 25 ms, nên bản thân OLED làm tải CPU tăng khoảng 2–3%.

**Ngưỡng cho mô hình**
- Mặc định đổi về cỡ mô hình xe đồ chơi: hiện S ở 5 cm, hiện số và bíp từ 20 cm, siêu âm chỉ coi là có xe trong 40 cm (tránh bắt nhầm người hay vật bên ngoài mô hình).
- Đứng yên 2 giây thì coi là đã đỗ, vắng 3 giây thì về trống, chạy ra quá vạch dừng 8 cm thì coi là xe ra.

**Hiện 1 chữ số to, dừng ở 3 cm**
- Mặc định: dưới 10 cm hiện **một chữ số to** (cao 7 điểm, dễ đọc từ xa): 9, 8, … 3; hàng dưới cùng là thanh vạch ngắn dần. Tới 3 cm hiện **S**.
- Vùng trễ 2 cm: đã hiện S thì phải ra quá 5 cm mới quay lại hiện số.
- Còi bíp từ 10 cm, nhanh dần tới 3 cm, rồi kêu liên tục.

**Lọc khoảng cách chắc hơn (chống số nhảy loạn ở cự ly gần)**
- Trung vị **7 mẫu** (trước là 5): loại được tối đa 3 lần đo sai trong mỗi cửa sổ.
- **Vùng trễ 6 mm**: kết quả chỉ đổi khi khoảng cách dịch từ 6 mm trở lên, nên số trên ma trận không nhảy qua lại ở ranh giới giữa hai số.
- Đổi lại, số trễ khoảng 0,2 s so với vị trí thật khi xe đang lùi.

**Chống nhảy số (bản hiện tại)**
Số trên ma trận đi qua 3 tầng:
1. `dist_filter`: mẫu lệch quá 3 cm so với kết quả hiện tại bị coi là gai nhiễu và bỏ; nếu 3 mẫu liền nhau cùng lệch và gần nhau (xe dịch nhanh thật) thì nhận ngay. Các mẫu còn lại lấy 5 mẫu gần nhất, bỏ mẫu cao nhất và thấp nhất, lấy trung bình 3 mẫu giữa. Mất tín hiệu 5 lần liên tiếp mới báo không có vật; bắt lại phải đủ 3 mẫu. Có hằng số bù `DIST_OFFSET_MM` để hiệu chỉnh theo thước.
2. `pk_stable_cm()` (khóa số): số đang hiện là n thì chỉ giảm khi xe vào sâu hơn n cm thêm 5 mm, và chỉ tăng khi ra xa hơn n+1 cm thêm 2 cm. Mất tín hiệu thoáng qua thì giữ số cũ.
3. Ma trận chỉ vẽ lại khi số thật sự đổi.

Giả lập với số đo nhiễu (lệch ±3 mm, thêm 15% lần đo sai hẳn): xe đứng yên ở đúng ranh giới 7,0 cm trong 60 giây, số **không đổi lần nào** sau giây đầu tiên (hiện thẳng số đo thô thì đổi khoảng 600 lần). Xe lùi đều từ 9,9 cm vào: số hiện lần lượt 9 8 7 6 5 4 3, không nhảy cóc, không quay ngược.

**Sửa đo khoảng cách bị trễ / sai khi xe đang chạy**
- Bộ lọc cũ (9 mẫu + chỉ cho dịch 1 cm mỗi lần đo) làm số hiện lớn hơn vị trí thật 2–3 cm khi xe lùi 10 cm/s, tới 5–8 cm khi lùi 20–30 cm/s.
- Bộ lọc mới: chặn gai nhiễu + trung vị 3 mẫu. Giả lập (nhiễu ±3 mm, 15% lần đo sai hẳn): lùi 10 cm/s lệch trung bình khoảng 0,8 cm, 20 cm/s khoảng 1,1 cm; xe đứng yên ở 6,5 / 7,0 cm trong 60 s số không đổi lần nào.
- **Xe dí sát (S) rồi số nhảy lên 6–7 cm:** do tiếng vọng kép, sóng dội qua lại giữa đuôi xe và mặt cảm biến nên Echo báo gấp khoảng 2 lần khoảng cách thật. Ở vùng dưới 5 cm, mẫu tăng vọt quá 1,5 cm trong một lần đo bị bỏ (chỉ nhận nếu kéo dài liên tục ~5 s); mất Echo ở vùng sát thì giữ số ~2 s thay vì 0,3 s.
- **Vẫn nhảy số khi nhiễu mạnh (±1 cm):** cửa sổ lọc 3 mẫu đổi thành 5 mẫu, khóa số tăng lại từ 1 cm lên 2 cm. Giả lập nhiễu ±8 mm + 15% lần đo sai: đứng yên ở chỗ xấu nhất số đổi 1 lần/60 s (trước 63 lần); đổi lại khi lùi 10 cm/s số trễ khoảng 2 cm.
- Thêm `DIST_OFFSET_MM` (mặc định 0) trong `dist_filter.h` để bù lệch cố định của module.

**Diễn biến:** chỉ khi **hồng ngoại** ở lối vào thấy xe thì ma trận mới hiện mũi tên "lùi vào". Siêu âm không tự bật bước này nữa, chỉ dùng để đo khoảng cách. Đứng yên ở vùng S đủ **4 giây** thì hiện P.

**Diễn biến theo mô tả mới của nhóm**
- Chưa có xe: ma trận **sáng đỏ toàn bộ** (độ sáng vẫn theo quang trở: trời tối sáng đậm hơn).
- Hồng ngoại thấy xe → mũi tên "đi vào" → dưới 10 cm đếm lùi 9..3 → 3 cm hiện S → đứng 4 s hiện P.
- Khi xe còn đỗ: **luôn hiện P**. Xe rời đi (qua cửa hồng ngoại) thì trở lại sáng đỏ toàn bộ.
- Bỏ thông số `detect`: siêu âm chỉ dùng để đo khoảng cách, không dùng để phát hiện xe nữa.
- Lưu ý nguồn: sáng đỏ cả 64 LED ở độ sáng 15 (ban đêm), MAX7219 có thể kéo khoảng 300 mA từ nguồn 5 V. Nên cấp nguồn 5 V tốt; nếu thấy số đo nhiễu hơn khi màn sáng hết, giảm `set night` (ví dụ 10).

**Còi kêu to hơn, không cần transistor**
- Còi chủ động: (+) nối **5V**, (−) nối chung **PB12 + PB13**. Hai chân để open-drain: kéo xuống GND thì còi nhận đủ 5 V, thả ra thì còi im. PB12/PB13 là chân chịu 5 V; hai chân song song để chia dòng (mỗi chân ~10–15 mA). Không làm cách này với PB1 hay chân PAx.
- Còi thụ động: đặt `BUZZER_PASSIVE 1`, nối (+) vào PB1, (−) vào GND (phát PWM 2,7 kHz bằng TIM3_CH4).

**Màn OLED theo đề cương**
```
 XE VAO              7.5cm     <- trạng thái đỗ xe + khoảng cách (đã lọc)
 ─────────────────────────
 CPU 4.8%                      <- % tải CPU (run-time stats), chữ to
 IH 4.6 thang 10%     125s     <- tải theo idle hook, thang đồ thị, thời gian chạy
 . . . . . . . . . . . . .
 ▂▃▃▅▃▂▂▃▅▆▃▂ ...              <- đồ thị tải CPU 60 giây gần nhất
```
- Trạng thái: TRONG / XE VAO / DUNG / DA DO / XE RA. Khoảng cách: "--" khi không đo được.
- Vẽ lại mỗi 0,5 s để khoảng cách cập nhật nhanh; đồ thị CPU vẫn 1 mẫu mỗi giây.

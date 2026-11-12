# Nhiem vu 2 - DisplayTask va LogTask

## 1. Muc tieu
Phu trach hien thi trang thai do xe, dieu khien canh bao
va ghi log trong he thong STM32F103 FreeRTOS.

## 2. Cac file phu trach
- app/src/display_task.c
- app/src/log_task.c
- app/src/parking_view.c
- app/inc/parking_view.h

## 3. DisplayTask
- Khoi tao MAX7219 va buzzer.
- Doc trang thai tu queue g_qParkView bang xQueuePeek.
- Su dung pv_render de tao du lieu hien thi.
- Su dung pv_beep de xac dinh che do canh bao.
- Chay dinh ky bang xTaskDelayUntil.

## 4. LogTask
- Nhan thong diep tu queue g_qLog.
- Su dung xQueueReceive de cho thong diep.
- Gui noi dung log ra UART thong qua console_write.

## 5. parking_view
- Chuyen trang thai bai do thanh du lieu hien thi.
- Xac dinh che do canh bao theo trang thai he thong.
- Tach logic hien thi khoi DisplayTask.

## 6. Tich hop
DisplayTask va LogTask duoc tao trong app/src/main.c.
Cac queue duoc quan ly boi app_interface.

## 7. Git workflow
Nhanh: feature/task2-documentation
Nhanh tich hop: develop

Tai lieu nay bo sung mo ta Nhiem vu 2.
Khong thay doi ma nguon STM32 FreeRTOS.

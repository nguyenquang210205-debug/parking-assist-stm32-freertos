/**
 * @file    parking_types.h
 * @brief   [APPLICATION] Kiểu dữ liệu dùng chung của hệ thống đỗ xe (không phụ thuộc FreeRTOS
 *          để dùng được trong unit test trên máy tính).
 * @owner   Cả nhóm thống nhất
 */
#ifndef PARKING_TYPES_H
#define PARKING_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/** Giá trị khoảng cách đặc biệt: không có vật trong tầm đo / đo lỗi. */
#define DIST_NONE  0xFFFFU

typedef enum {
    PK_EMPTY = 0,   /* chưa có xe: ma trận sáng đỏ toàn bộ */
    PK_APPROACH,    /* hồng ngoại thấy xe vào: mũi tên, dưới 10 cm đếm lùi 9..3 + còi bíp */
    PK_STOP,        /* chạm vạch dừng 3 cm: chữ S, còi kêu liên tục */
    PK_PARKED,      /* đứng yên ở vùng dừng đủ 4 s: chữ P, giữ suốt khi xe còn đỗ */
    PK_LEAVING,     /* xe đang ra: vẫn hiện P cho tới khi xe đi qua cửa (hồng ngoại) */
    PK_STATE_COUNT
} pk_state_t;

/** Tham số cài đặt (đơn vị mm và ms). Đổi được bằng lệnh UART. */
typedef struct {
    uint16_t stop_mm;          /* vạch dừng: khoảng cách <= giá trị này -> chữ S */
    uint16_t slow_mm;          /* vùng gần tường: hiện số cm + thanh vạch, còi bíp nhanh dần */
    uint16_t park_hold_ms;     /* đứng trong vùng dừng bao lâu thì coi là đã đỗ */
    uint16_t empty_timeout_ms; /* xe đang ra mà hồng ngoại không thấy gì bao lâu thì coi như đã đi */
    uint8_t  intensity_day;    /* độ sáng ma trận LED khi trời sáng (0..15) */
    uint8_t  intensity_night;  /* độ sáng ma trận LED khi trời tối  (0..15) */
} pk_settings_t;

#define PK_STOP_MM_MIN      30U      /* HC-SR04 có vùng mù ~2-3 cm */
#define PK_STOP_MM_MAX      1500U
#define PK_SLOW_MM_MAX      3000U
#define PK_HOLD_MS_MIN      1000U
#define PK_HOLD_MS_MAX      30000U
#define PK_INTENSITY_MAX    15U      /* MAX7219: 16 mức độ sáng 0..15 */
#define PK_CM_NONE          0xFFFFU  /* chưa có số cm để hiện */

/** Dữ liệu ControlTask công bố cho các task khác (mailbox qParkView). */
typedef struct {
    uint8_t       state;           /* pk_state_t */
    uint16_t      dist_mm;         /* khoảng cách đã lọc, DIST_NONE nếu không có */
    uint16_t      show_cm;         /* số cm HIỂN THỊ (đã khóa chống nhảy số), PK_CM_NONE nếu không có */
    bool          ir_detected;     /* cảm biến hồng ngoại ở cửa vào */
    bool          dark;            /* trời tối (cảm biến quang) */
    uint32_t      state_since_ms;  /* thời điểm vào trạng thái hiện tại (ms kể từ khởi động) */
    uint16_t      park_count;      /* số lượt đỗ */
    uint16_t      leave_count;     /* số lượt rời đi */
    pk_settings_t settings;
} park_view_t;

#endif /* PARKING_TYPES_H */

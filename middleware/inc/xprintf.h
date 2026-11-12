/**
 * @file    xprintf.h
 * @brief   [MIDDLEWARE] snprintf thu gọn, an toàn khi dùng trong nhiều task.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 *
 * Lý do không dùng printf/sprintf của thư viện C (slide FreeRTOS "Rủi ro khi dùng printf()"):
 * tốn nhiều Flash, cần stack lớn, có thể gọi malloc() và không thread-safe.
 * Hàm này không dùng biến toàn cục, không cấp phát động -> tái nhập (reentrant).
 *
 * Hỗ trợ: %d %i %u %x %X %c %s %%, độ rộng (ví dụ %5d), đệm số 0 (%03u), căn trái (%-8s).
 */
#ifndef XPRINTF_H
#define XPRINTF_H

#include <stdarg.h>
#include <stddef.h>

int xvsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int xsnprintf(char *buf, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

#endif /* XPRINTF_H */

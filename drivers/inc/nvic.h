/**
 * @file    nvic.h
 * @brief   [DRIVER] Cấu hình chung của bộ điều khiển ngắt NVIC (dùng SPL, misc.c).
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 */
#ifndef NVIC_H
#define NVIC_H

/**
 * Dành toàn bộ 4 bit ưu tiên cho preemption, không dùng sub-priority
 * (khuyến nghị của FreeRTOS cho Cortex-M, slide "Pre-emption priority và sub-priority").
 * Phải gọi trước khi cấu hình bất kỳ ngắt nào.
 */
void nvic_init_priority_group(void);

#endif /* NVIC_H */

/**
 * @file    delay.c
 * @brief   [DRIVER] Trễ micro giây bằng DWT->CYCCNT (thanh ghi lõi, khai báo trong CMSIS core_cm3.h).
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 */
#include "delay.h"
#include "clock.h"
#include "stm32f10x.h"

void delay_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* cho phép khối DWT */
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;             /* bộ đếm chu kỳ chạy */
}

void delay_us(uint32_t us)
{
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000UL);   /* clock thật (board_init đã cập nhật) */
    while ((DWT->CYCCNT - start) < cycles) { }        /* phép trừ không dấu tự xử lý tràn */
}

/**
 * @file    clock.c
 * @brief   [DRIVER] Xác nhận cấu hình clock 72 MHz (SPL: SystemCoreClockUpdate, RCC_GetClocksFreq).
 * @owner   Người 3 (Điều khiển + giám sát hệ thống)
 */
#include "clock.h"
#include "stm32f10x.h"

int clock_init_72mhz(void)
{
    RCC_ClocksTypeDef clk;

    SystemCoreClockUpdate();      /* cập nhật biến SystemCoreClock theo thanh ghi RCC thực tế */
    RCC_GetClocksFreq(&clk);

    if (clk.SYSCLK_Frequency != CLOCK_SYSCLK_HZ ||
        clk.PCLK1_Frequency  != CLOCK_PCLK1_HZ  ||
        clk.PCLK2_Frequency  != CLOCK_PCLK2_HZ) {
        return -1;
    }
    return 0;
}

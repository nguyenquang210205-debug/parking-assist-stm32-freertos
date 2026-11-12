/**
 * @file    buzzer.c
 * @brief   [MIDDLEWARE] Còi chip 2 chân.
 *          Còi chủ động: kéo PB12 + PB13 (open-drain, chịu 5 V) xuống GND để còi nhận 5 V.
 *          Còi thụ động: PWM bằng driver pwm (TIM3_CH4, PB1).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "buzzer.h"
#include "board.h"

#if BUZZER_PASSIVE
#include "pwm.h"

void buzzer_init(void)
{
    tim3_ch4_pwm_init(BUZZER_FREQ_HZ);    /* PB1 = TIM3_CH4, bắt đầu ở mức thấp (im) */
}

void buzzer_set(bool on)
{
    tim3_ch4_pwm_enable(on);
}

#else
#include "gpio.h"

void buzzer_set(bool on)
{
    /* Open-drain: ghi 0 = nối xuống GND (còi kêu); ghi 1 = thả nổi (còi im) */
    gpio_write(BOARD_BUZZER_PORT, BOARD_BUZZER_PIN_A, !on);
    gpio_write(BOARD_BUZZER_PORT, BOARD_BUZZER_PIN_B, !on);
}

void buzzer_init(void)
{
    buzzer_set(false);                    /* đặt mức "thả" vào ODR TRƯỚC khi bật output: không bíp lúc khởi động */
    gpio_config(BOARD_BUZZER_PORT, BOARD_BUZZER_PIN_A, GPIO_MODE_OUT_OD);
    gpio_config(BOARD_BUZZER_PORT, BOARD_BUZZER_PIN_B, GPIO_MODE_OUT_OD);
    buzzer_set(false);
}
#endif

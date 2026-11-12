/**
 * @file    hcsr04.c
 * @brief   [MIDDLEWARE] HC-SR04 dựa trên driver gpio (Trig), capture (Echo, TIM4) và delay (DWT).
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 */
#include "hcsr04.h"
#include "board.h"
#include "gpio.h"
#include "capture.h"
#include "delay.h"

void hcsr04_init(hcsr04_echo_cb_t cb)
{
    delay_init();
    gpio_config(BOARD_SONAR_TRIG_PORT, BOARD_SONAR_TRIG_PIN, GPIO_MODE_OUT_PP);
    gpio_reset(BOARD_SONAR_TRIG_PORT, BOARD_SONAR_TRIG_PIN);
    capture_tim4_init(BOARD_IRQ_PRIO_TIM4, cb);
}

void hcsr04_trigger(void)
{
    capture_tim4_arm();                   /* chỉ nhận xung Echo của lần kích này */
    gpio_set(BOARD_SONAR_TRIG_PORT, BOARD_SONAR_TRIG_PIN);
    delay_us(12U);                        /* datasheet yêu cầu >= 10 us */
    gpio_reset(BOARD_SONAR_TRIG_PORT, BOARD_SONAR_TRIG_PIN);
}

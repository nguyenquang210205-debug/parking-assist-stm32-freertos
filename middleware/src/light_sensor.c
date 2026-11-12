/**
 * @file    light_sensor.c
 * @brief   [MIDDLEWARE] Cảm biến quang (chân DO) dựa trên driver gpio + exti.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 */
#include "light_sensor.h"
#include "board.h"
#include "gpio.h"
#include "exti.h"
#include <stddef.h>

static light_sensor_edge_cb_t s_cb;

static void on_exti(uint8_t line)
{
    (void)line;
    if (s_cb != NULL) { s_cb(); }
}

void light_sensor_init(light_sensor_edge_cb_t cb)
{
    s_cb = cb;
    gpio_config_input_pull(BOARD_LIGHT_PORT, BOARD_LIGHT_PIN, GPIO_PULL_UP);
    exti_config(BOARD_LIGHT_PORT, BOARD_LIGHT_PIN, EXTI_EDGE_BOTH, BOARD_IRQ_PRIO_EXTI, on_exti);
}

bool light_sensor_is_dark(void)
{
    bool level = gpio_read(BOARD_LIGHT_PORT, BOARD_LIGHT_PIN);
    return level == (LIGHT_SENSOR_DARK_LEVEL != 0);
}

/**
 * @file    ir_sensor.c
 * @brief   [MIDDLEWARE] Cảm biến hồng ngoại (chân OUT) dựa trên driver gpio + exti.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 */
#include "ir_sensor.h"
#include "board.h"
#include "gpio.h"
#include "exti.h"
#include <stddef.h>

static ir_sensor_edge_cb_t s_cb;

static void on_exti(uint8_t line)
{
    (void)line;
    if (s_cb != NULL) { s_cb(); }
}

void ir_sensor_init(ir_sensor_edge_cb_t cb)
{
    s_cb = cb;
    gpio_config_input_pull(BOARD_IR_PORT, BOARD_IR_PIN, GPIO_PULL_UP);
    exti_config(BOARD_IR_PORT, BOARD_IR_PIN, EXTI_EDGE_BOTH, BOARD_IRQ_PRIO_EXTI, on_exti);
}

bool ir_sensor_detected(void)
{
    bool level = gpio_read(BOARD_IR_PORT, BOARD_IR_PIN);
    return level == (IR_SENSOR_ACTIVE_LEVEL != 0);
}

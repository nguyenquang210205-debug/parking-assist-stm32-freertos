/**
 * @file    exti.c
 * @brief   [DRIVER] EXTI dựa trên SPL: GPIO_EXTILineConfig (AFIO_EXTICR), EXTI_Init (IMR, RTSR, FTSR),
 *          NVIC_Init (ISER, IPR), EXTI_ClearITPendingBit (ghi 1 vào PR).
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 */
#include "exti.h"
#include <stddef.h>

static exti_callback_t s_callbacks[16];

static uint8_t port_source(GPIO_TypeDef *port)
{
    if (port == GPIOA) { return GPIO_PortSourceGPIOA; }
    if (port == GPIOB) { return GPIO_PortSourceGPIOB; }
    if (port == GPIOC) { return GPIO_PortSourceGPIOC; }
    return GPIO_PortSourceGPIOD;
}

static uint8_t line_to_irq(uint8_t line)
{
    if (line <= 4U) { return (uint8_t)(EXTI0_IRQn + line); }
    if (line <= 9U) { return (uint8_t)EXTI9_5_IRQn; }
    return (uint8_t)EXTI15_10_IRQn;
}

void exti_config(GPIO_TypeDef *port, uint8_t pin, exti_edge_t edge,
                 uint8_t irq_prio, exti_callback_t cb)
{
    s_callbacks[pin] = cb;

    /* Chọn port nối vào line `pin` (AFIO_EXTICRx). Cần đã bật clock AFIO. */
    GPIO_EXTILineConfig(port_source(port), pin);

    EXTI_InitTypeDef ex;
    ex.EXTI_Line    = (uint32_t)1U << pin;
    ex.EXTI_Mode    = EXTI_Mode_Interrupt;
    ex.EXTI_Trigger = (edge == EXTI_EDGE_BOTH)   ? EXTI_Trigger_Rising_Falling :
                      (edge == EXTI_EDGE_RISING) ? EXTI_Trigger_Rising : EXTI_Trigger_Falling;
    ex.EXTI_LineCmd = ENABLE;
    EXTI_ClearITPendingBit(ex.EXTI_Line);   /* xóa cờ chờ cũ */
    EXTI_Init(&ex);

    NVIC_InitTypeDef nv;
    nv.NVIC_IRQChannel                   = line_to_irq(pin);
    nv.NVIC_IRQChannelPreemptionPriority = irq_prio;
    nv.NVIC_IRQChannelSubPriority        = 0;
    nv.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nv);
}

/* Xử lý chung: duyệt các line trong khoảng [first, last] đang có cờ ngắt */
static void exti_dispatch(uint8_t first, uint8_t last)
{
    for (uint8_t line = first; line <= last; line++) {
        uint32_t mask = (uint32_t)1U << line;
        if (EXTI_GetITStatus(mask) != RESET) {
            EXTI_ClearITPendingBit(mask);   /* xóa cờ trước khi gọi callback */
            if (s_callbacks[line] != NULL) {
                s_callbacks[line](line);
            }
        }
    }
}

void EXTI0_IRQHandler(void)     { exti_dispatch(0, 0); }
void EXTI1_IRQHandler(void)     { exti_dispatch(1, 1); }
void EXTI2_IRQHandler(void)     { exti_dispatch(2, 2); }
void EXTI3_IRQHandler(void)     { exti_dispatch(3, 3); }
void EXTI4_IRQHandler(void)     { exti_dispatch(4, 4); }
void EXTI9_5_IRQHandler(void)   { exti_dispatch(5, 9); }
void EXTI15_10_IRQHandler(void) { exti_dispatch(10, 15); }

/**
 * @file    pwm.c
 * @brief   [DRIVER] TIM3_CH4 PWM trên PB1 (SPL: TIM_TimeBaseInit, TIM_OC4Init, TIM_SetCompare4).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "pwm.h"
#include "stm32f10x.h"

static uint16_t s_half;   /* CCR4 khi kêu = nửa chu kỳ */

/* Clock thật của TIM2..TIM4 (x2 nếu bộ chia APB1 khác 1), giống capture.c */
static uint32_t tim3_clock_hz(void)
{
    RCC_ClocksTypeDef clk;
    RCC_GetClocksFreq(&clk);
    return (clk.HCLK_Frequency == clk.PCLK1_Frequency) ? clk.PCLK1_Frequency
                                                       : 2U * clk.PCLK1_Frequency;
}

void tim3_ch4_pwm_init(uint32_t freq_hz)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB1 = TIM3_CH4 (không cần remap), alternate function push-pull */
    GPIO_InitTypeDef g;
    g.GPIO_Pin   = GPIO_Pin_1;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &g);

    uint32_t arr = 1000000UL / freq_hz - 1U;          /* đếm 1 MHz */
    s_half = (uint16_t)((arr + 1U) / 2U);

    TIM_TimeBaseInitTypeDef tb;
    tb.TIM_Prescaler         = (uint16_t)(tim3_clock_hz() / 1000000UL - 1U);
    tb.TIM_Period            = (uint16_t)arr;
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &tb);

    TIM_OCInitTypeDef oc;
    TIM_OCStructInit(&oc);
    oc.TIM_OCMode      = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse       = 0;                           /* bắt đầu: im */
    oc.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OC4Init(TIM3, &oc);
    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    TIM_Cmd(TIM3, ENABLE);
}

void tim3_ch4_pwm_enable(bool on)
{
    TIM_SetCompare4(TIM3, on ? s_half : 0U);
}

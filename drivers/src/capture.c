/**
 * @file    capture.c
 * @brief   [DRIVER] TIM4 PWM input (đo độ rộng xung) trên PB6, dùng SPL.
 * @owner   Người 1 (Đo khoảng cách + cảm biến)
 *
 * Chỉ nhận MỘT xung cho mỗi lần đo, theo 3 trạng thái:
 *   IDLE      : bỏ qua mọi cạnh (xung cũ, nhiễu, tiếng vọng muộn của lần đo trước).
 *   WAIT_RISE : capture_tim4_arm() đã gọi, chờ cạnh LÊN (CC1).
 *   WAIT_FALL : đã thấy cạnh lên (CNT vừa bị xóa về 0), chờ cạnh XUỐNG (CC2).
 * Ở cạnh xuống: nếu TIM4 đã tràn kể từ cạnh lên (xung dài hơn 65,5 ms) thì CCR2 là giá trị
 * đã quay vòng, KHÔNG phải độ rộng thật -> báo 0 (bên trên coi là đo lỗi).
 */
#include "capture.h"
#include "stm32f10x.h"
#include <stddef.h>

typedef enum { CAP_IDLE = 0, CAP_WAIT_RISE, CAP_WAIT_FALL } cap_state_t;

static capture_cb_t         s_cb;
static volatile cap_state_t s_state = CAP_IDLE;

/* Clock thật của TIM2..TIM4: = PCLK1 nếu bộ chia APB1 = 1, = 2 x PCLK1 nếu bộ chia APB1 > 1.
 * Tính từ thanh ghi RCC thay vì hằng số: nếu thạch anh HSE không chạy, chip rơi về HSI 8 MHz,
 * dùng hằng số 72 MHz sẽ làm mọi khoảng cách nhỏ đi 9 lần. */
static uint32_t tim4_clock_hz(void)
{
    RCC_ClocksTypeDef clk;
    RCC_GetClocksFreq(&clk);
    return (clk.HCLK_Frequency == clk.PCLK1_Frequency) ? clk.PCLK1_Frequency
                                                       : 2U * clk.PCLK1_Frequency;
}

void capture_tim4_init(uint8_t irq_prio, capture_cb_t cb)
{
    s_cb    = cb;
    s_state = CAP_IDLE;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB6 input floating: Echo của HC-SR04 là ngõ ra push-pull 5 V, PB6 là chân FT (chịu 5 V) */
    GPIO_InitTypeDef g;
    g.GPIO_Pin   = GPIO_Pin_6;
    g.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    g.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &g);

    TIM_TimeBaseInitTypeDef tb;
    tb.TIM_Prescaler         = (uint16_t)(tim4_clock_hz() / 1000000UL - 1U);   /* 1 MHz: 1 tick = 1 us */
    tb.TIM_Period            = 0xFFFF;                                         /* tối đa 65.5 ms */
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &tb);

    /* URS = 1: cờ UIF chỉ bật khi CNT TRÀN, không bật khi cạnh lên xóa CNT (slave reset).
     * Nhờ vậy UIF dùng được để phát hiện xung dài quá 65,5 ms. */
    TIM_UpdateRequestConfig(TIM4, TIM_UpdateSource_Regular);

    /* PWM input: TIM_PWMIConfig tự cấu hình IC1 (cạnh lên, TI1) và IC2 (cạnh xuống, TI1) */
    TIM_ICInitTypeDef ic;
    ic.TIM_Channel     = TIM_Channel_1;
    ic.TIM_ICPolarity  = TIM_ICPolarity_Rising;
    ic.TIM_ICSelection = TIM_ICSelection_DirectTI;
    ic.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    ic.TIM_ICFilter    = 0x3;          /* lọc số: tín hiệu phải ổn định 8 xung clock mới nhận */
    TIM_PWMIConfig(TIM4, &ic);

    TIM_SelectInputTrigger(TIM4, TIM_TS_TI1FP1);          /* TS = TI1FP1 */
    TIM_SelectSlaveMode(TIM4, TIM_SlaveMode_Reset);       /* SMS = 100: cạnh lên xóa CNT */
    /* MSM để mặc định (tắt): TIM4 không làm master cho timer nào, bật chỉ làm trễ việc xóa CNT */

    TIM_ClearITPendingBit(TIM4, TIM_IT_CC2 | TIM_IT_CC1 | TIM_IT_Update);
    /* CC1IE: cạnh lên (bắt đầu xung); CC2IE: cạnh xuống (kết thúc xung) */
    TIM_ITConfig(TIM4, TIM_IT_CC1 | TIM_IT_CC2, ENABLE);

    NVIC_InitTypeDef nv;
    nv.NVIC_IRQChannel                   = TIM4_IRQn;
    nv.NVIC_IRQChannelPreemptionPriority = irq_prio;
    nv.NVIC_IRQChannelSubPriority        = 0;
    nv.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nv);

    TIM_Cmd(TIM4, ENABLE);
}

void capture_tim4_arm(void)
{
    /* Gọi NGAY TRƯỚC khi phát xung Trig. Nếu lúc này Echo vẫn đang ở mức cao (module còn bận
     * với lần đo trước) thì cạnh lên của xung đó đã qua -> xung đó bị bỏ, lần đo này quá hạn. */
    TIM_ClearITPendingBit(TIM4, TIM_IT_CC1 | TIM_IT_CC2);
    s_state = CAP_WAIT_RISE;
}

void capture_tim4_enable(int enable)
{
    s_state = CAP_IDLE;
    TIM_ClearITPendingBit(TIM4, TIM_IT_CC1 | TIM_IT_CC2);
    TIM_ITConfig(TIM4, TIM_IT_CC1 | TIM_IT_CC2, enable ? ENABLE : DISABLE);
}

void TIM4_IRQHandler(void)
{
    /* Cạnh lên trước: xung rất ngắn có thể làm CC1 và CC2 cùng chờ trong một lần vào ngắt */
    if (TIM_GetITStatus(TIM4, TIM_IT_CC1) != RESET) {
        (void)TIM_GetCapture1(TIM4);              /* đọc CCR1 để xóa CC1IF */
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
        if (s_state == CAP_WAIT_RISE) {
            /* CNT vừa được phần cứng xóa về 0: xóa cờ tràn cũ (CNT chạy tự do giữa các lần đo
             * nên UIF gần như luôn đang bật). Độ trễ vào ngắt vài us << 65 ms nên không sót. */
            TIM_ClearFlag(TIM4, TIM_FLAG_Update);
            s_state = CAP_WAIT_FALL;
        }
    }

    if (TIM_GetITStatus(TIM4, TIM_IT_CC2) != RESET) {
        uint16_t width = TIM_GetCapture2(TIM4);   /* đọc CCR2 cũng tự xóa cờ CC2IF */
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC2);
        if (s_state == CAP_WAIT_FALL) {
            s_state = CAP_IDLE;
            if (TIM_GetFlagStatus(TIM4, TIM_FLAG_Update) == SET) {
                width = 0U;                       /* đã tràn: xung > 65,5 ms, giá trị không dùng được */
            }
            if (s_cb != NULL) {
                s_cb(width);
            }
        }
        /* Không ở WAIT_FALL: cạnh xuống của xung không thuộc lần đo này -> bỏ qua */
    }
}

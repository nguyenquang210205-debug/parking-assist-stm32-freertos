/**
 * @file    board.h
 * @brief   [MIDDLEWARE] Bản đồ chân của bo mạch (MỘT chỗ duy nhất định nghĩa chân)
 *          và LED báo hiệu PC13.
 * @owner   Người 3 (Điều khiển + giám sát hệ thống) - cả nhóm cùng thống nhất bảng chân
 *
 *  Chức năng                 Chân   Ngoại vi
 *  -----------------------   ----   ------------------------------------------
 *  Quang trở LM393 (DO)      PA0    GPIO input pull-up + EXTI0 (2 cạnh)
 *  Hồng ngoại vật cản (OUT)  PA1    GPIO input pull-up + EXTI1 (2 cạnh)
 *  HC-SR04 Trig                PB0    GPIO output push-pull (xung 10 us)
 *  HC-SR04 Echo                PB6    TIM4_CH1, PWM input (chân chịu 5 V - FT)
 *  Còi chủ động (−)          PB12+PB13  open-drain, kéo xuống GND để còi nhận 5 V (chân FT)
 *  Còi thụ động (+)          PB1    TIM3_CH4 PWM (chỉ khi BUZZER_PASSIVE = 1)
 *  MAX7219 CS                PA4    GPIO output
 *  MAX7219 CLK               PA5    SPI1 SCK
 *  MAX7219 DIN               PA7    SPI1 MOSI
 *  USB-TTL RX                PA9    USART1 TX (DMA1 kênh 4)
 *  USB-TTL TX                PA10   USART1 RX
 *  OLED SSD1306 SCL          PB10   I2C2 SCL (open-drain)
 *  OLED SSD1306 SDA          PB11   I2C2 SDA (open-drain)
 *  LED trên bo               PC13   GPIO output (sáng khi mức 0)
 *  (thang thời gian đo CPU)  -      TIM2 đếm tự do 20 kHz
 */
#ifndef BOARD_H
#define BOARD_H

#include "stm32f10x.h"

#define BOARD_LIGHT_PORT      GPIOA
#define BOARD_LIGHT_PIN       0U

#define BOARD_IR_PORT         GPIOA
#define BOARD_IR_PIN          1U

#define BOARD_SONAR_TRIG_PORT GPIOB
#define BOARD_SONAR_TRIG_PIN  0U
#define BOARD_SONAR_ECHO_PORT GPIOB   /* PB6 = TIM4_CH1, cấu hình trong driver capture */
#define BOARD_SONAR_ECHO_PIN  6U

#define BOARD_BUZZER_PORT     GPIOB
#define BOARD_BUZZER_PIN_A    12U    /* còi chủ động: 2 chân open-drain song song */
#define BOARD_BUZZER_PIN_B    13U

#define BOARD_MAX7219_CS_PORT GPIOA
#define BOARD_MAX7219_CS_PIN  4U

#define BOARD_LED_PORT        GPIOC
#define BOARD_LED_PIN         13U

/* Ưu tiên ngắt NVIC (4 bit, 0 = cao nhất). FreeRTOS: configMAX_SYSCALL_INTERRUPT_PRIORITY = 5.
 * ISR có gọi API ...FromISR phải có số ưu tiên >= 5. */
#define BOARD_IRQ_PRIO_TIM2   4U   /* chỉ đếm, KHÔNG gọi API -> được phép cao hơn ngưỡng */
#define BOARD_IRQ_PRIO_TIM4   5U   /* bắt xung Echo: cần chính xác nhất trong các ngắt có gọi API */
#define BOARD_IRQ_PRIO_UART   6U
#define BOARD_IRQ_PRIO_EXTI   7U

void board_init(void);        /* clock 72 MHz, clock GPIO, nhóm ưu tiên NVIC, LED */
void board_led_toggle(void);
void board_led_set(int on);

#endif /* BOARD_H */

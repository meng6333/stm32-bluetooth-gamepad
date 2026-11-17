#ifndef __WS2812B_H
#define __WS2812B_H

#include "stm32f10x.h"

#define WS2812_PIN        GPIO_Pin_8
#define WS2812_PORT       GPIOA
#define WS2812_TIM        TIM1
#define WS2812_TIM_CH     TIM_Channel_1
#define WS2812_DMA        DMA1_Channel1

#define LED_NUM           16

#define PWM_PERIOD        90
#define T0H               29
#define T1H               57

void WS2812B_Init(void);
void Set_LED_Color(uint8_t led_num, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_Update(void);
void Color_LED(uint8_t red, uint8_t green, uint8_t blue);   // 所有灯同一颜色

#endif

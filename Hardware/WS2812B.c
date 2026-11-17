#include "WS2812B.h"
#include "Delay.h"

// DMA缓冲区：存储每个LED的24位数据
uint16_t dma_buffer[LED_NUM * 24] = {0};

/*******************************************************************************
* 函数名: TIM1_PWM_Init
* 功能: 初始化TIM1_CH1产生PWM波形
* 说明: TIM1是高级定时器，需要使能主输出
*******************************************************************************/
void TIM1_PWM_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);  // TIM1在APB2总线上
    
    // 配置PA8为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = WS2812_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(WS2812_PORT, &GPIO_InitStructure);
    
    // 定时器基础配置
    TIM_TimeBaseStructure.TIM_Prescaler = 0;               // 不分频，72MHz
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = PWM_PERIOD - 1;     // 89，周期1.25μs
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(WS2812_TIM, &TIM_TimeBaseStructure);
    
    // PWM通道1配置
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(WS2812_TIM, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(WS2812_TIM, TIM_OCPreload_Disable);
    
    TIM_ARRPreloadConfig(WS2812_TIM, ENABLE);
    
    // ★★★ 关键：使能TIM1主输出（高级定时器特有）★★★
    TIM_CtrlPWMOutputs(WS2812_TIM, ENABLE);
    
    TIM_Cmd(WS2812_TIM, ENABLE);
}

/*******************************************************************************
* 函数名: WS2812_DMA_Init
* 功能: 初始化DMA1通道2，用于自动传输PWM比较值
*******************************************************************************/
void WS2812_DMA_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    
    // 使能DMA1时钟
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    // 复位DMA通道
    DMA_DeInit(WS2812_DMA);
    
    // DMA配置
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&WS2812_TIM->CCR1;  // TIM1比较寄存器1
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)dma_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = LED_NUM * 24;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(WS2812_DMA, &DMA_InitStructure);
    
    // 使能TIM1_CH1的DMA请求
    TIM_DMACmd(WS2812_TIM, TIM_DMA_CC1, ENABLE);
}

/*******************************************************************************
* 函数名: WS2812B_Init
* 功能: WS2812B初始化
*******************************************************************************/
void WS2812B_Init(void)
{
    TIM1_PWM_Init();
    WS2812_DMA_Init();
}

/*******************************************************************************
* 函数名: Set_LED_Color
* 功能: 设置单个LED的颜色
* 参数: led_num-LED编号，red/green/blue-RGB颜色值
*******************************************************************************/
void Set_LED_Color(uint8_t led_num, uint8_t red, uint8_t green, uint8_t blue)
{
    if(led_num >= LED_NUM) return;
    
    // 转换为GRB格式
    uint32_t color = (green << 16) | (red << 8) | blue;
    
    // 将24位数据转换为PWM占空比
    for(int i = 0; i < 24; i++)
    {
        if(color & (1 << (23 - i)))
        {
            dma_buffer[led_num * 24 + i] = T1H;
        }
        else
        {
            dma_buffer[led_num * 24 + i] = T0H;
        }
    }
}

/*******************************************************************************
* 函数名: WS2812_Update
* 功能: 更新所有LED显示
*******************************************************************************/
void WS2812_Update(void)
{
    // 启动DMA传输
    DMA_Cmd(WS2812_DMA, DISABLE);
    DMA_SetCurrDataCounter(WS2812_DMA, LED_NUM * 24);
    DMA_ClearFlag(DMA1_FLAG_TC2);  // DMA1通道2传输完成标志
    DMA_Cmd(WS2812_DMA, ENABLE);
    
    // 等待DMA传输完成（带超时保护）
    uint32_t timeout = 50000;
    while(DMA_GetFlagStatus(DMA1_FLAG_TC2) == RESET)
    {
        timeout--;
        if(timeout == 0) break;
    }
    DMA_ClearFlag(DMA1_FLAG_TC2);
    
    // ★★★ 复位时序 ★★★
    // 停止定时器
    TIM_Cmd(WS2812_TIM, DISABLE);
    TIM_CtrlPWMOutputs(WS2812_TIM, DISABLE);
    
    // 将PA8切换为普通推挽输出并拉低
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = WS2812_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(WS2812_PORT, &GPIO_InitStructure);
    GPIO_ResetBits(WS2812_PORT, WS2812_PIN);
    
    // 保持低电平200μs（WS2812B复位信号要求>50μs）
    Delay_us(200);
    
    // 恢复为复用推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(WS2812_PORT, &GPIO_InitStructure);
    
    // 重新启动定时器
    TIM_SetCompare1(WS2812_TIM, 0);
    TIM_CtrlPWMOutputs(WS2812_TIM, ENABLE);
    TIM_Cmd(WS2812_TIM, ENABLE);
}

/*******************************************************************************
* 效果函数
*******************************************************************************/

// 固定颜色
void Color_LED(uint8_t red, uint8_t green, uint8_t blue)
{
    for(uint8_t i = 0; i < LED_NUM; i++)
    {
        Set_LED_Color(i, red, green, blue);
    }
    WS2812_Update();
}





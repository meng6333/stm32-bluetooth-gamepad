#include "Key.h"

// 按键结构体
typedef struct {
    GPIO_TypeDef* GPIOx;
    uint16_t GPIO_Pin;
    uint8_t keyNum;
    uint8_t isPressed;
    uint8_t debounceCount;
} Key_t;

// 按键数组 - 15个按键
static Key_t keys[] = {
    // 方向键 (4个)
    {KEY_UP_PORT,    KEY_UP_PIN,    KEY_UP,    0, 0},
    {KEY_DOWN_PORT,  KEY_DOWN_PIN,  KEY_DOWN,  0, 0},
    {KEY_LEFT_PORT,  KEY_LEFT_PIN,  KEY_LEFT,  0, 0},
    {KEY_RIGHT_PORT, KEY_RIGHT_PIN, KEY_RIGHT, 0, 0},
    // 动作键 (4个)
    {KEY_X_PORT,     KEY_X_PIN,     KEY_X,     0, 0},
    {KEY_Y_PORT,     KEY_Y_PIN,     KEY_Y,     0, 0},
    {KEY_B_PORT,     KEY_B_PIN,     KEY_B,     0, 0},
    {KEY_A_PORT,     KEY_A_PIN,     KEY_A,     0, 0},
    // 菜单键 (3个)
    {KEY_HOME_PORT,  KEY_HOME_PIN,  KEY_HOME,  0, 0},
    {KEY_MENU_PORT,  KEY_MENU_PIN,  KEY_MENU,  0, 0},
    {KEY_VIEW_PORT,  KEY_VIEW_PIN,  KEY_VIEW,  0, 0},
    // 摇杆按键 (2个)
    {KEY_L3_PORT,    KEY_L3_PIN,    KEY_L3,    0, 0},
    {KEY_R3_PORT,    KEY_R3_PIN,    KEY_R3,    0, 0},
    // ★ 肩键 (2个)
    {KEY_LB_PORT,    KEY_LB_PIN,    KEY_LB,    0, 0},
    {KEY_RB_PORT,    KEY_RB_PIN,    KEY_RB,    0, 0}
};

// 自动计算按键总数
#define KEY_TOTAL (sizeof(keys) / sizeof(keys[0]))

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    // 释放PB3/PB4/PB5（JTAG调试口）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    // 配置为输入上拉模式
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    // 初始化GPIOA：PA6(L3), PA7(R3)
    GPIO_InitStructure.GPIO_Pin = KEY_L3_PIN | KEY_R3_PIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 初始化GPIOB：所有PB按键
    GPIO_InitStructure.GPIO_Pin = KEY_LEFT_PIN | KEY_DOWN_PIN | KEY_HOME_PIN | 
                                  KEY_MENU_PIN | KEY_VIEW_PIN | KEY_Y_PIN | 
                                  KEY_B_PIN | KEY_A_PIN | KEY_X_PIN | 
                                  KEY_RIGHT_PIN | KEY_UP_PIN |
                                  KEY_RB_PIN | KEY_LB_PIN; 
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/*******************************************************************************
* Function Name  : Key_UpdateStatus
*******************************************************************************/
void Key_UpdateStatus(void)
{
    for (uint8_t i = 0; i < KEY_TOTAL; i++)  // 15个按键
    {
        uint8_t pinState = GPIO_ReadInputDataBit(keys[i].GPIOx, keys[i].GPIO_Pin);
        
        if (pinState == 0)
        {
            if (keys[i].debounceCount < 3)
                keys[i].debounceCount++;
            else
                keys[i].isPressed = 1;
        }
        else
        {
            if (keys[i].debounceCount > 0)
                keys[i].debounceCount--;
            else
                keys[i].isPressed = 0;
        }
    }
}

/*******************************************************************************
* Function Name  : Key_GetSingleStatus
*******************************************************************************/
uint8_t Key_GetSingleStatus(uint8_t keyNum)
{
    if (keyNum < 1 || keyNum > KEY_TOTAL)
        return 0;
    return keys[keyNum - 1].isPressed;
}


uint16_t Key_GetAllStatus(void)
{
    uint16_t status = 0;
    
    for (uint8_t i = 0; i < KEY_TOTAL; i++)
    {
        if (keys[i].isPressed)
            status |= (1 << (keys[i].keyNum - 1));
    }
    
    return status;
	}

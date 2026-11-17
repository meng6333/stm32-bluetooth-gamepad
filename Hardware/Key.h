#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

// 按键总数：13个原有 + 2个肩键 = 15
#define KEY_COUNT 15

// 按键编号定义
#define KEY_UP     1   // PB11 - 方向上
#define KEY_DOWN   2   // PB1  - 方向下
#define KEY_LEFT   3   // PB0  - 方向左
#define KEY_RIGHT  4   // PB10 - 方向右
#define KEY_X      5   // PB9
#define KEY_Y      6   // PB6
#define KEY_B      7   // PB7
#define KEY_A      8   // PB8
#define KEY_HOME   9   // PB3
#define KEY_MENU   10  // PB4
#define KEY_VIEW   11  // PB5
#define KEY_L3     12  // PA6 - 左摇杆按下
#define KEY_R3     13  // PA7 - 右摇杆按下
#define KEY_LB     14  // PB15 - 左肩键 
#define KEY_RB     15  // PB14 - 右肩键

// 方向键引脚
#define KEY_UP_PIN     GPIO_Pin_11
#define KEY_UP_PORT    GPIOB
#define KEY_DOWN_PIN   GPIO_Pin_1
#define KEY_DOWN_PORT  GPIOB
#define KEY_LEFT_PIN   GPIO_Pin_0
#define KEY_LEFT_PORT  GPIOB
#define KEY_RIGHT_PIN  GPIO_Pin_10
#define KEY_RIGHT_PORT GPIOB

// 动作键引脚
#define KEY_X_PIN      GPIO_Pin_9
#define KEY_X_PORT     GPIOB
#define KEY_Y_PIN      GPIO_Pin_6
#define KEY_Y_PORT     GPIOB
#define KEY_B_PIN      GPIO_Pin_7
#define KEY_B_PORT     GPIOB
#define KEY_A_PIN      GPIO_Pin_8
#define KEY_A_PORT     GPIOB

// 菜单键引脚
#define KEY_HOME_PIN   GPIO_Pin_3
#define KEY_HOME_PORT  GPIOB
#define KEY_MENU_PIN   GPIO_Pin_4
#define KEY_MENU_PORT  GPIOB
#define KEY_VIEW_PIN   GPIO_Pin_5
#define KEY_VIEW_PORT  GPIOB

// 摇杆按键引脚
#define KEY_L3_PIN     GPIO_Pin_6
#define KEY_L3_PORT    GPIOA
#define KEY_R3_PIN     GPIO_Pin_7
#define KEY_R3_PORT    GPIOA

// ★ 新增肩键引脚
#define KEY_LB_PIN     GPIO_Pin_15
#define KEY_LB_PORT    GPIOB
#define KEY_RB_PIN     GPIO_Pin_14
#define KEY_RB_PORT    GPIOB

// 函数声明
void Key_Init(void);
uint16_t Key_GetAllStatus(void);
uint8_t Key_GetSingleStatus(uint8_t keyNum);
void Key_UpdateStatus(void);

#endif /* __KEY_H */

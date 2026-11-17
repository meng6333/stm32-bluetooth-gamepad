#include "stm32f10x.h"
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_istr.h"
#include "Delay.h"
#include "OLED.h"
#include "Key.h"
#include "AD.h"
#include "Serial.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>   

/* 外部变量声明 */
extern uint8_t Report_Buf[8];
extern __IO uint8_t PrevXferComplete;
extern volatile uint32_t bDeviceState;
extern uint8_t OLED_DisplayBuf[8][128];
extern void OLED_SetCursor(uint8_t Page, uint8_t X);
extern void OLED_WriteData(uint8_t *Data, uint8_t Count);

/* 函数声明 */
void GPIO_Configuration(void);
void USB_SendReport(void);
void LED_Update(void);
void Joystick_Init(void);
uint16_t Read_All_Buttons(void);
void Calibrate_Joysticks(void);
uint8_t Get_Hat_Switch_Value(void);
void OLED_ShowJoystick(void);
void OLED_SimpleProgressBar(uint16_t lt_val, uint16_t rt_val);
void OLED_RefreshDisplay(void);
void OLED_ParticleBootAnimation(void);
void ShowCalibrationStatus(uint8_t done);
void WaitForKeyPress(void);
void DrawMenu(void);
void ShowVersionInfo(void);
void DrawBtMenu(void);
void SendBtCommand(const char *cmd);

/* 全局变量 */
static uint8_t send_enabled = 0;

/* 摇杆校准值 */
static uint16_t joy1_center_x = 2048;
static uint16_t joy1_center_y = 2048;
static uint16_t joy2_center_x = 2048;
static uint16_t joy2_center_y = 2048;

/* 缓存ADC值（滤波后） */
static uint16_t cached_joy1_x = 2048;
static uint16_t cached_joy1_y = 2048;
static uint16_t cached_joy2_x = 2048;
static uint16_t cached_joy2_y = 2048;
static uint16_t cached_lt_val = 2048;
static uint16_t cached_rt_val = 2048;

/* 扳机校准值 */
static uint16_t lt_center = 2048;
static uint16_t rt_center = 2048;

/* ★ 可调参数 */
static uint16_t g_deadzone = 150;          // 死区 0~300
static uint8_t  g_filter_alpha = 80;       // 滤波系数 0~256

/* ★ 游戏手柄模式状态机 */
#define STATE_TEST      0   // 测试区（摇杆/扳机）
#define STATE_GAMEPAD   1   // 游戏手柄模式页面
#define STATE_MENU      2   // 菜单系统
static uint8_t current_state = STATE_TEST;

/* ★ 蓝牙模式状态机 */
#define BT_STATE_MAIN   0
#define BT_STATE_MENU   1
static uint8_t bt_state = BT_STATE_MAIN;

/* ★ 模式定义 */
#define MODE_GAMEPAD    0
#define MODE_BLUETOOTH  1
static uint8_t current_mode = MODE_GAMEPAD;   // 默认游戏手柄模式

/* ★ 菜单变量（游戏手柄菜单） */
static uint8_t menu_selection = 0;          // 0~3 对应四个选项
static uint8_t last_home = 0;               // HOME键上升沿检测
static uint8_t last_menu = 0;               // MENU键上升沿检测
static uint8_t last_up = 0;                 // 上键
static uint8_t last_down = 0;               // 下键
static uint8_t last_left = 0;               // 左键
static uint8_t last_right = 0;              // 右键
static uint8_t last_a = 0;                  // A键
static uint8_t last_view = 0;               // VIEW键上升沿检测（PB5）

/* ★ 蓝牙按键上升沿检测变量 */
static uint8_t last_lb = 0;
static uint8_t last_rb = 0;
static uint8_t last_x = 0;
static uint8_t last_y = 0;
static uint8_t last_b = 0;

/* ★ 蓝牙菜单配置 */
static uint8_t bt_menu_selection = 0;       // 0:包头包尾, 1:十六进制, 2:换行符
static uint8_t bt_header_type = 0;          // 0:无, 1:@-#$
static uint8_t bt_hex_enable = 0;           // 0:否, 1:是
static uint8_t bt_newline_enable = 1;       // 0:否, 1:是 (默认加换行)

/* 随机数生成器 */
static uint16_t rng_seed = 12345;
static uint8_t my_rand(uint8_t min, uint8_t max) {
    rng_seed = rng_seed * 25173 + 13849;
    return (uint8_t)((rng_seed >> 8) % (max - min + 1) + min);
}

/* 粒子结构体 */
typedef struct {
    uint8_t x, y;
    uint8_t targetX, targetY;
    uint8_t finished;
} Particle_t;

/* ======================== 一阶低通滤波 ======================== */
static uint16_t filter_lp(uint16_t prev, uint16_t raw)
{
    uint16_t alpha = g_filter_alpha;
    return (uint16_t)(((uint32_t)prev * (256 - alpha) + (uint32_t)raw * alpha) >> 8);
}

/* 滤波后的ADC更新函数 */
void Update_ADC_Cache(void)
{
    static uint16_t f_joy1_x = 2048, f_joy1_y = 2048;
    static uint16_t f_joy2_x = 2048, f_joy2_y = 2048;
    static uint16_t f_lt = 2048, f_rt = 2048;

    f_joy1_x = filter_lp(f_joy1_x, AD_GetValue(ADC_Channel_0));
    f_joy1_y = filter_lp(f_joy1_y, AD_GetValue(ADC_Channel_1));
    f_joy2_x = filter_lp(f_joy2_x, AD_GetValue(ADC_Channel_2));
    f_joy2_y = filter_lp(f_joy2_y, AD_GetValue(ADC_Channel_3));
    f_lt     = filter_lp(f_lt, AD_GetValue(ADC_Channel_4));
    f_rt     = filter_lp(f_rt, AD_GetValue(ADC_Channel_5));

    cached_joy1_x = f_joy1_x;
    cached_joy1_y = f_joy1_y;
    cached_joy2_x = f_joy2_x;
    cached_joy2_y = f_joy2_y;
    cached_lt_val = f_lt;
    cached_rt_val = f_rt;
}

/* ======================== 显示函数 ======================== */

void ShowCalibrationStatus(uint8_t done)
{
    OLED_Clear();
    if (!done) {
        OLED_ShowChinese(24, 24, "系统校准中");
    } else {
        OLED_ShowChinese(32, 24, "校准完成");
    }
    OLED_Update();
}

void WaitForKeyPress(void)
{
    uint16_t keyStatus;
    do {
        Key_UpdateStatus();
        keyStatus = Key_GetAllStatus();
        Delay_ms(20);
    } while (keyStatus == 0);
    do {
        Key_UpdateStatus();
        keyStatus = Key_GetAllStatus();
        Delay_ms(20);
    } while (keyStatus != 0);
}

/*******************************************************************************
* Function Name  : OLED_ParticleBootAnimation
*******************************************************************************/
void OLED_ParticleBootAnimation(void)
{
    #define PARTICLE_COUNT 200
    #define ANIMATION_STEPS 80
    #define FADE_FRAMES 15

    static Particle_t particles[PARTICLE_COUNT];
    static uint8_t targetPoints[700][2];
    uint16_t targetCount = 0;
    uint8_t i, step;

    const char *text = "STM32";
    uint8_t startX = 45;
    uint8_t startY = 24;

	
    for (uint8_t ch = 0; text[ch] != '\0'; ch++)
    {
        uint8_t charIndex = text[ch] - ' ';
        for (uint8_t col = 0; col < 8; col++)
        {
            uint8_t dataUpper = OLED_F8x16[charIndex][col];
            uint8_t dataLower = OLED_F8x16[charIndex][col + 8];
            for (uint8_t row = 0; row < 8; row++)
            {
                if (dataUpper & (0x01 << row))
                {
                    if (targetCount < 700) {
                        targetPoints[targetCount][0] = startX + ch * 8 + col;
                        targetPoints[targetCount][1] = startY + row;
                        targetCount++;
                    }
                }
                if (dataLower & (0x01 << row))
                {
                    if (targetCount < 700) {
                        targetPoints[targetCount][0] = startX + ch * 8 + col;
                        targetPoints[targetCount][1] = startY + 8 + row;
                        targetCount++;
                    }
                }
            }
        }
    }

    if (targetCount < PARTICLE_COUNT) {
        while (targetCount < PARTICLE_COUNT) {
            uint16_t idx = targetCount % (targetCount ? targetCount : 1);
            targetPoints[targetCount][0] = targetPoints[idx][0];
            targetPoints[targetCount][1] = targetPoints[idx][1];
            targetCount++;
        }
    }

    for (i = 0; i < targetCount; i++) {
        uint16_t j = my_rand(0, targetCount - 1);
        uint8_t tx = targetPoints[i][0];
        uint8_t ty = targetPoints[i][1];
        targetPoints[i][0] = targetPoints[j][0];
        targetPoints[i][1] = targetPoints[j][1];
        targetPoints[j][0] = tx;
        targetPoints[j][1] = ty;
    }

    for (i = 0; i < PARTICLE_COUNT; i++) {
        uint16_t tgtIdx = i % targetCount;
        particles[i].targetX = targetPoints[tgtIdx][0];
        particles[i].targetY = targetPoints[tgtIdx][1];

        uint8_t side = my_rand(0, 3);
        switch (side) {
        case 0:
            particles[i].x = my_rand(0, 127);
            particles[i].y = my_rand(0, 22);
            break;
        case 1:
            particles[i].x = my_rand(0, 127);
            particles[i].y = my_rand(28, 36);
            break;
        case 2:
            particles[i].x = my_rand(0, 12);
            particles[i].y = my_rand(0, 46);
            break;
        case 3:
            particles[i].x = my_rand(115, 127);
            particles[i].y = my_rand(0, 46);
            break;
        }
        if (particles[i].y > 46) particles[i].y = 46;
        particles[i].finished = 0;
    }

    OLED_Clear();
    for (step = 0; step < ANIMATION_STEPS; step++) {
        for (i = 0; i < PARTICLE_COUNT; i++) {
            if (particles[i].finished) continue;
            int16_t dx = particles[i].targetX - particles[i].x;
            int16_t dy = particles[i].targetY - particles[i].y;
            int16_t dist = abs(dx) + abs(dy);
            if (dist <= 2) {
                particles[i].x = particles[i].targetX;
                particles[i].y = particles[i].targetY;
                particles[i].finished = 1;
            } else {
                uint8_t speed = (dist > 20) ? 3 : (dist > 10) ? 2 : 1;
                if (dx > 0) particles[i].x += speed;
                else if (dx < 0) particles[i].x -= speed;
                if (dy > 0) particles[i].y += speed;
                else if (dy < 0) particles[i].y -= speed;
                if (particles[i].x > 127) particles[i].x = 127;
                if (particles[i].y > 46) particles[i].y = 46;
            }
        }

        OLED_Clear();
        for (i = 0; i < PARTICLE_COUNT; i++) {
            if (particles[i].y <= 46) {
                OLED_DrawPoint(particles[i].x, particles[i].y);
            }
        }
  if (step >= ANIMATION_STEPS * 3 / 4) {
            OLED_ShowString(45, 24, "STM32", OLED_8X16);
            OLED_ShowChinese(20, 24, "一");
            OLED_ShowChinese(90, 24, "二");
        }
				
        for (uint8_t col = 0; col < 128; col++) {
            OLED_DisplayBuf[6][col] = 0x00;
            OLED_DisplayBuf[7][col] = 0x00;
        }

        for (uint8_t page = 0; page < 6; page++) {
            OLED_SetCursor(page, 0);
            OLED_WriteData(OLED_DisplayBuf[page], 128);
        }
        Delay_ms(30);
    }

    OLED_Clear();
		
    OLED_ShowString(45, 24, "STM32", OLED_8X16);
		
	
		 
    OLED_Update();
}

void OLED_SimpleProgressBar(uint16_t lt_val, uint16_t rt_val)
{
    int16_t lt_offset = (int16_t)lt_val - 2048;
    int16_t rt_offset = (int16_t)rt_val - 2048;
    int16_t combined = rt_offset - lt_offset;
    int16_t mapped = (combined + 2048) * 128 / 4096;
    if(mapped < 0) mapped = 0;
    if(mapped > 128) mapped = 128;
    uint8_t fill_width = (uint8_t)mapped;

    for(uint8_t page = 0; page < 2; page++)
        for(uint8_t col = 0; col < 128; col++)
            OLED_DisplayBuf[page][col] = 0x00;

    for(uint8_t x = 0; x < 128; x++) {
        OLED_DisplayBuf[0][x] |= 0x01;
        OLED_DisplayBuf[1][x] |= 0x80;
    }
    for(uint8_t y = 0; y < 16; y++) {
        uint8_t page = y / 8;
        uint8_t bit = y % 8;
        OLED_DisplayBuf[page][0] |= (1 << bit);
        OLED_DisplayBuf[page][127] |= (1 << bit);
    }

    if(fill_width > 0) {
        for(uint8_t x = 0; x < fill_width; x++) {
            OLED_DisplayBuf[0][x] = 0xFF;
            OLED_DisplayBuf[1][x] = 0xFF;
        }
    }
}

void OLED_ShowJoystick(void)
{
    uint8_t l_area_cx = 31;
    uint8_t l_area_cy = 44;
    uint8_t l_area_half_w = 28;
    uint8_t l_area_half_h = 20;
    uint8_t l_circle_r = 3;

    uint8_t r_area_cx = 97;
    uint8_t r_area_cy = 44;
    uint8_t r_area_half_w = 26;
    uint8_t r_area_half_h = 20;
   

    int16_t l_diff_x = (int16_t)cached_joy1_x - (int16_t)joy1_center_x;
    int16_t l_diff_y = (int16_t)cached_joy1_y - (int16_t)joy1_center_y;
    if(l_diff_x > -(int16_t)g_deadzone && l_diff_x < (int16_t)g_deadzone) l_diff_x = 0;
    if(l_diff_y > -(int16_t)g_deadzone && l_diff_y < (int16_t)g_deadzone) l_diff_y = 0;
    int8_t l_offset_x = (int8_t)((int32_t)l_diff_y * l_area_half_w / 3100);
    int8_t l_offset_y = (int8_t)((int32_t)l_diff_x * l_area_half_h / 3200);
    uint8_t l_circle_x = l_area_cx + l_offset_x;
    uint8_t l_circle_y = l_area_cy + l_offset_y;
    uint8_t l_min_x = l_area_cx - l_area_half_w + l_circle_r + 1;
    uint8_t l_max_x = l_area_cx + l_area_half_w - l_circle_r - 1;
    uint8_t l_min_y = l_area_cy - l_area_half_h + l_circle_r + 1;
    uint8_t l_max_y = l_area_cy + l_area_half_h - l_circle_r - 1;
    if(l_circle_x < l_min_x) l_circle_x = l_min_x;
    if(l_circle_x > l_max_x) l_circle_x = l_max_x;
    if(l_circle_y < l_min_y) l_circle_y = l_min_y;
    if(l_circle_y > l_max_y) l_circle_y = l_max_y;

    int16_t r_diff_x = (int16_t)cached_joy2_x - (int16_t)joy2_center_x;
    int16_t r_diff_y = (int16_t)cached_joy2_y - (int16_t)joy2_center_y;
    if(r_diff_x > -(int16_t)g_deadzone && r_diff_x < (int16_t)g_deadzone) r_diff_x = 0;
    if(r_diff_y > -(int16_t)g_deadzone && r_diff_y < (int16_t)g_deadzone) r_diff_y = 0;
    int8_t r_offset_x = (int8_t)((int32_t)r_diff_y * r_area_half_w / 3300);
    int8_t r_offset_y = (int8_t)((int32_t)r_diff_x * r_area_half_h / 3400);
    uint8_t r_circle_x = r_area_cx + r_offset_x;
    uint8_t r_circle_y = r_area_cy + r_offset_y;
    uint8_t r_min_x = r_area_cx - r_area_half_w + l_circle_r + 1;
    uint8_t r_max_x = r_area_cx + r_area_half_w - l_circle_r - 1;
    uint8_t r_min_y = r_area_cy - r_area_half_h + l_circle_r + 1;
    uint8_t r_max_y = r_area_cy + r_area_half_h - l_circle_r - 1;
    if(r_circle_x < r_min_x) r_circle_x = r_min_x;
    if(r_circle_x > r_max_x) r_circle_x = r_max_x;
    if(r_circle_y < r_min_y) r_circle_y = r_min_y;
    if(r_circle_y > r_max_y) r_circle_y = r_max_y;

    OLED_DrawRectangle(l_area_cx - l_area_half_w, 
                       l_area_cy - l_area_half_h, 
                       l_area_half_w * 2, 
                       l_area_half_h * 2, 
                       OLED_UNFILLED);
    OLED_DrawPoint(l_area_cx, l_area_cy - 1);
    OLED_DrawPoint(l_area_cx, l_area_cy + 1);
    OLED_DrawPoint(l_area_cx - 1, l_area_cy);
    OLED_DrawPoint(l_area_cx + 1, l_area_cy);
    OLED_DrawCircle(l_circle_x, l_circle_y, l_circle_r, OLED_UNFILLED);

    OLED_DrawRectangle(r_area_cx - r_area_half_w, 
                       r_area_cy - r_area_half_h, 
                       r_area_half_w * 2, 
                       r_area_half_h * 2, 
                       OLED_UNFILLED);
    OLED_DrawPoint(r_area_cx, r_area_cy - 1);
    OLED_DrawPoint(r_area_cx, r_area_cy + 1);
    OLED_DrawPoint(r_area_cx - 1, r_area_cy);
    OLED_DrawPoint(r_area_cx + 1, r_area_cy);
    OLED_DrawCircle(r_circle_x, r_circle_y, l_circle_r, OLED_UNFILLED);
}

void OLED_RefreshDisplay(void)
{
    OLED_Clear();
    OLED_SimpleProgressBar(cached_lt_val, cached_rt_val);
    OLED_ShowJoystick();
    OLED_Update();
}

/* ★ 游戏手柄菜单绘制函数 */
void DrawMenu(void)
{
    OLED_Clear();
    char *items[] = {"摇杆死区：", "滤波强度：", "测试区域：", "版本信息："};
    for (uint8_t i = 0; i < 4; i++) {
        if (i == menu_selection) {
            OLED_ShowString(0, i*16, "->", OLED_8X16);
        } else {
            OLED_ShowString(0, i*16, "  ", OLED_8X16);
        }
        OLED_ShowChinese(20, i*16, items[i]);
        if (i == 0) {
            char buf[8];
            sprintf(buf, "%d", g_deadzone);
            OLED_ShowString(98, i*16, buf, OLED_8X16);
        } else if (i == 1) {
            char buf[8];
            sprintf(buf, "%d", g_filter_alpha);
            OLED_ShowString(98, i*16, buf, OLED_8X16);
        }
    }
    OLED_ShowString(90,48,"V1.0", OLED_8X16);
    OLED_Update();
}

/* ★ 显示版本信息 */
void ShowVersionInfo(void)
{
    OLED_Clear();
    OLED_ShowString(16, 12, "Version V1.0", OLED_8X16);
    OLED_ShowString(36, 36, "STM32", OLED_8X16);
    OLED_ShowChinese(0,36,"我的");
    OLED_ShowChinese(80,36,"又烧啦");
    OLED_Update();
}

/* ★ 蓝牙菜单绘制函数 */
void DrawBtMenu(void)
{
    OLED_Clear();
    
    // 第一行：包头包尾
    if (bt_menu_selection == 0) OLED_ShowString(0, 0, "->", OLED_8X16);
    else OLED_ShowString(0, 0, "  ", OLED_8X16);
    OLED_ShowChinese(24, 0, "包头尾：");
    if (bt_header_type == 0) OLED_ShowString(95, 0, "X", OLED_8X16);
    else OLED_ShowString(85, 0, "@-#$", OLED_8X16);

    // 第二行：十六进制
    if (bt_menu_selection == 1) OLED_ShowString(0, 23, "->", OLED_8X16);
    else OLED_ShowString(0, 23, "  ", OLED_8X16);
    OLED_ShowChinese(24, 23, "十六制：");
    if (bt_hex_enable == 0) OLED_ShowString(95, 23, "NO", OLED_8X16);
    else OLED_ShowString(95, 23, "YES", OLED_8X16);

    // 第三行：换行符
    if (bt_menu_selection == 2) OLED_ShowString(0, 47, "->", OLED_8X16);
    else OLED_ShowString(0, 47, "  ", OLED_8X16);
    OLED_ShowChinese(24, 47, "换行符：");
    if (bt_newline_enable == 0) OLED_ShowString(95, 47, "NO", OLED_8X16);
    else OLED_ShowString(95, 47, "YES", OLED_8X16);

    OLED_Update();
}

/* ★ 根据配置发送蓝牙指令 */
void SendBtCommand(const char *cmd)
{
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    char *ptr = buffer;
    if (bt_hex_enable)
    {
        uint8_t i = 0;
        while (cmd[i] != '\0')
        {
            ptr += sprintf(ptr, "%02X", (unsigned char)cmd[i]);
            i++;
        }
    }
    else
    {
        strcpy(ptr, cmd);
        ptr += strlen(cmd);
    }

    // 2. 包头包尾
    char final[128];
    memset(final, 0, sizeof(final));
    if (bt_header_type == 1)  // @-#$
    {
        sprintf(final, "@%s#$", buffer);
    }
    else
    {
        strcpy(final, buffer);
    }

    // 3. 换行符
    if (bt_newline_enable)
    {
        strcat(final, "\r\n");
    }

    Serial_SendString(final);
}


void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void Joystick_Init(void)
{
    AD_Init();
    Calibrate_Joysticks();
}

void Calibrate_Joysticks(void)
{
    uint32_t sum1_x = 0, sum1_y = 0, sum2_x = 0, sum2_y = 0;
    uint32_t sum_lt = 0, sum_rt = 0;
    uint16_t lt_val, rt_val;
    uint8_t i;

    Delay_ms(100);

    for(i = 0; i < 100; i++)
    {
        sum1_x += AD_GetValue(ADC_Channel_0);
        sum1_y += AD_GetValue(ADC_Channel_1);
        sum2_x += AD_GetValue(ADC_Channel_2);
        sum2_y += AD_GetValue(ADC_Channel_3);
        lt_val = AD_GetValue(ADC_Channel_4);
        rt_val = AD_GetValue(ADC_Channel_5);
        sum_lt += lt_val;
        sum_rt += rt_val;
        Delay_ms(30);
    }

    joy1_center_x = sum1_x / 100;
    joy1_center_y = sum1_y / 100;
    joy2_center_x = sum2_x / 100;
    joy2_center_y = sum2_y / 100;
    lt_center = sum_lt / 100;
    rt_center = sum_rt / 100;

    for(i = 0; i < 2; i++)
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        Delay_ms(100);
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        Delay_ms(100);
    }
}

uint16_t Read_All_Buttons(void)
{
    uint16_t buttons = 0;
    uint16_t all_keys = Key_GetAllStatus();
    buttons = (all_keys >> 4) & 0x7F;
    if(all_keys & (1 << (KEY_L3 - 1))) buttons |= (1 << 7);
    if(all_keys & (1 << (KEY_R3 - 1))) buttons |= (1 << 8);
    if(all_keys & (1 << (KEY_LB - 1))) buttons |= (1 << 9);
    if(all_keys & (1 << (KEY_RB - 1))) buttons |= (1 << 10);
    return buttons;
}

uint8_t Get_Hat_Switch_Value(void)
{
    uint8_t hat_value = 8;
    uint8_t up = 0, right = 0, down = 0, left = 0;
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0) up = 1;
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0) right = 1;
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0) down = 1;
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10) == 0) left = 1;
    if(up && !right && !down && !left)      hat_value = 0;
    else if(up && right && !down && !left)  hat_value = 1;
    else if(!up && right && !down && !left) hat_value = 2;
    else if(!up && right && down && !left)  hat_value = 3;
    else if(!up && !right && down && !left) hat_value = 4;
    else if(!up && !right && down && left)  hat_value = 5;
    else if(!up && !right && !down && left) hat_value = 6;
    else if(up && !right && !down && left)  hat_value = 7;
    else                                    hat_value = 8;
    return hat_value;
}

void USB_SendReport(void)
{
    if((PrevXferComplete) && (bDeviceState == CONFIGURED) && send_enabled && (current_mode == MODE_GAMEPAD))
    {
        PrevXferComplete = 0;
        uint16_t buttons = Read_All_Buttons();
        uint8_t hat_value = Get_Hat_Switch_Value();

        int16_t lx_diff = (int16_t)cached_joy1_y - (int16_t)joy1_center_y;
        int16_t ly_diff = (int16_t)cached_joy1_x - (int16_t)joy1_center_x;
        if(lx_diff > -(int16_t)g_deadzone && lx_diff < (int16_t)g_deadzone) lx_diff = 0;
        if(ly_diff > -(int16_t)g_deadzone && ly_diff < (int16_t)g_deadzone) ly_diff = 0;
        int16_t lx_hid = lx_diff * 127 / 2048;
        int16_t ly_hid = ly_diff * 127 / 2048;
        if(lx_hid > 127) lx_hid = 127;
        if(lx_hid < -127) lx_hid = -127;
        if(ly_hid > 127) ly_hid = 127;
        if(ly_hid < -127) ly_hid = -127;
        uint8_t x_value = 128 + lx_hid;
        uint8_t y_value = 128 + ly_hid;

        int16_t rx_diff = (int16_t)cached_joy2_y - (int16_t)joy2_center_y;
        int16_t ry_diff = (int16_t)cached_joy2_x - (int16_t)joy2_center_x;
        if(rx_diff > -(int16_t)g_deadzone && rx_diff < (int16_t)g_deadzone) rx_diff = 0;
        if(ry_diff > -(int16_t)g_deadzone && ry_diff < (int16_t)g_deadzone) ry_diff = 0;
        int16_t rx_hid = rx_diff * 127 / 2048;
        int16_t ry_hid = ry_diff * 127 / 2048;
        if(rx_hid > 127) rx_hid = 127;
        if(rx_hid < -127) rx_hid = -127;
        if(ry_hid > 127) ry_hid = 127;
        if(ry_hid < -127) ry_hid = -127;
        uint8_t rx_value = 128 + rx_hid;
        uint8_t ry_value = 128 - ry_hid;

        int16_t lt_diff = (int16_t)cached_lt_val - (int16_t)lt_center;
        int16_t rt_diff = (int16_t)cached_rt_val - (int16_t)rt_center;
        int16_t z_diff = rt_diff - lt_diff;
        int32_t z_temp = ((int32_t)z_diff + 4095) * 255 / 8190;
        uint8_t z_value = (uint8_t)z_temp;
        if(z_value > 250) z_value = 250;
        if(z_value < 5) z_value = 5;

        Report_Buf[0] = buttons & 0xFF;
        Report_Buf[1] = (buttons >> 8) & 0x07;
        Report_Buf[2] = hat_value & 0x0F;
        Report_Buf[3] = x_value;
        Report_Buf[4] = y_value;
        Report_Buf[5] = z_value;
        Report_Buf[6] = rx_value;
        Report_Buf[7] = ry_value;

        UserToPMABufferCopy(Report_Buf, ENDP1_TXADDR, 8);
        SetEPTxCount(ENDP1, 8);
        SetEPTxValid(ENDP1);
    }
}

void LED_Update(void)
{
    static uint8_t led_counter = 0;
    static uint8_t led_state = 0;
    uint16_t buttons = Read_All_Buttons();
    uint8_t hat_value = Get_Hat_Switch_Value();

    if(buttons != 0 || hat_value != 8)
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
    }
    else
    {
        led_counter++;
        if(led_counter >= 25)
        {
            led_state = !led_state;
            if(led_state)
                GPIO_ResetBits(GPIOC, GPIO_Pin_13);
            else
                GPIO_SetBits(GPIOC, GPIO_Pin_13);
            led_counter = 0;
        }
    }
}

/* ======================== main 函数 ======================== */
int main(void)
{
    uint8_t adc_ready = 0;

    Set_System();
    GPIO_Configuration();

    for(int i = 0; i < 3; i++)
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        Delay_ms(200);
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        Delay_ms(200);
    }

    USB_Init();
    Key_Init();
    OLED_Init();
    Serial_Init();
    OLED_ParticleBootAnimation();

    ShowCalibrationStatus(0);
    Joystick_Init();
    ShowCalibrationStatus(1);

    WaitForKeyPress();

    OLED_RefreshDisplay();
    adc_ready = 1;

    while(1)
    {
        Key_UpdateStatus();

        uint8_t home_now = Key_GetSingleStatus(KEY_HOME);
        uint8_t menu_now = Key_GetSingleStatus(KEY_MENU);
        uint8_t view_now = Key_GetSingleStatus(KEY_VIEW);
        uint8_t up_now = Key_GetSingleStatus(KEY_UP);
        uint8_t down_now = Key_GetSingleStatus(KEY_DOWN);
        uint8_t left_now = Key_GetSingleStatus(KEY_LEFT);
        uint8_t right_now = Key_GetSingleStatus(KEY_RIGHT);
        uint8_t a_now = Key_GetSingleStatus(KEY_A);
        uint8_t lb_now = Key_GetSingleStatus(KEY_LB);
        uint8_t rb_now = Key_GetSingleStatus(KEY_RB);
        uint8_t x_now = Key_GetSingleStatus(KEY_X);
        uint8_t y_now = Key_GetSingleStatus(KEY_Y);
        uint8_t b_now = Key_GetSingleStatus(KEY_B);

        // 模式切换
        if(view_now == 1 && last_view == 0)
        {
            if(current_mode == MODE_GAMEPAD)
            {
                current_mode = MODE_BLUETOOTH;
                bt_state = BT_STATE_MAIN;
                OLED_Clear();
                OLED_ShowChinese(14, 12, "蓝牙控制模式");
							 OLED_ShowChinese(55, 36, "二");
                OLED_Update();
            }
            else
            {
                current_mode = MODE_GAMEPAD;
                current_state = STATE_GAMEPAD;
                OLED_Clear();
                OLED_ShowChinese(14, 12, "游戏手柄模式");
							  OLED_ShowChinese(55, 36, "一");
                OLED_Update();
            }
        }

        switch(current_mode)
        {
            case MODE_GAMEPAD:
                switch(current_state)
                {
                    case STATE_TEST:
                        if(home_now == 1 && last_home == 0) {
                            current_state = STATE_GAMEPAD;
                            OLED_Clear();
                            OLED_ShowChinese(14, 12, "游戏手柄模式");
													 OLED_ShowChinese(55, 36, "一");
                            OLED_Update();
                        }
                        break;

                    case STATE_GAMEPAD:
                        if(menu_now == 1 && last_menu == 0) {
                            current_state = STATE_MENU;
                            menu_selection = 0;
                            DrawMenu();
                        }
                        break;

                    case STATE_MENU:
                        if(up_now == 1 && last_up == 0) {
                            if(menu_selection > 0) menu_selection--;
                            DrawMenu();
                        }
                        if(down_now == 1 && last_down == 0) {
                            if(menu_selection < 3) menu_selection++;
                            DrawMenu();
                        }
                        if(left_now == 1 && last_left == 0) {
                            if(menu_selection == 0) {
                                if(g_deadzone <= 280) g_deadzone += 10;
                                DrawMenu();
                            } else if(menu_selection == 1) {
                                if(g_filter_alpha <= 240) g_filter_alpha += 10;
                                DrawMenu();
                            }
                        }
                        if(right_now == 1 && last_right == 0) {
                            if(menu_selection == 0) {
                                if(g_deadzone >= 40) g_deadzone -= 10;
                                DrawMenu();
                            } else if(menu_selection == 1) {
                                if(g_filter_alpha >= 50) g_filter_alpha -= 10;
                                DrawMenu();
                            }
                        }
                        if(a_now == 1 && last_a == 0) {
                            if(menu_selection == 2) {
                                current_state = STATE_TEST;
                                OLED_RefreshDisplay();
                            } else if(menu_selection == 3) {
                                ShowVersionInfo();
                                while(1) {
                                    Key_UpdateStatus();
                                    if(Key_GetSingleStatus(KEY_HOME) == 1) {
                                        while(Key_GetSingleStatus(KEY_HOME) == 1) {
                                            Key_UpdateStatus();
                                            Delay_ms(10);
                                        }
                                        break;
                                    }
                                    Delay_ms(10);
                                }
                                DrawMenu();
                            }
                        }
                        if(home_now == 1 && last_home == 0) {
                            current_state = STATE_GAMEPAD;
                            OLED_Clear();
                            OLED_ShowChinese(14, 12, "游戏手柄模式");
													 OLED_ShowChinese(55, 36, "一");
                            OLED_Update();
                        }
                        break;
                }
                break;

            case MODE_BLUETOOTH:
                switch(bt_state)
                {
                    case BT_STATE_MAIN:
                     
                        if(menu_now == 1 && last_menu == 0)
                        {
                            bt_state = BT_STATE_MENU;
                            bt_menu_selection = 0;
                            DrawBtMenu();
                        }
                        
                        if(lb_now == 1 && last_lb == 0) {
                            SendBtCommand("LB");
                        }
                        if(rb_now == 1 && last_rb == 0) {
                            SendBtCommand("RB");
                        }
                        if(x_now == 1 && last_x == 0) {
                            SendBtCommand("X");
                        }
                        if(y_now == 1 && last_y == 0) {
                            SendBtCommand("Y");
                        }
                        if(b_now == 1 && last_b == 0) {
                            SendBtCommand("B");
                        }
                        if(a_now == 1 && last_a == 0) {
                            SendBtCommand("A");
                        }
                        if(up_now == 1 && last_up == 0) {
                            SendBtCommand("U");
                        }
                        if(down_now == 1 && last_down == 0) {
                            SendBtCommand("D");
                        }
                        if(left_now == 1 && last_left == 0) {
                            SendBtCommand("R");
                        }
                        if(right_now == 1 && last_right == 0) {
                            SendBtCommand("L");
                        }
                        break;

                    case BT_STATE_MENU:
                        // 上下键移动光标
                        if(up_now == 1 && last_up == 0) {
                            if(bt_menu_selection > 0) bt_menu_selection--;
                            DrawBtMenu();
                        }
                        if(down_now == 1 && last_down == 0) {
                            if(bt_menu_selection < 2) bt_menu_selection++;
                            DrawBtMenu();
                        }
                        
                        // 左右键切换选项
                        if(left_now == 1 && last_left == 0) {
                            if(bt_menu_selection == 0) {
                                bt_header_type = (bt_header_type == 0) ? 1 : 0;
                                DrawBtMenu();
                            } else if(bt_menu_selection == 1) {
                                bt_hex_enable = (bt_hex_enable == 0) ? 1 : 0;
                                DrawBtMenu();
                            } else if(bt_menu_selection == 2) {
                                bt_newline_enable = (bt_newline_enable == 0) ? 1 : 0;
                                DrawBtMenu();
                            }
                        }
                        if(right_now == 1 && last_right == 0) {
                            if(bt_menu_selection == 0) {
                                bt_header_type = (bt_header_type == 0) ? 1 : 0;
                                DrawBtMenu();
                            } else if(bt_menu_selection == 1) {
                                bt_hex_enable = (bt_hex_enable == 0) ? 1 : 0;
                                DrawBtMenu();
                            } else if(bt_menu_selection == 2) {
                                bt_newline_enable = (bt_newline_enable == 0) ? 1 : 0;
                                DrawBtMenu();
                            }
                        }
                        
                        // MENU或HOME返回主界面
                        if( (menu_now == 1 && last_menu == 0) || (home_now == 1 && last_home == 0) )
                        {
                            bt_state = BT_STATE_MAIN;
                            OLED_Clear();
                            OLED_ShowChinese(14, 17, "蓝牙控制模式");
													 OLED_ShowChinese(55, 36, "二");
                            OLED_Update();
                        }
                        break;
                }
                break;
        }

        // 更新上次按键状态
        last_view = view_now;
        last_home = home_now;
        last_menu = menu_now;
        last_up = up_now;
        last_down = down_now;
        last_left = left_now;
        last_right = right_now;
       last_a = a_now;
        last_lb = lb_now;
        last_rb = rb_now;
        last_x = x_now;
        last_y = y_now;
        last_b = b_now;

        // ---- LED更新 ----
        LED_Update();

        // ---- ADC更新 ----
        if (adc_ready) {
            Update_ADC_Cache();
        }

        // ---- OLED刷新----
        if (adc_ready && current_mode == MODE_GAMEPAD && current_state == STATE_TEST)
        {
            static uint16_t old_joy1_x, old_joy1_y, old_joy2_x, old_joy2_y;
            static uint16_t old_lt, old_rt;
            static uint8_t first_run = 1;

            if( first_run ||
                abs((int16_t)cached_joy1_x - (int16_t)old_joy1_x) > 4 ||
                abs((int16_t)cached_joy1_y - (int16_t)old_joy1_y) > 4 ||
                abs((int16_t)cached_joy2_x - (int16_t)old_joy2_x) > 4 ||
                abs((int16_t)cached_joy2_y - (int16_t)old_joy2_y) > 4 ||
                abs((int16_t)cached_lt_val - (int16_t)old_lt) > 4 ||
                abs((int16_t)cached_rt_val - (int16_t)old_rt) > 4 )
            {
                old_joy1_x = cached_joy1_x;
                old_joy1_y = cached_joy1_y;
                old_joy2_x = cached_joy2_x;
                old_joy2_y = cached_joy2_y;
                old_lt = cached_lt_val;
                old_rt = cached_rt_val;
                first_run = 0;
                OLED_RefreshDisplay();
            }
        }

        /* USB 发送 */
        if(bDeviceState == CONFIGURED)
        {
            static uint8_t usb_initialized = 0;
            if(!usb_initialized)
            {
                usb_initialized = 1;
                send_enabled = 1;
            }

            if(send_enabled)
            {
                static uint8_t usb_tick = 0;
                usb_tick++;
                if(usb_tick >= 1)
                {
                    USB_SendReport();
                    usb_tick = 0;
                }
            }
        }
        Delay_ms(30);
    }
} 


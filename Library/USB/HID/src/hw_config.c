#include "stm32f10x.h"
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_pwr.h"
#include "hw_config.h"
#include "platform_config.h"

/* 添加延时函数声明 */
void Delay_us(uint32_t xus);
void Delay_ms(uint32_t xms);
void Delay_s(uint32_t xs);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
// ????????USB?????
#ifndef RCC_USBCLKSource_PLLCLK_Div1_5
#define RCC_USBCLKSource_PLLCLK_Div1_5    ((uint32_t)0x00000000)
#endif

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
ErrorStatus HSEStartUpStatus;
EXTI_InitTypeDef EXTI_InitStructure;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void Set_USBClock(void);
static void USB_Interrupts_Config(void);

/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : Set_System
* Description    : Configures Main system clocks & power
* Input          : None.
* Return         : None.
*******************************************************************************/
void Set_System(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* 启用时钟 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
  
  /* 配置USB断开引脚（PB12，控制D+的1.5k上拉电阻） */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_ResetBits(GPIOB, GPIO_Pin_12);  // 先断开USB
  
  /* 配置USB D+/D-引脚（PA11=DM, PA12=DP） */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* 配置测试LED（PC13） */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;  // 改为2MHz
  GPIO_Init(GPIOC, &GPIO_InitStructure);  // 注意：PC13是GPIOC，不是GPIOA
  GPIO_ResetBits(GPIOC, GPIO_Pin_13);
  
  /* 配置USB时钟 */
  Set_USBClock();
  
  /* 启用USB中断 */
  USB_Interrupts_Config();
  
  /* 延迟后连接USB */
  Delay_ms(200);  // 改为Delay_ms(200)，不是Delay(200000)
  GPIO_SetBits(GPIOB, GPIO_Pin_12);  // 连接USB
}

/*******************************************************************************
* Function Name  : Set_USBClock
* Description    : Configures USB Clock input (48MHz)
* Input          : None.
* Return         : None.
*******************************************************************************/
static void Set_USBClock(void)
{
  /* ???????,??USB?48MHz??? */
  RCC_DeInit();
  
  /* ??HSE??????(8MHz) */
  RCC_HSEConfig(RCC_HSE_ON);
  HSEStartUpStatus = RCC_WaitForHSEStartUp();
  
  if(HSEStartUpStatus == SUCCESS)
  {
    /* ??PLL:HSE=8MHz ׹ =72MHz,USB??=72/1.5=48MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
    
    /* ???????PLL */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while(RCC_GetSYSCLKSource() != 0x08);
    
    /* ??AHB/APB?? */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);    // AHB=72MHz
    RCC_PCLK1Config(RCC_HCLK_Div2);     // APB1=36MHz(USB???APB1)
    RCC_PCLK2Config(RCC_HCLK_Div1);     // APB2=72MHz
  }
  
  /* USB?????(????F10x???) */
  RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1_5);

  /* ??USB???? */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

/*******************************************************************************
* Function Name  : USB_Interrupts_Config
* Description    : Configures the USB interrupts
* Input          : None.
* Return         : None.
*******************************************************************************/
static void USB_Interrupts_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* 2 bit for pre-emption priority, 2 bits for subpriority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

  /* Enable the USB interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
* Function Name  : Enter_LowPowerMode
* Description    : Power-off system clocks and power while entering suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Enter_LowPowerMode(void)
{
  /* Set the device state to suspend */
  bDeviceState = SUSPENDED;
}

/*******************************************************************************
* Function Name  : Leave_LowPowerMode
* Description    : Restores system clocks and power while exiting suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Leave_LowPowerMode(void)
{
  DEVICE_INFO *pInfo = &Device_Info;

  /* Set the device state to the correct state */
  if (pInfo->Current_Configuration != 0)
  {
    /* Device configured */
    bDeviceState = CONFIGURED;
  }
  else
  {
    bDeviceState = ATTACHED;
  }
}

/*******************************************************************************
* Function Name  : USB_Configured_LED
* Description    : Configures the USB configured LED.
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Configured_LED(void)
{
  // ??:?????LED??(??PC13)
  GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/*******************************************************************************
* Function Name  : USB_Suspend_LED
* Description    : Configures the USB suspend LED.
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Suspend_LED(void)
{
  // ??:?????LED??
  GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}

/*******************************************************************************
* Function Name  : USB_Resume_LED
* Description    : Configures the USB resume LED.
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Resume_LED(void)
{
  // ??:?????LED??
  GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/*******************************************************************************
* Function Name  : USB_Reset_LED
* Description    : Configures the USB reset LED.
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Reset_LED(void)
{
  // ??:?????LED??
  GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}

uint32_t SysTick_GetCurrent(void)
{
  return (SysTick->VAL);
}

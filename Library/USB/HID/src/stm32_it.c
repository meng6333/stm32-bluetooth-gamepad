/**
  ******************************************************************************
  * @file    stm32_it.c
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32_it.h"
#include "usb_istr.h"
#include "usb_lib.h"
#include "usb_pwr.h"
#include "hw_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint8_t Send_Buffer[2];
extern __IO uint8_t PrevXferComplete;
extern uint32_t ADC_ConvertedValueX;
extern uint32_t ADC_ConvertedValueX_1;
extern __IO uint32_t TimingDelay;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M Processor Exceptions Handlers                         */
/******************************************************************************/
void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    TimingDelay--;
}

/******************************************************************************/
/*            STM32 Peripherals Interrupt Handlers                        */
/******************************************************************************/
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS) || defined (STM32F37X)
void USB_LP_IRQHandler(void)
#else
void USB_LP_CAN1_RX0_IRQHandler(void)
#endif
{
  USB_Istr();
}

#if defined(STM32L1XX_MD)

void EXTI15_10_IRQHandler(void)
{
  /* Check on the RIGHT button */
  if(EXTI_GetITStatus(RIGHT_BUTTON_EXTI_LINE) != RESET)
  {  
    if ((PrevXferComplete) && (bDeviceState == CONFIGURED))
    {
      Send_Buffer[0] = 0x05; 
      
      if (STM_EVAL_PBGetState(Button_RIGHT) == Bit_RESET)
      {
        Send_Buffer[1] = 0x01;
      }
      else 
      {
        Send_Buffer[1] = 0x00;
      }  
      
      USB_SIL_Write(EP1_IN, (uint8_t*) Send_Buffer, 2);  
      SetEPTxValid(ENDP1);
      PrevXferComplete = 0;
    }
    EXTI_ClearITPendingBit(RIGHT_BUTTON_EXTI_LINE);
  }  

  /* Check on the LEFT button */
  if(EXTI_GetITStatus(LEFT_BUTTON_EXTI_LINE) != RESET)
  {  
    if ((PrevXferComplete) && (bDeviceState == CONFIGURED))
    {
      Send_Buffer[0] = 0x06; 
      
      if (STM_EVAL_PBGetState(Button_LEFT) == Bit_RESET)
      {
        Send_Buffer[1] = 0x01;
      }
      else 
      {
        Send_Buffer[1] = 0x00;
      }  
      
      USB_SIL_Write(EP1_IN, (uint8_t*) Send_Buffer, 2);  
      SetEPTxValid(ENDP1);
      PrevXferComplete = 0;
    }
    EXTI_ClearITPendingBit(LEFT_BUTTON_EXTI_LINE);
  }  
}
*/
#endif

void DMA1_Channel1_IRQHandler(void)
{  
  Send_Buffer[0] = 0x07;
  
  if((ADC_ConvertedValueX >>4) - (ADC_ConvertedValueX_1 >>4) > 4)
  {
    if ((PrevXferComplete) && (bDeviceState == CONFIGURED))
    {
      Send_Buffer[1] = (uint8_t)(ADC_ConvertedValueX >>4);
      USB_SIL_Write(EP1_IN, (uint8_t*) Send_Buffer, 2);  
      SetEPTxValid(ENDP1);
      ADC_ConvertedValueX_1 = ADC_ConvertedValueX;
      PrevXferComplete = 0;
    }
  }
  
  DMA_ClearFlag(DMA1_FLAG_TC1);
}

#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
void EXTI0_IRQHandler(void)
#elif defined (STM32F37X)
void EXTI2_TS_IRQHandler(void)
#else
void EXTI9_5_IRQHandler(void)
#endif
{
  // ?????????,???EXTI9_5?????
  EXTI_ClearITPendingBit(EXTI_Line5 | EXTI_Line6 | EXTI_Line7 | EXTI_Line8 | EXTI_Line9);
}

#if !defined(STM32L1XX_MD) &&  !defined(STM32L1XX_HD) && !defined(STM32L1XX_MD_PLUS)&& ! defined (STM32F37X) && !defined(STM32F303xC) && !defined(STM32F303xE)
void EXTI15_10_IRQHandler(void)
{
  // ?????????,???EXTI15_10?????
  EXTI_ClearITPendingBit(EXTI_Line10 | EXTI_Line11 | EXTI_Line12 | EXTI_Line13 | EXTI_Line14 | EXTI_Line15);
}
#endif

#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
void USB_FS_WKUP_IRQHandler(void)
#else
void USBWakeUp_IRQHandler(void)
#endif
{
  EXTI_ClearITPendingBit(EXTI_Line18);
}

/*void PPP_IRQHandler(void)
{
}*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

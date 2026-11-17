#include "hw_config.h"
#include "usb_lib.h"
#include "usb_istr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* 修改Report_Buf大小声明，与usb_prop.c保持一致 */
extern uint8_t Report_Buf[3];  /* 从8改为2字节 */
extern __IO uint8_t PrevXferComplete;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : EP1_IN_Callback
* Description    : EP1 IN Callback Routine
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_IN_Callback(void)
{
  /* IN传输完成回调 - 设置标志位允许下次发送 */
  PrevXferComplete = 1;
}

/*******************************************************************************
* Function Name  : EP1_OUT_Callback
* Description    : EP1 OUT Callback Routine
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_OUT_Callback(void)
{
  /* 读取接收的数据（如果需要） */
  uint8_t Receive_Buffer[3];  /* 改为2字节，与端点包大小一致 */
  
  /* 获取接收的数据长度 */
  uint16_t DataLength = GetEPRxCount(ENDP1);
  
  /* 确保不超过缓冲区大小 */
  if (DataLength > 3)
  {
    DataLength = 3;
  }
  
  /* 从USB端点读取数据 */
  USB_SIL_Read(EP1_OUT, Receive_Buffer);
  
  /* 重新使能端点接收 */
  SetEPRxStatus(ENDP1, EP_RX_VALID);
  
  /* 如果需要，可以在这里处理接收到的数据 */
  /* 例如：if (Receive_Buffer[0] == 0x01) { ... } */
}

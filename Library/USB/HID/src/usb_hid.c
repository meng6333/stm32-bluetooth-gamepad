/**
  * @file    usb_hid.c
  * @brief   USB HID??????
  */
#include "usb_hid.h"
#include "usb_core.h"
#include "usb_istr.h"
#include "usb_pwr.h"

/* ???? */
u8 HID_Report_Buf[HID_REPORT_BUF_SIZE] = {0};  // HID?????
u8 HID_Send_Flag = 0;                          // ????

/**
  * @brief  ??HID???????
  * @param  buf: ???????
  * @param  len: ????(??4??)
  * @retval ?
  */
void HID_SendReport(u8 *buf, u16 len)
{
  if(len > HID_REPORT_BUF_SIZE) len = HID_REPORT_BUF_SIZE;
  
  // ??????????
  for(u16 i=0; i<len; i++)
  {
    HID_Report_Buf[i] = buf[i];
  }
  
  // ??USB??,????
  if(bDeviceState == CONFIGURED)  // USB???
  {
    UserToPMABufferCopy(HID_Report_Buf, ENDP1_TXADDR, len);
    SetEPTxCount(ENDP1, len);
    SetEPTxValid(ENDP1);          // ????1??
  }
}

/**
  * @brief  USB HID???
  * @retval ?
  */
void HID_Init(void)
{
  USB_Init();  // ??USB?????
}

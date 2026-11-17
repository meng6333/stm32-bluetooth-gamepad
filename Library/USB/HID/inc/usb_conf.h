#ifndef __USB_CONF_H
#define __USB_CONF_H


#define EP_NUM     (2)


/* 修正的缓冲区地址定义 */
#define BTABLE_ADDRESS      (0x00)

/* EP0 控制端点 */
#define ENDP0_RXADDR        (0x40)    /* 端点0接收缓冲区地址 (64字节) */
#define ENDP0_TXADDR        (0x80)    /* 端点0发送缓冲区地址 (64字节) */

/* EP1 中断端点 */
#define ENDP1_TXADDR        (0xC0)    /* 端点1发送缓冲区地址 (64字节) */
#define ENDP1_RXADDR        (0x100)   /* 端点1接收缓冲区地址 (8字节) */

/* 启用EP1_IN_Callback */
#define  EP1_IN_Callback   EP1_IN_Callback
/*-------------------------------------------------------------*/
/* -------------------   ISTR events  -------------------------*/
/*-------------------------------------------------------------*/
/* IMR_MSK */
/* mask defining which events has to be handled */
/* by the device application software */

#define IMR_MSK (CNTR_CTRM  | CNTR_WKUPM | CNTR_SUSPM | CNTR_ERRM  | CNTR_SOFM \
                 | CNTR_ESOFM | CNTR_RESETM )

/* CTR service routines */
/* associated to defined endpoints */
/* #define  EP1_IN_Callback   NOP_Process */
#define  EP2_IN_Callback   NOP_Process
#define  EP3_IN_Callback   NOP_Process
#define  EP4_IN_Callback   NOP_Process
#define  EP5_IN_Callback   NOP_Process
#define  EP6_IN_Callback   NOP_Process
#define  EP7_IN_Callback   NOP_Process

//#define  EP1_OUT_Callback   NOP_Process
#define  EP2_OUT_Callback   NOP_Process
#define  EP3_OUT_Callback   NOP_Process
#define  EP4_OUT_Callback   NOP_Process
#define  EP5_OUT_Callback   NOP_Process
#define  EP6_OUT_Callback   NOP_Process
#define  EP7_OUT_Callback   NOP_Process

#endif /*__USB_CONF_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


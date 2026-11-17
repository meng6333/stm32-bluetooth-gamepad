/**
  ******************************************************************************
  * @file    usb_desc.c
  * @author  MCD Application Team
  * @version V4.1.0
  * @date    26-May-2017
  * @brief   Descriptors for Custom HID Demo
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"
#include "usb_desc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* USB Standard Device Descriptor (18字节) */
const uint8_t CustomHID_DeviceDescriptor[CUSTOMHID_SIZ_DEVICE_DESC] =
{
  0x12,                       /* bLength */
  USB_DEVICE_DESCRIPTOR_TYPE, /* bDescriptorType */
  0x00, 0x02,                 /* bcdUSB (2.00) */
  0x00,                       /* bDeviceClass */
  0x00,                       /* bDeviceSubClass */
  0x00,                       /* bDeviceProtocol */
  0x40,                       /* bMaxPacketSize40 */
  0x83, 0x04,                 /* idVendor (0x0483) */
  0x50, 0x57,                 /* idProduct = 0x5750 */
  0x00, 0x02,                 /* bcdDevice (2.00) */
  0x01,                       /* iManufacturer */
  0x02,                       /* iProduct */
  0x03,                       /* iSerialNumber */
  0x01                        /* bNumConfigurations */
};

/* USB Configuration Descriptor (41字节) */
const uint8_t CustomHID_ConfigDescriptor[CUSTOMHID_SIZ_CONFIG_DESC] =
{
  /* 配置描述符 (9字节) */
  0x09,                       /* bLength: Configuration Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
  CUSTOMHID_SIZ_CONFIG_DESC, 0x00, /* wTotalLength: 41 bytes */
  0x01,                       /* bNumInterfaces: 1 interface */
  0x01,                       /* bConfigurationValue: Configuration value */
  0x00,                       /* iConfiguration: Index of string descriptor */
  0x80,                       /* bmAttributes: Bus powered */
  0x32,                       /* MaxPower 100 mA */
  
  /* 接口描述符 (9字节) */
  0x09,                       /* bLength: Interface Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: Interface */
  0x00,                       /* bInterfaceNumber: Number of Interface */
  0x00,                       /* bAlternateSetting: Alternate setting */
  0x02,                       /* bNumEndpoints: 2 endpoints */
  0x03,                       /* bInterfaceClass: HID */
  0x00,                       /* bInterfaceSubClass: 0=no boot */
  0x00,                       /* bInterfaceProtocol: 0=none */
  0x00,                       /* iInterface: Index of string descriptor */
  
  /* HID描述符 (9字节) */
  0x09,                       /* bLength: HID Descriptor size */
  HID_DESCRIPTOR_TYPE,        /* bDescriptorType: HID */
  0x10, 0x01,                 /* bcdHID: HID Class Spec release number (1.10) */
  0x00,                       /* bCountryCode: Hardware target country */
  0x01,                       /* bNumDescriptors: Number of HID class descriptors */
  0x22,                       /* bDescriptorType: Report descriptor */
  CUSTOMHID_SIZ_REPORT_DESC, 0x00, /* wItemLength: Report descriptor size */
  
  /* 端点1 IN 描述符 (7字节) */
  0x07,                       /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: Endpoint */
  0x81,                       /* bEndpointAddress: Endpoint Address (IN 1) */
  0x03,                       /* bmAttributes: Interrupt endpoint */
  0x08, 0x00,                 /* wMaxPacketSize: 1 byte */
  0x08,                       /* bInterval: Polling Interval (8 ms) */
  
  /* 端点1 OUT 描述符 (7字节) */
  0x07,                       /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: Endpoint */
  0x01,                       /* bEndpointAddress: Endpoint Address (OUT 1) */
  0x03,                       /* bmAttributes: Interrupt endpoint */
  0x08, 0x00,                 /* wMaxPacketSize: 1 byte */
  0x08                        /* bInterval: Polling Interval (8 ms) */
};
const uint8_t CustomHID_ReportDescriptor[CUSTOMHID_SIZ_REPORT_DESC] = 
{
  /* 通用桌面控制器 - 游戏手柄 */
  0x05, 0x01,        /* USAGE_PAGE (Generic Desktop) */
  0x09, 0x05,        /* USAGE (Game Pad) */
  0xA1, 0x01,        /* COLLECTION (Application) */
  
  /* ★ 15个按钮 (2字节，用15位) */
  0x05, 0x09,        /*   USAGE_PAGE (Button) */
  0x19, 0x01,        /*   USAGE_MINIMUM (Button 1) */
  0x29, 0x0B,        /* ★ 原来是0x09，改为0x10 (Button 16) */
  0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
  0x25, 0x01,        /*   LOGICAL_MAXIMUM (1) */
  0x75, 0x01,        /*   REPORT_SIZE (1) */
  0x95, 0x10,        /*   REPORT_COUNT (16) - 2字节，16位 */
  0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
  
  /* 方向键 (POV Hat) - 4位 */
  0x05, 0x01,        /*   USAGE_PAGE (Generic Desktop) */
  0x09, 0x39,        /*   USAGE (Hat switch) */
  0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
  0x25, 0x07,        /*   LOGICAL_MAXIMUM (7) */
  0x35, 0x00,        /*   PHYSICAL_MINIMUM (0) */
  0x46, 0x3B, 0x01,  /*   PHYSICAL_MAXIMUM (315) */
  0x65, 0x14,        /*   UNIT (Eng Rot: Angular Pos) */
  0x75, 0x04,        /*   REPORT_SIZE (4) */
  0x95, 0x01,        /*   REPORT_COUNT (1) */
  0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
  
  /* 填充位 (4位) */
  0x75, 0x04,        /*   REPORT_SIZE (4) */
  0x95, 0x01,        /*   REPORT_COUNT (1) */
  0x81, 0x03,        /*   INPUT (Cnst,Var,Abs) */
  
  /* X轴 (左摇杆左右) */
  0x05, 0x01,        /*   USAGE_PAGE (Generic Desktop) */
  0x09, 0x30,        /*   USAGE (X) */
  0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
  0x26, 0xFF, 0x00,  /*   LOGICAL_MAXIMUM (255) */
  0x75, 0x08,        /*   REPORT_SIZE (8) */
  0x95, 0x01,        /*   REPORT_COUNT (1) */
  0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
  
  /* Y轴 (左摇杆上下) */
  0x09, 0x31,        /*   USAGE (Y) */
  0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
  0x26, 0xFF, 0x00,  /*   LOGICAL_MAXIMUM (255) */
  0x75, 0x08,        /*   REPORT_SIZE (8) */
  0x95, 0x01,        /*   REPORT_COUNT (1) */
  0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
  
  /* Z轴 (左扳机) */
  0x09, 0x32,        /*   USAGE (Z) */
  0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
  0x26, 0xFF, 0x00,  /*   LOGICAL_MAXIMUM (255) */
  0x75, 0x08,        /*   REPORT_SIZE (8) */
  0x95, 0x01,        /*   REPORT_COUNT (1) */
  0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
  
  /* X旋转 (右摇杆左右) */
  0x09, 0x33,        /*   USAGE (Rx) */
  0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
  0x26, 0xFF, 0x00,  /*   LOGICAL_MAXIMUM (255) */
  0x75, 0x08,        /*   REPORT_SIZE (8) */
  0x95, 0x01,        /*   REPORT_COUNT (1) */
  0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
  
  /* Y旋转 (右摇杆上下) */
  0x09, 0x34,        /*   USAGE (Ry) */
  0x15, 0x00,        /*   LOGICAL_MINIMUM (0) */
  0x26, 0xFF, 0x00,  /*   LOGICAL_MAXIMUM (255) */
  0x75, 0x08,        /*   REPORT_SIZE (8) */
  0x95, 0x01,        /*   REPORT_COUNT (1) */
  0x81, 0x02,        /*   INPUT (Data,Var,Abs) */
  
  0xC0               /* END_COLLECTION */
};


/* USB 字符串描述符 - 语言ID */
const uint8_t CustomHID_StringLangID[CUSTOMHID_SIZ_STRING_LANGID] =
{
  CUSTOMHID_SIZ_STRING_LANGID,
  USB_STRING_DESCRIPTOR_TYPE,
  0x09, 0x04
};

/* USB 字符串描述符 - 厂商 */
const uint8_t CustomHID_StringVendor[CUSTOMHID_SIZ_STRING_VENDOR] =
{
  CUSTOMHID_SIZ_STRING_VENDOR,
  USB_STRING_DESCRIPTOR_TYPE,
  'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
  'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
  'c', 0, 's', 0
};

/* USB 字符串描述符 - 产品 */
const uint8_t CustomHID_StringProduct[CUSTOMHID_SIZ_STRING_PRODUCT] =
{
  CUSTOMHID_SIZ_STRING_PRODUCT,
  USB_STRING_DESCRIPTOR_TYPE,
  'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0
};

/* USB 字符串描述符 - 序列号 */
uint8_t CustomHID_StringSerial[CUSTOMHID_SIZ_STRING_SERIAL] =
{
  CUSTOMHID_SIZ_STRING_SERIAL,
  USB_STRING_DESCRIPTOR_TYPE,
  'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0
};

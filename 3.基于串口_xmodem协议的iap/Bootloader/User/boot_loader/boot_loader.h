#ifndef __BOOT_LOADER_H__
#define __BOOT_LOADER_H__

#include "stm32f4xx.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
typedef  void (*pFunction)(void);

#define FLASH_APP_ADDR		0x08010000  	// Ӧ�ó�����ʼ��ַ(�����FLASH)
											                  // ���� 0X08000000~0X0800FFFF �Ŀռ�Ϊ Bootloader ʹ��(��64KB)	  

void iap_jump_app(u32 appxaddr);			  // ��ת�� APP ����ִ��

#endif /* __USART1_H */

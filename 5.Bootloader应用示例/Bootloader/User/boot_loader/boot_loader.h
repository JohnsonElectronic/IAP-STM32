#ifndef __BOOT_LOADER_H__
#define __BOOT_LOADER_H__

#include "stm32f4xx.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
typedef  void (*pFunction)(void);

typedef struct
{
	uint8_t  update_flag;    // ���±�־
	uint32_t size;       // app ��С
} app_info_t;

#define SPI_FLASH_APP_ADDR     22020096    // app ���ⲿ��ƫ�Ƶ�ַ�ӵ� 21MB ��ʼ��
#define FLASH_APP_ADDR		     0x8004000   // Ӧ�ó�����ʼ��ַ(�����FLASH)
											                     // ���� 0X08000000~0X0800FFFF �Ŀռ�Ϊ Bootloader ʹ��(��64KB)	  

void iap_jump_app(u32 appxaddr);			     // ��ת�� APP ����ִ��
int sflash_to_iflash(uint32_t des_addr, uint32_t src_addr, uint32_t size);

#endif /* __USART1_H */

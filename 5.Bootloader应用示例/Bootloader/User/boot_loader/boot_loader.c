/**
  ******************************************************************************
  * @file    boot_loader.c
  * @author  fire
  * @version V1.0
  * @date    2019-xx-xx
  * @brief   ��תʵ��
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ��  STM32 F429 ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 

#include "./boot_loader/boot_loader.h" 
#include "./flash/bsp_spi_flash.h"
#include "./internalFlash/bsp_internalFlash.h"
#include "./lcd/bsp_lcd.h"

pFunction JumpToApplication; 
uint8_t buff[1024*10];

//��ת��Ӧ�ó����
//appxaddr:�û�������ʼ��ַ.
void iap_jump_app(u32 appaddr)
{ 
	if(((*(vu32*)appaddr)&0x2FF00000)==0x20000000)	                        // ���ջ����ַ�Ƿ�Ϸ�.
	{
    /* Initialize user application's Stack Pointer & Jump to user application */
		JumpToApplication = (pFunction) (*(__IO uint32_t*) (appaddr + 4));		// �û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
		__set_MSP(*(__IO uint32_t*) appaddr);				                         	// ��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
		JumpToApplication();									                                // ��ת��APP.
	}
}

/**
 * @brief   Xmodem ����Ҫ����������ݵ�����.
 * @param   address �����ݵ�ַ����������
 * @return  ���ص�ǰ����ʣ��Ĵ�С
 */
uint32_t i_flash_erasure(uint32_t address)
{
  sector_t sector_info;
  if (erasure_sector(address, 1))    // ������ǰ��ַ��������
  {
    return 0;    // ����ʧ��
  }

  sector_info = GetSector(address);    // �õ���ǰ��������Ϣ

  return sector_info.size + sector_info.start_addr - address;     // ���ص�ǰ����ʣ���С
}

/**
 * @brief   �ļ����ݽ�����ɻص�.
 * @param   *ptr: ���ƾ��.
 * @param   *file_name: �ļ�����.
 * @param   file_size: �ļ���С����Ϊ0xFFFFFFFF����˵����С��Ч.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int w_app_to_flash(uint32_t addr, char *file_data, uint32_t w_size)
{
  static uint32_t sector_size = 0;    /* ����ʣ���С. */
  
  /* ��ǰ���������˲�����һ��. */
  if (sector_size <= w_size)
  {
    sector_size += i_flash_erasure(addr + sector_size);

    if (0 == sector_size)
    {
      return -1;
    }
  }
  
  if (flash_write_data(addr, (uint8_t *)file_data, w_size) == 0)    // д������
  {
    sector_size -= w_size;
    return 0;
  }
  else 
  {
    return -1;
  }
}

/**
 * @brief   ���ⲿflash���ݿ������ڲ�flash.
 * @param   des:�ڲ�FLASH��ַ
 * @param   src:�ⲿFLASH��ַ
 * @param   size:Ҫ�������ļ���С
 * @return  0:�ļ����ճɹ� -1:�ļ�����ʧ��
 */
int sflash_to_iflash(uint32_t des_addr, uint32_t src_addr, uint32_t size)
{
  uint32_t flash_address = des_addr;
  uint32_t w_size = 1024;
  uint32_t s_size = size;
  char cbuff[128];
  
  /* ��ʾ������ʾ */
  sprintf(cbuff, "                  ��������Ӧ�ó���");
  LCD_SetTextColor(LCD_COLOR565_WHITE);
  LCD_DisplayStringLine_EN_CH(LINE(3),(uint8_t* )cbuff);
  
  /* ���ƽ��������� */
  LCD_SetTextColor(LCD_COLOR565_WHITE);
  L_DrawRect(100, 147, 600, 10);
  
  for (uint32_t i=0; i<size; i+=w_size)
  {
    if (size - i < w_size)
    {
      w_size = size - i;
    }
    /* �� SPI FLASH ��������� */
    SPI_FLASH_BufferRead(buff, src_addr + i, w_size);
    
    /* д���ڲ� FLASH */
    if (w_app_to_flash(flash_address + i, (char *)buff, w_size) == -1)
    {
      return -1;
    }
    
    /* ���ƽ����� */
    LCD_SetTextColor(LCD_COLOR565_RED);
    L_DrawRect(101, 148, 598 * i / s_size, 8);
  }

  return 0;
}


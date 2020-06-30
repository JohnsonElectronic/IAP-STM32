/**
  ******************************************************************************
  * @file    iap_arch.c
  * @author  long
  * @version V1.0
  * @date    2019-11-23
  * @brief   xmodem ����ӿ��ļ�
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� STM32 F429 ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "./iap_arch/iap_arch.h"   
#include <stdio.h>
#include "./usart/bsp_debug_usart.h"
#include "./internalFlash/bsp_internalFlash.h" 
#include "./boot_loader/boot_loader.h"
#include "./lcd/bsp_lcd.h"

static uint32_t xmodem_actual_flash_address = FLASH_APP_ADDR;       /* д��ĵ�ַ. */

/**
 * @brief   Xmodem ����һ���ַ��Ľӿ�.
 * @param   ch �����͵�����
 * @return  ���ط���״̬
 */
int x_transmit_ch(uint8_t ch)
{
	Usart_SendByte(DEBUG_USART, ch);
	
	return 0;
}

/**
 * @brief   Xmodem ����Ҫ����������ݵ�����.
 * @param   address �����ݵ�ַ����������
 * @return  ���ص�ǰ����ʣ��Ĵ�С
 */
uint32_t x_receive_flash_erasure(uint32_t address)
{
  sector_t sector_info;
  if (erasure_sector(address, 1))    // ������ǰ��ַ��������
  {
    return 0;    // ����ʧ��
  }

  sector_info = GetSector(address);    // �õ���ǰ��������Ϣ

  return (sector_info.size + sector_info.start_addr - address);     // ���ص�ǰ����ʣ���С
}

/**
  * @brief   Xmodem �����ܵ������ݱ��浽flash.
  * @param  start_address ��Ҫд�����ʼ��ַ
  * @param  *data : ��Ҫ���������
	* @param  len ������
  * @return д��״̬
 */
int x_receive_flash_writea(uint32_t start_address, const void *data, uint32_t len)
{
  if (flash_write_data(start_address, (uint8_t *)data, len) == 0)    // ������ǰ��ַ��������
  {
    return 0;    // д��ɹ�
  }
  else
  {
    return -1;    // д��ʧ��
  }
}

/**
 * @brief   �ļ����ݽ�����ɻص�.
 * @param   *ptr: ���ƾ��.
 * @param   *file_name: �ļ�����.
 * @param   file_size: �ļ���С����Ϊ0xFFFFFFFF����˵����С��Ч.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_file_data_callback(void *ptr, char *file_data, uint32_t w_size)
{
  static uint32_t sector_size = 0;    /* ����ʣ���С. */
  static uint32_t recv_size = 0;    /* ����ʣ���С. */
  uint8_t buff[128];
  
  /* ��ǰ���������˲�����һ��. */
  if (sector_size <= w_size)
  {
    sector_size += x_receive_flash_erasure(xmodem_actual_flash_address + sector_size);

    if (0 == sector_size)
    {
      return -1;
    }
  }
  
  if (flash_write_data(xmodem_actual_flash_address, (uint8_t *)file_data, w_size) == 0)    // д������
  {
    xmodem_actual_flash_address += w_size;
    recv_size += w_size;
    sector_size -= w_size;
    sprintf((char*)buff, "                 �ѽ��գ�%d�ֽڣ�", recv_size);
    LCD_DisplayStringLine_EN_CH(LINE(3), buff);
    return 0;
  }
  else 
  {
    return -1;
  }
}

/**
 * @brief   �ļ�������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_file_callback(void *ptr)
{
//  printf("��ʼ���� App��\r\n");
//	iap_jump_app(FLASH_APP_ADDR);
  LCD_DisplayStringLine_EN_CH(LINE(5), "                 Ӧ�ó��������ɣ�");
  
  return 0;
}




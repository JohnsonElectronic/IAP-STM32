/**
  ******************************************************************************
  * @file    iap_arch.c
  * @author  long
  * @version V1.0
  * @date    2019-11-23
  * @brief   ymodem ����ӿ��ļ�
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
#include "./ymodem/ymodem.h"

/**
 * @brief   Ymodem ����һ���ַ��Ľӿ�.
 * @param   ch �����͵�����
 * @return  ���ط���״̬
 */
int y_transmit_ch(uint8_t ch)
{
	Usart_SendByte(DEBUG_USART, ch);
	
	return 0;
}

/**
 * @brief   Ymodem ����Ҫ����������ݵ�����.
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


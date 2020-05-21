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

/**
 * @brief   Xmodem �������ݵĽӿ�.
 * @param   *data ����������
 * @param   *len ���������ݵĳ���
 * @return  �������ݵ�״̬
 */

int x_receive(__IO uint8_t **data, uint32_t len)
{
	__IO uint32_t timeout = RECEIVE_TIMEOUT;
	
	while (data_rx_flag == 0 && timeout--)   // �ȴ����ݽ������
	{
		if (timeout == 0)
		{
			return -1;    // ��ʱ����
		}
	}
	
	/* ��ȡ�������� */
	*data = get_rx_data();
//	(void)data;
	get_rx_len();
	// if (len != )
	// {
	// 	return -1;    // ���Ȳ���ȷ���ش���
	// }
		
	return 0;
}

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


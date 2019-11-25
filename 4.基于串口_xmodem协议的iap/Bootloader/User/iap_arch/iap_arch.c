/**
  ******************************************************************************
  * @file    iap_arch.c
  * @author  long
  * @version V1.0
  * @date    2019-11-23
  * @brief   �ڲ�FLASH��д���Է���
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

/**
 * @brief   Xmodem �������ݵĽӿ�.
 * @param   *data ����������
 * @param   *len ���������ݵĳ���
 * @return  �������ݵ�״̬
 */
int x_receive(volatile uint8_t *data, uint32_t len)
{
	while (data_rx_flag == 1)   // �ȴ����ݽ������
	{}
	
	/* ��ȡ�������� */
	data = get_rx_data();
		
	if (len == get_rx_len())
	{
		return -1;    // ���Ȳ���ȷ���ش���
	}
		
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


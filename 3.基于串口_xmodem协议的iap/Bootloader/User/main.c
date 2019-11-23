/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   ʹ�ð������Ʋʵ�
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ��  STM32 F429 ������
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "stm32f4xx.h"
#include "./led/bsp_led.h"
#include "./key/bsp_key.h" 
#include "./boot_loader/boot_loader.h" 
#include "./usart/bsp_debug_usart.h"
#include "./tim/bsp_basic_tim.h"
#include "./internalFlash/bsp_internalFlash.h"  

/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{
	uint8_t *app_data = NULL;
  uint32_t app_len = 0;
	
  /*��ʼ������*/
  Key_GPIO_Config();
	Debug_USART_Config();
	TIMx_Configuration();
	
	printf(" IAP ��ʾ DEMO��\r\n");

	/* ��ѯ����״̬��������������תLED */ 
	while(1)
	{
    
    if (data_rx_flag == 1 > 0 && get_sec_timestamp() > 2)    // ���յ����ݲ����Ѿ�����2s����û�н��յ��µ�����
    {
      data_rx_flag = 2;
      printf("�µ� App �Ѿ�������ɣ�\r\n");
    }
    
		if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON  )
		{
			/* ��ȡ�������� */
      app_data = get_rx_data();
			app_len  = get_rx_len();
			
			printf("��ʼ���� App ���� %d ���ֽ�\r\n", app_len);
			
			for (uint32_t i=0; i<app_len; i++)
			{
				printf("%02X ", app_data[i]);
			}
      
      /* д App �� FLASH */
			if (flash_write_data(FLASH_APP_ADDR, app_data, app_len) == 0)
			{
				printf("���� App ��ɣ�\r\n");
			}
			else
			{
				printf("���� App ʧ�ܣ�\r\n");
			}
			
			reset_rx_data();
		}
    
    if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON  )
		{
			/*LED2��ת*/
			printf("��ʼ���� App��\r\n");
			iap_jump_app(FLASH_APP_ADDR);
		}
	}
}



/*********************************************END OF FILE**********************/


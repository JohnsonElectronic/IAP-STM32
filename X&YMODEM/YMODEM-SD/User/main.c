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
#include "./ymodem/ymodem.h"
#include "./FATFS/ff.h"

FATFS fs;													/* FatFs�ļ�ϵͳ���� */

/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{
	uint8_t *app_data = NULL;
  uint32_t app_len = 0;
	uint8_t update_flag = 0;
  uint8_t count = 1;    // Ҫ���յ��ļ�����
  FRESULT res_sd;                /* �ļ�������� */
	
  /*��ʼ������*/
  Key_GPIO_Config();
	Debug_USART_Config();
//	TIMx_Configuration();
  
  //���ⲿ sd �����ļ�ϵͳ
	res_sd = f_mount(&fs, "0:" ,1);
	if(res_sd!=FR_OK)
  {
    printf("����SD�������ļ�ϵͳʧ�ܡ�(%d)\r\n",res_sd);
    printf("��������ԭ��SD����ʼ�����ɹ���\r\n");
		while(1);
  }
  else
  {
    printf("���ļ�ϵͳ���سɹ�\r\n");
  }
	
	printf(" IAP ��ʾ DEMO��\r\n");

	/* ��ѯ����״̬��������������תLED */ 
	while(1)
	{
		if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON  )
		{
			/* ��ȡ�������� */
      update_flag = 1;
		}
    
    if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON  )
		{
			/*LED2��ת*/
			count++;
      printf("��Ҫ����%d���ļ�\r\n", count);
		}
		
		if (update_flag)
		{
			ymodem_receive(count);
      update_flag = 0;
		}
	}
}



/*********************************************END OF FILE**********************/


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

/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{
  /*��ʼ������*/
  Key_GPIO_Config();
	
	/* ��ѯ����״̬��������������תLED */ 
	while(1)
	{	   
		if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON  )
		{
			/*LED1��ת*/
			iap_jump_app(0x8042000);
		}   
    
    if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON  )
		{
			/*LED2��ת*/
			iap_jump_app(0x08010000);
		}   
	}
}



/*********************************************END OF FILE**********************/


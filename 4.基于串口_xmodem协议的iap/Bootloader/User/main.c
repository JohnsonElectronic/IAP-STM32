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
#include "./xmodem/xmodem.h"

/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{
	uint8_t update_flag = 0;
	
  /*��ʼ������*/
  Key_GPIO_Config();
	Debug_USART_Config();
  LED_GPIO_Config();
//	TIMx_Configuration();
	
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
			save_data_flash(FLASH_APP_ADDR, 0, 1152);
      iap_jump_app(FLASH_APP_ADDR);
		}
		
		if (update_flag)
		{
      LED2_ON;
			if (xmodem_receive() == 0)
      { 
        LED2_OFF;
        printf("��ʼ���� App��\r\n");
        iap_jump_app(FLASH_APP_ADDR);
      }
      else
      {
        LED3_ON;
      }
      LED2_OFF;
      update_flag = 0;
		}
	}
}



/*********************************************END OF FILE**********************/


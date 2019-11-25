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


pFunction JumpToApplication; 


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















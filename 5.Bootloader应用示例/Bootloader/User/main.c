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
#include "./lcd/bsp_lcd.h"
#include "./flash/bsp_spi_flash.h"

sFONT Font16x24 = {
  0,
  16, /* Width */
  24, /* Height */
};

/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{
	uint8_t update_flag[1];
	
  /*��ʼ������*/
  Key_GPIO_Config();
	Debug_USART_Config();
  LED_GPIO_Config();
  
  /*��ʼ��Һ����*/
  LCD_Init();
  LCD_LayerInit();
  LTDC_Cmd(ENABLE);
	
	/*�ѱ�����ˢ��ɫ*/
  LCD_SetLayer(LCD_BACKGROUND_LAYER);  
	LCD_Clear(LCD_COLOR_BLACK);
	
  /*��ʼ����Ĭ��ʹ��ǰ����*/
	LCD_SetLayer(LCD_FOREGROUND_LAYER); 
	/*Ĭ�����ò�͸��	���ú�������Ϊ��͸���ȣ���Χ 0-0xff ��0Ϊȫ͸����0xffΪ��͸��*/
  LCD_SetTransparency(0xFF);
	LCD_Clear(LCD_COLOR_BLACK);
	/*����LCD_SetLayer(LCD_FOREGROUND_LAYER)������
	����Һ����������ǰ����ˢ�£��������µ��ù�LCD_SetLayer�������ñ�����*/
  
  /*ʹ�ò�͸��ǰ����*/
	LCD_SetLayer(LCD_FOREGROUND_LAYER);  
  LCD_SetTransparency(0xff);
	
  LCD_Clear(LCD_COLOR_BLACK);	/* ��������ʾȫ�� */

	/*����������ɫ������ı�����ɫ(�˴��ı�������ָLCD�ı����㣡ע������)*/
  LCD_SetColors(LCD_COLOR_WHITE,LCD_COLOR_BLACK);
  
  LCD_SetFont(&Font16x24);

	LCD_DisplayStringLine_EN_CH(LINE(1),(uint8_t* )"                 emXGUI ����װ�س���");
  
  /* ��ס KEY1 �������������Ӧ���ļ�ģʽ����ɺ�ֱ����ת��Ӧ�ó��� */
  if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON  )
  {
    /* ��ȡ .bin �ļ��������� */
    LED2_ON;
    if (xmodem_receive() == 0)
    { 
      LED2_OFF;
      iap_jump_app(FLASH_APP_ADDR);
    }
    else
    {
      LED3_ON;
    }
    LED2_OFF;
  }
  
  /* �����±�־ */
  SPI_FLASH_BufferRead(update_flag, 0, sizeof(update_flag));
  
  if (update_flag[0] == 0)
  {
    /* ���µ� APP ��ʼ����Ӧ�ó��� */
    
  }
  
  /* ������ֱ����ת��Ӧ�ó��� */
  iap_jump_app(FLASH_APP_ADDR);

	while(1);
}



/*********************************************END OF FILE**********************/


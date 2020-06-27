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
  * ʵ��ƽ̨:Ұ�� STM32 F429 ������
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "stm32f4xx.h"
#include "./led/bsp_led.h"
#include "./key/bsp_key.h" 
#include "./usart/bsp_debug_usart.h"
#include "./ymodem/ymodem.h"
#include "./FATFS/ff.h"
#include "./flash/bsp_spi_flash.h"

/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{
	uint8_t recv_flag = 0;
	
  /*��ʼ������*/
  LED_GPIO_Config();
  Key_GPIO_Config();
	Debug_USART_Config();    // Ĭ�ϲ�������1500000
  SPI_FLASH_Init();

	/* ��ѯ����״̬��������������תLED */ 
	while(1)
	{
		if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON  )
		{
			/* ��ȡ�������� */
      recv_flag = 1;
		}
    
    if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON  )
		{
      LED2_ON;
			/* ������Ƭ FLASH */
      SPI_FLASH_BulkErase();
      LED2_OFF;
		}
		
		if (recv_flag)
		{
      LED2_ON;
			ymodem_receive();
      recv_flag = 0;
      LED2_OFF;
		}

#if 0
    int GetResOffset(const char *res_name);
    uint8_t buff[50];
    int32_t addr;
    addr = GetResOffset("abc.txt");
    if (addr != -1)
      SPI_FLASH_BufferRead(buff, addr, 11);

    buff[10] = 0;
    printf("%s", buff);
#endif
	}
}



/*********************************************END OF FILE**********************/


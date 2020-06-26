/**
  ******************************************************************************
  * @file    ymodem_app.c
  * @author  long
  * @version V1.0
  * @date    2020-xx-xx
  * @brief   ymodemӦ��ʾ��
  ******************************************************************************
***/

#include "./ymodem/ymodem.h"
#include "./ymodem_app/ymodem_app.h"
#include "./usart/bsp_debug_usart.h"
#include "./FATFS/ff.h"

/* ȫ�ֱ��� */
static FIL fnew;												/* �ļ����� */
static FRESULT res_sd;                  /* �ļ�������� */
static UINT fnum;            					  /* �ļ��ɹ���д���� */

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
 * @brief   �ļ����ʹ�С������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @param   *file_name: �ļ�����.
 * @param   file_size: �ļ���С����Ϊ0xFFFFFFFF����˵����С��Ч.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_nanme_size_callback(void *ptr, char *file_name, uint32_t file_size)
{
  char buff[50];
  FIL **f_ptr = ptr;    /* �ļ����� */
  
  ptr = &fnew;
  
  sprintf(buff, "0:/%s", file_name);
  res_sd = f_open(*f_ptr, buff, FA_OPEN_APPEND | FA_WRITE );
  if ( res_sd != FR_OK )
  {	
    /* Flash ����. */
    return -1;
  }
  
  return 0;
}

/**
 * @brief   �ļ����ݽ�����ɻص�.
 * @param   *ptr: ���ƾ��.
 * @param   *file_name: �ļ�����.
 * @param   file_size: �ļ���С����Ϊ0xFFFFFFFF����˵����С��Ч.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_file_data_callback(void *ptr, char *file_data, uint32_t w_size)
{
  FIL **f_ptr = ptr;    /* �ļ����� */
  
  res_sd=f_write(*f_ptr, (uint8_t *)file_data, (uint32_t)w_size, &fnum);
  if(res_sd != FR_OK)
  {
    /* ���ٶ�д���ر��ļ� */
    f_close(*f_ptr);
    /* Flash ����. */
    return -1;
  }
  
  return 0;
}

/**
 * @brief   һ���ļ�������ɻص�.
 * @param   *ptr: ���ƾ��.
 * @return  ����д��Ľ����0���ɹ���-1��ʧ��.
 */
int receive_file_callback(void *ptr)
{
  FIL **f_ptr = ptr;    /* �ļ����� */
  f_close(*f_ptr);
  
  return 0;
}


/**
  ******************************************************************************
  * @file    ymodem_app.c
  * @author  long
  * @version V1.0
  * @date    2020-xx-xx
  * @brief   ymodemӦ��ʾ��
  ******************************************************************************
***/

#include "./xmodem/xmodem.h"
#include "./xmodem_app/xmodem_app.h"
#include "./usart/bsp_debug_usart.h"
#include "./FATFS/ff.h"

/* ȫ�ֱ��� */
static FIL fnew;												/* �ļ����� */
static FRESULT res_sd;                  /* �ļ�������� */
static UINT fnum;            					  /* �ļ��ɹ���д���� */
static uint8_t recv_flag = 0;
#define RECV_FILE_NAME     "recv_file.txt"

/**
 * @brief   Ymodem ����һ���ַ��Ľӿ�.
 * @param   ch �����͵�����
 * @return  ���ط���״̬
 */
int x_transmit_ch(uint8_t ch)
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
int open_file_receive(void *ptr, char *file_name)
{
  char buff[50];
  
  sprintf(buff, "0:/%s", file_name);
  res_sd = f_open(&fnew, buff, FA_CREATE_ALWAYS | FA_WRITE );
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
  if (recv_flag == 0)
  {
    recv_flag = 1;    // ������ڽ���һ���ļ�
    if(open_file_receive(&fnew, RECV_FILE_NAME) == -1)
    {	
      /* Flash ����. */
      return -1;
    }
  }
  
  res_sd=f_write(&fnew, (uint8_t *)file_data, (uint32_t)w_size, &fnum);
  if(res_sd != FR_OK)
  {
    /* ���ٶ�д���ر��ļ� */
    f_close(&fnew);
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
  f_close(&fnew);
  recv_flag = 0;
  
  return 0;
}


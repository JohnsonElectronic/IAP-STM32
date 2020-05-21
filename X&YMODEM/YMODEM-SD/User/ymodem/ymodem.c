/**
  ******************************************************************************
  * @file    ymodem.c
  * @author  long
  * @version V1.0
  * @date    2019-xx-xx
  * @brief   ymodem-1k Э��
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� STM32 F429 ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */

#include "./ymodem/ymodem.h"
#include "./boot_loader/boot_loader.h" 
#include "./iap_arch/iap_arch.h"
#include "./FATFS/ff.h"
#include <string.h>

/* ȫ�ֱ���. */
static uint8_t ymodem_packet_number = 0u;                           /* ������. */
static uint8_t x_first_packet_received = Y_IS_PACKET;               /* �ǲ��ǰ�ͷ. */
static uint8_t eot_num = 0;                                         /* �յ� EOT �Ĵ��� */

FIL fnew;													/* �ļ����� */
FRESULT res_sd;                   /* �ļ�������� */
UINT fnum;            					  /* �ļ��ɹ���д���� */

/* �ֲ�����. */
static uint16_t ymodem_calc_crc(uint8_t *data, uint16_t length);
static ymodem_status ymodem_handle_packet(uint8_t *header, uint16_t *num);
static ymodem_status ymodem_error_handler(uint8_t *error_number, uint8_t max_error_number);

void delay(void)
{
  __IO uint32_t conut = 0xFFFFFF;
  while(conut--);
}

/**
 * @brief   ���������XmodemЭ��Ļ���.
 *          �������ݲ���������.
 * @param   rec_num:��Ҫ���յ��ļ�����
 * @return  ��
 */
void ymodem_receive(uint16_t rec_num)
{
  volatile ymodem_status status = Y_OK;
  uint8_t error_number = 0u;

  x_first_packet_received = Y_NO_PACKET;
  ymodem_packet_number = 0u;

  /* ѭ����ֱ��û���κδ���(����ֱ����ת���û�Ӧ�ó���). */
  while (Y_OK == status && rec_num)
  {
    uint8_t *header = 0x00u;

    /* ��ȡ����ͷ. */
    int receive_status = x_receive(&header, 1u);

    /* ��ACSII "C"���͸���λ��(ֱ�������յ�һЩ����), ������λ������Ҫʹ�� CRC-16 . */
    if ((0 != receive_status) && (Y_NO_PACKET == x_first_packet_received))
    {
      (void)x_transmit_ch(Y_C);    // ����λ������ ACSII "C" ��������λ����ʹ�� CRC-16 
    }
    /* ��ʱ����������. */
    else if ((0 != receive_status) && (Y_IS_PACKET == x_first_packet_received))
    {
      status = ymodem_error_handler(&error_number, Y_MAY_ERRORS);
    }
    else
    {
      /* û�д���. */
//			header = data_rx_buff;
    }

    /* ��ͷ����ʹ: SOH, STX, EOT and CAN. */
		ymodem_status packet_status = Y_ERROR;
    switch(header[0])
    {
      /* 128��1024�ֽڵ�����. */
      case Y_SOH:
      case Y_STX:
        /* ���ݴ��� */
        packet_status = ymodem_handle_packet(header, &rec_num);
				/* �������ɹ�������һ�� ACK. */
        if (Y_OK == packet_status)
        {
          (void)x_transmit_ch(Y_ACK);
        }
        /* ���������flash��أ����������������������Ϊ���ֵ (������ֹ����). */
        else if (Y_ERROR_FLASH == packet_status)
        {
          error_number = Y_MAY_ERRORS;
          status = ymodem_error_handler(&error_number, Y_MAY_ERRORS);
        }
        /* �������ݰ�ʱ����Ҫô����һ�� NAK��Ҫôִ�д�����ֹ. */
        else
        {
          status = ymodem_error_handler(&error_number, Y_MAY_ERRORS);
        }
        break;
      /* �������. */
      case Y_EOT:
        /* ACK����������λ��(���ı���ʽ)��Ȼ����ת���û�Ӧ�ó���. */
        if (++eot_num > 1)
        {
          x_transmit_ch(Y_ACK);
          delay();    // ��ʱһ���ڷ���
          x_transmit_ch(Y_C);
          
//          rec_num--;           // ������һ���ļ�
//          x_first_packet_received = Y_NO_PACKET;
//          ymodem_packet_number = 0;
//          f_close(&fnew);
        }
        else
        {
          x_transmit_ch(Y_NAK);    /* ��һ���յ�EOT */
        }
        break;
      /* Abort from host. */
      case Y_CAN:
        status = Y_ERROR;
        break;
      default:
        /* Wrong header. */
       if (0 == receive_status)
        {
          status = ymodem_error_handler(&error_number, Y_MAY_ERRORS);
        }
        break;
    }
  }
}

/**
 * @brief   ������յ����� CRC-16.
 * @param   *data:  Ҫ��������ݵ�����.
 * @param   length: ���ݵĴ�С��128�ֽڻ�1024�ֽ�.
 * @return  status: ����CRC.
 */
static uint16_t ymodem_calc_crc(uint8_t *data, uint16_t length)
{
    uint16_t crc = 0u;
    while (length)
    {
        length--;
        crc = crc ^ ((uint16_t)*data++ << 8u);
        for (uint8_t i = 0u; i < 8u; i++)
        {
            if (crc & 0x8000u)
            {
                crc = (crc << 1u) ^ 0x1021u;
            }
            else
            {
                crc = crc << 1u;
            }
        }
    }
    return crc;
}

/**
 * @brief   ��������������Ǵ�ymodemЭ���л�õ����ݰ�.
 * @param   header: SOH ���� STX.
 * @return  status: ������.
 */
static ymodem_status ymodem_handle_packet(uint8_t *header, uint16_t *num)
{
  ymodem_status status = Y_OK;
  uint16_t size = 0u;
  char file_name[50];
  
  if (Y_SOH == header[0])
  {
    size = Y_PACKET_128_SIZE;
  }
  else if (Y_STX == header[0])
  {
    size = Y_PACKET_1024_SIZE;
  }
  else
  {
    /* ���������. */
    status = Y_ERROR;
  }
  uint16_t length = size + Y_PACKET_DATA_INDEX + Y_PACKET_CRC_SIZE;
  uint8_t received_data[Y_PACKET_1024_SIZE + Y_PACKET_DATA_INDEX + Y_PACKET_CRC_SIZE];

  /* ��������. */
  int receive_status = 0;
		
	memcpy(&received_data[0u], header, length);
	
  /* ��������ֽ�������������CRC. */
  uint16_t crc_received = ((uint16_t)received_data[length-2u] << 8u) | ((uint16_t)received_data[length-1u]);
  /* У��. */
  uint16_t crc_calculated = ymodem_calc_crc(&received_data[Y_PACKET_DATA_INDEX], size);

  /* ���������д�� flash. */
  if (Y_OK == status)
  {
    if (0 != receive_status)
    {
      /* �������. */
      status |= Y_ERROR_UART;
    }
    
    if (ymodem_packet_number != received_data[Y_PACKET_NUMBER_INDEX])
    {
      /* ���������������ƥ��. */
      status |= Y_ERROR_NUMBER;
    }
    
    if (255u != (received_data[Y_PACKET_NUMBER_INDEX] +  received_data[Y_PACKET_NUMBER_COMPLEMENT_INDEX]))
    {
      /* ���źͰ��Ų���֮�Ͳ���255. */
      /* �ܺ�Ӧ������255. */
      status |= Y_ERROR_NUMBER;
    }
    
    if (crc_calculated != crc_received)
    {
      /* �����CRC�ͽ��յ�CRC��ͬ. */
      status |= Y_ERROR_CRC;
    }
    
    if (received_data[Y_PACKET_NUMBER_INDEX] == 0x00 && x_first_packet_received == Y_NO_PACKET)    // ��һ����
    {
      strcpy(file_name, (char *)&received_data[Y_PACKET_DATA_INDEX]);
      if (strlen(file_name) == 0)
      {
        /* Flash ����. */
        status |= Y_ERROR_FLASH;
      }
      else
      {
        char buff[50];
        sprintf(buff, "0:/%s", file_name);
        res_sd = f_open(&fnew, buff, FA_OPEN_APPEND | FA_WRITE );
        if ( res_sd != FR_OK )
        {	
          /* Flash ����. */
          status |= Y_ERROR_FLASH;
        }
      }
    }
    else
    {
      if (received_data[Y_PACKET_NUMBER_INDEX] == 0x00 && eot_num > 1)
      {
        (*num)--;           // ������һ���ļ�
        x_first_packet_received = Y_NO_PACKET;
        ymodem_packet_number = 0;
        f_close(&fnew);
      }
      else
      {
        res_sd=f_write(&fnew, (uint8_t *)&received_data[Y_PACKET_DATA_INDEX], (uint32_t)size, &fnum);
        if(res_sd != FR_OK)
        {
          /* ���ٶ�д���ر��ļ� */
          f_close(&fnew);
          /* Flash ����. */
          status |= Y_ERROR_FLASH;
        }
        
        /* ��ǽ��յ�һ����. */
        x_first_packet_received = Y_IS_PACKET;
      }
    }
  }

  /* ���Ӱ������͵�ַ�����ٵ�ǰ������ʣ������ (���û���κδ���Ļ�). */
  if (Y_OK == status)
  { 
    ymodem_packet_number++;
  }

  return status;
}

/**
 * @brief   ����ymodem����.
*          ���Ӵ����������Ȼ��������������ﵽ�ٽ磬������ֹ��������һ�� NAK.
 * @param   *error_number:    ��ǰ������(��Ϊָ�봫��).
 * @param   max_error_number: �������������.
 * @return  status: Y_ERROR �ﵽ������������, Y_OK ��������.
 */
static ymodem_status ymodem_error_handler(uint8_t *error_number, uint8_t max_error_number)
{
  ymodem_status status = Y_OK;
  /* �������������. */
  (*error_number)++;
  /* ����������ﵽ���ֵ������ֹ. */
  if ((*error_number) >= max_error_number)
  {
    /* ��ֹ����. */
    (void)x_transmit_ch(Y_CAN);
    (void)x_transmit_ch(Y_CAN);
    status = Y_ERROR;
  }
  /* ����һ��NAK�����ظ�. */
  else
  {
    (void)x_transmit_ch(Y_NAK);
    status = Y_OK;
  }
  return status;
}
  
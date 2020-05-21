/**
  ******************************************************************************
  * @file    xmodem.c
  * @author  long
  * @version V1.0
  * @date    2019-xx-xx
  * @brief   xmodem Э�飨֧��128B��1K��
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ��  STM32 F429 ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */

#include "./xmodem/xmodem.h"
#include "./boot_loader/boot_loader.h" 
#include "./iap_arch/iap_arch.h"
#include <string.h>

/* ȫ�ֱ���. */
static uint8_t xmodem_packet_number = 1u;               /* ������. */
static uint32_t xmodem_actual_flash_address = FLASH_APP_ADDR;       /* д��ĵ�ַ. */
static uint8_t x_first_packet_received = X_IS_PACKET;   /* �ǲ��ǰ�ͷ. */
static uint32_t sector_size = 0;                        /* ����ʣ���С. */

/* �ֲ�����. */
static uint16_t xmodem_calc_crc(uint8_t *data, uint16_t length);
static xmodem_status xmodem_handle_packet(uint8_t *size);
static xmodem_status xmodem_error_handler(uint8_t *error_number, uint8_t max_error_number);
#define RX_MAX_LEN     (128*1024)
extern uint8_t data_rx_buff[RX_MAX_LEN];
/**
 * @brief   ���������XmodemЭ��Ļ���.
 *          �������ݲ���������.
 * @param   ��
 * @return  ��
 */
void xmodem_receive(void)
{
  volatile xmodem_status status = X_OK;
  uint8_t error_number = 0u;

  x_first_packet_received = X_IS_PACKET;
  xmodem_packet_number = 1u;

  /* ѭ����ֱ��û���κδ���(����ֱ����ת���û�Ӧ�ó���). */
  while (X_OK == status)
  {
    uint8_t *header = 0x00u;

    /* ��ȡ����ͷ. */
    int receive_status = x_receive(&header, 1u);

    /* ��ACSII "C"���͸���λ��(ֱ�������յ�һЩ����), ������λ������Ҫʹ�� CRC-16 . */
    if ((0 != receive_status) && (X_IS_PACKET == x_first_packet_received))
    {
      (void)x_transmit_ch(X_C);    // ����λ������ ACSII "C" ��������λ����ʹ�� CRC-16 
    }
    /* ��ʱ����������. */
    else if ((0 != receive_status) && (X_NO_PACKET == x_first_packet_received))
    {
      status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
    }
    else
    {
      /* û�д���. */
//			header = data_rx_buff;
    }

    /* ��ͷ����ʹ: SOH, STX, EOT and CAN. */
		xmodem_status packet_status = X_ERROR;
    switch(header[0])
    {
      /* 128��1024�ֽڵ�����. */
      case X_SOH:
      case X_STX:
        /* ���ݴ��� */
        packet_status = xmodem_handle_packet(header);
				/* �������ɹ�������һ�� ACK. */
        if (X_OK == packet_status)
        {
          (void)x_transmit_ch(X_ACK);
        }
        /* ���������flash��أ����������������������Ϊ���ֵ (������ֹ����). */
        else if (X_ERROR_FLASH == packet_status)
        {
          error_number = X_MAX_ERRORS;
          status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
        }
        /* �������ݰ�ʱ����Ҫô����һ�� NAK��Ҫôִ�д�����ֹ. */
        else
        {
          status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
        }
        break;
      /* �������. */
      case X_EOT:
        /* ACK����������λ��(���ı���ʽ)��Ȼ����ת���û�Ӧ�ó���. */
        sector_size = 0;    // ��λ������С
				xmodem_actual_flash_address = FLASH_APP_ADDR;    // ��λapp��ַ
        (void)x_transmit_ch(X_ACK);
        (void)printf("\n\rFirmware updated!\n\r");
        (void)printf("Jumping to user application...\n\r");
				iap_jump_app(FLASH_APP_ADDR);
        break;
      /* Abort from host. */
      case X_CAN:
        status = X_ERROR;
        break;
      default:
        /* Wrong header. */
       if (0 == receive_status)
        {
          status = xmodem_error_handler(&error_number, X_MAX_ERRORS);
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
static uint16_t xmodem_calc_crc(uint8_t *data, uint16_t length)
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
 * @brief   ��������������Ǵ�xmodemЭ���л�õ����ݰ�.
 * @param   header: SOH ���� STX.
 * @return  status: ������.
 */
static xmodem_status xmodem_handle_packet(uint8_t *header)
{
  xmodem_status status = X_OK;
  uint16_t size = 0u;
  if (X_SOH == header[0])
  {
    size = X_PACKET_128_SIZE;
  }
  else if (X_STX == header[0])
  {
    size = X_PACKET_1024_SIZE;
  }
  else
  {
    /* ���������. */
    status = X_ERROR;
  }
  uint16_t length = size + X_PACKET_DATA_INDEX + X_PACKET_CRC_SIZE;
  uint8_t received_data[X_PACKET_1024_SIZE + X_PACKET_DATA_INDEX + X_PACKET_CRC_SIZE];

  /* ��������. */
  int receive_status = 0;
		
	memcpy(&received_data[0u], header, length);
	
  /* ��������ֽ�������������CRC. */
  uint16_t crc_received = ((uint16_t)received_data[length-2u] << 8u) | ((uint16_t)received_data[length-1u]);
  /* У��. */
  uint16_t crc_calculated = xmodem_calc_crc(&received_data[X_PACKET_DATA_INDEX], size);

  /* ��ǰ���������˲�����һ��. */
  if (sector_size <= size)
  {
    sector_size += x_receive_flash_erasure(xmodem_actual_flash_address + sector_size);

    if (0 != sector_size)
    {
      x_first_packet_received = 1;
    }
    else
    {
      status |= X_ERROR_FLASH;
    }
  }

  /* ���������д�� flash. */
  if (X_OK == status)
  {
    if (0 != receive_status)
    {
      /* �������. */
      status |= X_ERROR_UART;
    }
    if (xmodem_packet_number != received_data[X_PACKET_NUMBER_INDEX])
    {
      /* ���������������ƥ��. */
      status |= X_ERROR_NUMBER;
    }
    if (255u != (received_data[X_PACKET_NUMBER_INDEX] +  received_data[X_PACKET_NUMBER_COMPLEMENT_INDEX]))
    {
      /* ���źͰ��Ų���֮�Ͳ���255. */
      /* �ܺ�Ӧ������255. */
      status |= X_ERROR_NUMBER;
    }
    if (crc_calculated != crc_received)
    {
      /* �����CRC�ͽ��յ�CRC��ͬ. */
      status |= X_ERROR_CRC;
    }
    /* û�д���д�� flash. */
    if (0 != x_receive_flash_writea(xmodem_actual_flash_address, (uint8_t *)&received_data[X_PACKET_DATA_INDEX], (uint32_t)size))
    {
      /* Flash д����. */
      status |= X_ERROR_FLASH;
    }
  }

  /* ���Ӱ������͵�ַ�����ٵ�ǰ������ʣ������ (���û���κδ���Ļ�). */
  if (X_OK == status)
  { 
    xmodem_packet_number++;
    xmodem_actual_flash_address += size;
    sector_size -= size;
  }

  return status;
}

/**
 * @brief   ����xmodem����.
*          ���Ӵ����������Ȼ��������������ﵽ�ٽ磬������ֹ��������һ�� NAK.
 * @param   *error_number:    ��ǰ������(��Ϊָ�봫��).
 * @param   max_error_number: �������������.
 * @return  status: X_ERROR �ﵽ������������, X_OK ��������.
 */
static xmodem_status xmodem_error_handler(uint8_t *error_number, uint8_t max_error_number)
{
  xmodem_status status = X_OK;
  /* �������������. */
  (*error_number)++;
  /* ����������ﵽ���ֵ������ֹ. */
  if ((*error_number) >= max_error_number)
  {
    /* ��ֹ����. */
    (void)x_transmit_ch(X_CAN);
    (void)x_transmit_ch(X_CAN);
    status = X_ERROR;
  }
  /* ����һ��NAK�����ظ�. */
  else
  {
    (void)x_transmit_ch(X_NAK);
    status = X_OK;
  }
  return status;
}

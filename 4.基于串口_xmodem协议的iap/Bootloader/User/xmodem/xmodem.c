/**
 * @file    xmodem.c
 * @author  Ferenc Nemeth
 * @date    21 Dec 2018
 * @brief   This module is the implementation of the Xmodem protocol.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */

#include "./xmodem/xmodem.h"
#include "./boot_loader/boot_loader.h" 
#include "./iap_arch/iap_arch.h"

/* ȫ�ֱ���. */
static uint8_t xmodem_packet_number = 1u;               /* ������. */
static uint32_t xmodem_actual_flash_address = 0u;       /* д��ĵ�ַ. */
static uint8_t x_first_packet_received = X_IS_PACKET;   /* �ǲ��ǰ�ͷ. */
static uint32_t sector_size = 0;                        /* ����ʣ���С. */

/* �ֲ�����. */
static uint16_t xmodem_calc_crc(uint8_t *data, uint16_t length);
static xmodem_status xmodem_handle_packet(uint8_t size);
static xmodem_status xmodem_error_handler(uint8_t *error_number, uint8_t max_error_number);

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
  xmodem_actual_flash_address = FLASH_APP_ADDR;

  /* ѭ����ֱ��û���κδ���(����ֱ����ת���û�Ӧ�ó���). */
  while (X_OK == status)
  {
    uint8_t header = 0x00u;

    /* ��ȡ����ͷ. */
    int receive_status = x_receive(&header, 1u);

    /* Spam the host (until we receive something) with ACSII "C", to notify it, we want to use CRC-16. */
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
      /* Do nothing. */
    }

    /* The header can be: SOH, STX, EOT and CAN. */
		xmodem_status packet_status = X_ERROR;
    switch(header)
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
      /* End of Transmission. */
      case X_EOT:
        /* ACK, feedback to user (as a text), then jump to user application. */
        sector_size = 0;    // ��λ������С
        (void)x_transmit_ch(X_ACK);
        (void)printf("\n\rFirmware updated!\n\r");
        (void)printf("Jumping to user application...\n\r");
//        flash_jump_to_app();
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
static xmodem_status xmodem_handle_packet(uint8_t header)
{
  xmodem_status status = X_OK;
  uint16_t size = 0u;
  if (X_SOH == header)
  {
    size = X_PACKET_128_SIZE;
  }
  else if (X_STX == header)
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
  int receive_status = x_receive(&received_data[0u], length);
	
  /* ��������ֽ�������������CRC. */
  uint16_t crc_received = ((uint16_t)received_data[length-2u] << 8u) | ((uint16_t)received_data[length-1u]);
  /* У��. */
  uint16_t crc_calculated = xmodem_calc_crc(&received_data[X_PACKET_DATA_INDEX], size);

  /* ��ǰ���������˲�����һ��. */
  if (sector_size <= size)
  {
    sector_size += x_receive_flash_erasure(FLASH_APP_ADDR);

    if (0 == sector_size)
    {
      x_first_packet_received = 1;
    }
    else
    {
      status |= X_ERROR_FLASH;
    }
  }

  /* Error handling and flashing. */
  if (X_OK == status)
  {
    if (0 != receive_status)
    {
      /* UART error. */
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
    if (0 != x_receive_flash_writea(xmodem_actual_flash_address, (uint8_t *)&received_data[X_PACKET_DATA_INDEX], (uint32_t)size/4u))
    {
      /* Flashing error. */
      status |= X_ERROR_FLASH;
    }
  }

  /* Raise the packet number and the address counters (if there wasn't any error). */
  if (X_OK == status)
  { 
    xmodem_packet_number++;
    xmodem_actual_flash_address += size;
    sector_size -= size;
  }

  return status;
}

/**
 * @brief   Handles the xmodem error.
 *          Raises the error counter, then if the number of the errors reached critical, do a graceful abort, otherwise send a NAK.
 * @param   *error_number:    Number of current errors (passed as a pointer).
 * @param   max_error_number: Maximal allowed number of errors.
 * @return  status: X_ERROR in case of too many errors, X_OK otherwise.
 */
static xmodem_status xmodem_error_handler(uint8_t *error_number, uint8_t max_error_number)
{
  xmodem_status status = X_OK;
  /* Raise the error counter. */
  (*error_number)++;
  /* If the counter reached the max value, then abort. */
  if ((*error_number) >= max_error_number)
  {
    /* Graceful abort. */
    (void)x_transmit_ch(X_CAN);
    (void)x_transmit_ch(X_CAN);
    status = X_ERROR;
  }
  /* Otherwise send a NAK for a repeat. */
  else
  {
    (void)x_transmit_ch(X_NAK);
    status = X_OK;
  }
  return status;
}
